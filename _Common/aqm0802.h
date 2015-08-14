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

#  define AQM0802_I2C_ADDRESS 0x7c

// ----------------------------------------------------------------
// [Enum] Row Select
typedef enum{
 ROW_SELECT_0 =0x80,
 ROW_SELECT_1 =0xC0,
} AQM0802_EnRowSelect;
const AQM0802_EnRowSelect ROW_SELECT[] = {
  ROW_SELECT_0 ,
  ROW_SELECT_1 ,
} ;

// ----------------------------------------------------------------
// [Prototypes]
PRIVATE Uint08_t AQM0802_SendData( Uint08_t controlByte , Uint08_t dataByte ) ;

// ----------------------------------------------------------------
// [Function] Send String
Uint08_t AQM0802_SendString( Uint08_t position , const char* stringPtr ) {
#  ifdef AQM0802_USE_RETURN_CODE

  Uint08_t errorCode = 0x00 ;

  if ( errorCode = AQM0802_SendData( 0x00 , position ) ) return errorCode ;

  Uint08_t i = 1 ;
  while ( *stringPtr ) {
    if ( errorCode = AQM0802_SendData( 0x40 , *stringPtr ) ) return (errorCode | ( i << 4 ) ) ;
    stringPtr++ ;
    i++ ;
  }

#  else

  AQM0802_SendData( 0x00 , position ) ;

  while ( *stringPtr ) {
    AQM0802_SendData( 0x40 , *stringPtr ) ;
    stringPtr++ ;
  }

#  endif

  return 0x00 ;
}


// ----------------------------------------------------------------
// [Function] Send String Clearing
Uint08_t AQM0802_SendStringClearing( Uint08_t position ,const char* stringPtr ) {

  Uint08_t col = 0 ;

#  ifdef AQM0802_USE_RETURN_CODE

  Uint08_t errorCode = 0x00 ;

  if ( errorCode = AQM0802_SendData( 0x00 , position & 0xF0 ) ) return errorCode ;

  Uint08_t i = 1 ;
  while ( col != 16 ) {
    if ( col++ >= ( position & 0x0F ) && ( *stringPtr ) ) {
      if ( errorCode = AQM0802_SendData( 0x40 , *stringPtr ) ) return (errorCode | ( i << 4 ) ) ;
      stringPtr++ ;
    }
    else {
      if ( errorCode = AQM0802_SendData( 0x40 , ' ' ) ) return (errorCode | ( i << 4 ) ) ;
    }
    i++ ;
  }

#  else

  AQM0802_SendData( 0x00 , position & 0xF0 ) ;

  while ( col != 16 ) {
    if ( col++ >= ( position & 0x0F ) && ( *stringPtr ) ) {
      AQM0802_SendData( 0x40 , *stringPtr ) ;
      stringPtr++ ;
    }
    else {
      AQM0802_SendData( 0x40 , ' ' ) ;
    }
  }

#  endif

  return 0x00 ;
}

// ----------------------------------------------------------------
// [Function] Send Character
Uint08_t AQM0802_SendCharacter( Uint08_t position , char character ) {
#  ifdef AQM0802_USE_RETURN_CODE

  Uint08_t errorCode = 0x00 ;

  if ( errorCode = AQM0802_SendData( 0x00 , position ) ) return errorCode ;
  if ( errorCode = AQM0802_SendData( 0x40 , character ) ) return errorCode ;

  return 0x00 ;

#  else

  AQM0802_SendData( 0x00 , position ) ;
  AQM0802_SendData( 0x40 , character ) ;

#  endif

  return 0x00 ;
}


// ----------------------------------------------------------------
// [Function] ClearRow
Uint08_t AQM0802_ClearRow( Uint08_t rowSelect ) {
#  ifdef AQM0802_USE_RETURN_CODE

  Uint08_t errorCode = 0x00 ;
  if ( errorCode = AQM0802_SendData( 0x00 , rowSelect & 0xF0 ) ) return errorCode ;

  Uint08_t count = 0 ;
  while ( count++ != 16 )
    if ( errorCode = AQM0802_SendData( 0x40 , ' ' ) ) return (errorCode | ( count << 4 ) ) ;

#  else

  AQM0802_SendData( 0x00 , rowSelect & 0xF0 ) ;

  Uint08_t count = 0 ;
  while ( count++ != 16 )
    AQM0802_SendData( 0x40 , ' ' ) ;

#  endif

  return 0x00 ;
}

