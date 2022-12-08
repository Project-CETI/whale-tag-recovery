/** @file main.c
 * @author Shashank Swaminathan
 * Core file for the recovery board. Defines all configurations, initializes all
 * modules, and runs appropriate timers/interrupts based on received data.
 */

#include "../generated/generated.h"
#include "aprs/aprs.h"
#include "aprs/aprs_sleep.h"
#include "aprs/vhf.h"
#include "constants.h"
#include "gps/gps.h"
#include "gps/ublox-config.h"
#include "hardware/clocks.h"
#include "hardware/rosc.h"
#include "hardware/structs/scb.h"
#include "pico/sleep.h"
#include "pico/stdlib.h"
#include "pico/time.h"
#include "tag.h"
#include <math.h>
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
const gps_config_s gps_config = {GPS_TX_PIN, GPS_RX_PIN, 9600, uart0, false};

gps_data_s gps_data = {
    {DEFAULT_LAT, DEFAULT_LON}, {0, 0, 0}, "$GN 15.31383, -61.30075,0,360,0*19",
    "$DT 20190408195123,V*41",  false,     false};

/** @struct Defines unchanging configuration parameters for tag communication.
 * TAG_TX, TAG_RX, TAG_BAUD, UART_NUM, SEND_INTERVAL, ACK_WAIT_TIME_US,
 * ACK_WAIT_TIME_MS, NUM_TRIES
 */
const tag_config_s tag_config = {TAG_TX_PIN, TAG_RX_PIN, 115200, uart1,
                                 10000,      1000,       1000,   3};

void initAll(const gps_config_s *gps_cfg, const tag_config_s *tag_cfg);
bool txAprs(void);
void startTag(const tag_config_s *tag_cfg, repeating_timer_t *tagTimer);
bool txTag(repeating_timer_t *rt);
float getVin();
void startupBroadcasting();

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
        setLed(APRS_LED_PIN, true);
        sendPacket(&aprs_config, (float[]){latlon.lat, latlon.lon},
                   gps_data.acs);
        setLed(APRS_LED_PIN, false);

        // sleep_ms(aprs_timing_delay);
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
        setLed(APRS_LED_PIN, true);
        sendPacket(&aprs_config, (float[]){latlon.lat, latlon.lon},
                   gps_data.acs);

        setLed(APRS_LED_PIN, false);
    }
    return true;
}

float getVin() {
    // uint16_t r1 = 10000;
    // uint16_t r2 = 10000;
    // float maxAdcVal = pow(2, 13) - 1;
    // float adcRefVoltage = 3.3;
    // uint16_t adcVal = analogRead(26);
    // float vinVal = (((float)adcVal) / maxAdcVal) * adcRefVoltage;
    // float voltage = vinVal * (r1 + r2) / r2;
    // return voltage;
    return 3.3;  // need to fix this to initialize the adc correctly
}

void initAll(const gps_config_s *gps_cfg, const tag_config_s *tag_cfg) {
    set_bin_desc();
    stdio_init_all();
    // Start the RTC
    // rtc_init();

    initLed(APRS_LED_PIN);
    setLed(APRS_LED_PIN, true);

    uint32_t gpsBaud = gpsInit(gps_cfg);

    // Initialize APRS and the VHF
    initializeAPRS();

    uint32_t tagBaud = initTagComm(tag_cfg);
    setLed(APRS_LED_PIN, false);
    srand(to_ms_since_boot(get_absolute_time()));

    remaskDAC();
    printf("DAC REMASKING\n");
    for (uint8_t index = 0; index < NUM_SINES; index++) {
        printf("\t%d -> %d\n", sineValues[index], remaskedValues[index]);
    }
}

int main() {
    // Setup
    initAll(&gps_config, &tag_config);
    printf("[MAIN INIT] %s-%d \n", aprs_config.callsign, aprs_config.ssid);
    clock_config.scb_orig = scb_hw->scr;
    clock_config.clock0_orig = clocks_hw->sleep_en0;
    clock_config.clock1_orig = clocks_hw->sleep_en1;

    // setLed(true);

    // wakeVstState(true);

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
        if (fabs(simulation_.latlon[0]) > 0 &&
            fabs(simulation_.latlon[1]) > 0) {
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
                case BUSY_SLEEP:;
                    uint32_t sleep_time =
                        (((aprs_config.interval + rand_modifier) >
                          VHF_WAKE_TIME_MS)
                             ? ((aprs_config.interval + rand_modifier) -
                                VHF_WAKE_TIME_MS)
                             : VHF_WAKE_TIME_MS);
                    printf("[BUSY SLEEP] %d ms\n", sleep_time);
                    sleep_ms(sleep_time);

                    wakeVHF();  // wake here so there's enough time to fully
                                // wake up

                    sleep_ms(VHF_WAKE_TIME_MS);
                    break;
                case DAY_NIGHT_SLEEP:
                    printf("[DAY/NIGHT SLEEP]\n");

                    if (gps_data.timestamp[0] > timing_.day_trigger &&
                        gps_data.timestamp[0] < timing_.night_trigger) {
                        printf("[DAYTIME]: %d:%d:%d sleeping for %d\n",
                               gps_data.timestamp[0], gps_data.timestamp[1],
                               gps_data.timestamp[2], timing_.day_interval);

                        // Interval given in seconds
                        sleep_ms(timing_.day_interval * 100);

                        // Unused for now until the dormant issue gets fixes
                        // rtc_sleep(timing_.day_interval);
                        // recover_from_sleep();
                    } else {
                        printf("[NIGHTTIME]: %d:%d:%d sleeping for %d\n",
                               gps_data.timestamp[0], gps_data.timestamp[1],
                               gps_data.timestamp[2], timing_.night_interval);
                        // Interval given in seconds
                        sleep_ms(timing_.night_interval * 100);

                        // Unused for now until the dormant issues gets fixed
                        // rtc_sleep(timing_.night_interval);
                        // recover_from_sleep();
                    }
                    break;
                case DEAD_SLEEP:
                    printf("[DEAD SLEEP]\n");
                    sleepVHF();
                    // Sleep the GPS forever as well
                    writeSingleConfiguration(gps_config.uart, sleep_temp, 16);
                    // Sets the dormant clock to the crystal oscillator
                    sleep_run_from_xosc();
                    // unused GPIO pin 8: unused pin so it never wakes up
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
