// ----------------------------------------------------------------
// Parallel LCD (with 8-bit mode)
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

#ifndef PARALLEL_LCD_H
#  define PARALLEL_LCD_H

#  include "Typedef.h"

// ----------------------------------------------------------------
// Configuration Enum

// Config Function
typedef enum {
  PARALLEL_LCD_CONFIG_FUNCTION_NONE = 0x00 ,
  PARALLEL_LCD_CONFIG_5x11DOT_MODE = 0x04 , // <-> 5x8DOT_MODE
  PARALLEL_LCD_CONFIG_2LINE_MODE = 0x08 , // <-> 1LINE_MODE
  PARALLEL_LCD_CONFIG_8BIT_MODE = 0x10 , // <-> 4BIT_MODE
} ParallelLCD_EnConfigFunction ;

// Config Display
typedef enum {
  PARALLEL_LCD_CONFIG_DISPLAY_NONE = 0x00 ,
  PARALLEL_LCD_CONFIG_BLINK_ON = 0x01 ,
  PARALLEL_LCD_CONFIG_CURSOR_ON = 0x02 ,
  PARALLEL_LCD_CONFIG_DISPLAY_ON = 0x04 ,
} ParallelLCD_EnConfigDisplay ;

// Config Cursor
typedef enum {
  PARALLEL_LCD_CONFIG_CURSOR_NONE = 0x00 ,
  PARALLEL_LCD_CONFIG_CURSOR_MOVEMENT_LEFT = 0x04 , // <-> CURSOR_MOVEMENT_RIGHT
  PARALLEL_LCD_CONFIG_DISPLAY_SHIFT = 0x08 , // <-> CURSOR_MOVEMENT
} ParallelLCD_EnConfigCursor ;

// Config Entry Mode
typedef enum {
  PARALLEL_LCD_CONFIG_ENTRY_MODE_NONE = 0x00 ,
  PARALLEL_LCD_CONFIG_SHIFT_ON = 0x01 , // <-> SHIFT_OFF
  PARALLEL_LCD_CONFIG_INCREMENTAL = 0x02 , // <-> DECREMENTAL
} ParallelLCD_EnConfigEntryMode ;

// ----------------------------------------------------------------
// [Macro Define]
typedef enum {
  PARALLEL_LCD_ROW_SELECT_0 = 0x80 ,
  PARALLEL_LCD_ROW_SELECT_1 = 0xC0 ,
} ParallelLCD_EnRowSelect ;
const ParallelLCD_EnRowSelect PARALLEL_LCD_ROW_SELECT[] = {
  PARALLEL_LCD_ROW_SELECT_0 ,
  PARALLEL_LCD_ROW_SELECT_1 ,
} ;
typedef ParallelLCD_EnRowSelect Position_t ;

// ----------------------------------------------------------------
// [Constants]
PRIVATE const Char_t HEX_TABLE[] = { '0' , '1' , '2' , '3' , '4' , '5' , '6' , '7' , '8' , '9' , 'A' , 'B' , 'C' , 'D' , 'E' , 'F' , } ;

// ----------------------------------------------------------------
// [Prototypes]
PRIVATE void ParallelLCD_WriteByte( Uint08_t data , Bool_t r ) ;

// ----------------------------------------------------------------
// [Function] Initialize
void ParallelLCD_Initialize(
        ParallelLCD_EnConfigFunction configFunction ,
        ParallelLCD_EnConfigDisplay configDisplay ,
        ParallelLCD_EnConfigCursor configCursor ,
        ParallelLCD_EnConfigEntryMode configEntryMode
        ) {
  ParallelLCD_WriteByte( ( configFunction & 0x1C ) | 0x20 , BOOL_FALSE ) ;
  ParallelLCD_WriteByte( ( configDisplay & 0x0C ) | 0x08 , BOOL_FALSE ) ;
  ParallelLCD_WriteByte( ( configCursor & 0x07 ) | 0x10 , BOOL_FALSE ) ;
  ParallelLCD_WriteByte( ( configEntryMode & 0x03 ) | 0x04 , BOOL_FALSE ) ;
}

