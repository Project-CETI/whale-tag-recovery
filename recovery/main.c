#include <stdio.h>
#include "pico/stdlib.h"
#include "pico/time.h"
#include "pico/binary_info.h"
#include "aprs.h"
#include "vhf.h"
#include "gps.h"
#include "tag.h"

#define VHF_TX_LEN 800
#define LED_PIN 29

void set_bin_desc(void);
void setLed(bool state);
void initLed(void);
bool txAprs(repeating_timer_t *rt);
void startAPRS(repeating_timer_t *aprsTimer);
void startTag(repeating_timer_t *tagTimer);
bool txTag(repeating_timer_t *rt);
void prepFishTx(float txFreq);
bool vhf_pulse_callback(repeating_timer_t *rt);
void initAll(void);

// status arrays (updated in-place by functions)
float coords[2] = {0,0};
uint16_t aCS[3] = {0,0,0};
char lastGpsUpdate[100] = "$CMD_ERROR";
char lastDtUpdate[100] = "$CMD_ERROR";

// APRS communication config (change per tag)
const uint32_t aprsInterval = 30000; // APRS TX interval
bool aprsDebug = false;
int aprsStyle = 2;
extern uint8_t* sinValues;

// GPS communication config (change ONLY when necessary)
const uint TX_GPS = 0;
const uint RX_GPS = 1;
const uint BAUD_GPS = 9600;

// TAG communication config (change ONLY when necessary)
const uint TX_TAG = 4;
const uint RX_TAG = 5;
const uint BAUD_TAG = 115200;
const uint32_t tagInterval = 10000;

// VHF config
float vhfTxFreq = 148.056;
const uint32_t yagiInterval = 600000;

// runtime vars
bool gpsInteractive = false;
uint32_t prevYagiTx = 0;

void set_bin_desc() {
  bi_decl(bi_program_description("Recovery process binary for standalone recovery board 2-#"));
  bi_decl(bi_1pin_with_name(LED_PIN, "On-board LED"));
  bi_decl(bi_1pin_with_name(TX_GPS, "TX UART to GPS"));
  bi_decl(bi_1pin_with_name(RX_GPS, "RX UART to GPS"));
  bi_decl(bi_1pin_with_name(TX_TAG, "TX UART to TAG modem"));
  bi_decl(bi_1pin_with_name(RX_TAG, "RX UART to TAG modem"));
  describeConfig();
}

void setLed(bool state) {gpio_put(LED_PIN, state);}
void initLed(void) {gpio_init(LED_PIN); gpio_set_dir(LED_PIN, GPIO_OUT);}

bool txAprs(repeating_timer_t *rt) {
  getPos(coords);
  getACS(aCS);
  setLed(true);
  if (aprsDebug)
    sendTestPackets(aprsStyle);
  else
    sendPacket(coords, aCS);
  setLed(false);
	return true;
}

void startAPRS(repeating_timer_t *aprsTimer) {
	configureAPRS_TX(145.05);
	add_repeating_timer_ms(-aprsInterval, txAprs, NULL, aprsTimer);
}

bool txTag (repeating_timer_t *rt) {
  getLastPDtBufs(lastGpsUpdate, lastDtUpdate);
  writeGpsToTag(lastGpsUpdate, lastDtUpdate);
  // detachTag();
  // reqTagState();
}

void startTag(repeating_timer_t *tagTimer) {
	add_repeating_timer_ms(-tagInterval, txTag, NULL, tagTimer);
}

/** @brief Re-configure the VHF module to tracker transmission frequency
 * @see configureDra818v */
void prepFishTx(float txFreq) {
		configureDra818v(txFreq,txFreq,8,false,false,false);
}

/** @brief Callback for a repeating timer dictating transmissions.
 * Transmits a pulse for length defined in #YAGI_TX_LEN
 * Callback for repeating_timer functions */
bool vhf_pulse_callback(repeating_timer_t *rt) {
	setPttState(true);
	uint32_t stepLen = VHF_HZ * 32;
	printf("Calc: %ld %ld %ld", stepLen, VHF_TX_LEN, 1000);
	int numSteps = stepLen * VHF_TX_LEN / 1000;
	stepLen = (int) 1000000/stepLen;
	for (int i = 0; i < numSteps; i++) {
		setOutput(sinValues[i % numSinValues]);
		busy_wait_us_32(stepLen);
	}
	setOutput(0x00);
	setPttState(false);
  // setVhfState(false);
	printf("Pulsed at %d Hz ==> %d times, %d each.\n", VHF_HZ, numSteps, stepLen);
	return true;
}

void initAll() {
  set_bin_desc();
  stdio_init_all();
  gpsInteractive = true;

  initLed();
  setLed(true);

  gpsInit(TX_GPS, RX_GPS, BAUD_GPS, uart0, gpsInteractive);

  // APRS communication config (change per tag)
	char mycall[8] = "J75Y";
	int myssid = 1;
	char dest[8] = "APLIGA";
	char digi[8] = "WIDE2";
	int digissid = 1;
	char mycomment[128] = "Ceti b1.1 GPS Standalone";
	initAPRS(mycall, myssid, dest, digi, digissid, mycomment);

  initTagComm(TX_TAG, RX_TAG, BAUD_TAG, uart1);
  setLed(false);
}

int main() {
  // Setup
  initAll();

	repeating_timer_t aprsTimer;
	// startAPRS(&aprsTimer);
	repeating_timer_t tagTimer;
	// startTag(&tagTimer);
	repeating_timer_t yagiTimer;
	printf("Timers created.\n");
  // Loop
  while (true) {
    readFromGps();
		// if ((to_ms_since_boot(get_absolute_time()) - prevYagiTx) >= yagiInterval) {
			printf("Clear other timers.\n");
			// cancel_repeating_timer(&aprsTimer);
			// cancel_repeating_timer(&tagTimer);
			printf("Set VHF timer.\n");
			prevYagiTx = to_ms_since_boot(get_absolute_time());
			prepFishTx(vhfTxFreq);
			setVhfState(true);
			setPttState(true);
			add_repeating_timer_ms(-1000, vhf_pulse_callback, NULL, &yagiTimer);
			sleep_ms(60000);
			setPttState(false);
			setVhfState(false);
			cancel_repeating_timer(&yagiTimer);
			printf("Timer cleared.\n");
			printf("Restart main operations.");
			// startAPRS(&aprsTimer);
			// startTag(&tagTimer);
			// }
  }
	return 0;
}
