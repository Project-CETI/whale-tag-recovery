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

/// Set how long the pure VHF pulse should be
#define VHF_TX_LEN 800
/// Location of the LED pin
#define LED_PIN 29

void set_bin_desc(void);
void setLed(bool state);
void initLed(void);
bool txAprs(repeating_timer_t *rt);
void startAPRS(repeating_timer_t *aprsTimer);
void startTag(repeating_timer_t *tagTimer);
bool txTag(repeating_timer_t *rt);
void initAll(const gps_config_s * gps_cfg, const tag_config_s * tag_cfg);

// status arrays (updated in-place by functions)
float coords[2] = {0,0};
uint16_t aCS[3] = {0,0,0};
char lastGpsUpdate[100] = "$CMD_ERROR";
char lastDtUpdate[100] = "$CMD_ERROR";

// APRS communication config (change per tag)
const uint32_t aprsInterval = 30000; // APRS TX interval
bool aprsDebug = false;
int aprsStyle = 2;

// TAG communication config (change ONLY when necessary)
const uint TX_TAG = 4;
const uint RX_TAG = 5;
const uint BAUD_TAG = 115200;
const uint32_t tagInterval = 10000;

// VHF config
float vhfTxFreq = 148.056;
const uint32_t yagiInterval = 600000;

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

void set_bin_desc() {
  bi_decl(bi_program_description("Recovery process binary for standalone recovery board 2-#"));
  bi_decl(bi_1pin_with_name(LED_PIN, "On-board LED"));
  bi_decl(bi_1pin_with_name(gps_config.txPin, "TX UART to GPS"));
  bi_decl(bi_1pin_with_name(gps_config.rxPin, "RX UART to GPS"));
  bi_decl(bi_1pin_with_name(TX_TAG, "TX UART to TAG modem"));
  bi_decl(bi_1pin_with_name(RX_TAG, "RX UART to TAG modem"));
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
		if (aprsDebug)
			sendTestPackets(aprsStyle);
		else
			sendPacket(gps_data.latlon, gps_data.acs);
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

void startAPRS(repeating_timer_t *aprsTimer) {
	configureAPRS_TX(145.05);
	add_repeating_timer_ms(-aprsInterval, txAprs, NULL, aprsTimer);
}

bool txTag (repeating_timer_t *rt) {
  // getLastPDtBufs(lastGpsUpdate, lastDtUpdate);
  writeGpsToTag(&tag_config, gps_data.lastGpsBuffer, gps_data.lastDtBuffer);
  // detachTag();
  // reqTagState();
}

void startTag(repeating_timer_t *tagTimer) {
	add_repeating_timer_ms(-tagInterval, txTag, NULL, tagTimer);
}

void initAll(const gps_config_s * gps_cfg, const tag_config_s * tag_cfg) {
  set_bin_desc();
  stdio_init_all();

  initLed();
  setLed(true);

  gpsInit(gps_cfg);

  // APRS communication config (change per tag)
	char mycall[8] = "J75Y";
	int myssid = 1;
	char dest[8] = "APLIGA";
	char digi[8] = "WIDE2";
	int digissid = 1;
	char mycomment[128] = "Ceti b1.1 GPS Standalone";
	initAPRS(mycall, myssid, dest, digi, digissid, mycomment);

  initTagComm(tag_cfg);
  setLed(false);
}

int main() {
  // Setup
  initAll(&gps_config, &tag_config);

	repeating_timer_t aprsTimer;
	repeating_timer_t tagTimer;
	repeating_timer_t yagiTimer;
	prepFishTx(vhfTxFreq);
	setVhfState(true);
	add_repeating_timer_ms(-1000, vhf_pulse_callback, NULL, &yagiTimer);
	bool yagiIsOn = true;
  // Loop
  while (true) {
    readFromGps(&gps_config, &gps_data);
		if (gps_data.datCheck && yagiIsOn) {
			cancel_repeating_timer(&yagiTimer);
			setVhfState(false);
			startAPRS(&aprsTimer);
			startTag(&tagTimer);
		}
		else if (gps_data.datCheck) sleep_ms(1000);
		else if (!gps_data.datCheck && !yagiIsOn) {
			setVhfState(true);
			cancel_repeating_timer(&aprsTimer);
			// DO NOT CANCEL TAG TIMER, WE WILL REPORT THROUGH DATA LOSS
			add_repeating_timer_ms(-1000, vhf_pulse_callback, NULL, &yagiTimer);
			yagiIsOn = true;
		}
		else {
			sleep_ms(1000);
		}
  }
	return 0;
}
