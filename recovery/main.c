/** @file main.c
 * @author Shashank Swaminathan
 * Core file for the recovery board. Defines all configurations, initializes all
 * modules, and runs appropriate timers/interrupts based on received data.
 */
#include "../generated/generated.h"
#include "aprs/aprs.h"
#include "aprs/vhf.h"
#include "constants.h"
#include "gps/gps.h"
#include "gps/ublox-config.h"
#include "hardware/clocks.h"
#include "hardware/regs/io_bank0.h"
#include "hardware/rosc.h"
#include "hardware/structs/scb.h"
#include "pico/binary_info.h"
#include "pico/sleep.h"
#include "pico/stdlib.h"
#include "pico/time.h"
#include "tag.h"
#include <stdio.h>
#include <stdlib.h>

// #ifdef DEBUG
// # define DEBUG_PRINT(x) printf x
// #else
// # define DEBUG_PRINT(x) do {} while (0)
// #endif

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

struct clock_config_t {
    uint scb_orig;
    uint clock0_orig;
    uint clock1_orig;
} clock_config;

void initAll(const gps_config_s *gps_cfg, const tag_config_s *tag_cfg);
void set_bin_desc(void);
void setLed(bool state);
void initLed(void);
bool txAprs(void);
void startTag(const tag_config_s *tag_cfg, repeating_timer_t *tagTimer);
bool txTag(repeating_timer_t *rt);
static void rtc_sleep(uint32_t sleep_time);
void recover_from_sleep();
float getVin();
void startupBroadcasting();

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

void recover_from_sleep() {
    // uint32_t event = IO_BANK0_DORMANT_WAKE_INTE0_GPIO0_EDGE_HIGH_BITS;
    // // Clear the irq so we can go back to dormant mode again if we want
    // gpio_acknowledge_irq(6, event);
    printf("Recovering...\n");

    // Re-enable ring Oscillator control
    rosc_write(&rosc_hw->ctrl, ROSC_CTRL_ENABLE_BITS);
    // // reset procs back to default
    scb_hw->scr = clock_config.scb_orig;
    clocks_hw->sleep_en0 = clock_config.clock0_orig;
    clocks_hw->sleep_en1 = clock_config.clock1_orig;
    // // printf("Recover step 2 \n");

    // // reset clocks
    clocks_init();

    set_bin_desc();

    stdio_init_all();

    initLed();
    setLed(true);  
    printf("Re-init GPS and APRS \n");
  
    // gpsInit(&gps_config);
    // initializeAPRS();
    // wakeVHF();
    // sleep_ms(1000);
    printf("Recovered from sleep \n");
    setLed(false);
    return;
}

static void rtc_sleep(uint32_t sleep_time) {
    ubx_cfg_t sleep[16];
    createUBXSleepPacket(sleep_time, sleep);
    writeSingleConfiguration(gps_config.uart, sleep, 16);
    // uart_deinit(uart0);

    // sleep_ms(100);
    sleep_run_from_xosc();

    datetime_t t = {
            .year  = 2020,
            .month = 06,
            .day   = 05,
            .dotw  = 5, // 0 is Sunday, so 5 is Friday
            .hour  = 1,
            .min   = 01,
            .sec   = 00
    };

    // Alarm 10 seconds later
    datetime_t t_alarm = {
            .year  = 2020,
            .month = 06,
            .day   = 05,
            .dotw  = 5, // 0 is Sunday, so 5 is Friday
            .hour  = 1,
            .min   = 01,
            .sec   = 10
    };

    // Start the RTC
    rtc_init();
    rtc_set_datetime(&t);

    // uart_default_tx_wait_blocking();

    sleep_goto_sleep_until(&t_alarm, &recover_from_sleep);
    // sleep_goto_dormant_until_edge_high(6);
}

