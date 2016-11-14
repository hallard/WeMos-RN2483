// **********************************************************************************
// RN2483 source file for WeMos Lora RN2483 Shield
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
#include "rn2483.h"

// RN2483 Internal Status register 
uint32_t  _rn2483_mac_status;

// our RN2483 Radio State Machine
rn2483_state_e _rn2483_state = RADIO_IDLE;

void rn2483Init(uint32_t baudrate) 
{
	// Close proper
  SERIAL_DEVICE.flush(); 
  //SERIAL_DEVICE.end();

  // enable auto baud rate (see RN2483 datasheet)
  SERIAL_DEVICE.begin(baudrate);
  SERIAL_DEVICE.write((byte) 0x00);
  SERIAL_DEVICE.flush(); 
  SERIAL_DEVICE.write(0x55);
  SERIAL_DEVICE.flush(); 
  SERIAL_DEVICE.write(0x0A);
  SERIAL_DEVICE.write(0x0D);
  
  _rn2483_state = RADIO_IDLE;
}

void rn2483Reset(void) 
{
  // Run startup script if any
  execCommand(NULL, PSTR("read /startup.ini") );
  _rn2483_state = RADIO_IDLE;
}

// Send a radio command
void rn2483Exec( char * cmd) 
{
  // Set to blue immediatly
  LedRGBON(COLOR_BLUE, true);
  ws.textAll(cmd);
  execCommand(NULL, cmd);
}

// Send a radio command
void rn2483Exec_P( PGM_P cmd) 
{
	String str = cmd;	
	rn2483Exec( (char *) str.c_str());
}

void rn2483printMACStatus(uint32_t status) 
{
	if (status & RN2483_STATUS_MAC_STATE) {
		rn2483_mac_state_e mac_state = (rn2483_mac_state_e) ((status & RN2483_STATUS_MAC_STATE) >> 1);

		if (mac_state==MAC_IDLE) {
			SERIAL_DEBUG.print(F("Idle"));
		} else if (mac_state==MAC_TRANSMIT) {
			SERIAL_DEBUG.print(F("Transmission occuring"));
		} else if (mac_state==MAC_BEFORE_W1) {
			SERIAL_DEBUG.print(F("Before openning receive W1"));
		} else if (mac_state==MAC_W1_OPEN) {
			SERIAL_DEBUG.print(F("Receive W1 open"));
		} else if (mac_state==MAC_BETWEEN_W1_W2) {
			SERIAL_DEBUG.print(F("Between receive W1 and W2"));
		} else if (mac_state==MAC_W2_OPEN) {
			SERIAL_DEBUG.print(F("Receive W2 open"));
		} else if (mac_state==MAC_ACK_TIMEOUT) {
			SERIAL_DEBUG.print(F("ACK Timeout"));
		} else {
			SERIAL_DEBUG.print(F("??????"));
		}
	}

	if (status&RN2483_STATUS_JOINED)     			{SERIAL_DEBUG.print(F(" Joined"));}
	if (status&RN2483_STATUS_AUTO_REPLY) 			{SERIAL_DEBUG.print(F(" Auto-Reply"));}
	if (status&RN2483_STATUS_ADR) 						{SERIAL_DEBUG.print(F(" ADR"));}
	if (status&RN2483_STATUS_SILENT) 					{SERIAL_DEBUG.print(F(" Silent"));}
	if (status&RN2483_STATUS_MAC_PAUSED) 			{SERIAL_DEBUG.print(F(" Paused"));}
	if (status&RN2483_STATUS_RFU) 						{SERIAL_DEBUG.print(F(" RFU"));}
	if (status&RN2483_STATUS_LINK_CHECK) 			{SERIAL_DEBUG.print(F(" Link-Check"));}
	if (status&RN2483_STATUS_CHAN_UPDATED) 		{SERIAL_DEBUG.print(F(" Chan-Upd"));}
	if (status&RN2483_STATUS_OUTPWR_UPDATED)	{SERIAL_DEBUG.print(F(" Powr-Upd"));}
	if (status&RN2483_STATUS_NB_REP_UPDATED)	{SERIAL_DEBUG.print(F(" NRep-Upd"));}
	if (status&RN2483_STATUS_PRESC_UPDATED) 	{SERIAL_DEBUG.print(F(" Prsc-Upd"));}
	if (status&RN2483_STATUS_SEC_RCV_UPDATED)	{SERIAL_DEBUG.print(F(" 2Win-Upd"));}
	if (status&RN2483_STATUS_TX_SETUP_UPDATED){SERIAL_DEBUG.print(F(" TX-Upd"));}
	if (status&RN2483_REJOIN_NEEDED)					{SERIAL_DEBUG.print(F(" Rejoin"));}

	SERIAL_DEBUG.println("");
}

void rn2483printRadioState(void) 
{
	rn2483_state_e state = _rn2483_state;
	if (state == RADIO_IDLE)     			{SERIAL_DEBUG.print(F("Idle"));}
	if (state == RADIO_SENDING)     	{SERIAL_DEBUG.print(F("Sending"));}
	if (state == RADIO_RECEIVED_DATA) {SERIAL_DEBUG.print(F("Received"));}
	if (state == RADIO_WAIT_OK)     	{SERIAL_DEBUG.print(F("Wait_OK"));}
	if (state == RADIO_WAIT_OK_SEND)  {SERIAL_DEBUG.print(F("Wait_OK_Send"));}
	if (state == RADIO_ERROR)     		{SERIAL_DEBUG.print(F("Error"));}
}