// ----------------------------------------------------------------
// [Function] Write Character
void ParallelLCD_WriteCharacter( Position_t position , Char_t character ) {
  ParallelLCD_WriteByte( position , BOOL_FALSE ) ;
  ParallelLCD_WriteByte( character , BOOL_TRUE ) ;
}

// ----------------------------------------------------------------
// [Function] Write String
void ParallelLCD_WriteString( Position_t position , const Char_t* stringPtr ) {
  ParallelLCD_WriteByte( position , BOOL_FALSE ) ;
  while ( *stringPtr ) {
    ParallelLCD_WriteByte( *stringPtr++ , BOOL_TRUE ) ;
  }
}

// ----------------------------------------------------------------
// [Function] Write String Clearing
void ParallelLCD_WriteStringClearing( Position_t position , const Char_t* stringPtr ) {
  ParallelLCD_WriteByte( position & 0xF0 , BOOL_FALSE ) ;
  for ( Uint08_t i = 0 ; i != PARALLEL_LCD_WIDTH ; i++ ) {
    if ( i >= ( position & 0x0F ) && ( *stringPtr ) )
      ParallelLCD_WriteByte( *stringPtr++ , BOOL_TRUE ) ;
    else
      ParallelLCD_WriteByte( ' ' , BOOL_TRUE ) ;

  }
}

// ----------------------------------------------------------------
// [Function] Write Hex Number
void ParallelLCD_WriteHexNumber( Position_t position , Uint08_t number ) {
  ParallelLCD_WriteByte( position , BOOL_FALSE ) ;
  ParallelLCD_WriteByte( HEX_TABLE[ number >> 4 ] , BOOL_TRUE ) ;
  ParallelLCD_WriteByte( HEX_TABLE[ number & 0x0F ] , BOOL_TRUE ) ;
}

// ----------------------------------------------------------------
// [Function] Clear Row
void ParallelLCD_ClearRow( ParallelLCD_EnRowSelect rowSelect ) {
  ParallelLCD_WriteByte( rowSelect & 0xF0 , BOOL_FALSE ) ;
  for ( Uint08_t i = 0 ; i != PARALLEL_LCD_WIDTH ; i++ )
    ParallelLCD_WriteByte( ' ' , BOOL_TRUE ) ;
}

// ----------------------------------------------------------------
// [Function] Clear Partial
void ParallelLCD_ClearPartial( Position_t position , Uint08_t length ) {
  ParallelLCD_WriteByte( position , BOOL_FALSE ) ;
  for ( Uint08_t i = 0 ; i != length ; i++ )
    ParallelLCD_WriteByte( ' ' , BOOL_TRUE ) ;
}

// ----------------------------------------------------------------
// [Function] Set CGRAM
void ParallelLCD_SetCgram( Char_t charCode , const Char_t* bitmap ) {
  ParallelLCD_WriteByte( ( ( charCode << 3 ) & 0b00111111 ) | 0b01000000 , BOOL_FALSE ) ;
  for ( Uint08_t i = 0 ; i != 8 ; i++ , bitmap++ )
    ParallelLCD_WriteByte( *bitmap , BOOL_TRUE ) ;
}

// ----------------------------------------------------------------
// [Function] Clear Display
void ParallelLCD_ClearDisplay( ) {
  ParallelLCD_ClearRow( PARALLEL_LCD_ROW_SELECT_0 ) ;
  ParallelLCD_ClearRow( PARALLEL_LCD_ROW_SELECT_1 ) ;
  //  ParallelLCD_WriteByte( 0x01 , BOOL_FALSE ) ;
  //  __delay_ms( 2 ) ;
}

// ----------------------------------------------------------------
// Private
// ----------------------------------------------------------------

// ----------------------------------------------------------------
// [Function] Write Byte
PRIVATE void ParallelLCD_WriteByte( Uint08_t data , Bool_t r ) {
  PARALLEL_LCD_WaitTimer( ) ;
  PARALLEL_LCD_SetData( data ) ;
  PARALLEL_LCD_SelectResister( r ) ;
  PARALLEL_LCD_TriggerWrite( ) ;
  PARALLEL_LCD_ResetTimer( ) ;
}

#endif	/* PARALLEL_LCD_H */

