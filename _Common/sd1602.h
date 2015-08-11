// ----------------------------------------------------------------
// SD1602 (with 8-bit mode)
// Revision 2015/03/11
//
// ----------------------------------------------------------------
// Description
//
// Following macros must be defined
//
// #define _XTAL_FREQ
// #define SD1602_EnableWrite( )
// #define SD1602_SelectResister( r )
// #define SD1602_SetData( data )

#ifndef SD1602_H
#  define SD1602_H

#  include "typedef.h"

#  define ROW_SELECT_0 0x80
#  define ROW_SELECT_1 0xC0

const uint08 ROW_SELECT[] = {
  ROW_SELECT_0 ,
  ROW_SELECT_1 ,
} ;

//----------------------------------------------------------------
// [Prototypes]
void _private_sd1602_WriteByte( uint08 data , uint08 r ) ;

// ----------------------------------------------------------------
// [Function] Initialize 
void _sd1602_Initialize( ) {

  _private_sd1602_WriteByte( 0x30 , 0 ) ;
  __delay_ms( 5 ) ;
  _private_sd1602_WriteByte( 0x30 , 0 ) ;
  __delay_us( 200 ) ;
  _private_sd1602_WriteByte( 0x30 , 0 ) ;

  // Set 8-bit Mode, 2-Lines, 5x7-dots
  _private_sd1602_WriteByte( 0x38 , 0 ) ;
  // Display:OFF
  _private_sd1602_WriteByte( 0x08 , 0 ) ;
  // Display:ON, Cursor:OFF, Blink:OFF
  _private_sd1602_WriteByte( 0x0C , 0 ) ;
  //_sd1602_WriteByte( 0x0F , 0 ) ;
  // Entry Mode:Increment, Display Shift:ON
  _private_sd1602_WriteByte( 0x06 , 0 ) ;

  // Clear Display
  _private_sd1602_WriteByte( 0x01 , 0 ) ;
  __delay_ms( 2 ) ;

}

// ----------------------------------------------------------------
// [Function] Send Character 
void _sd1602_WriteCharacter( uint08 position , char character ) {
  _private_sd1602_WriteByte( position , 0 ) ;
  _private_sd1602_WriteByte( character , 1 ) ;
}

// ----------------------------------------------------------------
// [Function] Send String 
void _sd1602_WriteString( uint08 position , char* stringPtr ) {
  _private_sd1602_WriteByte( position , 0 ) ;
  while ( *stringPtr ) {
    _private_sd1602_WriteByte( *stringPtr , 1 ) ;
    stringPtr++ ;
  }
}

// ----------------------------------------------------------------
// [Function] Write String Clearing
void _sd1602_WriteStringClearing( uint08 position , char* stringPtr ) {

  uint08 col = 0 ;
  _private_sd1602_WriteByte( position & 0xF0 , 0 ) ;

  while ( col != 16 ) {
    if ( col++ >= ( position & 0x0F ) && ( *stringPtr ) ) {
      _private_sd1602_WriteByte( *stringPtr , 1 ) ;
      stringPtr++ ;
    }
    else {
      _private_sd1602_WriteByte( ' ' , 1 ) ;
    }
  }

}

// ----------------------------------------------------------------
// [Function] Set CGRAM 
void _sd1602_SetCgram( uint08 address , char bitmap[] ) {
  _private_sd1602_WriteByte( ( ( address << 3 )& 0b00111111 ) | 0b01000000 , 0 ) ;
  for ( uint08 i = 0 ; i < 8 ; i++ )
    _private_sd1602_WriteByte( bitmap[i] , 1 ) ;
}

// ----------------------------------------------------------------
// [Function] Clear 
void _sd1602_Clear( ) {
  _private_sd1602_WriteByte( 0x01 , 0 ) ;
  __delay_ms( 2 ) ;
}

// ----------------------------------------------------------------
// [Function] Clear Partial 
void _sd1602_ClearPartial( uint08 position , uint08 length ) {
  _private_sd1602_WriteByte( position , 0 ) ;
  for ( uint08 i = 0 ; i < length ; i++ )
    _private_sd1602_WriteByte( ' ' , 1 ) ;
}

// ----------------------------------------------------------------
// [Function] Fill Partial
void _sd1602_FillPartial( uint08 position , char character , uint08 length ) {
  _private_sd1602_WriteByte( position , 0 ) ;
  for ( uint08 i = 0 ; i < length ; i++ )
    _private_sd1602_WriteByte( character , 1 ) ;
}

// ----------------------------------------------------------------
// [Function] Clear Row
void _sd1602_ClearRow( uint08 rowSelect ) {

  _private_sd1602_WriteByte( rowSelect & 0xF0 , 0 ) ;

  for ( uint08 i = 0 ; i != 16 ; i++ )
    _private_sd1602_WriteByte( ' ' , 1 ) ;
}

// ----------------------------------------------------------------
// [Function] Write Byte 
void _private_sd1602_WriteByte( uint08 data , uint08 r ) {

  SD1602_SetData( data ) ;
  SD1602_SelectResister( r ) ;
  SD1602_EnableWrite( )

  __delay_us( 40 ) ;
}

#endif	/* SD1602_H */