uint32_t rn2483getMACStatus(void) 
{
	uint32_t mac_status = RN2483_STATUS_ERROR;

  SERIAL_DEBUG.print(F("getRadioStatus()..."));
  rn2483Exec_P( PSTR("mac get status"));

  // Wait until response '\n' from device
  if (handleSerial(false) ) {
  	// We got it ?
  	if ( rn2483MacStatusResponse(inputString)) {
  		// Ok our reponse will be the one we just received 
  		mac_status = rn2483MacStatus();
  	}
  } else {
    SERIAL_DEBUG.println(F("time-out"));
  }
  // Clear Serial response
  inputString = "";

  return mac_status;
}

// Send data 
bool rn2483Send(char * cmd) 
{
  if (_rn2483_state != RADIO_IDLE) 
    return false;

  // Set to pink immediatly
  LedRGBON(COLOR_PINK, true);
  ws.textAll(cmd);
  execCommand(NULL, cmd);
  _rn2483_state = RADIO_WAIT_OK_SEND;
  return true;
}

// Put RN2483 into listening mode
void rn2483Listen(void) {
/*
	// Idle or previously listening ?
	if ( _rn2483_state==RADIO_IDLE || _rn2483_state==RADIO_LISTENING ) {
	  char cmd[32];

	  // Set receive Watchdog to 1H
	  //sprintf_P( cmd, PSTR("radio set wdt 3600000"));
	  //ws.textAll(cmd);
	  //execCommand(NULL, cmd);
	  //delay(250);

	  // Enable Receive mode
	  sprintf_P( cmd, PSTR("radio rx 0"));
	  ws.textAll(cmd);
	  execCommand(NULL, cmd);

	  // Wait ok listenning
	  //_rn2483_state = RADIO_WAIT_OK_LISTENING;
	}
	*/
}

// Check is reponse could be a mac status response
bool rn2483MacStatusResponse(String inputString) {

	uint32_t mac_status;

	// Is it a MAC Status response (4 HEX byte long)
	if (inputString.length() == 8) {
  	bool hexok = true;
  	// Check it's a valid Hex 
  	for (uint8_t i=0; i<8; i++) {
			if ( !isxdigit(inputString.charAt(i)) ) {
				hexok=false;
				break;
			}
		}
		// if all is okay
		if (hexok) {
      mac_status = strtoul(inputString.c_str(), NULL, 16);
  		if (mac_status) {
  			// It's good save it and display
 			  _rn2483_mac_status = mac_status ;
				rn2483printMACStatus(mac_status);
				return true;
	 		} else {
	  		SERIAL_DEBUG.println(F("empty\n"));
  		}
		} else {
      SERIAL_DEBUG.println(F("bad hex value"));
		}
  } else {
    SERIAL_DEBUG.println(F("bad response lenght"));
  }
  return false;
}

// we received a RN2483 serial response
bool rn2483Response(String s) {

  if (_rn2483_state == RADIO_WAIT_OK_SEND) {
		if ( 	s != "ok" ) {
		  LedRGBON(COLOR_RED, true);
	  	_rn2483_state = RADIO_IDLE;
		} else {
			_rn2483_state = RADIO_SENDING;
		} 			

  } else if (_rn2483_state == RADIO_SENDING) {
	  _rn2483_state = RADIO_IDLE;
		// Unconfirmed Transmit was okay
		if (inputString == "mac_tx_ok" ) {
		  LedRGBON(COLOR_GREEN, true);
		// Confirmed Transmit was okay
		} else if (inputString.startsWith("mac_rx ")) {
		  _rn2483_state = RADIO_RECEIVED_DATA;
		  LedRGBON(COLOR_GREEN, true);
		  // got something to do (read value)
		  return (true);
		// radio_err
		} else  {
		  LedRGBON(COLOR_RED, true);
		}

	// Received something (without sent before or already received one response)	
  } else if (inputString.startsWith("mac_rx ")) {
		  _rn2483_state = RADIO_RECEIVED_DATA;
		  LedRGBON(COLOR_YELLOW_GREEN, true);
		  // got something to do (read value)
		  return (true);

  // Assume we done 
	// TO DO other mode/functions
	} else {
	  _rn2483_state = RADIO_IDLE;
	}

	return false;
}

// Manage radio state machine
void rn2483ManageState(btn_action_e btn) {
	static rn2483_state_e old_rn2483_state = _rn2483_state;

	// Action due to push button?
	if (btn != BTN_NONE) {
	  char cmd[32];

    if ( btn == BTN_QUICK_PRESS) {
    	// Send unconfirmed packet (millis())
	 		sprintf_P(cmd, PSTR("mac tx uncnf 1 %lu"), seconds);
      rn2483Send(cmd);
    } else if ( btn == BTN_PRESSED_12) {
    	// Send confirmed packet (millis())
	 		sprintf_P(cmd, PSTR("mac tx cnf 1 %lu"), seconds);
      rn2483Send(cmd);
    } else if ( btn == BTN_PRESSED_23) {
    	/*
 			if (!rn2483_continuous_send) {
	      LedRGBON(COLOR_MAGENTA, true);
 				rn2483_continuous_send = true;
 			} else {
	      LedRGBOFF();
 				rn2483_continuous_send = false;
 			}
 			*/
    }
	}

  // Check radio state changed ?
  if (_rn2483_state != old_rn2483_state) {
    old_rn2483_state = _rn2483_state;

    // We received data
    if (_rn2483_state == RADIO_RECEIVED_DATA) {
    	// To DO parse date
    	_rn2483_state = RADIO_IDLE;

    } 

  } // Radio state changed
}

// get radio state machine
uint8_t rn2483State(void) {
  return _rn2483_state;
}

// get radio MAC status
uint32_t rn2483MacStatus(void) {
  return _rn2483_mac_status;
}