// APRS //
void startupBroadcasting() {
    printf("[BROADCAST] Starting broadcast, waiting for gps lock\n");
    gps_get_lock(&gps_config, &gps_data, startup_.gps_wait);

    for (int i = 0; i < startup_.transmissions; i++) {
        gps_get_lock(&gps_config, &gps_data, 1000);
        txAprs();
        sleep_ms(aprs_timing_startup);
    }
    printf("[BROADCAST] Ending broadcast\n");
}

bool txAprs(void) {
    if (gps_data.posCheck) {
        struct gps_lat_lon_t latlon;
        getBestLatLon(&gps_data, &latlon);
        printf("[APRS TX:%s] %s-%d @ %.6f, %.6f \n", latlon.type,
               aprs_config.callsign, aprs_config.ssid, latlon.lat, latlon.lon);
        setLed(true);
        sendPacket(&aprs_config, (float[]){latlon.lat, latlon.lon},
                   gps_data.acs);
        setLed(false);

        sleep_ms(aprs_timing_delay);
    }

    return gps_data.posCheck;
}

void startTag(const tag_config_s *tag_cfg, repeating_timer_t *tagTimer) {
    printf("[TAG TX INIT]\n");
    add_repeating_timer_ms(-tag_cfg->interval, txTag, NULL, tagTimer);
}

bool txTag(repeating_timer_t *rt) {
    gps_get_lock(&gps_config, &gps_data, 1000);
    struct gps_lat_lon_t latlon;
    getBestLatLon(&gps_data, &latlon);
    writeGpsToTag(&tag_config, latlon.msg, gps_data.lastDtBuffer);
    if (gps_data.posCheck) {
        setLed(true);
        sendPacket(&aprs_config, (float[]){latlon.lat, latlon.lon},
                   gps_data.acs);

        setLed(false);
    }
    return true;
}

float getVin() {
    uint16_t r1 = 10000;
    uint16_t r2 = 10000;
    float maxAdcVal = pow(2, 13) - 1; 
    float adcRefVoltage = 3.3;
    uint16_t adcVal = analogRead(26);
    float vinVal = (((float)adcVal) / maxAdcVal) * adcRefVoltage;
    float voltage = vinVal * (r1 + r2) / r2;
    return voltage;
}

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

    uint32_t tagBaud = initTagComm(tag_cfg);
    setLed(false);
    srand(to_ms_since_boot(get_absolute_time()));
}

