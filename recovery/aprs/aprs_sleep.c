
#include "aprs_sleep.h"
#include "hardware/clocks.h"
#include "hardware/rosc.h"
#include "hardware/structs/scb.h"
#include "pico/sleep.h"
#include "pico/time.h"
#include "hardware/regs/io_bank0.h"
#include "pico/binary_info.h"
#include "pico/stdlib.h"

void set_bin_desc() {
    bi_decl(bi_program_description(
        "Recovery process binary for standalone recovery board 2-#"));
    bi_decl(bi_1pin_with_name(APRS_LED_PIN, "On-board LED"));
    bi_decl(bi_1pin_with_name(GPS_TX_PIN, "TX UART to GPS"));
    bi_decl(bi_1pin_with_name(GPS_RX_PIN, "RX UART to GPS"));
    bi_decl(bi_1pin_with_name(TAG_TX_PIN, "TX UART to TAG modem"));
    bi_decl(bi_1pin_with_name(TAG_RX_PIN, "RX UART to TAG modem"));
    describeConfig();
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

    initLed(APRS_LED_PIN);
    setLed(APRS_LED_PIN, true);
    printf("Re-init GPS and APRS \n");

    // gpsInit(&gps_config);
    // initializeAPRS();
    // wakeVHF();
    // sleep_ms(1000);
    printf("Recovered from sleep \n");
    setLed(APRS_LED_PIN, false);
    return;
}

void rtc_sleep(const gps_config_s gps_config, uint32_t sleep_time) {
    ubx_cfg_t sleep[16];
    createUBXSleepPacket(sleep_time, sleep);
    writeSingleConfiguration(gps_config.uart, sleep, 16);
    // uart_deinit(uart0);

    // sleep_ms(100);
    sleep_run_from_xosc();

    datetime_t t = {.year = 2020,
                    .month = 06,
                    .day = 05,
                    .dotw = 5,  // 0 is Sunday, so 5 is Friday
                    .hour = 1,
                    .min = 01,
                    .sec = 00};

    // Alarm 10 seconds later
    datetime_t t_alarm = {.year = 2020,
                          .month = 06,
                          .day = 05,
                          .dotw = 5,  // 0 is Sunday, so 5 is Friday
                          .hour = 1,
                          .min = 01,
                          .sec = 10};

    // Start the RTC
    rtc_init();
    rtc_set_datetime(&t);

    // uart_default_tx_wait_blocking();

    sleep_goto_sleep_until(&t_alarm, &recover_from_sleep);
    // sleep_goto_dormant_until_edge_high(6);
}