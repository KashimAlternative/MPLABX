#include "xc.h"
#include <pic12f1822.h>
#include "pic12f1822_init.h"

// ################################################################
// Ignore Warning for "possible use of "=" instead of "=="
#pragma warning disable 758
// ################################################################

// Definition for AQM0802
#define _XTAL_FREQ 1000000L
#define EnableStartBit() SSP1CON2bits.SEN=1;while(SSP1CON2bits.SEN);
#define EnableStopBit() SSP1CON2bits.PEN=1;while(SSP1CON2bits.PEN);
#define SendByte(data) SSP1BUF=data;
#define WaitFlag() while(!PIR1bits.SSP1IF);PIR1bits.SSP1IF=0;
#define ClearFlag() PIR1bits.SSP1IF=0;
#define IsError() SSP1CON2bits.ACKSTAT
#define WaitTimer() while(!INTCONbits.T0IF);
#define ResetTimer() TMR0=0x00;INTCONbits.T0IF=0;

#include "../_Common/typedef.h"
#include "../_Common/aqm0802.h"

#define SOFTWARE_VERSION " 00.112 "
// --------------------------------
// Keys
typedef union {
  uint08_t byte ;
  struct {
    uint08_t down : 1 ;
    uint08_t _0 : 2 ;
    uint08_t menu : 1 ;
    uint08_t _1 : 1 ;
    uint08_t up : 1 ;
  } ;
} UniPortA ;
UniPortA portAState_ ;
#define ReadKeyState() (~PORTA);

#define PRESS_COUNT 0x08
#define PRESS_COUNT_LIMIT 0x80

#define MEASURE_MASK 0xC0

#define CHAR_CURSOR 0x07
#define CHAR_HOLD 'H'

uint08_t measureCounter = 0 ;
struct {
  uint08_t up ;
  uint08_t down ;
  uint08_t upDown ;
  uint08_t menu ;
} keyCount = { 0 , 0 , 0 , 0 } ;

// --------------------------------
// Measure Mode
typedef enum {
  MEASURE_MODE_VOLTAGE ,
  MEASURE_MODE_AD_VALUE ,
  MEASURE_MODE_BAR ,
} MeasureMode ;
MeasureMode measureMode = MEASURE_MODE_VOLTAGE ;

// --------------------------------
// Mode
typedef enum {
  STATE_BOOT ,
  STATE_MEASURE ,
  STATE_MENU ,
  STATE_VERSION ,
} MachineState ;
MachineState machineState = STATE_BOOT ;

// --------------------------------
// Message
const struct {
  const char* BOOT ;
  const char* VOLTAGE ;
  const char* BAR ;
  const char* AD_VALUE ;
  const char* VERSION ;
}
MESSAGE = {
  "Boot..." ,
  "Voltage" ,
  "Bar" ,
  "A/D Val" ,
  "Version" ,
} ;

// --------------------------------
// Menu
#define MENU_SIZE ( sizeof( MESSAGE_MENU ) / sizeof( MESSAGE_MENU[0] ) )
enum Menu {
  MENU_RETURN = 0 ,
  MENU_VOLTAGE ,
  MENU_BAR ,
  MENU_AD_VALUE ,
  MENU_VERSION ,
} ;
const char* MESSAGE_MENU[] = {
  "<Return" ,
  "Voltage" ,
  "Bar" ,
  "A/D Val" ,
  "Version" ,
} ;
struct {
  uint08_t select ;
  uint08_t cursor ;
} menuState ;

// --------------------------------
uint16_t adValue = 0x0000 ;

// --------------------------------
// Event
#define SetEvent(event)    event=1
#define ClearEvent(event)    event=0
#define EvalEvent( event ) (event&&!(event=0))
union {
  uint08_t all ;
  struct {
    uint08_t keyPressMenu : 1 ;
    uint08_t keyPressUp : 1 ;
    uint08_t keyPressDown : 1 ;
    uint08_t messageChange : 1 ;
    uint08_t dummy : 4 ;
  } ;
} events_ = 0 ;

