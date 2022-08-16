/** @file main.c
 * @author Shashank Swaminathan
 * Core file for the recovery board. Defines all configurations, initializes all
 * modules, and runs appropriate timers/interrupts based on received data.
 */
#include "aprs.h"
#include "gps.h"
#include "pico/binary_info.h"
#include "pico/stdlib.h"
#include "pico/time.h"
#include "tag.h"
#include "vhf.h"
#include <stdio.h>

// Callsign and SSIS configuration
#define CALLSIGN "J73MAB"
#define SSID 1`

/// Location of the LED pin
#define LED_PIN 29
/// Choose whether or not to use interrupts for APRS
#define Q_IRQ 0
/// Turns on communications with the main tag (only for fully CONNected tags)
#define TAG_CONN 0
/// Turns on fish tracker behavior when no GPS
#define USE_YAGI 0

void set_bin_desc(void);
void setLed(bool state);
void initLed(void);
bool txVHF(repeating_timer_t *rt);
void startVHF(repeating_timer_t *vhfTimer);
bool txAprsIRQ(repeating_timer_t *rt);
bool txAprsNoIRQ(void);
void startAPRS(const aprs_config_s *aprs_cfg, repeating_timer_t *aprsTimer);
void startTag(const tag_config_s *tag_cfg, repeating_timer_t *tagTimer);
bool txTag(repeating_timer_t *rt);
void initAll(const gps_config_s *gps_cfg, const tag_config_s *tag_cfg);

/** @struct Defines unchanging configuration parameters for APRS.
 * callsign, ssid
 * dest, digi, digissid, comment
 * interval, debug, debug style
 */
const aprs_config_s aprs_config = {
    CALLSIGN, SSID, "APLIGA", "WIDE2", 1, "Ceti b1.2 4-S", 30000, false, 2};

/** @struct Defines unchanging configuration parameters for GPS communication.
 * GPS_TX, GPS_RX, GPS_BAUD, UART_NUM, QINTERACTIVE
 */
const gps_config_s gps_config = {0, 1, 9600, uart0, false};
gps_data_s gps_data = {
    {15.31383, -61.30075},     {0, 0, 0}, "$GN 15.31383, -61.30075,0,360,0*19",
    "$DT 20190408195123,V*41", false,     false};
/** @struct Defines unchanging configuration parameters for tag communication.
 * TAG_TX, TAG_RX, TAG_BAUD, UART_NUM, SEND_INTERVAL, ACK_WAIT_TIME_US,
 * ACK_WAIT_TIME_MS, NUM_TRIES
 */
const tag_config_s tag_config = {4, 5, 115200, uart1, 10000, 1000, 1000, 3};

const struct vhf_config_t {
  float txFreq;
  const uint32_t interval;
} vhf_config = {148.056, 5000};

void set_bin_desc(void) {
  bi_decl(bi_program_description(
      "Recovery process binary for standalone recovery board 2-#"));
  bi_decl(bi_1pin_with_name(LED_PIN, "On-board LED"));
  bi_decl(bi_1pin_with_name(gps_config.txPin, "TX UART to GPS"));
  bi_decl(bi_1pin_with_name(gps_config.rxPin, "RX UART to GPS"));
  bi_decl(bi_1pin_with_name(tag_config.txPin, "TX UART to TAG modem"));
  bi_decl(bi_1pin_with_name(tag_config.rxPin, "RX UART to TAG modem"));
  describeConfig();
}

void setLed(bool state) { gpio_put(LED_PIN, state); }
void initLed(void) {
  gpio_init(LED_PIN);
  gpio_set_dir(LED_PIN, GPIO_OUT);
}

bool txVHF(repeating_timer_t *rt) {
  printf("blast time\n");
  vhf_pulse_callback();
  gps_get_lock(&gps_config, &gps_data);
  return !gps_data.posCheck;
}

void startVHF(repeating_timer_t *vhfTimer) {
  prepFishTx(vhf_config.txFreq);
  add_repeating_timer_ms(-vhf_config.interval, txVHF, NULL, vhfTimer);
}

