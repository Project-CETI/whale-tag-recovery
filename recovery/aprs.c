/** @file aprs.c
 * @brief Defines APRS functionality for the RP2040 on the Recovery Board v1.0
 * @author Shashank Swaminathan
 * @author Louis Adamian
 * @details Core APRS functions derived from https://github.com/handiko/Arduino-APRS/blob/master/Arduino-Sketches/Example/APRS_Mixed_Message/APRS_Mixed_Message.ino.
 */
#include <stdio.h>
#include <math.h>
#include <string.h>
#include "pico/stdlib.h"
#include "pico/time.h"
#include "aprs.h"
#include "vhf.h"

/// Re-define default alarm pool to make every sleep a busy wait
#define PICO_TIME_DEFAULT_ALARM_POOL_DISABLED   0
/// Number of values in the sinValues DAC output array
#define numSinValues 32
// APRS VARS [START] ----------------------------------------------------
// APRS protocol config (don't change)
const uint16_t _FLAG = 0x7e; ///< Flag for APRS transmissions. Must be 0x7e.
const uint16_t _CTRL_ID = 0x03; ///< CTRL ID for APRS transmission. Must be 0x03.
const uint16_t _PID = 0xf0; ///< PID for APRS transmission. Must be 0xf0.
const char _DT_POS = '!'; ///< APRS character, defines transmissions as real-time.
bool nada = 0;
const char sym_ovl = '\\'; ///< Symbol Table ID. \ for the Alternative symbols.
const char sym_tab = '-'; ///< Symbol Code.
char bitStuff = 0; ///< Tracks the bit position in byte being transmitted.
unsigned short crc = 0xffff; ///< CRC-16 CCITT value. Initialize at 0xffff.
uint8_t currOutput = 0; ///< Tracks location in FSK wave output. Range 0-31.

bool q145 = false; ///< Boolean for selecting 144.39MHz or 145.05MHz transmission.

char callsign[8]="J73MAB"; ///< Personal APRS callsign.
int ssid = 15; ///< Personal APRS SSID. Range 0-15.
char destsign[8] = "APLIGA"; ///< Destination APRS callsign.
char digisign[8] = "WIDE2"; ///< Digipeater callsign. Use WIDE1 or WIDE2.
int dssid = 1; ///< Digipeater SSID. Use 1 if digisign is WIDE2.
/** ASCII comment to append to APRS transmission.
 * Ceti b#.# tag# OR Ceti b#.# #-#
 */
char comment[128] = "Ceti b1.0 2-S";

// APRS payload arrays
char lati[9]; ///< Character array to hold the latitude coordinate.
char lon[10]; ///< Character array to hold the longitude coordinate.
char cogSpeed[8]; ///< Character array to hold the course/speed information.

// APRS signal TX config
uint64_t endTimeAPRS = 0; ///< us counter on how long to hold steps in DAC output.
const uint64_t bitPeriod = 832; ///< Total length of signal period per bit.
const uint64_t delay1200 = 23; ///< Delay for 1200Hz signal.
const uint64_t delay2200 = 15; ///< Delay for 2200Hz signal.
/** Array of sin values for the DAC output.
 * Corresponds to one whole 8-bit sine output.
 */
const uint8_t sinValues[numSinValues] = {
  152, 176, 198, 217, 233, 245, 252, 255, 252, 245, 233,
  217, 198, 176, 152, 127, 103, 79,  57,  38,  22,  10,
  3,   1,   3,   10,  22,  38,  57,  79,  103, 128
};
// APRS VARS [END] ------------------------------------------------------


// Low-level TX functions
/** Sets the DAC output during each stage of DAC sine wave output.
 * Loops through the DAC output array using currOutput.
 */
void setNextSin(void) {
  currOutput++;
  if (currOutput == numSinValues) currOutput = 0;
  setOutput(sinValues[currOutput]);
}

/** Sets a 1200Hz output wave via the DAC.
 * Uses the setNextSin function.
 * @see setNextSin
 */
void setNada1200(void) {
  endTimeAPRS = to_us_since_boot(get_absolute_time()) + bitPeriod;
  while (to_us_since_boot(get_absolute_time()) < endTimeAPRS) {
    setNextSin();
    busy_wait_us(delay1200);
  }
}

/** Sets a 2200Hz output wave via the DAC.
 * @see setNextSin
 */
void setNada2400(void) {
  endTimeAPRS = to_us_since_boot(get_absolute_time()) + bitPeriod;
  while (to_us_since_boot(get_absolute_time()) < endTimeAPRS) {
    setNextSin();
    busy_wait_us(delay2200);
  }
}

/** Does the FSK shifting based on the input bit value.
 * Sets the DAC output to either 1200Hz or 2200Hz using either setNada1200 or setNada2400 accordingly.
 * @param nada Input bit to send using FSK method.
 * @see setNada1200
 * @see setNada2400
 */
void set_nada(bool nada) {
  if (nada)
    setNada1200();
  else
    setNada2400();
}