int main() {
    // Setup
    initAll(&gps_config, &tag_config);
    printf("[MAIN INIT] %s-%d \n", aprs_config.callsign, aprs_config.ssid);
    clock_config.scb_orig = scb_hw->scr;
    clock_config.clock0_orig = clocks_hw->sleep_en0;
    clock_config.clock1_orig = clocks_hw->sleep_en1;

    // setLed(true);
   
    // wakeVHF();

    // setPttState(true);

    // while(true) {
    //     // nothing
    // }

    enum SleepType sleep_type;
    if (timing_.day_trigger > 0 && timing_.night_trigger > 0 &&
        timing_.day_interval > 0 && timing_.night_interval > 0) {
        sleep_type = DAY_NIGHT_SLEEP;
    } else {
        sleep_type = BUSY_SLEEP;
    }

    // APRS STARTUP
    if (startup_.flag) {
        startupBroadcasting();
    }

    // TAG INITIALIZATION
    // Start tag transmission is the board type is a tag, or if the dev
    // simulation has tag tx enabled.
    if (board_ == TAG || simulation_.tag_tx) {
        repeating_timer_t tagTimer;
        startTag(&tag_config, &tagTimer);
    }

    // Trackers for various things during the main loop
    // Includes the random modifier, successful tx transmission flag, and
    // counter for attempts at TX
    int32_t rand_modifier = 0;
    bool successfulTX = false;
    uint8_t txAttempt = 0;
    // Loop
    while (true) {
        // calculate a random value between 0 and the variance to add/subtract
        // to the aprs timing. This ensures that aprs messages aren't being sent
        // at the same time by accident
        rand_modifier = rand() % aprs_config.variance;
        rand_modifier = rand_modifier <= (aprs_config.variance / 2)
                            ? rand_modifier
                            : -rand_modifier;
        // Get the GPS lock for this cycle
        gps_get_lock(&gps_config, &gps_data, 1000);

        // if we want to simulation gps coordinates, then seed them with the
        // simulation values. Also check to see if the coordinates should
        // randomly move around or not
        if (simulation_.latlon[0] > 0 && simulation_.latlon[1] > 0) {
            float sim_gps_variance = 0;
            if (simulation_.sim_move) {
                sim_gps_variance = ((float)(rand_modifier % 100) / 10000);
                sim_gps_variance = (((uint32_t)sim_gps_variance * 10) % 2)
                                       ? sim_gps_variance
                                       : -sim_gps_variance;
            }

            gps_data.latlon[GPS_SIM][0] =
                simulation_.latlon[0] + sim_gps_variance;
            gps_data.latlon[GPS_SIM][1] =
                simulation_.latlon[1] + sim_gps_variance;
            gps_data.gpsReadFlags[GPS_SIM] = 1;  // todo change this to sim one
            gps_data.posCheck = true;
        }

        // Check to see the current battery voltage and if it needs to go to
        // sleep
        if (timing_.deep_sleep && getVin() < VIN_SLEEP_LIMIT) {
            sleep_type = DEAD_SLEEP;
        }

        // Sending aprs tx when not connected to a tag
        // Basically send if: floater, or dev board with aprs enabled
        if (board_ == FLOATER || (board_ == DEV && simulation_.aprs_tx) ||
            board_ == TAG) {
            // APRS won't send if there's a valid gps/position lock
            // Try to send the APRS message the max number of attempts, before
            // trying to sleep If after 5 attempts there's still no position
            // lock, and no APRS transmission, go into the sleep pattern and try
            // again when woken up
            successfulTX = txAprs();
            if (!successfulTX && txAttempt <= MAX_APRS_TX_ATTEMPTS) {
                txAttempt++;
                continue;
            }

            switch (sleep_type) {
                case BUSY_SLEEP:
                    printf("[BUSY SLEEP]\n");
                    sleep_ms(((aprs_config.interval + rand_modifier) >
                              VHF_WAKE_TIME_MS)
                                 ? ((aprs_config.interval + rand_modifier) -
                                    VHF_WAKE_TIME_MS)
                                 : VHF_WAKE_TIME_MS);

                    wakeVHF();  // wake here so there's enough time to fully
                                // wake up

                    sleep_ms(VHF_WAKE_TIME_MS);
                    break;
                case DAY_NIGHT_SLEEP:
                    printf("[DAY/NIGHT SLEEP]\n");

                    if (gps_data.timestamp[0] > timing_.day_trigger &&
                        gps_data.timestamp[0] < timing_.night_trigger) {
                        printf("[DAYTIME]: %d:%d:%d sleeping for %d\n", gps_data.timestamp[0],
                               gps_data.timestamp[1], gps_data.timestamp[2], timing_.day_interval);
                        rtc_sleep(timing_.day_interval);
                        // recover_from_sleep();
                    } else {
                        printf("[NIGHTTIME]: %d:%d:%d sleeping for %d\n", gps_data.timestamp[0],
                               gps_data.timestamp[1], gps_data.timestamp[2], timing_.night_interval);
                        rtc_sleep(timing_.night_interval);
                        // recover_from_sleep();
                    }
                    break;
                case DEAD_SLEEP:
                    printf("[DEAD SLEEP]\n");
                    sleepVHF();
                    writeSingleConfiguration(gps_config.uart, sleep_temp, 16);
                    // Sets the dormant clock to the crystal oscillator
                    sleep_run_from_xosc();
                    // unused GPIO pin 8
                    sleep_goto_dormant_until_edge_high(8);
                    break;
                default:
                    printf("[NO SLEEP]\n");
                    break;
                    // no sleep
            }
            // Reset trackers and flags
            successfulTX = false;
            txAttempt = 0;
        }
    }
    return 0;
}
