/** @file main.c
 * @author Shashank Swaminathan
 * Core file for the recovery board. Defines all configurations, initializes all
 * modules, and runs appropriate timers/interrupts based on received data.
 */
#include "aprs.h"
#include "constants.h"
#include "gps.h"
#include "hardware/clocks.h"
#include "hardware/rosc.h"
#include "hardware/structs/scb.h"
#include "pico/binary_info.h"
#include "pico/stdlib.h"
#include "pico/time.h"
#include "sleep.h"
#include "tag.h"
#include "ublox-config.h"
#include "vhf.h"
#include <stdio.h>
#include <stdlib.h>

#define APRS_TESTING 1
#define TAG_CONNECTED 0
#define FLOATER 0
static bool deep_sleep = false;
#if TAG_CONNECTED
#define APRS_RETRANSMIT 3
#define APRS_DELAY 15000
#else
#define APRS_RETRANSMIT 1
#define APRS_DELAY 3000
#endif

/** @struct Defines unchanging configuration parameters for APRS.
 * callsign, ssid
 * dest, digi, digissid, comment
 * interval, debug, debug style
 */
static aprs_config_s aprs_config = {CALLSIGN,        SSID,  "APRS", "WIDE2", 2,
                                    "CETI-TagHeard", 40000, false,  2};

const aprs_config_s tag_aprs_config = {
    CALLSIGN, SSID, "APLIGA", "WIDE2-", 1, "Ceti b1.2 4-S", 120000, false, 2};

const aprs_config_s floater_aprs_config = {
    CALLSIGN, SSID, "APLIGA", "WIDE2", 2, "Ceti b1.2 4-S", 0, false, 2};

const aprs_config_s testing_aprs_config = {CALLSIGN,     SSID,  "APLIGA",
                                           "WIDE2",      1,     "Ceti b1.2 4-S",
                                           TESTING_TIME, false, 2};

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

// struct clock_config_t {
//     uint scb_orig;
//     uint clock0_orig;
//     uint clock1_orig;
// } clock_config;

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
// void gps_callback();
// static void rtc_sleep(uint32_t sleepTime);
// void recover_from_sleep();
// static void sleepCallback();
// float getVin();
// void startupBroadcasting();
void pauseForLock();

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

// void recover_from_sleep() {
//     printf("Recover from sleep \n");

//     // Re-enable ring Oscillator control
//     rosc_write(&rosc_hw->ctrl, ROSC_CTRL_ENABLE_BITS);
//     // // reset procs back to default
//     scb_hw->scr = clock_config.scb_orig;
//     clocks_hw->sleep_en0 = clock_config.clock0_orig;
//     clocks_hw->sleep_en1 = clock_config.clock1_orig;
//     // // printf("Recover step 2 \n");

//     // // reset clocks
//     clocks_init();

//     set_bin_desc();

//     stdio_init_all();
//     // printf("Recover step 3 \n");

//     initLed();
//     gpsInit(&gps_config);
//     initializeAPRS();
//     wakeVHF();
//     sleep_ms(1000);
//     printf("Recovered from sleep \n");

//     return;
// }

// static void sleep_callback() { printf("Waking from sleep\n"); }

// static void rtc_sleep(uint32_t sleepTime) {
//     // Do calculations first
//     // datetime_t end = gps_data.dt;

//     // uint16_t hours = sleepTime / (1000 * 60 * 60);
//     // uint16_t minutes = (sleepTime % (1000 * 60 * 60)) / (1000 * 60);
//     // uint16_t seconds = (sleepTime % (1000 * 60 * 60)) % (1000 * 60) /
//     1000;

//     // // printf("sleeping for %d ms:  %d hours and %d minutes and %d
//     // seconds\n",
//     // //        sleepTime, hours, minutes, seconds);
//     // if (end.sec + seconds >= 60) {
//     //     end.sec = end.sec + seconds - 60;
//     //     end.min += 1;
//     // } else {
//     //     end.sec += seconds;
//     // }
//     // if (end.min + minutes >= 60) {
//     //     end.min = end.min + minutes - 60;
//     //     end.hour += 1;
//     // } else {
//     //     end.min += minutes;
//     // }
//     // if (end.hour + hours >= 24) {
//     //     end.hour = end.hour + hours - 24;
//     //     end.day += 1;
//     // } else {
//     //     end.sec += hours;
//     // }
//     // // todo do i need to do month rollover?
//     // printf("starting %d: %d-%d-%d %d:%d:%d \tending %d: %d-%d-%d
//     %d:%d:%d\n",
//     //        start->dotw, start->year, start->month, start->day,
//     start->hour,
//     //        start->min, start->sec, end.dotw, end.year, end.month, end.day,
//     //        end.hour, end.min, end.sec);
//     // transmission!
//     txAprs();
//     // sleepy time
//     sleepVHF();
//     calculateUBXChecksum(16, day_sleep);
//     writeSingleConfiguration(gps_config.uart, day_sleep, 16);
//     printf("Sleeping....%d ms\n", sleepTime);
//     rec_sleep_run_from_xosc();

