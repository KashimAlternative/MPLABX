// ----------------------------------------------------------------
// Parallel LCD (with 8-bit mode)
// Revision 2015/08/09
// ----------------------------------------------------------------
// Description
//
// Following macros must be defined
//   #define PARALLEL_LCD_WIDTH
//   #define PARALLEL_LCD_ROW_COUNT
//   #define PARALLEL_LCD_EnableWrite( )
//   #define PARALLEL_LCD_SelectResister( r )
//   #define PARALLEL_LCD_SetData( data )
//   #define PARALLEL_LCD_WaitTimer()
//   #define PARALLEL_LCD_ResetTimer()
//
// Following macros needed to user Clear function
//   #define PARALLEL_LCD_USE_CLEAR
//   #define _XTAL_FREQ

#ifndef PARALLEL_LCD_H
#  define PARALLEL_LCD_H

#  include "parallel_LCD_configuration.h"
#  include "typedef.h"

// ----------------------------------------------------------------
// [Macro Define]

// Row Select
#  define PARALLEL_LCD_ROW_SELECT_0 0x80
#  define PARALLEL_LCD_ROW_SELECT_1 0xC0

const uint08 PARALLEL_LCD_ROW_SELECT[] = {
  PARALLEL_LCD_ROW_SELECT_0 ,
  PARALLEL_LCD_ROW_SELECT_1 ,
} ;

// ----------------------------------------------------------------
// [Constants]
const char HEX_TABLE[] = { '0' , '1' , '2' , '3' , '4' , '5' , '6' , '7' , '8' , '9' , 'A' , 'B' , 'C' , 'D' , 'E' , 'F' , } ;

// ----------------------------------------------------------------
// [Prototypes]
void _private_parallel_lcd_WriteByte( uint08 data , uint08 r ) ;

// ----------------------------------------------------------------
// [Function] Initialize
void _parallel_lcd_Initialize( ) {
  _private_parallel_lcd_WriteByte( CONFIG_FUNCTION_SET | CONFIG_8BIT_MODE | CONFIG_2LINE_MODE | CONFIG_5x8DOT_MODE , 0 ) ;
  _private_parallel_lcd_WriteByte( CONFIG_DISPLAY | CONFIG_DISPLAY_ON , 0 ) ;
  _private_parallel_lcd_WriteByte( CONFIG_DISPLAY_OR_CURSOR | CONFIG_CURSOR_MOVEMENT | CONFIG_CURSOR_MOVEMENT_RIGHT , 0 ) ;
  _private_parallel_lcd_WriteByte( CONFIG_ENTRY_MODE | CONFIG_INCREMENTAL | CONFIG_SHIFT_OFF , 0 ) ;
}

// ----------------------------------------------------------------
// [Function] Write Character
void _parallel_lcd_WriteCharacter( uint08 position , char character ) {
  _private_parallel_lcd_WriteByte( position , 0 ) ;
  _private_parallel_lcd_WriteByte( character , 1 ) ;
}

// ----------------------------------------------------------------
// [Function] Write String
void _parallel_lcd_WriteString( uint08 position , const char* stringPtr ) {
  _private_parallel_lcd_WriteByte( position , 0 ) ;
  while ( *stringPtr ) {
    _private_parallel_lcd_WriteByte( *stringPtr++ , 1 ) ;
  }
}

// ----------------------------------------------------------------
// [Function] Write String Clearing
void _parallel_lcd_WriteStringClearing( uint08 position , const char* stringPtr ) {
  _private_parallel_lcd_WriteByte( position & 0xF0 , 0 ) ;
  for ( uint08 i = 0 ; i != PARALLEL_LCD_WIDTH ; i++ ) {
    if ( i >= ( position & 0x0F ) && ( *stringPtr ) )
      _private_parallel_lcd_WriteByte( *stringPtr++ , 1 ) ;
    else
      _private_parallel_lcd_WriteByte( ' ' , 1 ) ;

  }
}

// ----------------------------------------------------------------
// [Function] Write Hex Number
void _parallel_lcd_WriteHexNumber( uint08 position , uint08 number ) {
  _private_parallel_lcd_WriteByte( position , 0 ) ;
  _private_parallel_lcd_WriteByte( HEX_TABLE[ number >> 4 ] , 1 ) ;
  _private_parallel_lcd_WriteByte( HEX_TABLE[ number & 0x0F ] , 1 ) ;
}

// ----------------------------------------------------------------
// [Function] Clear Row
void _parallel_lcd_ClearRow( uint08 rowSelect ) {
  _private_parallel_lcd_WriteByte( rowSelect & 0xF0 , 0 ) ;
  for ( uint08 i = 0 ; i != PARALLEL_LCD_WIDTH ; i++ )
    _private_parallel_lcd_WriteByte( ' ' , 1 ) ;
}

// ----------------------------------------------------------------
// [Function] Clear Partial
void _parallel_lcd_ClearPartial( uint08 position , uint08 length ) {
  _private_parallel_lcd_WriteByte( position , 0 ) ;
  for ( uint08 i = 0 ; i != length ; i++ )
    _private_parallel_lcd_WriteByte( ' ' , 1 ) ;
}

// ----------------------------------------------------------------
// [Function] Set CGRAM
void _parallel_lcd_SetCgram( char charCode , const char* bitmap ) {
  _private_parallel_lcd_WriteByte( ( ( charCode << 3 ) & 0b00111111 ) | 0b01000000 , 0 ) ;
  for ( uint08 i = 0 ; i != 8 ; i++ , bitmap++ )
    _private_parallel_lcd_WriteByte( *bitmap , 1 ) ;
}

#  ifdef PARALLEL_LCD_USE_CLEAR
// ----------------------------------------------------------------
// [Function] Clear
void _parallel_lcd_Clear( ) {
  _private_parallel_lcd_WriteByte( 0x01 , 0 ) ;
  __delay_ms( 2 ) ;
}
#  endif

// ----------------------------------------------------------------
// Private
// ----------------------------------------------------------------

// ----------------------------------------------------------------
// [Function] Write Byte
void _private_parallel_lcd_WriteByte( uint08 data , uint08 r ) {
  PARALLEL_LCD_WaitTimer( ) ;
  PARALLEL_LCD_SetData( data ) ;
  PARALLEL_LCD_SelectResister( r ) ;
  PARALLEL_LCD_TriggerWrite( ) ;
  PARALLEL_LCD_ResetTimer( ) ;
}

#endif	/* PARALLEL_LCD_H */

