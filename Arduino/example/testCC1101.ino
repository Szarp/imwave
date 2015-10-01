/*
        mesh CC1101 example
        for more information: www.imwave

	Copyright (c) 2015 Dariusz Mazur
        
        v0.1 - 2015.09.01
	    Basic implementation - functions and comments are subject to change
        
	Permission is hereby granted, free of charge, to any person obtaining a copy of this software
	and associated documentation files (the "Software"), to deal in the Software without restriction,
	including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense,
	and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so.

	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT
	LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
	IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
	LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
	WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

//#include <SoftwareSerial.h>
//#include <avr/wdt.h>
//#.define DBGLVL 2
#include "imdebug.h"


#include "imtrans.h"
#include "imbroadcast.h"
#include "uart.h"



/******************************** Configuration *************************************/

#define MID  0x00  //My ID
#define TID  0x00  //Default target ID
// #define HOPS 0x01  //Max hops - currently unused



#define RADIO_BUFFSIZE 725  //Buffer for radio
#define RADIO_SENDTHRES 710  //Start writing when this full

#define RESEND_TIMEOUT 75  //Wait this time in ms for an radio ack
#define PACKET_GAP 10 //Delay between receive and outgoing ack

//Used for error signaling (ON when restransmitting, OFF on receive success)
#define ERRLEDON() digitalWrite(13,HIGH)
#define ERRLEDOFF() digitalWrite(13,LOW)
#define ERRLEDINIT() pinMode(13, OUTPUT)



/******************************* Module specific settings **********************/




  
  #define bridgeSpeed 9600
  #define bridgeBurst 10  //Chars written out per call
  #define radioDelay 3000  //Time between calls - Let hardware serial write out async





#define INDEX_NWID 0  //Network ID
#define INDEX_SRC  1  //Source ID
#define INDEX_DEST 2  //Target ID
#define INDEX_SEQ  3  //Senders current sequence number



/****************** Vars *******************************/



char radioBuf[RADIO_BUFFSIZE] = {0,};
unsigned short radioBufLen = 0;
unsigned short radio_writeout = 0xFFFF;
unsigned long radioOut_delay = 0;


IMCC1101  cc1101;
Transceiver trx;
IMBroadcast broadcast(cc1101);
/************************* Functions **********************/


void printRadio()
{
    DBGINFO("Radio:");
    DBGINFO(radioBufLen);
    for (int i=0 ; i<bridgeBurst && (radio_writeout<radioBufLen) ; radio_writeout++, i++ )
    {
      DBGINFO(radioBuf[radio_writeout]);
    }
    DBGINFO(";\r\n");

}




//Initialize the system
void setup()
{
//  wdt_enable(WDTO_8S);  //Watchdog 8s
  INITDBG();

  ERRLEDINIT(); ERRLEDOFF();
  trx.Init(cc1101);
  trx.myID= MID;
  DBGINFO("classtest");
  DBGINFO(Transceiver::ClassTest());
}

//Main loop is called over and over again

byte GetData()
{
  static IMFrame rxFrame;
  if (trx.GetData()  )
    {
      trx.printReceive();
      if (trx.GetFrame(rxFrame))
      {
        DBGINFO(" RSSI: ");           DBGINFO(trx.Rssi());            DBGINFO("dBm");
//        DBGINFO(" CRC: ");            DBGINFO(trx.crc);             DBGINFO(" rr: ");
        if (rxFrame.Knock())
           DBGINFO(" Knock ");
        else if (rxFrame.Welcome())
           DBGINFO(" Welcome ");
//          trx.parseKnock()
        else if (rxFrame.ACK())
        {
           trx.ReceiveACK(rxFrame);

           DBGINFO(" ACK ");
        } else{
          if (rxFrame.NeedACK())
             trx.SendACK(rxFrame);
          if  (rxFrame.Destination()!=MID)
          {
             if (trx.Routing(rxFrame))
             DBGINFO(" Route ");
          }
          else
          {
            radioBufLen+=rxFrame.Get((uint8_t*)&(radioBuf[radioBufLen]));
            if (radio_writeout == 0xFFFF) radio_writeout = 0;  //signal the uart handler to start writing out
          }
          if (rxFrame.Repeat())
              return 1;
        }
      }
      else
      {
        DBGERR("!VALID");
      }
      DBGINFO("\r\n");
  }
  return 0;

}


void loop()
{
  ERRLEDON();         delay(50);         ERRLEDOFF();
  static IMFrame frame;




  trx.StartReceive();
  /************** radio to UART ************************/
  if ( radioOut_delay<millis())
  {
    radioOut_delay = millis()+radioDelay;
    printRadio();
    radio_writeout = 0xFFFF;  //signal write complete to radio reception
    radioBufLen = 0;
  }

  delay(500);

  do {} while (GetData());

  // prepare data
  generatorUart();    DBGINFO(" R  ");


  if ((millis() %10) <7)
  {
      UartPrepareData(frame);
      trx.Push(frame);
      DBGINFO("PUSH (");
  }

  if (trx.Retry())
  {
      DBGINFO("Retry");
  }

   
  if (trx.Transmit())
  {
      DBGINFO("transmit:");  DBGINFO(millis());    DBGINFO(" ");
      DBGINFO(trx.TX_buffer.len);    DBGINFO(",");
  }
  DBGINFO(")\r\n");

  
//  Serial.flush();

}

