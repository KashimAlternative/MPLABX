// ----------------------------------------------------------------
// AQM0802
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
//#  define AQM0802_USE_RETURN_CODE

#ifndef AQM0802_H
#  define	AQM0802_H

#  include "typedef.h"

#  define I2C_ADDRESS 0x7c


#  define ROW_SELECT_0 0x80
#  define ROW_SELECT_1 0xC0

const uint08_t ROW_SELECT[] = {
  ROW_SELECT_0 ,
  ROW_SELECT_1 ,
} ;

// ----------------------------------------------------------------
// [Prototypes]
PRIVATE uint08_t _private_aqm0802_SendData( uint08_t controlByte , uint08_t dataByte ) ;

// ----------------------------------------------------------------
// [Function] Send String
uint08_t _aqm0802_SendString( uint08_t position , const char* stringPtr ) {
#  ifdef AQM0802_USE_RETURN_CODE

  uint08_t errorCode = 0x00 ;

  if ( errorCode = _private_aqm0802_SendData( 0x00 , position ) ) return errorCode ;

  uint08_t i = 1 ;
  while ( *stringPtr ) {
    if ( errorCode = _private_aqm0802_SendData( 0x40 , *stringPtr ) ) return (errorCode | ( i << 4 ) ) ;
    stringPtr++ ;
    i++ ;
  }

#  else

  _private_aqm0802_SendData( 0x00 , position ) ;

  while ( *stringPtr ) {
    _private_aqm0802_SendData( 0x40 , *stringPtr ) ;
    stringPtr++ ;
  }

#  endif

  return 0x00 ;
}


// ----------------------------------------------------------------
// [Function] Send String Clearing
uint08_t _aqm0802_SendStringClearing( uint08_t position ,const char* stringPtr ) {

  uint08_t col = 0 ;

#  ifdef AQM0802_USE_RETURN_CODE

  uint08_t errorCode = 0x00 ;

  if ( errorCode = _private_aqm0802_SendData( 0x00 , position & 0xF0 ) ) return errorCode ;

  uint08_t i = 1 ;
  while ( col != 16 ) {
    if ( col++ >= ( position & 0x0F ) && ( *stringPtr ) ) {
      if ( errorCode = _private_aqm0802_SendData( 0x40 , *stringPtr ) ) return (errorCode | ( i << 4 ) ) ;
      stringPtr++ ;
    }
    else {
      if ( errorCode = _private_aqm0802_SendData( 0x40 , ' ' ) ) return (errorCode | ( i << 4 ) ) ;
    }
    i++ ;
  }

#  else

  _private_aqm0802_SendData( 0x00 , position & 0xF0 ) ;

  while ( col != 16 ) {
    if ( col++ >= ( position & 0x0F ) && ( *stringPtr ) ) {
      _private_aqm0802_SendData( 0x40 , *stringPtr ) ;
      stringPtr++ ;
    }
    else {
      _private_aqm0802_SendData( 0x40 , ' ' ) ;
    }
  }

#  endif

  return 0x00 ;
}

// ----------------------------------------------------------------
// [Function] Send Character
uint08_t _aqm0802_SendCharacter( uint08_t position , char character ) {
#  ifdef AQM0802_USE_RETURN_CODE

  uint08_t errorCode = 0x00 ;

  if ( errorCode = _private_aqm0802_SendData( 0x00 , position ) ) return errorCode ;
  if ( errorCode = _private_aqm0802_SendData( 0x40 , character ) ) return errorCode ;

  return 0x00 ;

#  else

  _private_aqm0802_SendData( 0x00 , position ) ;
  _private_aqm0802_SendData( 0x40 , character ) ;

#  endif

  return 0x00 ;
}


// ----------------------------------------------------------------
// [Function] ClearRow
uint08_t _aqm0802_ClearRow( uint08_t rowSelect ) {
#  ifdef AQM0802_USE_RETURN_CODE

  uint08_t errorCode = 0x00 ;
  if ( errorCode = _private_aqm0802_SendData( 0x00 , rowSelect & 0xF0 ) ) return errorCode ;

  uint08_t count = 0 ;
  while ( count++ != 16 )
    if ( errorCode = _private_aqm0802_SendData( 0x40 , ' ' ) ) return (errorCode | ( count << 4 ) ) ;

#  else

  _private_aqm0802_SendData( 0x00 , rowSelect & 0xF0 ) ;

  uint08_t count = 0 ;
  while ( count++ != 16 )
    _private_aqm0802_SendData( 0x40 , ' ' ) ;

#  endif

  return 0x00 ;
}

