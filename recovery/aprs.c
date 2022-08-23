/** @file aprs.c
 * @brief Defines APRS functionality for the RP2040 on the Recovery Board v1.0
 * @author Shashank Swaminathan
 * @author Louis Adamian
 * @details Core APRS functions derived from
 * https://github.com/handiko/Arduino-APRS/blob/master/Arduino-Sketches/Example/APRS_Mixed_Message/APRS_Mixed_Message.ino.
 */
#include "aprs.h"
#include "pico/stdlib.h"
#include "pico/time.h"
#include "vhf.h"
#include <math.h>
#include <stdio.h>
#include <string.h>

/// Re-define as 1 to make every sleep a busy wait
#define PICO_TIME_DEFAULT_ALARM_POOL_DISABLED 0
// APRS VARS [START] ----------------------------------------------------

static bool vhf_clk = false;
/// APRS protocol config (don't change)
const struct aprs_pc_s {
    const uint16_t _FLAG;     ///< Flag for APRS transmissions. Must be 0x7e.
    const uint16_t _CTRL_ID;  ///< CTRL ID for APRS transmission. Must be 0x03.
    const uint16_t _PID;      ///< PID for APRS transmission. Must be 0xf0.
    const char
        _DT_POS;  ///< APRS character, defines transmissions as real-time.
    const char sym_ovl;  ///< Symbol Table ID. \ for the Alternative symbols.
    const char sym_tab;  ///< Symbol Code.
} aprs_pc = {0x7e, 0x03, 0xf0, '!', '1', 's'};

struct aprs_cc_s {
    bool nada;
    char bitStuff;       ///< Tracks the bit position in byte being transmitted.
    uint16_t crc;        ///< CRC-16 CCITT value. Initialize at 0xffff.
    uint8_t currOutput;  ///< Tracks location in FSK wave output. Range 0-31.
} aprs_cc = {0, 0, 0xffff, 0};

/// APRS signal TX config
const struct aprs_sig_s {
    const uint64_t bitPeriod;  ///< Total length of signal period per bit.
    const uint32_t delay1200;  ///< Delay for 1200Hz signal.
    const uint32_t delay2200;  ///< Delay for 2200Hz signal.
} aprs_sig = {832, 26, 14};

uint64_t endTimeAPRS;  ///< us counter on how long to hold steps in DAC output.
/// APRS payload arrays
struct aprs_pyld_s {
    char lati[9];  ///< Character array to hold the latitude coordinate.
    char lon[10];  ///< Character array to hold the longitude coordinate.
    char
        cogSpeed[8];  ///< Character array to hold the course/speed information.
} aprs_pyld;

///< Boolean for selecting 144.39MHz or 145.05MHz transmission.
bool shouldBe145 = false;

// Low-level TX functions
/** Sets the DAC output during each stage of DAC sine wave output.
 * Loops through the DAC output array using currOutput.
 */
static void setNextSin(void) {
    // printf("%d | %d\n", aprs_cc.currOutput, sinValues[aprs_cc.currOutput]);
    if (aprs_cc.currOutput == NUM_SINS) aprs_cc.currOutput = 0;
    setOutput(sinValues[aprs_cc.currOutput++]);
}

/** Sets a 1200Hz output wave via the DAC.
 * Uses the setNextSin function.
 * @see setNextSin
 */
static void setNada1200(void) {
    endTimeAPRS = to_us_since_boot(get_absolute_time()) + aprs_sig.bitPeriod;
    while (to_us_since_boot(get_absolute_time()) < endTimeAPRS) {
        setNextSin();
        busy_wait_us_32(aprs_sig.delay1200);
    }
}

/** Sets a 2200Hz output wave via the DAC.
 * @see setNextSin
 */
static void setNada2400(void) {
    endTimeAPRS = to_us_since_boot(get_absolute_time()) + aprs_sig.bitPeriod;
    while (to_us_since_boot(get_absolute_time()) < endTimeAPRS) {
        setNextSin();
        busy_wait_us_32(aprs_sig.delay2200);
    }
}

/** Does the FSK shifting based on the input bit value.
 * Sets the DAC output to either 1200Hz or 2200Hz using either setNada1200 or
 * setNada2400 accordingly.
 * @param nada Input bit to send using FSK method.
 * @see setNada1200
 * @see setNada2400
 */
