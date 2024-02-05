//-----------------------------------------------------------------------------
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include "samd21.h"
#include "hal_gpio.h"

//-----------------------------------------------------------------------------
#define UART_TX_DMA_CH  0
#define UART_RX_DMA_CH  1

#define UART_RX_SIZE    10

HAL_GPIO_PIN(LED,      B, 30)
HAL_GPIO_PIN(BUTTON,   A, 15)
HAL_GPIO_PIN(UART_TX,  A, 22)
HAL_GPIO_PIN(UART_RX,  A, 23)

//-----------------------------------------------------------------------------
static volatile DmacDescriptor dma_desc[DMAC_CH_NUM];
static volatile DmacDescriptor dma_wb[DMAC_CH_NUM];

uint8_t uart_rx_buf[UART_RX_SIZE];

//-----------------------------------------------------------------------------
static void uart_init(uint32_t baud)
{
  uint64_t br = (uint64_t)65536 * (F_CPU - 16 * baud) / F_CPU;

  HAL_GPIO_UART_TX_out();
  HAL_GPIO_UART_TX_pmuxen(PORT_PMUX_PMUXE_C_Val);
  HAL_GPIO_UART_RX_in();
  HAL_GPIO_UART_RX_pmuxen(PORT_PMUX_PMUXE_C_Val);

  PM->APBCMASK.reg |= PM_APBCMASK_SERCOM3;

  GCLK->CLKCTRL.reg = GCLK_CLKCTRL_ID(SERCOM3_GCLK_ID_CORE) |
      GCLK_CLKCTRL_CLKEN | GCLK_CLKCTRL_GEN(0);

  SERCOM3->USART.CTRLA.reg = SERCOM_USART_CTRLA_RUNSTDBY |
      SERCOM_USART_CTRLA_DORD | SERCOM_USART_CTRLA_MODE_USART_INT_CLK |
      SERCOM_USART_CTRLA_RXPO(1/*PAD1*/) | SERCOM_USART_CTRLA_TXPO(0/*PAD0*/);

  SERCOM3->USART.CTRLB.reg = SERCOM_USART_CTRLB_RXEN | SERCOM_USART_CTRLB_TXEN |
      SERCOM_USART_CTRLB_CHSIZE(0/*8 bits*/);

  SERCOM3->USART.BAUD.reg = (uint16_t)br;

  SERCOM3->USART.CTRLA.reg |= SERCOM_USART_CTRLA_ENABLE;

  // Setup UART DMA TX
  DMAC->CHID.reg = UART_TX_DMA_CH;
  DMAC->CHCTRLA.reg = DMAC_CHCTRLA_SWRST;
  while (DMAC->CHCTRLA.bit.SWRST);

  DMAC->CHINTENSET.reg = DMAC_CHINTENSET_TERR | DMAC_CHINTENSET_SUSP | DMAC_CHINTENSET_TCMPL;

  DMAC->CHCTRLB.reg = DMAC_CHCTRLB_LVL(0) | DMAC_CHCTRLB_TRIGACT_BEAT |
      DMAC_CHCTRLB_TRIGSRC(SERCOM3_DMAC_ID_TX);

  dma_desc[UART_TX_DMA_CH].BTCTRL.reg = DMAC_BTCTRL_VALID | DMAC_BTCTRL_STEPSEL |
      DMAC_BTCTRL_SRCINC;
  dma_desc[UART_TX_DMA_CH].DSTADDR.reg = (uint32_t)&SERCOM3->USART.DATA.reg;
  dma_desc[UART_TX_DMA_CH].DESCADDR.reg = 0;

  // Setup UART DMA RX
  DMAC->CHID.reg = UART_RX_DMA_CH;
  DMAC->CHCTRLA.reg = DMAC_CHCTRLA_SWRST;
  while (DMAC->CHCTRLA.bit.SWRST);

  DMAC->CHINTENSET.reg = DMAC_CHINTENSET_TERR | DMAC_CHINTENSET_SUSP | DMAC_CHINTENSET_TCMPL;

  DMAC->CHCTRLB.reg = DMAC_CHCTRLB_LVL(0) | DMAC_CHCTRLB_TRIGACT_BEAT |
      DMAC_CHCTRLB_TRIGSRC(SERCOM3_DMAC_ID_RX);

  dma_desc[UART_RX_DMA_CH].BTCTRL.reg = DMAC_BTCTRL_VALID | DMAC_BTCTRL_STEPSEL |
      DMAC_BTCTRL_DSTINC;
  dma_desc[UART_RX_DMA_CH].SRCADDR.reg = (uint32_t)&SERCOM3->USART.DATA.reg;
  dma_desc[UART_RX_DMA_CH].DSTADDR.reg = (uint32_t)(uart_rx_buf + UART_RX_SIZE);
  dma_desc[UART_RX_DMA_CH].BTCNT.reg = UART_RX_SIZE;
  dma_desc[UART_RX_DMA_CH].DESCADDR.reg = 0;

  DMAC->CHINTFLAG.reg = DMAC->CHINTFLAG.reg;
  DMAC->CHCTRLA.reg = DMAC_CHCTRLA_ENABLE;
}

//-----------------------------------------------------------------------------
static void uart_putc(char c)
{
  //while (!(SERCOM3->USART.INTFLAG.reg & SERCOM_USART_INTFLAG_DRE));
  SERCOM3->USART.DATA.reg = c;
  while (!(SERCOM3->USART.INTFLAG.reg & SERCOM_USART_INTFLAG_DRE));
}