// ----------------------------------------------------------------
// [Function] Main
int main( void ) {

  initialize( ) ;

  ResetTimer( ) ;
  WaitTimer( ) ;

  OPTION_REGbits.PS = 0b000 ;
  //  T2CONbits.T2OUTPS = 0b0000 ; // Postscaler 1/1
  //  PR2 = 7 ;

  ResetTimer( ) ;

  _aqm0802_Initialize( ) ;
  _aqm0802_SendStringClearing( ROW_SELECT_0 | 0x0 , MESSAGE.BOOT ) ;

  uint08_t barImage = 0b11111 ;

  // Set CGRAM for Bar Indicator
  _private_aqm0802_SendData( 0x00 , 0x00 ) ;

  for( uint08_t i = 5 ; i != 0xFF ; i-- ) {
    _private_aqm0802_SendData( 0x00 , ( i << 3 ) | 0x40 ) ;

    for( uint08_t j = 0 ; j != 8 ; j++ ) {
      if( j == 0 || j == 7 )
        _private_aqm0802_SendData( 0x40 , 0b11111 ) ;
      else
        _private_aqm0802_SendData( 0x40 , barImage ) ;
    }

    barImage >>= 1 ;
  }

  machineState = STATE_MEASURE ;
  SetEvent( events_.messageChange ) ;

  // Start Timer Interrupt
  INTCONbits.GIE = 1 ;
  INTCONbits.PEIE = 1 ;
  PIR1bits.TMR2IF = 0 ;
  PIE1bits.TMR2IE = 1 ;

  for( ; ; ) {

    static UniPortA prevPortAState = { 0x00 } ;
    UniPortA keyPressed ;

    keyPressed.byte = ( portAState_.byte ^ prevPortAState.byte ) & portAState_.byte ;
    prevPortAState.byte = portAState_.byte ;

    // Mode Key
    if( keyPressed.menu )
      SetEvent( events_.keyPressMenu ) ;
    //        if( portAState_.menu ) {
    //      if( keyCount.menu != PRESS_COUNT_LIMIT ) keyCount.menu++ ;
    //      if( keyCount.menu == PRESS_COUNT ) SetEvent( events_.keyPressMenu ) ;
    //    }
    //    else
    //      keyCount.menu = 0 ;

    // Up Key
    if( keyPressed.up )
      SetEvent( events_.keyPressUp ) ;
    //    if( portAState_.up && ( !keyCount.down ) && ( !keyCount.upDown ) ) {
    //      if( keyCount.up != PRESS_COUNT_LIMIT ) keyCount.up++ ;
    //      if( keyCount.up == PRESS_COUNT ) SetEvent( events_.keyPressUp ) ;
    //    }
    //    else
    //      keyCount.up = 0 ;

    // Down Key
    if( keyPressed.down )
      SetEvent( events_.keyPressDown ) ;
    //    if( portAState_.down && ( !keyCount.up ) && ( !keyCount.upDown ) ) {
    //      if( keyCount.down != PRESS_COUNT_LIMIT ) keyCount.down++ ;
    //      if( keyCount.down == PRESS_COUNT ) SetEvent( events_.keyPressDown ) ;
    //    }
    //    else
    //      keyCount.down = 0 ;

    //    if( portAState_.up && portAState_.down ) {
    //      if( ++keyCount.upDown == 0xFF ) RESET( ) ;
    //    }
    //    else if( !( portAState_.up && portAState_.down ) )
    //      keyCount.upDown = 0 ;

    // Menu Key
    if( EvalEvent( events_.keyPressMenu ) ) {

      SetEvent( events_.messageChange ) ;

      switch( machineState ) {

        case STATE_MEASURE:
          machineState = STATE_MENU ;
          menuState.select = 0 ;
          menuState.cursor = 0 ;
          break ;

        case STATE_MENU:
          machineState = STATE_MEASURE ;

          switch( menuState.select ) {
            case MENU_VOLTAGE:
              measureMode = MEASURE_MODE_VOLTAGE ;
              break ;

            case MENU_BAR:
              measureMode = MEASURE_MODE_BAR ;
              break ;

            case MENU_AD_VALUE:
              measureMode = MEASURE_MODE_AD_VALUE ;
              break ;

            case MENU_VERSION:
              machineState = STATE_VERSION ;
              break ;
          }
          break ;

        case STATE_VERSION:
          machineState = STATE_MEASURE ;
          break ;
      }
    }

    // Up/Down Key
    switch( machineState ) {

      case STATE_MENU:
        if( EvalEvent( events_.keyPressUp ) ) {
          if( menuState.select != 0 ) {
            menuState.select-- ;
            if( menuState.cursor != 0 ) menuState.cursor-- ;
            SetEvent( events_.messageChange ) ;
          }
        }
        if( EvalEvent( events_.keyPressDown ) ) {
          if( menuState.select != ( MENU_SIZE - 1 ) ) {
            menuState.select++ ;
            if( menuState.cursor != 1 ) menuState.cursor++ ;
            SetEvent( events_.messageChange ) ;
          }
        }

        break ;
    }

    // Message Change
    if( EvalEvent( events_.messageChange ) ) {
      switch( machineState ) {

        case STATE_MEASURE:
          switch( measureMode ) {
            case MEASURE_MODE_VOLTAGE:
              _aqm0802_SendStringClearing( ROW_SELECT_0 | 0x0 , MESSAGE.VOLTAGE ) ;
              break ;

            case MEASURE_MODE_BAR:
              _aqm0802_SendStringClearing( ROW_SELECT_0 | 0x0 , MESSAGE.BAR ) ;
              break ;

            case MEASURE_MODE_AD_VALUE:
              _aqm0802_SendStringClearing( ROW_SELECT_0 | 0x0 , MESSAGE.AD_VALUE ) ;
              break ;

          }

          _aqm0802_ClearRow( ROW_SELECT_1 | 0x0 ) ;

          break ;

        case STATE_MENU:
          _aqm0802_SendStringClearing( ROW_SELECT_0 | 0x1 , MESSAGE_MENU[ menuState.select - menuState.cursor ] ) ;
          _aqm0802_SendStringClearing( ROW_SELECT_1 | 0x1 , MESSAGE_MENU[ menuState.select - menuState.cursor + 1 ] ) ;
          _aqm0802_SendCharacter( ROW_SELECT[ menuState.cursor ] | 0x0 , CHAR_CURSOR ) ;
          break ;

        case STATE_VERSION:
          _aqm0802_SendStringClearing( ROW_SELECT_0 | 0x0 , MESSAGE.VERSION ) ;
          _aqm0802_SendStringClearing( ROW_SELECT_1 | 0x0 , SOFTWARE_VERSION ) ;
          break ;

      }
    }

  } ;

}