static void set_nada(bool nada) {
    if (nada)
        setNada1200();
    else
        setNada2400();
}

// Mid-level TX functions
/** Calculates the CRC-16 CCITT value.
 * Continuously updates the crc value based on the bits being sent throughout
 * the message. Uses 0x1021 as the polynomial value for the CRC check.
 * Initializes 0xffff.
 * @param inBit The bit being transmitted currently.
 */
static void calc_crc(bool inBit) {
    unsigned short xorIn;

    xorIn = aprs_cc.crc ^ inBit;
    aprs_cc.crc >>= 1;

    if (xorIn & 0x01) aprs_cc.crc ^= 0x8408;
}

/** Sends the calculated CRC value via FSK transmission.
 * Sends it in reversed bit order, as per AX.25 protocol.
 * @see calc_crc
 */
static void sendCrc(void) {
    uint8_t crcLo = aprs_cc.crc ^ 0xff;
    uint8_t crcHi = (aprs_cc.crc >> 8) ^ 0xff;

    sendCharNRZI(crcLo, true);
    sendCharNRZI(crcHi, true);
}
/** Converts one byte into an FSK signal one bit at a time, LSB first.
 *
 * Concurrently updates the CRC calculation for every bit sent.
 * Encodes the bits using NRZI (Non Return to Zero, Inverted).
 * bit 1: transmitted as no change in tone.
 * bit 0: transmitted as change in tone.
 *
 * @param in_byte Input byte to transmit via AFSK method.
 * @param enBitStuff Is this a flag or not.
 * @see set_nada
 * @see calc_crc
 */
static void sendCharNRZI(unsigned char in_byte, bool enBitStuff) {
    bool bits;

    for (int i = 0; i < 8; i++) {
        vhf_clk = !vhf_clk;
        // gpio_put(VHF_PIN, vhf_clk);

        bits = in_byte & 0x01;

        calc_crc(bits);

        if (bits) {
            set_nada(aprs_cc.nada);
            aprs_cc.bitStuff++;

            if ((enBitStuff) && (aprs_cc.bitStuff == 5)) {
                aprs_cc.nada ^= 1;
                set_nada(aprs_cc.nada);

                aprs_cc.bitStuff = 0;
            }
        } else {
            aprs_cc.nada ^= 1;
            set_nada(aprs_cc.nada);

            aprs_cc.bitStuff = 0;
        }
        in_byte >>= 1;
    }
}
/** Takes an ASCII character string and converts it into an FSK signal.
 *
 * Follows AX.25 protocol to convert an ASCII string into an binary array.
 * Converts binary array into an FSK FM signal using sendCharNRZI.
 * @param in_string Input array of ASCII characters
 * @param len Length of the input array.
 * @see sendCharNRZI
 */
static void sendStrLen(const char *in_string, int len) {
    for (int j = 0; j < len; j++) sendCharNRZI(in_string[j], true);
}

/** Sends the flag character defined in AX.25 protocol via an FSK signal.
 * Marks the start and end of the transmission.
 * @param flag_len Amount of times to send the flag character.
 */
static void sendFlag(int flag_len) {
    for (int j = 0; j < flag_len; j++) sendCharNRZI(aprs_pc._FLAG, false);
}

// High-level TX functions
/** Sets the strings containing the payload information.
 * Payload consists of real-time position information.
 * Needs Latitude-Longitude coordinates and course/speed.
 * @param latlon Two-value float array of lat-lon coordinates.
 * @param acs Three-value 2-byte array of altitude, course, and speed.
 * @see sendPacket
 */
