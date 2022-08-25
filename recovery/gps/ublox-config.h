#ifndef _RECOVERY_UBLOX_CONFIG_H_
#define _RECOVERY_UBLOX_CONFIG_H_

#define NUM_UBLOX_CONFIGS 6

typedef char ubx_cfg_t;

/**
 *  +--------+-------+----+-----+---------+------+------+
 *  | header | class | ID | len | payload | ck_a | ck_b |
 *  +--------+-------+----+-----+---------+------+------+
 *  header will always be 0xB5 0x62
 *  class: single byte
 *  ID: single byte
 *  len: 2 bytes
 */

// UBX-ACK-ACK
static const ubx_cfg_t ack_header[6] = {0xB5, 0x62, 0x05, 0x02, 0x00, 0x00};

// UBX-ACK-NAK
static const ubx_cfg_t nak_header[6] = {0xB5, 0x62, 0x05, 0x02, 0x00, 0x00};

// UBX-CFG-CFG
// Save the configuration to memory
static ubx_cfg_t cfg_save[21] = {/* header */ 0xB5,
                                 0x62,
                                 /* class & id */ 0x06,
                                 0x09,
                                 /* length */ 0x0D,
                                 0x00,
                                 /* payload */ 0x00,
                                 0x00,
                                 0x00,
                                 0x00,
                                 0xFF,
                                 0xFF,
                                 0x00,
                                 0x00,
                                 0x00,
                                 0x00,
                                 0x00,
                                 0x00,
                                 0x17,
                                 /* checksum */ 0x31,
                                 0xBF};

static ubx_cfg_t cfg_test[21] = {/* header */ 0xB5,
                                 0x62,
                                 /* class & id */ 0x06,
                                 0x09,
                                 /* length */ 0x0D,
                                 0x00,
                                 /* payload */ 0x00,
                                 0x00,
                                 0x00,
                                 0x00,
                                 0xFF,
                                 0xFF,
                                 0x00,
                                 0x00,
                                 0x00,
                                 0x00,
                                 0x00,
                                 0x00,
                                 0x03,
                                 /* checksum */ 0x1D,
                                 0xAB};

// UBX-CFG-CFG
// Load the configuration from memory
static ubx_cfg_t cfg_load[22] = {/* header */ 0xB5,
                                 0x62,
                                 /* class & id */ 0x06,
                                 0x09,
                                 /* length */ 0x00,
                                 0x0C,
                                 /* payload */ 0x00,
                                 0x00,
                                 0x00,
                                 0x00,
                                 0x00,
                                 0x00,
                                 0x00,
                                 0x00,
                                 0x00,
                                 0x00,
                                 0x1F,
                                 0x1F,
                                 0x01,
                                 0x00,
                                 /* checksum */ 0x00,
                                 0x00};

// UBX-CFG-MSG
// Load the configuration from memory
static ubx_cfg_t msg_rmc[16] = {/* header */ 0xB5,
                                0x62,
                                /* class & id */ 0x06,
                                0x01,
                                /* length */ 0x03,
                                0x00,
                                /* payload */ 0xF0,
                                0x04,
                                0x00,
                                0x00,
                                0x00,
                                0x00,
                                0x00,
                                0x00,
                                /* checksum */ 0x03,
                                0x3F};

// UBX-CFG-MSG
// Load the configuration from memory
static ubx_cfg_t msg_zda[11] = {/* header */ 0xB5,     0x62,
                                /* class & id */ 0x06, 0x01,
                                /* length */ 0x03,     0x00,
                                /* payload */ 0xF0,    0x08, 0x00,
                                /* checksum */ 0x07,   0x5B};

// turn of GBS messages (satellite fault detection)
static ubx_cfg_t msg_gbs[11] = {/* header */ 0xB5,     0x62,
                                /* class & id */ 0x06, 0x01,
                                /* length */ 0x03,     0x00,
                                /* payload */ 0xF0,    0x09, 0x00,
                                /* checksum */ 0x00,   0x00};

// turn of GSA messages (satellite fault detection)
static ubx_cfg_t msg_gsa[11] = {/* header */ 0xB5,     0x62,
                                /* class & id */ 0x06, 0x01,
                                /* length */ 0x03,     0x00,
                                /* payload */ 0xF0,    0x02, 0x00,
                                /* checksum */ 0x00,   0x00};

