#ifndef _RECOVERY_UBLOX_CONFIG_H_
#define _RECOVERY_UBLOX_CONFIG_H_

#define NUM_UBLOX_CONFIGS 4

typedef uint8_t ubx_cfg_t;

/**
 *  +--------+-------+----+-----+---------+------+------+
 *  | header | class | ID | len | payload | ck_a | ck_b |
 *  +--------+-------+----+-----+---------+------+------+
 *  header will always be 0xB5 0x62
 *  class: single byte
 *  ID: single byte
 *  len: 2 bytes
 */

// UBX-CFG-CFG
// Save the configuration to memory
static ubx_cfg_t cfg_save[22] = { /* header */       0xB5, 0x62, 
                                   /* class & id */  0x06, 0x09, 
                                   /* length */      0x00, 0x0C, 
                                   /* payload */     0x00, 0x00, 0x00, 0x00,
                                                     0x00, 0x00, 0x1F, 0x1F,
                                                     0x00, 0x00, 0x00, 0x00,
                                                     0x01, 0x02,
                                   /* checksum */    0x00, 0x00};

// UBX-CFG-CFG
// Load the configuration from memory
static ubx_cfg_t cfg_load[22] = { /* header */       0xB5, 0x62, 
                                   /* class & id */  0x06, 0x09, 
                                   /* length */      0x00, 0x0C, 
                                   /* payload */     0x00, 0x00, 0x00, 0x00,
                                                     0x00, 0x00, 0x00, 0x00,
                                                     0x00, 0x00, 0x1F, 0x1F,
                                                     0x01, 0x00,
                                   /* checksum */    0x00, 0x00};


// UBX-CFG-MSG
// Load the configuration from memory
static ubx_cfg_t msg_rmc[22] = { /* header */     0xB5, 0x62, 
                            /* class & id */  0x06, 0x01, 
                            /* length */      0x00, 0x08, 
                            /* payload */     0xF0, 0x04, 0x00, 0x00,
                                              0x00, 0x00, 0x00, 0x00,
                           /* checksum */     0x00, 0x00};

// UBX-CFG-MSG
// Load the configuration from memory
static ubx_cfg_t msg_zda[22] = { /* header */     0xB5, 0x62, 
                            /* class & id */  0x06, 0x01, 
                            /* length */      0x00, 0x08, 
                            /* payload */     0xF0, 0x08, 0x00, 0x00,
                                              0x00, 0x00, 0x00, 0x00,
                           /* checksum */     0x00, 0x00};
// // UBX-CFG-PM2
// // Load the configuration from memory
// static ubx_cfg_t pm2[22] = { /* header */     0xB5, 0x62, 
//                             /* class & id */  0x06, 0x09, 
//                             /* length */      0x00, 0x0C, 
//                             /* payload */     0x00, 0x00, 0x00, 0x00,
//                                                 0x00, 0x00, 0x00, 0x00,
//                                                 0x00, 0x00, 0x1F, 0x1F,
//                                                 0x01, 0x00,
//                             /* checksum */    0x00, 0x00};

// UBX-CFG-RXM
// Load the configuration from memory
// static ubx_cfg_t rxm[22] = { /* header */ 0xB5, 0x62, 
//                                    /* class & id */  0x06, 0x11, 
//                                    /* length */      0x00, 0x02, 
//                                    /* payload */     0x00, 0x01,
//                                    /* checksum */    0x00, 0x00};

// These should be in order of how they are going to be sent to the UBLOX GPS module
static ubx_cfg_t* ubx_configurations[NUM_UBLOX_CONFIGS] = {
    msg_rmc, msg_zda, cfg_save, cfg_load
};

#endif // _RECOVERY_UBLOX_CONFIG_H_