static void setPayload(float *latlon, uint16_t *acs) {
    float latFloat = latlon[0];
    bool south = false;
    if (latFloat < 0) {
        south = true;
        latFloat = fabs(latFloat);
    }
    int latDeg = (int)latFloat;
    float latMin = latFloat - latDeg;  // get decimal degress from float
    latMin = latMin * 60;              // convert decimal degrees to minutes
    uint8_t latMinInt = (int)latMin;
    uint8_t latMinDec = round((latMin - latMinInt) * 100);

    if (south) {
        snprintf(aprs_pyld.lati, 9, "%02d%02d.%02dS", latDeg, latMinInt,
                 latMinDec);
    } else {
        snprintf(aprs_pyld.lati, 9, "%02d%02d.%02dN", latDeg, latMinInt,
                 latMinDec);
    }
    float lonFloat = latlon[1];
    bool west = false;
    if (lonFloat < 0) {
        west = true;
        lonFloat = fabs(lonFloat);
    }
    int lonDeg = (int)lonFloat;
    float lonMin = lonFloat - lonDeg;  // get decimal degress from float
    lonMin = lonMin * 60;
    uint8_t lonMinInt = (int)lonMin;
    uint8_t lonMinDec = round((lonMin - lonMinInt) * 100);
    if (west) {
        snprintf(aprs_pyld.lon, 10, "%03d%02d.%02dW", lonDeg, lonMinInt,
                 lonMinDec);
    } else {
        snprintf(aprs_pyld.lon, 10, "%03d%02d.%02dE", lonDeg, lonMinInt,
                 lonMinDec);
    }

    double speed = acs[2] / 1.852;  // convert speed from km/h to knots
    uint16_t course = (uint16_t)acs[1];
    if (course == 0) {
        // APRS wants course in 1-360 and swarm provides it as 0-359
        course = 360;
    }
    int sog = (int)acs[2];
    snprintf(aprs_pyld.cogSpeed, 8, "%03d/%03d", course, sog);
}

/** Transmits the headers as defined in the AX.25 packet protocol.
 * Uses the callsigns and ssids defined as per APRS protocol.
 * @see sendPacket
 */
static void sendHeader(const aprs_config_s *aprs_cfg) {
    int temp;
    /********* DEST ***********/

    temp = strlen(aprs_cfg->dest);
    for (int j = 0; j < temp; j++) sendCharNRZI(aprs_cfg->dest[j] << 1, true);
    if (temp < 6) {
        for (int j = temp; j < 6; j++) sendCharNRZI(' ' << 1, true);
    }
    sendCharNRZI('0' << 1, true);

    /********* SOURCE *********/
    temp = strlen(aprs_cfg->callsign);
    for (int j = 0; j < temp; j++)
        sendCharNRZI(aprs_cfg->callsign[j] << 1, true);
    if (temp < 6) {
        for (int j = temp; j < 6; j++) sendCharNRZI(' ' << 1, true);
    }
    sendCharNRZI((aprs_cfg->ssid + '0') << 1, true);

    //  /********* DIGI ***********/
    // char digi2[8] = "WIDE1-";
    // int dssid_2 = 1;
    // temp = strlen(digi2);
    // for (int j = 0; j < temp; j++) sendCharNRZI(digi2[j] << 1, true);
    // if (temp < 6) {
    //     for (int j = temp; j < 6; j++) sendCharNRZI(' ' << 1, true);
    // }
    // sendCharNRZI((dssid_2 + '0') << 1, true);

    // sendCharNRZI(' ', true);

    temp = strlen(aprs_cfg->digi);
    for (int j = 0; j < temp; j++) sendCharNRZI(aprs_cfg->digi[j] << 1, true);
    if (temp < 6) {
        for (int j = temp; j < 6; j++) sendCharNRZI(' ' << 1, true);
    }
    sendCharNRZI(((aprs_cfg->dssid + '0') << 1) + 1, true);

    /***** CTRL FLD & PID *****/
    sendCharNRZI(aprs_pc._CTRL_ID, true);
    sendCharNRZI(aprs_pc._PID, true);
}

/** Sends the APRS packet via FSK transmission through the VHF module.
 *
 * Uses the AX.25 packet protocol.
 * If the latitude indicates it is close to mainland North America, switches
 * frequencies to 144.39MHz (if not already on it). Does likewise to 145.05MHz,
 * if not close. Triggers both VHF sleep/wake and PTT. Sets payload prior to
 * commencing transmission to avoid delays due to string magic. Sends 150 flags
 * to start transmission and 3 to end transmission.
 * @param latlon Two-value float array of lat-lon coordinates, from main loop.
 * @param acs Three-value 2-byte array of altitude, course, and speed, from main
 * loop.
 */