//     rec_sleep_goto_dormant_until_edge_high(1);
// }

// APRS //
// void startupBroadcasting() {
//     // printf("Initial APRS broadcasting\n");
//     for (int i = 0; i < 10; i++) {
//         //  wakeVHF();
//         gps_get_lock(&gps_config, &gps_data, 1000);
//         if (gps_data.posCheck) {
//             struct gps_lat_lon_t latlon;
//             getBestLatLon(&gps_data, &latlon);
//             printf("[APRS TX:%s] %s-%d @ %.6f, %.6f \n", latlon.type,
//             CALLSIGN,
//                    SSID, latlon.lat, latlon.lon);
//             // printPacket(&aprs_config);

//             setLed(true);
//             sendPacket(&aprs_config, (float[]){latlon.lat, latlon.lon},
//                        gps_data.acs);
//             setLed(false);

//             sleep_ms(APRS_DELAY);
//         }

//         sleep_ms(60000 - APRS_DELAY);
//     }
// }

void pauseForLock(void) { gps_get_lock(&gps_config, &gps_data, 300000); }

void startAPRS(const aprs_config_s *aprs_cfg, repeating_timer_t *aprsTimer) {
    configureAPRS_TX(DEFAULT_FREQ);
    add_repeating_timer_ms(-aprs_cfg->interval, txAprsIRQ, NULL, aprsTimer);
}

bool txAprsIRQ(repeating_timer_t *rt) { return txAprs(); }

bool txAprs(void) {
    // gps_get_lock(&gps_config, &gps_data);

    // wake up the VHF since we're going to transmit APRS
    if (aprs_config.debug)
        sendTestPackets(&aprs_config);
    else if (gps_data.posCheck) {
        struct gps_lat_lon_t latlon;
        getBestLatLon(&gps_data, &latlon);
        printf("[APRS TX:%s] %s-%d @ %.6f, %.6f \n", latlon.type, CALLSIGN,
               SSID, latlon.lat, latlon.lon);
        // printPacket(&aprs_config);

        // for (uint8_t retrans = 0; retrans < APRS_RETRANSMIT; retrans++) {
        setLed(true);
        sendPacket(&aprs_config, (float[]){latlon.lat, latlon.lon},
                   gps_data.acs);
        setLed(false);

        sleep_ms(APRS_DELAY);
        // }
    }

    // return gps_data.posCheck;
}

void startTag(const tag_config_s *tag_cfg, repeating_timer_t *tagTimer) {
    // printf("[TAG INIT]\n");
    add_repeating_timer_ms(-tag_cfg->interval, txTag, NULL, tagTimer);
}

bool txTag(repeating_timer_t *rt) {
    gps_get_lock(&gps_config, &gps_data, 1000);
    struct gps_lat_lon_t latlon;
    getBestLatLon(&gps_data, &latlon);
    writeGpsToTag(&tag_config, latlon.msg, gps_data.lastDtBuffer);
    return true;
}

// float getVin(){
//     float maxAdcVal, adcRefVolatge = 0;
//     uint16_t adcVal = analogRead(vinPin);
//     float vinVal = float(adcVal) / maxAdcVal * adcRefVoltage;
//     float voltage = vinVal * (r1 + r2) / r2;
//     return voltage;
//     }

// void gps_callback() { gps_get_lock(&gps_config, &gps_data, 1000); }

void initAll(const gps_config_s *gps_cfg, const tag_config_s *tag_cfg) {
    set_bin_desc();
    stdio_init_all();
    // Start the RTC
    // rtc_init();

    initLed();
    setLed(true);

    uint32_t gpsBaud = gpsInit(gps_cfg);

    // Initialize APRS and the VHF
    initializeAPRS();
    // wakeVHF();  // wake here so there's enough time to fully wake up
    // sleep_ms(VHF_WAKE_TIME_MS * 2);

    uint32_t tagBaud = initTagComm(tag_cfg);
    setLed(false);
    // srand(get_absolute_time());

    // gpio_init(VHF_PIN);
    // gpio_set_dir(VHF_PIN, GPIO_OUT);
}

