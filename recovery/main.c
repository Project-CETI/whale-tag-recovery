/** @file main.c
 * @author Shashank Swaminathan
 * Core file for the recovery board. Defines all configurations, initializes all modules, and runs appropriate timers/interrupts based on received data.
 */
#include <stdio.h>
#include "pico/stdlib.h"
#include "pico/time.h"
#include "pico/binary_info.h"
#include "aprs.h"
#include "vhf.h"
#include "gps.h"
#include "tag.h"
#include "ublox-config.h"

/// Set how long the pure VHF pulse should be
// #define VHF_TX_LEN 800
/// Location of the LED pin
#define LED_PIN 29

void set_bin_desc(void);
void setLed(bool state);
void initLed(void);
bool txAprs(repeating_timer_t *rt);
void startAPRS(const aprs_config_s * aprs_cfg, repeating_timer_t *aprsTimer);
void startTag(const tag_config_s * tag_cfg, repeating_timer_t *tagTimer);
bool txTag(repeating_timer_t *rt);
void initAll(const gps_config_s * gps_cfg, const tag_config_s * tag_cfg);

/** @struct Defines unchanging configuration parameters for APRS.
 * callsign, ssid, dest, digi, digissid, comment
 */
const aprs_config_s aprs_config = {
	"J75Y", 1,
	"APLIGA", "WIDE2", 1, "Ceti b1.1 GPS Standalone",
	30000, false, 2
};

/** @struct Defines unchanging configuration parameters for GPS communication.
 * GPS_TX, GPS_RX, GPS_BAUD, UART_NUM, QINTERACTIVE
 */
const gps_config_s gps_config = {1, 0, 9600, uart0, false};
gps_data_s gps_data = {{42.3648,0},
											{0,0,0},
											"$GN 42.3648,-71.1247,0,360,0*38",
											"$DT 20190408195123,V*41",
											false, false};
/** @struct Defines unchanging configuration parameters for tag communication.
 * TAG_TX, TAG_RX, TAG_BAUD, UART_NUM, SEND_INTERVAL, ACK_WAIT_TIME_US, ACK_WAIT_TIME_MS, NUM_TRIES
 */
const tag_config_s tag_config = {4, 5, 115200, uart1, 10000, 1000, 1000, 3};

void set_bin_desc(void) {
  bi_decl(bi_program_description("Recovery process binary for standalone recovery board 2-#"));
  bi_decl(bi_1pin_with_name(LED_PIN, "On-board LED"));
  bi_decl(bi_1pin_with_name(gps_config.txPin, "TX UART to GPS"));
  bi_decl(bi_1pin_with_name(gps_config.rxPin, "RX UART to GPS"));
  bi_decl(bi_1pin_with_name(tag_config.txPin, "TX UART to TAG modem"));
  bi_decl(bi_1pin_with_name(tag_config.rxPin, "RX UART to TAG modem"));
  describeConfig();
}

void setLed(bool state) {gpio_put(LED_PIN, state);}
void initLed(void) {gpio_init(LED_PIN); gpio_set_dir(LED_PIN, GPIO_OUT);}

bool txAprs(repeating_timer_t *rt) {
	drainGpsFifo(&gps_config, &gps_data);
	sleep_ms(100);
	int i = 0;
CHECK_POS: readFromGps(&gps_config, &gps_data);
	if (gps_data.posCheck != true) {
		/* getPos(coords); */
		/* getACS(aCS); */
		setLed(true);
		if (aprs_config.debug)
			sendTestPackets(&aprs_config);
		else
			sendPacket(&aprs_config, gps_data.latlon, gps_data.acs);
		setLed(false);
		return true;
	}
	else {
		i++;
		sleep_ms(1000);
		if (i < 1) goto CHECK_POS;
	}
	return false;
}

void startAPRS(const aprs_config_s * aprs_cfg, repeating_timer_t *aprsTimer) {
	configureAPRS_TX(145.05);
	add_repeating_timer_ms(-aprs_cfg->interval, txAprs, NULL, aprsTimer);
}

bool txTag (repeating_timer_t *rt) {
  // getLastPDtBufs(lastGpsUpdate, lastDtUpdate);
  writeGpsToTag(&tag_config, gps_data.lastGpsBuffer, gps_data.lastDtBuffer);
  // detachTag();
  // reqTagState();
}

void startTag(const tag_config_s * tag_cfg, repeating_timer_t *tagTimer) {
	add_repeating_timer_ms(-tag_cfg->interval, txTag, NULL, tagTimer);
}

void initAll(const gps_config_s * gps_cfg, const tag_config_s * tag_cfg) {
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

	// APRS comms interrupt setup
	repeating_timer_t aprsTimer;

	// Tag comms interrupt setup
	repeating_timer_t tagTimer;

	// VHF pulse interrupt setup
	float vhfTxFreq = 148.056;
	const uint32_t yagiInterval = 600000;
	repeating_timer_t yagiTimer;
	prepFishTx(vhfTxFreq);

	// Start the VHF pulsing until GPS lock
	setVhfState(true);
	add_repeating_timer_ms(-1000, vhf_pulse_callback, NULL, &yagiTimer);
	bool yagiIsOn = true;

	

  // Loop
  while (true) {
    readFromGps(&gps_config, &gps_data);
		if (gps_data.datCheck && yagiIsOn) {
			cancel_repeating_timer(&yagiTimer);
			setVhfState(false);
			startAPRS(&aprs_config, &aprsTimer);
			startTag(&tag_config, &tagTimer);
		}
		else if (gps_data.datCheck) sleep_ms(1000);
		else if (!gps_data.datCheck && !yagiIsOn) {
			setVhfState(true);
			cancel_repeating_timer(&aprsTimer);
			// DO NOT CANCEL TAG TIMER, WE WILL REPORT THROUGH DATA LOSS
			prepFishTx(vhfTxFreq);
			add_repeating_timer_ms(-1000, vhf_pulse_callback, NULL, &yagiTimer);
			yagiIsOn = true;
		}
		else {
			
			sleep_ms(1000);
		}
  }
	return 0;
}