//-----------------------------------------------------------------------------
static void uart_puts(char *s)
{
  while (*s)
    uart_putc(*s++);
}

//-----------------------------------------------------------------------------
static void uart_puts_dma(char *data, int size)
{
  __disable_irq();  // TODO: actual critical section

  DMAC->CHID.reg = UART_TX_DMA_CH;
  
  if (0 == (DMAC->CHCTRLA.reg & DMAC_CHCTRLA_ENABLE) ||
      0 != (DMAC->CHINTFLAG.reg & DMAC_CHINTFLAG_TCMPL))
  {
    dma_desc[UART_TX_DMA_CH].SRCADDR.reg = (uint32_t)(data + size);
    dma_desc[UART_TX_DMA_CH].BTCNT.reg = size;
    DMAC->CHINTFLAG.reg = DMAC->CHINTFLAG.reg;
    DMAC->CHCTRLA.reg = DMAC_CHCTRLA_ENABLE;
  }
  else
  {
    while (1);
  }

  __enable_irq();
}

//-----------------------------------------------------------------------------
static void dma_init(void)
{
  DMAC->BASEADDR.reg = (uint32_t)dma_desc;
  DMAC->WRBADDR.reg = (uint32_t)dma_wb;
  
  DMAC->PRICTRL0.reg = DMAC_PRICTRL0_RRLVLEN0 | DMAC_PRICTRL0_RRLVLEN1 |
      DMAC_PRICTRL0_RRLVLEN2 | DMAC_PRICTRL0_RRLVLEN3;
  
  NVIC_EnableIRQ(DMAC_IRQn);
  NVIC_ClearPendingIRQ(DMAC_IRQn);
  
  DMAC->CTRL.reg = DMAC_CTRL_DMAENABLE | DMAC_CTRL_LVLEN0 | DMAC_CTRL_LVLEN1 |
      DMAC_CTRL_LVLEN2 | DMAC_CTRL_LVLEN3;
}

//-----------------------------------------------------------------------------
void irq_handler_dmac(void)
{
  if (DMAC->INTSTATUS.bit.CHINT0)
  {
    DMAC->CHID.reg = UART_TX_DMA_CH;
    
    if (DMAC->CHINTFLAG.bit.TERR)
    {
      //uart_putc('E');
      asm("nop");
      while (1);
    }

    else if (DMAC->CHINTFLAG.bit.SUSP)
    {
      //uart_putc('S');
      asm("nop");
      while (1);
    }

    else if (DMAC->CHINTFLAG.bit.TCMPL)
    {
      //uart_putc('C');
      asm("nop");
    }

    DMAC->CHINTFLAG.reg = DMAC->CHINTFLAG.reg;
  }

  if (DMAC->INTSTATUS.bit.CHINT1)
  {
    DMAC->CHID.reg = UART_RX_DMA_CH;
    
    if (DMAC->CHINTFLAG.bit.TERR)
    {
      uart_putc('E');
      asm("nop");
      while (1);
    }

    else if (DMAC->CHINTFLAG.bit.SUSP)
    {
      uart_putc('S');
      asm("nop");
      while (1);
    }

    else if (DMAC->CHINTFLAG.bit.TCMPL)
    {
      uart_putc('C');
      DMAC->CHCTRLA.reg = DMAC_CHCTRLA_ENABLE;
      asm("nop");
    }

    DMAC->CHINTFLAG.reg = DMAC->CHINTFLAG.reg;
  }
}

//-----------------------------------------------------------------------------
static void sys_init(void)
{
  // Switch to 8MHz clock (disable prescaler)
  SYSCTRL->OSC8M.bit.PRESC = 0;

  GCLK->GENCTRL.reg = GCLK_GENCTRL_ID(0) | GCLK_GENCTRL_SRC(GCLK_SOURCE_OSC8M) |
      GCLK_GENCTRL_RUNSTDBY | GCLK_GENCTRL_GENEN;
  while (GCLK->STATUS.reg & GCLK_STATUS_SYNCBUSY);

  // Enable interrupts
  asm volatile ("cpsie i");
}

//-----------------------------------------------------------------------------
int main(void)
{
  uint32_t cnt = 0;

  sys_init();
  dma_init();
  uart_init(115200);

  uart_puts("\r\nHello, world!\r\n");

  HAL_GPIO_LED_out();
  HAL_GPIO_LED_clr();

  HAL_GPIO_BUTTON_in();
  HAL_GPIO_BUTTON_pullup();

  PM->SLEEP.reg = PM_SLEEP_IDLE(0); 
  //SCB->SCR |= SCB_SCR_SLEEPDEEP_Msk;
  SCB->SCR &= ~SCB_SCR_SLEEPDEEP_Msk;

  while (1)
  {
    if (HAL_GPIO_BUTTON_read())
      cnt = 0;
    else if (cnt < 5001)
      cnt++;

    if (5000 == cnt)
    {
      uart_putc('.');
      __WFI();
      uart_putc('#');

      //uart_puts_dma("0123456789qwertyuiopasdfghjklzxcvbnm0123456789qwertyuiopasdfghjklzxcvbnm", 70);
    }
  }

  return 0;
}