bool txAprsIRQ(repeating_timer_t *rt) {
  gps_get_lock(&gps_config, &gps_data);
  if (aprs_config.debug)
    sendTestPackets(&aprs_config);
  else {
	printf("APRS WITH IRQ\n");
    setLed(true);
    sendPacket(&aprs_config, gps_data.latlon, gps_data.acs);
    setLed(false);
  }
  return gps_data.posCheck;
}
bool txAprsNoIRQ(void) {
  gps_get_lock(&gps_config, &gps_data);
  if (aprs_config.debug)
    sendTestPackets(&aprs_config);
  else {
    printf("APRS NO IRQ: %s-%d @ %.6f, %.6f\n", CALLSIGN, SSID, gps_data.latlon[0], gps_data.latlon[1]);
    setLed(true);
    sendPacket(&aprs_config, gps_data.latlon, gps_data.acs);
    setLed(false);
  }
  return gps_data.posCheck;
}

void startAPRS(const aprs_config_s *aprs_cfg, repeating_timer_t *aprsTimer) {
  configureAPRS_TX(144.39);
  add_repeating_timer_ms(-aprs_cfg->interval, txAprsIRQ, NULL, aprsTimer);
}

bool txTag(repeating_timer_t *rt) {
  printf("doing taxes\n");
  // getLastPDtBufs(lastGpsUpdate, lastDtUpdate);
  writeGpsToTag(&tag_config, gps_data.lastGpsBuffer, gps_data.lastDtBuffer);
  // detachTag();
  // reqTagState();
  return true;
}

void startTag(const tag_config_s *tag_cfg, repeating_timer_t *tagTimer) {
  add_repeating_timer_ms(-tag_cfg->interval, txTag, NULL, tagTimer);
}

void initAll(const gps_config_s *gps_cfg, const tag_config_s *tag_cfg) {
  set_bin_desc();
  stdio_init_all();

  initLed();
  setLed(true);

  gpsInit(gps_cfg);

  initAPRS();

  initTagComm(tag_cfg);
  setLed(false);
}

int main() {
  // Setup
  initAll(&gps_config, &tag_config);
  printf("Initialized: %s-%d @ %.2f, %.2f\n", CALLSIGN, SSID, gps_data.latlon[0], gps_data.latlon[1]);

  // APRS comms interrupt setup
  repeating_timer_t aprsTimer;

// Tag comms interrupt setup
#if TAG_CONN
  repeating_timer_t tagTimer;
  startTag(&tag_config, &tagTimer);
#endif

#if USE_YAGI
  // VHF pulse interrupt setup
  repeating_timer_t yagiTimer;
  // Start the VHF pulsing until GPS lock
  setVhfState(true);
  startVHF(&yagiTimer);
  bool yagiIsOn = true;
#endif
  // printf("Init yagi.\n");

  // Loop
  while (true) {
#if USE_YAGI
    if (gps_data.posCheck && yagiIsOn) {
      setVhfState(true);
      yagiIsOn = false;
// printf("We're in the timer zone now.\n");
#if Q_IRQ
      startAPRS(&aprs_config, &aprsTimer);
      printf("Started aprs timer.\n");
#else
      configureAPRS_TX(144.39);
#endif
    } else if (!gps_data.datCheck && !yagiIsOn) {
      setVhfState(true);
      // DO NOT CANCEL TAG TIMER, WE WILL REPORT THROUGH DATA LOSS
      startVHF(&yagiTimer);
      yagiIsOn = true;
    } else {
#if Q_IRQ
      gps_get_lock(&gps_config, &gps_data);
#else
      txAprsNoIRQ();
#endif
    }
#else
    txAprsNoIRQ();
#endif
    // printf("repeating sleep over here ...\n");
    sleep_ms(aprs_config.interval);  // sleep for two minutes before
                                     // re-evaluating gps quality
  }
  return 0;
}
