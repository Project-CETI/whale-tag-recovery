#include <stdio.h>
#include "pico/stdlib.h"
#include "pico/binary_info.h"
#include "aprs.h"

const uint LED_PIN = 29;

// status arrays (updated in-place by functions)
float coords[2] = {0,0};
uint16_t aCS[3] = {0,0,0};

// APRS communication config (change per tag)
char mycall[8] = "KC1QXQ";
int myssid = 15;
char dest[8] = "APLIGA";
char digi[8] = "WIDE2";
int digissid = 1;
char comment[128] = "Ceti v1.0 2-SDK";

// Which system are running
bool aprsRunning = false;

// runtime vars
uint32_t prevAprsTx = 0;

// APRS TX intervals (uncomment ONLY one)
//uint32_t aprsInterval = 5000;
uint32_t aprsInterval = 10000;
//uint32_t aprsInterval = 15000;
//uint32_t aprsInterval = 60000;
//uint32_t aprsInterval = 120000;

void set_bin_desc(void);
void setLed(bool state);
void initLed(void);
void setup(void);

void set_bin_desc() {
  bi_decl(bi_program_description("This is the WIP SDK-based Recovery Board firmware for CETI."));
  bi_decl(bi_1pin_with_name(LED_PIN, "On-board LED"));
  pinDescribe();
}

void setLed(bool state) {gpio_put(LED_PIN, state);}
void initLed() {gpio_init(LED_PIN); gpio_set_dir(LED_PIN, GPIO_OUT);}

void txAprs(bool aprsDebug, int style) {
  prevAprsTx = to_ms_since_boot(get_absolute_time());
  printf("%d",prevAprsTx);
  setLed(true);
  if (aprsDebug)
    sendTestPackets(mycall, myssid, dest, digi, digissid, comment, style);
  else
    sendPacket(coords, aCS, mycall, myssid, dest, digi, digissid, comment);
  setLed(false);
  printf("Packet sent in %d ms\n", to_ms_since_boot(get_absolute_time()) - prevAprsTx);
  printPacket(mycall, myssid, dest, digi, digissid, comment);
}

void setup() {
  set_bin_desc();
  stdio_init_all();

  aprsRunning = true;

  initLed();
  setLed(true);

  if (aprsRunning) configureVHF();
  setLed(false);
}

int main() {
  // Setup
  setup();

  // Loop
  while (true) {
    if (((to_ms_since_boot(get_absolute_time()) - prevAprsTx) >= aprsInterval) && aprsRunning) {
      setVhfState(true);
      txAprs(true, 2); 
      setVhfState(false);
    }
  }
}