// ----------------------------------------------------------------
// [Function] Initialize
uint08_t _aqm0802_Initialize( ) {

#  ifdef AQM0802_USE_RETURN_CODE
  uint08_t errorCode = 0x00 ;
#  endif

#  ifdef AQM0802_USE_RETURN_CODE
  if ( errorCode = _private_aqm0802_SendData( 0x00 , 0x39 ) ) return errorCode | 0x10 ;
  if ( errorCode = _private_aqm0802_SendData( 0x00 , 0x14 ) ) return errorCode | 0x20 ;
  if ( errorCode = _private_aqm0802_SendData( 0x00 , 0x70 ) ) return errorCode | 0x30 ;
  if ( errorCode = _private_aqm0802_SendData( 0x00 , 0x56 ) ) return errorCode | 0x40 ;
  if ( errorCode = _private_aqm0802_SendData( 0x00 , 0x6c ) ) return errorCode | 0x50 ;
#  else
  _private_aqm0802_SendData( 0x00 , 0x39 ) ;
  _private_aqm0802_SendData( 0x00 , 0x14 ) ;
  _private_aqm0802_SendData( 0x00 , 0x70 ) ;
  _private_aqm0802_SendData( 0x00 , 0x56 ) ;
  _private_aqm0802_SendData( 0x00 , 0x6c ) ;
#  endif

  __delay_ms( 200 ) ;

#  ifdef AQM0802_USE_RETURN_CODE
  if ( errorCode = _private_aqm0802_SendData( 0x00 , 0x38 ) ) return errorCode | 0x60 ;
  if ( errorCode = _private_aqm0802_SendData( 0x00 , 0x0C ) ) return errorCode | 0x70 ;
#  else
  _private_aqm0802_SendData( 0x00 , 0x38 ) ;
  _private_aqm0802_SendData( 0x00 , 0x0C ) ;
#  endif

  return 0x00 ;

}

// ----------------------------------------------------------------
// [Function] Clear
uint08_t _aqm0802_Clear( ) {

#  ifdef AQM0802_USE_RETURN_CODE
  uint08_t errorCode = 0x00 ;
  if ( errorCode = _private_aqm0802_SendData( 0x00 , 0x01 ) ) return errorCode ;
#  else
  _private_aqm0802_SendData( 0x00 , 0x01 ) ;
#  endif

  __delay_ms( 2 ) ;

  return 0x00 ;
}

// ----------------------------------------------------------------
// [Function] Set CGRAM
uint08_t _aqm0802_SetCgram( char charCode , uint08_t data[] ) {
#  ifdef AQM0802_USE_RETURN_CODE

  uint08_t errorCode = 0x00 ;

  if ( errorCode = _private_aqm0802_SendData( 0x00 , ( ( charCode << 3 ) & 0b00111111 ) | 0b01000000 ) ) return errorCode ;

  for ( uint08_t i = 0 ; i < 8 ; i++ )
    if ( errorCode = _private_aqm0802_SendData( 0x40 , data[i] ) ) return (errorCode | ( i << 4 ) ) ;

#  else

  _private_aqm0802_SendData( 0x00 , ( ( charCode << 3 ) & 0b00111111 ) | 0b01000000 ) ;

  for ( uint08_t i = 0 ; i < 8 ; i++ )
    _private_aqm0802_SendData( 0x40 , data[i] ) ;

#  endif

  return 0x00 ;
}

// ----------------------------------------------------------------
// [Function] SendData
PRIVATE uint08_t _private_aqm0802_SendData( uint08_t controlByte , uint08_t dataByte ) {

  WaitTimer( ) ;

  EnableStartBit( ) ;
  ClearFlag( ) ;

#  ifdef AQM0802_USE_RETURN_CODE

  uint08_t errorCode = 0x00 ;

  // Send Address
  if ( !errorCode ) {
    SendByte( I2C_ADDRESS ) ;
    WaitFlag( ) ;
    if ( IsError( ) ) errorCode = 0x01 ;
  }

  // Send Control Byte
  if ( !errorCode ) {
    SendByte( controlByte ) ;
    WaitFlag( ) ;
    if ( IsError( ) ) errorCode = 0x02 ;
  }

  // Send Data Byte
  if ( !errorCode ) {
    SendByte( dataByte ) ;
    WaitFlag( ) ;
    if ( IsError( ) ) errorCode = 0x03 ;
  }

#  else

  // Send Address
  SendByte( I2C_ADDRESS ) ;
  WaitFlag( ) ;

  // Send Control Byte
  SendByte( controlByte ) ;
  WaitFlag( ) ;

  // Send Data Byte
  SendByte( dataByte ) ;
  WaitFlag( ) ;

#  endif

  EnableStopBit( ) ;
  ResetTimer( ) ;

#  ifdef AQM0802_USE_RETURN_CODE
  return errorCode ;
#  else
  return 0x00 ;
#  endif

}

#endif	/* AQM0802_H */