// ----------------------------------------------------------------
// [Interrupt]
void interrupt _( void ) {
  if( !PIR1bits.TMR2IF ) return ;
  PIR1bits.TMR2IF = 0 ;

  portAState_.byte = ReadKeyState( ) ;

  if( machineState != STATE_MEASURE ) return ;

  // Get A/D Value
  ADCON0bits.GO = 1 ;
  while( ADCON0bits.GO ) ;

  adValue += ( ( ( (uint16_t)ADRESH ) << 8 ) | ADRESL ) ;

  if( ++measureCounter & 0x3F ) return ;

  // Display Averaged Value

  uint08_t digit ;
  char string[9] = "  00000 " ;
  uint16_t compareUnit = 10000 ;

  switch( measureMode ) {
    case MEASURE_MODE_VOLTAGE:
    case MEASURE_MODE_AD_VALUE:

      digit = 2 ;
      while( compareUnit ) {

        while( adValue >= compareUnit ) {
          string[digit]++ ;
          adValue -= compareUnit ;
        }

        digit++ ;
        compareUnit /= 10 ;

      }

      if( measureMode == MEASURE_MODE_VOLTAGE ) {
        string[1] = string[2] ;
        string[2] = '.' ;
        string[7] = 'V' ;
      }

      _aqm0802_SendStringClearing( ROW_SELECT_1 | 0x0 , &string ) ;

      break ;

    case MEASURE_MODE_BAR:
    {
      uint08_t tmpValue = adValue >> 11 ;
      for( uint08_t i = 7 ; i != 1 ; i-- ) {
        if( tmpValue >= 5 ) {
          _aqm0802_SendCharacter( ROW_SELECT_1 | i , 0x05 ) ;
          tmpValue -= 5 ;
        }
        else {
          _aqm0802_SendCharacter( ROW_SELECT_1 | i , tmpValue ) ;
          tmpValue = 0 ;
        }
      }
    }
      break ;
  }

  adValue = 0x0000 ;

}