void sendPacket(const aprs_config_s *aprs_cfg, float *latlon, uint16_t *acs) {
    // Only reconfigure if out of range
    if (latlon[0] < 17.71468 && !shouldBe145) {
        configureDra818v(145.05, 145.05, 8, false, false, false);
        shouldBe145 = true;
    } else if (latlon[0] >= 17.71468 && shouldBe145) {
        configureDra818v(DEFAULT_FREQ, DEFAULT_FREQ, 8, false, false, false);
        shouldBe145 = false;
    }
    wakeVHF();
    setPttState(true);
    // sleep_ms(100);

    // aprs_cc.currOutput = 0;
    // setNextSin();
    // busy_wait_us_32(750);

    setPayload(latlon, acs);
    // TODO TEST IF THIS IMPROVES RECEPTION
    // Send initialize sequence for receiver
    // sendCharNRZI(0x99, true);
    // sendCharNRZI(0x99, true);
    // sendCharNRZI(0x99, true);

    sendFlag(150);
    aprs_cc.crc = 0xffff;
    sendHeader(aprs_cfg);
    // send payload
    sendCharNRZI(aprs_pc._DT_POS, true);
    sendStrLen(aprs_pyld.lati, strlen(aprs_pyld.lati));
    sendCharNRZI(aprs_pc.sym_ovl, true);
    sendStrLen(aprs_pyld.lon, strlen(aprs_pyld.lon));
    sendCharNRZI(aprs_pc.sym_tab, true);
    sendStrLen(aprs_pyld.cogSpeed, strlen(aprs_pyld.cogSpeed));
    sendStrLen(aprs_cfg->comment, strlen(aprs_cfg->comment));

    sendCrc();
    sendFlag(3);
    setPttState(false);
    // setOutput(0x00);
    sleepVHF();
}

// Debug TX functions
void printPacket(const aprs_config_s *aprs_cfg) {
    printf("%s0%s%d%s%d%x%x%c%s%c%s%c%s%s\n", aprs_cfg->dest,
           aprs_cfg->callsign, aprs_cfg->ssid, aprs_cfg->digi, aprs_cfg->dssid,
           aprs_pc._CTRL_ID, aprs_pc._PID, aprs_pc._DT_POS, aprs_pyld.lati,
           aprs_pc.sym_ovl, aprs_pyld.lon, aprs_pc.sym_tab, aprs_pyld.cogSpeed,
           aprs_cfg->comment);
}

void sendTestPackets(const aprs_config_s *aprs_cfg) {
    float latlon[2] = {0, 0};
    uint16_t acs[3] = {0, 0, 0};
    switch (aprs_cfg->style) {
        case 1:
            // printf("Style 1: My latlon is: %f %f\n",latlon[0],latlon[1]);
            sendPacket(aprs_cfg, latlon, acs);
            break;
        case 2:
            latlon[0] = 42.3648;
            latlon[1] = -71.1247;
            // printf("Style 2: My latlon is: %f %f\n",latlon[0],latlon[1]);
            sendPacket(aprs_cfg, latlon, acs);
            break;
        case 3:
            sendHeader(aprs_cfg);
            sendStrLen(">...", strlen(">..."));
            break;
        case 4:
            aprs_cc.crc = 0xffff;
            sendStrLen("123456789", 9);
            sendCrc();
            aprs_cc.crc = 0xffff;
            sendStrLen("A", 1);
            sendCrc();
            break;
        default:
            // printf("Style D: My latlon is: %f %f\n",latlon[0],latlon[1]);
            sendPacket(aprs_cfg, latlon, acs);
            break;
    }
}

// Configuration functions
/** Sets all the APRS configuration parameters for the APRS packet header.
 * Not calling this function still allows APRS to function, but relies on
 * default values. VHF can be re-configured outside of this function using
 * VHF-specific functions directly.
 * @param mcall Personal callsign (default: J73MAB)
 * @param mssid Personal SSID (0-15) (default: 1)
 * @param dst Destination callsign (default: APLIGA)
 * @param dgi Digipeater callsign (default: WIDE2)
 * @param dgssid Digipeater SSID (0-2) (default: 1)
 * @param cmt Comment to append to end of APRS packet (default: Ceti b1.0 2-S)
 * @see sendHeader
 */     
void initializeAPRS(void) {
    initializeVHF();
    shouldBe145 = false;
}

void configureAPRS_TX(float txFrequency) {
    configureDra818v(txFrequency, txFrequency, 4, false, false, false);
    
    if (txFrequency < 145.0)
		shouldBe145 = false;
	else
		shouldBe145 = true;
}

/** Adds any relevant information to the compiled binary.
 * Currently, only adds the VHF module.
 */
void describeConfig(void) { pinDescribe(); }
// APRS HEADERS [END] ---------------------------------------------------
