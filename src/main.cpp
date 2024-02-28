
#include <Arduino.h>
#include <dma_core.h>

void setup() {
  Serial.begin(9600);
  while(!Serial);
  Serial.println("****************************************");
  Serial.println("CONNECTED");
  Serial.println("****************************************");

  MCLK->AHBMASK.bit.DMAC_ = 1;

}

void loop() {

}
