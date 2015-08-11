// ----------------------------------------------------------------
// DS1307
// Revision 2015/03/08
//
// ----------------------------------------------------------------
// Description
//
// Following macros must be defined
//
//#define _XTAL_FREQ
//#  define EnableStartBit()
//#  define EnableStopBit()
//#  define SendByte(data)
//#  define WaitFlag()
//#  define ClearFlag()
//#  define IsError()

#ifndef DS1307_H
#  define	DS1307_H

#  include "date_time.h"
#  include "typedef.h"

#  define XTAL_FREQ 16000000

#  define I2C_ADDRESS 0xD0

#  define BIT_SEN SSP1CON2bits.SEN
#  define BIT_PEN SSP1CON2bits.PEN
#  define BIT_RCEN SSP1CON2bits.RCEN
#  define BIT_RSEN SSP1CON2bits.RSEN
#  define BIT_ACKDT SSP1CON2bits.ACKDT
#  define BIT_ACKEN SSP1CON2bits.ACKEN
#  define BIT_ACKSTAT SSP1CON2bits.ACKSTAT
#  define BIT_BF SSPSTATbits.BF

#  define WAIT_MICROSECOND 28

// [Prototypes] ----------------
void _date_time_ConvertByteToDiscrete( DateTime* date , char* dateString , DateSelect select ) ;

// [Function] Counfigurate ----------------
uint08 _ds1307_Counfigurate( ) {

  // SEN
  BIT_SEN = 1 ;
  while ( BIT_SEN ) ;
  SSP1IF = 0 ;

  // Send I2C Address
  SSP1BUF = I2C_ADDRESS ;
  while ( !SSP1IF ) ;
  SSP1IF = 0 ;
  if ( BIT_ACKSTAT ) return 1 ;

  // Send RAM Address
  SSP1BUF = 0x07 ;
  while ( !SSP1IF ) ;
  SSP1IF = 0 ;
  if ( BIT_ACKSTAT ) return 2 ;

  // Send Data
  SSP1BUF = 0x10 ;
  while ( !SSP1IF ) ;
  SSP1IF = 0 ;
  if ( BIT_ACKSTAT ) return 3 ;

  // PEN
  BIT_PEN = 1 ;
  while ( BIT_PEN ) ;

  return 0 ;
}

// [Function] Get Data ----------------
uint08 _ds1307_GetData( DateTime* date , uint08 ramAddress , uint08 length ) {

  // SEN
  BIT_SEN = 1 ;
  while ( BIT_SEN ) ;
  SSP1IF = 0 ;

  // Send I2C Address
  SSP1BUF = I2C_ADDRESS ;
  while ( !SSP1IF ) ;
  SSP1IF = 0 ;
  if ( BIT_ACKSTAT ) return 1 ;

  // Send RAM Address
  SSP1BUF = ramAddress ;
  while ( !SSP1IF ) ;
  SSP1IF = 0 ;
  if ( BIT_ACKSTAT ) return 2 ;

  BIT_RSEN = 1 ;
  while ( BIT_RSEN ) ;

  // Send I2C Address
  SSP1BUF = I2C_ADDRESS | 0b1 ;
  while ( !SSP1IF ) ;
  SSP1IF = 0 ;
  if ( BIT_ACKSTAT ) return 3 ;

  // Receive Data
  uint08 dataCount = 0 ;
  while ( dataCount < length ) {
    __delay_ms( 2 ) ;
    BIT_RCEN = 1 ;
    while ( BIT_RCEN ) ;
    while ( !BIT_BF ) ;
    date->array[ dataCount++ ] = SSP1BUF ;
    BIT_ACKDT = ( dataCount == length ) ;
    BIT_ACKEN = 1 ;
  }

  // PEN
  BIT_PEN = 1 ;
  while ( BIT_PEN ) ;

  return 0 ;

}

// [Function] Write Data ----------------
uint08 _ds1307_SetClock( DateTime* date ) {

  // SEN
  BIT_SEN = 1 ;
  while ( BIT_SEN ) ;
  SSP1IF = 0 ;

  // Send I2C Address
  SSP1BUF = I2C_ADDRESS ;
  while ( !SSP1IF ) ;
  SSP1IF = 0 ;
  if ( BIT_ACKSTAT ) return 1 ;

  // Send RAM Address
  SSP1BUF = 0x00 ;
  while ( !SSP1IF ) ;
  SSP1IF = 0 ;
  if ( BIT_ACKSTAT ) return 2 ;

  for ( uint08 i = 0 ; i < sizeof ( DateTime ) ; i++ ) {
    // Send Data
    SSP1BUF = date->array[i] ;
    while ( !SSP1IF ) ;
    SSP1IF = 0 ;
    if ( BIT_ACKSTAT ) return 3 ;
  }

  // PEN
  BIT_PEN = 1 ;
  while ( BIT_PEN ) ;

  return 0 ;
}

#endif	/* DS1307_H */