// Mid-level TX functions
/** Calculates the CRC-16 CCITT value.
 * Continuously updates the crc value based on the bits being sent throughout the message.
 * Uses 0x1021 as the polynomial value for the CRC check.
 * Initializes 0xffff.
 * @param inBit The bit being transmitted currently.
 */
void calc_crc(bool inBit) {
  unsigned short xorIn;

  xorIn = crc ^ inBit;
  crc >>= 1;

  if (xorIn & 0x01) crc ^= 0x8408;
}

/** Sends the calculated CRC value via FSK transmission.
 * Sends it in reversed bit order, as per AX.25 protocol.
 * @see calc_crc
 */
void sendCrc(void) {
  uint8_t crcLo = crc ^ 0xff;
  uint8_t crcHi = (crc >> 8) ^ 0xff;

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
void sendCharNRZI(unsigned char in_byte, bool enBitStuff) {
  bool bits;

  for (int i = 0; i < 8; i++) {
    bits = in_byte & 0x01;

    calc_crc(bits);

    if (bits) {
      set_nada(nada);
      bitStuff++;

      if ((enBitStuff) && (bitStuff == 5)) {
        nada ^= 1;
        set_nada(nada);

        bitStuff = 0;
      }
    } else {
      nada ^= 1;
      set_nada(nada);

      bitStuff = 0;
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
void sendStrLen(const char *in_string, int len) {
  for (int j = 0; j < len; j++) sendCharNRZI(in_string[j], true);
}

/** Sends the flag character defined in AX.25 protocol via an FSK signal.
 * Marks the start and end of the transmission.
 * @param flag_len Amount of times to send the flag character.
 */
void sendFlag(int flag_len) {
  for (int j = 0; j < flag_len; j++) sendCharNRZI(_FLAG, false);
}

// High-level TX functions
/** Sets the strings containing the payload information.
 * Payload consists of real-time position information.
 * Needs Latitude-Longitude coordinates and course/speed.
 * @param latlon Two-value float array of lat-lon coordinates.
 * @param acs Three-value 2-byte array of altitude, course, and speed.
 * @see sendPacket
 */
void setPayload(float *latlon, uint16_t *acs) {
  float latFloat = latlon[0];
  bool south = false;
  if (latFloat < 0) {
    south = true;
    latFloat = fabs(latFloat);
  }
  int latDeg = (int) latFloat;
  float latMin = latFloat - latDeg; // get decimal degress from float
  latMin = latMin * 60; // convert decimal degrees to minutes
  uint8_t latMinInt = (int)latMin;
  uint8_t latMinDec = round((latMin - latMinInt) * 100);

  if (south) {
    snprintf(lati, 9, "%02d%02d.%02dS", latDeg, latMinInt, latMinDec);
  } else {
    snprintf(lati, 9, "%02d%02d.%02dN", latDeg, latMinInt, latMinDec);
  }
  float lonFloat = latlon[1];
  bool west = false;
  if (lonFloat < 0) {
    west = true;
    lonFloat = fabs(lonFloat);
  }
  int lonDeg = (int) lonFloat;
  float lonMin = lonFloat - lonDeg; // get decimal degress from float
  lonMin = lonMin * 60;
  uint8_t lonMinInt = (int)lonMin;
  uint8_t lonMinDec = round((lonMin - lonMinInt) * 100);
  if (west) {
    snprintf(lon, 10, "%03d%02d.%02dW", lonDeg, lonMinInt, lonMinDec);
  } else {
    snprintf(lon, 10, "%03d%02d.%02dE", lonDeg, lonMinInt, lonMinDec);
  }

  double speed = acs[2] / 1.852;  // convert speed from km/h to knots
  uint16_t course = (uint16_t)acs[1];
  if (course ==  0) {
    // APRS wants course in 1-360 and swarm provides it as 0-359
    course = 360;
  }
  int sog = (int) acs[2];
  snprintf(cogSpeed, 8, "%03d/%03d", course, sog);
}

/** Transmits the headers as defined in the AX.25 packet protocol.
 * Uses the callsigns and ssids defined as per APRS protocol.
 * @see sendPacket
 */
void sendHeader(void) {
  int temp;
  /********* DEST ***********/

  temp = strlen(destsign);
  for (int j = 0; j < temp; j++) sendCharNRZI(destsign[j] << 1, true);
  if (temp < 6) {
    for (int j = temp; j < 6; j++) sendCharNRZI(' ' << 1, true);
  }
  sendCharNRZI('0' << 1, true);

  /********* SOURCE *********/
  temp = strlen(callsign);
  for (int j = 0; j < temp; j++) sendCharNRZI(callsign[j] << 1, true);
  if (temp < 6) {
    for (int j = temp; j < 6; j++) sendCharNRZI(' ' << 1, true);
  }
  sendCharNRZI((ssid + '0') << 1, true);

//  /********* DIGI ***********/
  temp = strlen(digisign);
  for (int j = 0; j < temp; j++) sendCharNRZI(digisign[j] << 1, true);
  if (temp < 6) {
    for (int j = temp; j < 6; j++) sendCharNRZI(' ' << 1, true);
  }
  sendCharNRZI(((dssid + '0') << 1) + 1, true);

  /***** CTRL FLD & PID *****/
  sendCharNRZI(_CTRL_ID, true);
  sendCharNRZI(_PID, true);
}

/** Sends the APRS packet via FSK transmission through the VHF module.
 *
 * Uses the AX.25 packet protocol.
 * If the latitude indicates it is close to mainland North America, switches frequencies to 144.39MHz (if not already on it). Does likewise to 145.05MHz, if not close.
 * Triggers both VHF sleep/wake and PTT.
 * Sets payload prior to commencing transmission to avoid delays due to string magic.
 * Sends 150 flags to start transmission and 3 to end transmission.
 * @param latlon Two-value float array of lat-lon coordinates, from main loop.
 * @param acs Three-value 2-byte array of altitude, course, and speed, from main loop.
 */
void sendPacket(float *latlon, uint16_t *acs) {
  if (latlon[0] < 17.71468 && !q145) {
			configureDra818v(145.05,145.05,8,false,false,false);
    q145 = true;
  }
  else if (latlon[0] >= 17.71468 && q145) {
			configureDra818v(144.39,144.39,8,false,false,false);
    q145 = false;
  }
  setVhfState(true);
  setPttState(true);
  setPayload(latlon, acs);

  // TODO TEST IF THIS IMPROVES RECEPTION
  // Send initialize sequence for receiver
  //sendCharNRZI(0x99, true);
  //sendCharNRZI(0x99, true);
  //sendCharNRZI(0x99, true);

  sendFlag(150);
  crc = 0xffff;
  sendHeader();
  // send payload
  sendCharNRZI(_DT_POS, true);
  sendStrLen(lati, strlen(lati));
  sendCharNRZI(sym_ovl, true);
  sendStrLen(lon, strlen(lon));
  sendCharNRZI(sym_tab, true);
  sendStrLen(cogSpeed, strlen(cogSpeed));
  sendStrLen(comment, strlen(comment));

  sendCrc();
  sendFlag(3);
  setPttState(false);
  setVhfState(false);
}

// Debug TX functions
void printPacket(void) {
  printf("%s0%s%d%s%d%x%x%c%s%c%s%c%s%s\n", destsign, callsign, ssid, digisign, dssid, _CTRL_ID, _PID, _DT_POS, lati, sym_ovl, lon, sym_tab, cogSpeed, comment);
}
void sendTestPackets(int style) {
  float latlon[2] = {0,0};
  uint16_t acs[3] = {0,0,0};
  switch(style) {
    case 1:
      // printf("Style 1: My latlon is: %f %f\n",latlon[0],latlon[1]);
      sendPacket(latlon, acs);
      break;
    case 2:
      latlon[0] = 42.3648;
      latlon[1] = -71.1247;
      // printf("Style 2: My latlon is: %f %f\n",latlon[0],latlon[1]);
      sendPacket(latlon, acs);
      break;
    case 3:
      sendHeader();
      sendStrLen(">...",strlen(">..."));
      break;
    case 4:
      crc = 0xffff;
      sendStrLen("123456789",9);
      sendCrc();
      crc = 0xffff;
      sendStrLen("A",1);
      sendCrc();
      break;
    default:
      // printf("Style D: My latlon is: %f %f\n",latlon[0],latlon[1]);
      sendPacket(latlon, acs);
      break;
  }
}

// Configuration functions
/** Sets all the APRS configuration parameters for the APRS packet header.
 * Not calling this function still allows APRS to function, but relies on default values.
 * VHF can be re-configured outside of this function using VHF-specific functions directly.
 * @param mcall Personal callsign (default: J73MAB)
 * @param mssid Personal SSID (0-15) (default: 1)
 * @param dst Destination callsign (default: APLIGA)
 * @param dgi Digipeater callsign (default: WIDE2)
 * @param dgssid Digipeater SSID (0-2) (default: 1)
 * @param cmt Comment to append to end of APRS packet (default: Ceti b1.0 2-S)
 * @see sendHeader
 */
void configureAPRS(char *mcall, int mssid, char *dst, char *dgi, int dgssid, char *cmt) {
  configureVHF();
  q145 = false;
  memcpy(callsign, mcall, 8 * sizeof(*mcall));
  memcpy(destsign, dst, 8 * sizeof(*dst));
  memcpy(digisign, dgi, 8 * sizeof(*dgi));
  memcpy(comment, cmt, 128 * sizeof(*cmt));
  ssid = mssid;
  dssid = dgssid;
}

/** Adds any relevant information to the compiled binary.
 * Currently, only adds the VHF module.
 */
void describeConfig(void) {
  pinDescribe();
}
// APRS HEADERS [END] ---------------------------------------------------
