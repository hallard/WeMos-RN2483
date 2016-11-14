// **********************************************************************************
// RN2483 header file for WeMos Lora RN2483 Shield
// **********************************************************************************
// Creative Commons Attrib Share-Alike License
// You are free to use/extend this library but please abide with the CC-BY-SA license:
// http://creativecommons.org/licenses/by-sa/4.0/
//
// Written by Charles-Henri Hallard http://ch2i.eu
//
// History : V1.20 2016-06-11 - Creation
//
// All text above must be included in any redistribution.
//
// **********************************************************************************

// Master Project File
#include "WeMos-rn2483.h"

#ifndef _RN2483_H
#define _RN2483_H

#define RN2483_RESET_PIN 15

#define RN2483_STATUS_JOINED            0x00000001
#define RN2483_STATUS_MAC_STATE         0x0000000E
#define RN2483_STATUS_AUTO_REPLY        0x00000010
#define RN2483_STATUS_ADR               0x00000020
#define RN2483_STATUS_SILENT            0x00000040
#define RN2483_STATUS_MAC_PAUSED        0x00000080
#define RN2483_STATUS_RFU               0x00000100
#define RN2483_STATUS_LINK_CHECK        0x00000200
#define RN2483_STATUS_CHAN_UPDATED      0x00000400
#define RN2483_STATUS_OUTPWR_UPDATED    0x00000800
#define RN2483_STATUS_NB_REP_UPDATED    0x00001000
#define RN2483_STATUS_PRESC_UPDATED     0x00002000
#define RN2483_STATUS_SEC_RCV_UPDATED   0x00004000
#define RN2483_STATUS_TX_SETUP_UPDATED  0x00008000
#define RN2483_REJOIN_NEEDED            0x00010000
#define RN2483_STATUS_ERROR             0xFFFFFFFF


// The RN2483 radio mac state (extracted from MAC status)
// See datasheet page 40 MAC SATUS BIT MAPPED REGISTERS
typedef enum {
  MAC_IDLE = 0,
  MAC_TRANSMIT,
  MAC_BEFORE_W1,
  MAC_W1_OPEN,
  MAC_BETWEEN_W1_W2,
  MAC_W2_OPEN,      
  MAC_ACK_TIMEOUT,
} rn2483_mac_state_e;

// The radio state machine when talking with RN2483
typedef enum {
  RADIO_IDLE,
  RADIO_SENDING,
  RADIO_RECEIVED_DATA,
  RADIO_WAIT_OK,					 /* just Wait Ok  */
  RADIO_WAIT_OK_SEND, 		 /* Wait Ok and after we sent a packet */
  RADIO_ERROR
} rn2483_state_e ;

// exported vars
extern unsigned long rn2483_status;

// exported functions
void rn2483Exec( char * cmd);
void rn2483Exec_P( PGM_P cmd);
void rn2483Init(uint32_t baudrate);
void rn2483Reset(void);
void rn2483Idle(void);
bool rn2483Send(char * cmd);
void rn2483Listen(void);
bool rn2483MacStatusResponse(String inputString);
bool rn2483Response(String inputString);
void rn2483ManageState(btn_action_e btn);
void rn2483printMACStatus(uint32_t status); 
void rn2483printRadioState(void) ;
uint32_t rn2483getMACStatus(void);
uint32_t rn2483MacStatus(void);
uint8_t rn2483State(void);

#endif