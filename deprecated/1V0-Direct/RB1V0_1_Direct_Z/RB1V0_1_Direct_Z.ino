#include "pico/stdlib.h"
#include <stdio.h>
#include <math.h>
#include <time.h>

// Board LED
bool ledState = false;
bool ledQ = false;
uint64_t prevLedTime = 0;
uint32_t ledOn = 0;
void setLed(bool state) {digitalWrite(ledPin, state);}
void initLed() {pinMode(ledPin, OUTPUT);}

// Enables
bool swarmRunning = false;
bool waitForAcks = false;
bool aprsRunning = false;

void txSwarm() {
  if (initDTAck) {
    writeToModem("$TD \"Transmit Queue\"");
  }
}

void txAprs() {
  prevAprsTx = millis();
  // What are we sending, hopefully?
//  Serial.print("GPS Data|| lat: ");
//  Serial.print(latlon[0],4);
//  Serial.print(" | lon: ");
//  Serial.print(latlon[1],4);
//  Serial.printf(" | A: %d | C: %d | S: %d\n", acs[0], acs[1], acs[2]);
  // End Debug
  uint32_t startPacket = millis();
  sendPacket();
  uint32_t packetDuration = millis() - startPacket;
  Serial.println("Packet sent in: " + String(packetDuration) + " ms");
  printPacket();
}

void setup() {
//  swarmRunning = true;
  aprsRunning = true;
//  waitForAcks = true;
//  swarmInteractive = true;
  initLed();
  setLed(true);

  // Set SWARM first, we need to grab those acks
  Serial.begin(115200);
  Serial1.setTX(0);
  Serial1.setRX(1);
  Serial1.begin(115200);
  swarmBootSequence(waitForAcks && swarmRunning);
  Serial.println("Swarm booted.");

  // DRA818V initialization
  Serial.println("Configuring DRA818V...");
  // Initialize DRA818V
  initializeDra818v();
  configureDra818v();
  setPttState(false);
  setVhfState(false);
  Serial.println("DRA818V configured");

  // Initialize DAC
  Serial.println("Configuring DAC...");
  initializeOutput();
  Serial.println("DAC configured");

  if(swarmRunning) swarmInit(swarmRunning && waitForAcks);
}

void loop() {
  serialCopyToModem();
  readFromModem();
  // Update LED state
  ledQ = (millis() - prevLedTime >= ledOn);
  setLed(!ledQ);
  ledState = !ledQ;
  // Transmit APRS
  if ((millis() - prevAprsTx >= aprsInterval) && aprsRunning) {
    setVhfState(true);
    delay(100);
    txAprs();
    setVhfState(false);
  }
}
