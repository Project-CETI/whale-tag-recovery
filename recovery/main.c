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
#include "sleep.h"
#include "tag.h"
#include "vhf.h"
#include <stdio.h>
#include <stdlib.h>
#include "ublox-config.h"


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
void gps_callback();

/** @struct Defines unchanging configuration parameters for APRS.
 * callsign, ssid
 * dest, digi, digissid, comment
 * interval, debug, debug style
 */
const aprs_config_s aprs_config = {
    CALLSIGN, SSID, "APLIGA", "WIDE2", 2, "Ceti b1.2 4-S", 10000, false, 2};

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

// APRS //

void startAPRS(const aprs_config_s *aprs_cfg, repeating_timer_t *aprsTimer) {
    // configureAPRS_TX(DEFAULT_FREQ);
    add_repeating_timer_ms(-aprs_cfg->interval, txAprsIRQ, NULL, aprsTimer);
}

bool txAprsIRQ(repeating_timer_t *rt) { return txAprs(); }

bool txAprs(void) {
    gps_get_lock(&gps_config, &gps_data);

    // wake up the VHF since we're going to transmit APRS
    if (aprs_config.debug)
        sendTestPackets(&aprs_config);
    else if (gps_data.posCheck) {
        struct gps_lat_lon_t latlon;
        getBestLatLon(&gps_data, &latlon);
        printf("[APRS TX:%s] %s-%d @ %.6f, %.6f \n", latlon.type, CALLSIGN,
               SSID, latlon.lat, latlon.lon);

        for (uint8_t retrans = 0; retrans < APRS_RETRANSMIT; retrans++) {
            setLed(true);
            sendPacket(&aprs_config, (float []){latlon.lat, latlon.lon}, gps_data.acs);
            setLed(false);
            sleep_ms(3000);
        }
    }

    return gps_data.posCheck;
}

void startTag(const tag_config_s *tag_cfg, repeating_timer_t *tagTimer) {
    // printf("[TAG INIT]\n");
    add_repeating_timer_ms(-tag_cfg->interval, txTag, NULL, tagTimer);
}

bool txTag(repeating_timer_t *rt) {
    gps_get_lock(&gps_config, &gps_data);
    struct gps_lat_lon_t latlon;
    getBestLatLon(&gps_data, &latlon);
    writeGpsToTag(&tag_config, latlon.msg, gps_data.lastDtBuffer);
    return true;
}

void gps_callback() {
    gps_get_lock(&gps_config, &gps_data);
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
    srand(get_absolute_time());
}

int main() {
    // Setup
    initAll(&gps_config, &tag_config);
    printf("[MAIN INIT] %s-%d \n", CALLSIGN, SSID);

    // // APRS comms interrupt setup
    // repeating_timer_t aprsTimer;
    // startAPRS(&aprs_config, &aprsTimer);

    int gpsIRQ = gps_config.uart == uart0 ? UART0_IRQ : UART1_IRQ;

    // // And set up and enable the interrupt handlers
    irq_set_exclusive_handler(gpsIRQ, gps_callback);
    irq_set_enabled(gpsIRQ, true);

    // // Now enable the UART to send interrupts - RX only
    uart_set_irq_enables(gps_config.uart, true, false);

    int32_t rand_modifier = 0;
// Tag comms interrupt setup
#if TAG_CONNECTED
    repeating_timer_t tagTimer;
    startTag(&tag_config, &tagTimer);
    rand_modifier = rand() % 60000;
#endif

    // wakeVHF();  // wake here so there's enough time to fully wake up

    // sleep_ms(VHF_WAKE_TIME_MS);

    // Loop
    while (true) {
        while (deep_sleep) {
            irq_remove_handler(gpsIRQ,gps_callback);
            uart_deinit(uart0);
            uart_deinit(uart1);
            sleepVHF();
            calculateUBXChecksum(16, sleep);
            writeSingleConfiguration(gps_config.uart, sleep, 16);
            rec_sleep_run_from_xosc();
            rec_sleep_goto_dormant_until_edge_high(8);
        }
#if TAG_CONNECTED
        txAprs();
        sleepVHF();
        rand_modifier = rand_modifier <= 30000 ? rand_modifier : -rand_modifier;
        sleep_ms(
            ((aprs_config.interval + rand_modifier) > VHF_WAKE_TIME_MS)
                ? ((aprs_config.interval + rand_modifier) - VHF_WAKE_TIME_MS)
                : VHF_WAKE_TIME_MS);

        wakeVHF();  // wake here so there's enough time to fully wake up

        sleep_ms(VHF_WAKE_TIME_MS);
#elif FLOATER
        
#endif
    }
    return 0;
}
