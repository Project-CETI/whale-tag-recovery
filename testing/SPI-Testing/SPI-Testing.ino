#include <SPI.h>

const uint8_t csPin = 1;
const uint8_t txPin = 3;
const uint8_t clkPin = 2;

void setOutput(uint16_t currVal) {
  SPI.setCS(LOW);
  byte msg[2];
  bitWrite(msg[0], 4, currVal & 0b0000000000000001);
  bitWrite(msg[0], 5, currVal & 0b0000000000000010);
  bitWrite(msg[0], 6, currVal & 0b0000000000000100);
  bitWrite(msg[0], 7, currVal & 0b0000000000001000);
  bitWrite(msg[1], 0, currVal & 0b0000000100000000);
  bitWrite(msg[1], 1, currVal & 0b0000001000000000);
  bitWrite(msg[1], 2, currVal & 0b0000010000000000);
  bitWrite(msg[1], 3, currVal & 0b0000100000000000);
  bitWrite(msg[1], 4, currVal & 1);  // active
  bitWrite(msg[1], 5, currVal & 1);
  bitWrite(msg[1], 6, currVal & 0);
  bitWrite(msg[1], 7, currVal & 0);
  SPI.beginTransaction(SPISettings(20000000, LSBFIRST, SPI_MODE0));
  SPI.transfer(msg, 2);
  SPI.endTransaction();
  SPI.setCS(HIGH);
}

void setup() {
  Serial.begin(115200);
  SPI.setCS(csPin);
  SPI.setSCK(clkPin);
  SPI.setTX(txPin);
  // SPI.setBitOrder(LSBFIRST);
  SPI.begin();
}

void loop() {
  setOutput(4095);
  Serial.print("Updating");
  delay(5000);
}