int main() {
    // Setup
    initAll(&gps_config, &tag_config);
    printf("[MAIN INIT] %s-%d \n", CALLSIGN, SSID);

    // clock_config.scb_orig = scb_hw->scr;
    // clock_config.clock0_orig = clocks_hw->sleep_en0;
    // clock_config.clock1_orig = clocks_hw->sleep_en1;

    // // APRS comms interrupt setup
    // repeating_timer_t aprsTimer;
    // startAPRS(&aprs_config, &aprsTimer);

    // int gpsIRQ = gps_config.uart == uart0 ? UART0_IRQ : UART1_IRQ;

    // // And set up and enable the interrupt handlers
    // irq_set_exclusive_handler(gpsIRQ, gps_callback);
    // irq_set_enabled(gpsIRQ, true);

    // // // Now enable the UART to send interrupts - RX only
    // uart_set_irq_enables(gps_config.uart, true, false);

    uint32_t variance = 0;
    int32_t rand_modifier = 0;
    // Tag comms interrupt setup
    // #if TAG_CONNECTED
    //     aprs_config = tag_aprs_config;
    //     startupBroadcasting();
    //     repeating_timer_t tagTimer;
    //     startTag(&tag_config, &tagTimer);
    //     variance = TAG_VARIANCE;
    // #elif FLOATER
    //     aprs_config = floater_aprs_config;
    //     variance = FLOATER_VARIANCE;
    // #elif APRS_TESTING
    //     aprs_config = testing_aprs_config;
    variance = TESTING_VARIANCE;
    // pauseForLock();
    // #endif

    // Loop
    while (true) {
#if TAG_CONNECTED || APRS_TESTING
        rand_modifier = rand() % variance;
		rand_modifier =
            rand_modifier <= (variance / 2) ? rand_modifier : -rand_modifier;
        // printf("Testing aprs\n");
        gps_get_lock(&gps_config, &gps_data, 1000);
        gps_data.latlon[0][0] = 42.3648 + ((float)(rand_modifier % 100) / 10000);
        gps_data.latlon[0][1] = -71.1247 + ((float)(rand_modifier % 100) / 10000);
        gps_data.gpsReadFlags[0] = 1;

        txAprs();

        // sleep_ms(aprs_config.interval);
        // sleepVHF();

        
        // sleep_ms(aprs_config.interval + rand_modifier);
        // printf("Sleeping for %d milliseconds\n",
        //        rand_modifier + aprs_config.interval);
        sleep_ms(
            ((aprs_config.interval + rand_modifier) > VHF_WAKE_TIME_MS)
                ? ((aprs_config.interval + rand_modifier) - VHF_WAKE_TIME_MS)
                : VHF_WAKE_TIME_MS);

        wakeVHF();  // wake here so there's enough time to fully wake up

        sleep_ms(VHF_WAKE_TIME_MS);
#elif FLOATER
        // Floater deep sleep shutoff
        while (deep_sleep) {
            // irq_remove_handler(gpsIRQ, gps_callback);
            uart_deinit(uart0);
            uart_deinit(uart1);
            sleepVHF();
            calculateUBXChecksum(16, sleep);
            writeSingleConfiguration(gps_config.uart, sleep, 16);
            // Sets the dormant clock to the crystal oscillator
            rec_sleep_run_from_xosc();
            rec_sleep_goto_dormant_until_edge_high(8);
        }

        gps_get_lock(&gps_config, &gps_data);

        rtc_sleep(DAY_SLEEP);
        recover_from_sleep();

        if (gps_data.inDominica && gps_data.posCheck) {
            // printf("[DOMINICA] ");

            printf("DT: %d %d %d\n", gps_data.dt.hour, gps_data.dt.min,
                   gps_data.dt.sec);
            if (gps_data.dt.hour > NIGHT_DAY_ROLLOVER &&
                gps_data.dt.hour < DAY_NIGHT_ROLLOVER) {
                printf("[DAYTIME]\n");
                rtc_sleep(DAY_SLEEP);
                recover_from_sleep();

                // wakeVHF();
                // printf("Here\n");
                // sleep_ms(1000);
            } else {
                printf("[NIGHTTIME]\n");
                rtc_sleep(NIGHT_SLEEP);
                recover_from_sleep();
            }
        } else if (gps_data.posCheck && gps_data.dtCheck) {
            printf("[NOT DOMINICA]\n");

            rtc_sleep(GEOFENCE_SLEEP);
            recover_from_sleep();

        } else {
            // printf("Waiting...\n");
            // sleep_ms(2000);
        }
        // wakeVHF();
#endif
    }
    return 0;
}
