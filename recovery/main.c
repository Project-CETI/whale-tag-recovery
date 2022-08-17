/** @file main.c
 * @author Shashank Swaminathan
 * Core file for the recovery board. Defines all configurations, initializes all
 * modules, and runs appropriate timers/interrupts based on received data.
 */
#include "aprs.h"
#include "constants.h"
#include "gps.h"
#include "pico/binary_info.h"
#include "pico/stdlib.h"
#include "pico/time.h"
#include "tag.h"
#include "vhf.h"
#include <stdio.h>

void set_bin_desc(void);
void setLed(bool state);
void initLed(void);
bool txVHF(repeating_timer_t *rt);
void startVHF(repeating_timer_t *vhfTimer);
bool txAprsIRQ(repeating_timer_t *rt);
bool txAprs(void);
void startAPRS(const aprs_config_s *aprs_cfg, repeating_timer_t *aprsTimer);
void startTag(const tag_config_s *tag_cfg, repeating_timer_t *tagTimer);
bool txTag(repeating_timer_t *rt);
void initAll(const gps_config_s *gps_cfg, const tag_config_s *tag_cfg);

/** @struct Defines unchanging configuration parameters for APRS.
 * callsign, ssid
 * dest, digi, digissid, comment
 * interval, debug, debug style
 */
const aprs_config_s aprs_config = {CALLSIGN,        SSID, "APLIGA", "WIDE2", 1,
                                   "Ceti b1.2 4-S", 5000, false,    2};

/** @struct Defines unchanging configuration parameters for GPS communication.
 * GPS_TX, GPS_RX, GPS_BAUD, UART_NUM, QINTERACTIVE
 */
const gps_config_s gps_config = {0, 1, 9600, uart0, false};
gps_data_s gps_data = {
    {DEFAULT_LAT, DEFAULT_LON}, {0, 0, 0}, "$GN 15.31383, -61.30075,0,360,0*19",
    "$DT 20190408195123,V*41",  false,     false};

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

// bool txVHF(repeating_timer_t *rt) {
//     printf("blast time\n");
//     vhf_pulse_callback();
//     gps_get_lock(&gps_config, &gps_data);
//     return !gps_data.posCheck;
// }

// APRS //

void startAPRS(const aprs_config_s *aprs_cfg, repeating_timer_t *aprsTimer) {
    // configureAPRS_TX(DEFAULT_FREQ);
    add_repeating_timer_ms(-aprs_cfg->interval, txAprsIRQ, NULL, aprsTimer);
}

bool txAprsIRQ(repeating_timer_t *rt) { return txAprs(); }

bool txAprs(void) {
    // wake up the VHF since we're going to transmit APRS
    // gps_get_lock(&gps_config, &gps_data);
    if (aprs_config.debug)
        sendTestPackets(&aprs_config);
    else if (gps_data.posCheck) {
        printf("[APRS TX] %s-%d @ %.6f, %.6f\n", CALLSIGN, SSID,
               gps_data.latlon[0], gps_data.latlon[1]);
        setLed(true);
        sendPacket(&aprs_config, gps_data.latlon, gps_data.acs);
        setLed(false);
    } else {
        printf("[APRS TX] no valid position\n");
    }

    return gps_data.posCheck;
}

void startTag(const tag_config_s *tag_cfg, repeating_timer_t *tagTimer) {
    // printf("[TAG INIT]\n");
    add_repeating_timer_ms(-tag_cfg->interval, txTag, NULL, tagTimer);
}

bool txTag(repeating_timer_t *rt) {
    // getLastPDtBufs(lastGpsUpdate, lastDtUpdate);
    writeGpsToTag(&tag_config, gps_data.lastGpsBuffer, gps_data.lastDtBuffer);
    // detachTag();
    // reqTagState(&tag_config);
    return true;
}

void initAll(const gps_config_s *gps_cfg, const tag_config_s *tag_cfg) {
    set_bin_desc();
    stdio_init_all();

    initLed();
    setLed(true);

    uint32_t gpsBaud = gpsInit(gps_cfg);

    // Initialize APRS and the VHF
    initializeAPRS();

    uint32_t tagBaud = initTagComm(tag_cfg);
    setLed(false);
}

int main() {
    // Setup
    initAll(&gps_config, &tag_config);
    printf("[MAIN INIT] %s-%d @ %.2f, %.2f\n", CALLSIGN, SSID,
           gps_data.latlon[0], gps_data.latlon[1]);

    // // APRS comms interrupt setup
    // repeating_timer_t aprsTimer;
    // startAPRS(&aprs_config, &aprsTimer);

// Tag comms interrupt setup
#if TAG_CONNECTED
    repeating_timer_t tagTimer;
    startTag(&tag_config, &tagTimer);
#endif

    wakeVHF();  // wake here so there's enough time to fully wake up

    sleep_ms(VHF_WAKE_TIME_MS);

    // Loop
    while (true) {
        gps_get_lock(&gps_config, &gps_data);
        txAprs();

        sleepVHF();

        sleep_ms((aprs_config.interval > VHF_WAKE_TIME_MS)
                     ? (aprs_config.interval - VHF_WAKE_TIME_MS)
                     : VHF_WAKE_TIME_MS);

        wakeVHF();  // wake here so there's enough time to fully wake up

        sleep_ms(VHF_WAKE_TIME_MS);
    }
    return 0;
}