// turn of vtg messages (satellite fault detection)
static ubx_cfg_t msg_vtg[11] = {/* header */ 0xB5,     0x62,
                                /* class & id */ 0x06, 0x01,
                                /* length */ 0x03,     0x00,
                                /* payload */ 0xF0,    0x05, 0x00,
                                /* checksum */ 0x00,   0x00};

// turn of vtg messages (satellite fault detection)
static ubx_cfg_t msg_gga[11] = {/* header */ 0xB5,     0x62,
                                /* class & id */ 0x06, 0x01,
                                /* length */ 0x03,     0x00,
                                /* payload */ 0xF0,    0x00, 0x00,
                                /* checksum */ 0x00,   0x00};

// Set navigation to 2d fix
static ubx_cfg_t msg_nav5[44] = {/* header */ 0xB5,
                                 0x62,
                                 /* class & id */ 0x06,
                                 0x24,
                                 /* length */ 0x24,
                                 0x00,
                                 /* payload */ 0xFF,
                                 0x0FF,
                                 0x05,
                                 0x01,
                                 0x00,
                                 0x00,
                                 0x00,
                                 0x00,
                                 0x50,
                                 0xC3,
                                 0x00,
                                 0x00,
                                 0x05,
                                 0x00,
                                 0xFA,
                                 0x00,
                                 0xFA,
                                 0x00,
                                 0x64,
                                 0x00,
                                 0x2C,
                                 0x01,
                                 0x00,
                                 0x3C,
                                 0x00,
                                 0x00,
                                 0x00,
                                 0x00,
                                 0x00,
                                 0x00,
                                 0x00,
                                 0x00,
                                 0x00,
                                 0x00,
                                 0x00,
                                 0x00,
                                 /* checksum */ 0x2B,
                                 0xF7};

static ubx_cfg_t mon_ver[8] = {0xB5, 0x62, 0x0A, 0x04, 0, 0, 0x0E, 0x34};
/*01 06 00 00 0E 90 40 01 E8 03

00 00 10 27 00 00 00 00 00 00 02 00 00 00 2C 01

00 00 4F C1 03 00 87 02 00 00 FF 00 00 00 64 40

01 00 E4 62 */
static ubx_cfg_t prep_sleep[52] = {
    /* header */ 0xB5,     0x62,
    /* class & id */ 0x06, 0x3B,
    /* length */ 0x2C,     0x00,
    /* startup */ 0x01,    0x06, 0x00, 0x00,
    /* flags */ 0x0E, 0x90, 0x40, 0x01,
    /* update period*/ 0xE8, 0x03, 0x00, 0x00,
    /*search period*/ 0x10, 0x27, 0x00, 0x00,
    /*grid offset*/ 0x00, 0x00, 0x00, 0x00,
    /*on time*/ 0x02, 0x00, 
    /*minacqtime*/0x00, 0x00,
    /*reserved*/ 0x2C, 0x01, 0x00, 0x00, 0x4F, 0xC1, 0x03, 0x00, 0x87, 0x02, 0x00, 0x00, 0xFF, 0x00, 0x00, 0x00, 0x64, 0x40, 0x01, 0x00,
    /*checksum*/ 0xE4, 0x62};

static ubx_cfg_t sleep[16] = {0xB5, 0x62, 0x02, 0x41, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x4D, 0x3B};

static ubx_cfg_t day_sleep[16] = {0xB5, 0x62, 0x02, 0x41, 0x08, 0x00, 0x20, 0x4E, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x4D, 0x3B};
static ubx_cfg_t night_sleep[16] = {0xB5, 0x62, 0x02, 0x41, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x4D, 0x3B};
static ubx_cfg_t big_sleep[16] = {0xB5, 0x62, 0x02, 0x41, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x4D, 0x3B};


// These should be in order of how they are going to be sent to the UBLOX GPS
// module
static ubx_cfg_t* ubx_configurations[NUM_UBLOX_CONFIGS] = {
    msg_gbs, msg_gsa, msg_vtg, msg_gga, msg_nav5, cfg_save};

#endif  // _RECOVERY_UBLOX_CONFIG_H_