// ----------------------------------------------------------------
// [Function] Initialize
Uint08_t AQM0802_Initialize( ) {

#  ifdef AQM0802_USE_RETURN_CODE
  Uint08_t errorCode = 0x00 ;
#  endif

#  ifdef AQM0802_USE_RETURN_CODE
  if ( errorCode = AQM0802_SendData( 0x00 , 0x39 ) ) return errorCode | 0x10 ;
  if ( errorCode = AQM0802_SendData( 0x00 , 0x14 ) ) return errorCode | 0x20 ;
  if ( errorCode = AQM0802_SendData( 0x00 , 0x70 ) ) return errorCode | 0x30 ;
  if ( errorCode = AQM0802_SendData( 0x00 , 0x56 ) ) return errorCode | 0x40 ;
  if ( errorCode = AQM0802_SendData( 0x00 , 0x6c ) ) return errorCode | 0x50 ;
#  else
  AQM0802_SendData( 0x00 , 0x39 ) ;
  AQM0802_SendData( 0x00 , 0x14 ) ;
  AQM0802_SendData( 0x00 , 0x70 ) ;
  AQM0802_SendData( 0x00 , 0x56 ) ;
  AQM0802_SendData( 0x00 , 0x6c ) ;
#  endif

  __delay_ms( 200 ) ;

#  ifdef AQM0802_USE_RETURN_CODE
  if ( errorCode = AQM0802_SendData( 0x00 , 0x38 ) ) return errorCode | 0x60 ;
  if ( errorCode = AQM0802_SendData( 0x00 , 0x0C ) ) return errorCode | 0x70 ;
#  else
  AQM0802_SendData( 0x00 , 0x38 ) ;
  AQM0802_SendData( 0x00 , 0x0C ) ;
#  endif

  return 0x00 ;

}

// ----------------------------------------------------------------
// [Function] Clear
Uint08_t AQM0802_Clear( ) {

#  ifdef AQM0802_USE_RETURN_CODE
  Uint08_t errorCode = 0x00 ;
  if ( errorCode = AQM0802_SendData( 0x00 , 0x01 ) ) return errorCode ;
#  else
  AQM0802_SendData( 0x00 , 0x01 ) ;
#  endif

  __delay_ms( 2 ) ;

  return 0x00 ;
}

// ----------------------------------------------------------------
// [Function] Set CGRAM
Uint08_t AQM0802_SetCgram( char charCode , Uint08_t data[] ) {
#  ifdef AQM0802_USE_RETURN_CODE

  Uint08_t errorCode = 0x00 ;

  if ( errorCode = AQM0802_SendData( 0x00 , ( ( charCode << 3 ) & 0b00111111 ) | 0b01000000 ) ) return errorCode ;

  for ( Uint08_t i = 0 ; i < 8 ; i++ )
    if ( errorCode = AQM0802_SendData( 0x40 , data[i] ) ) return (errorCode | ( i << 4 ) ) ;

#  else

  AQM0802_SendData( 0x00 , ( ( charCode << 3 ) & 0b00111111 ) | 0b01000000 ) ;

  for ( Uint08_t i = 0 ; i < 8 ; i++ )
    AQM0802_SendData( 0x40 , data[i] ) ;

#  endif

  return 0x00 ;
}

// ----------------------------------------------------------------
// [Function] SendData
PRIVATE Uint08_t AQM0802_SendData( Uint08_t controlByte , Uint08_t dataByte ) {

  WaitTimer( ) ;

  EnableStartBit( ) ;
  ClearFlag( ) ;

#  ifdef AQM0802_USE_RETURN_CODE

  Uint08_t errorCode = 0x00 ;

  // Send Address
  if ( !errorCode ) {
    SendByte( AQM0802_I2C_ADDRESS ) ;
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
  SendByte( AQM0802_I2C_ADDRESS ) ;
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

