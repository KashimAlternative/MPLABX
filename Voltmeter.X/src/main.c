#include <xc.h>
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
#define WaitTimer() while(!PIR1bits.TMR2IF);
#define ResetTimer() TMR2=0x00;PIR1bits.TMR2IF=0;

#include "../../_Common/typedef.h"
#include "../../_Common/aqm0802.h"

#define SOFTWARE_VERSION " 00.112 "

// --------------------------------
// Keys
typedef union {
  uint08_t byte ;
  struct {
    unsigned down : 1 ;
    unsigned : 2 ;
    unsigned menu : 1 ;
    unsigned : 1 ;
    unsigned up : 1 ;
  } ;
} UniPortA ;
UniPortA portAState_ ;
#define ReadKeyState() (~PORTA& 0b00101001);

#define PRESS_COUNT 0x08
#define PRESS_COUNT_LIMIT 0x80

#define MEASURE_MASK 0xC0

#define CHAR_CURSOR 0x07
#define CHAR_HOLD 'H'

// --------------------------------
// State
typedef enum {
  STATE_BOOT ,
  STATE_MEASURE ,
  STATE_MENU ,
  STATE_VERSION ,
  STATE_ERROR ,
} MachineState ;
MachineState machineState_ = STATE_BOOT ;
Bool_t isHold_ = BOOL_FALSE ;

// --------------------------------
// Measure Mode
typedef enum {
  MEASURE_MODE_VOLTAGE ,
  MEASURE_MODE_AD_VALUE ,
  MEASURE_MODE_BAR ,
} MeasureMode ;
MeasureMode measureMode_ = MEASURE_MODE_VOLTAGE ;

// --------------------------------
// Message
const struct {
  const char* BOOT ;
  const char* VOLTAGE ;
  const char* BAR ;
  const char* AD_VALUE ;
  const char* VERSION ;
  const char* ERROR ;
}
MESSAGE = {
  "Boot..." ,
  "Voltage" ,
  "Bar" ,
  "A/D Val" ,
  "Version" ,
  "Error" ,
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
} menuState_ ;

// --------------------------------
// A/D Converter
#define SIZE_OF_AD_BUFFER 16
typedef uint16_t ADValue_t ;
uint16_t sumOfBuffer_ ;

// --------------------------------
// Event
#define SetEvent(event) event=1
#define ClearEvent(event) event=0
#define EvalEvent(event) (event&&!(event=0))
union {
  uint08_t all ;
  struct {
    unsigned keyPressMenu : 1 ;
    unsigned keyPressUp : 1 ;
    unsigned keyPressDown : 1 ;
    unsigned keyPressUpDown : 1 ;
    unsigned changeValue : 1 ;
    unsigned changeMessage : 1 ;
    unsigned error : 1 ;
  } ;
} events_ = 0 ;


// ----------------------------------------------------------------
// [Function] Main
int main( void ) {

  initialize( ) ;

  T2CONbits.TMR2ON = 1 ;

  WaitTimer( ) ;

  T2CONbits.T2OUTPS = 0b0000 ; // Postscaler 1/1
  PR2 = 7 ;

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

  machineState_ = STATE_MEASURE ;
  SetEvent( events_.changeMessage ) ;

  // Start Timer Interrupt
  TMR0IE = 1 ;

  // Start Main Loop
  for( ; ; ) {

    static UniPortA prevPortAState = { 0x00 } ;
    UniPortA keyPressed ;

    keyPressed.byte = ( portAState_.byte ^ prevPortAState.byte ) & portAState_.byte ;
    prevPortAState.byte = portAState_.byte ;

    // Set Key Event
    if( keyPressed.menu )
      SetEvent( events_.keyPressMenu ) ;

    if( keyPressed.up ) {
      if( portAState_.down )
        SetEvent( events_.keyPressUpDown ) ;
      else
        SetEvent( events_.keyPressUp ) ;
    }
    if( keyPressed.down ) {
      if( portAState_.up )
        SetEvent( events_.keyPressUpDown ) ;
      else
        SetEvent( events_.keyPressDown ) ;
    }

    if( EvalEvent( events_.error ) ) {
      machineState_ = STATE_ERROR ;
      SetEvent( events_.changeMessage ) ;
    }

    // Up Donw Key
    if( EvalEvent( events_.keyPressUpDown ) ) {
      switch( machineState_ ) {
        case STATE_MEASURE:
          if( isHold_ )
            isHold_ = BOOL_FALSE ;
          else
            isHold_ = BOOL_TRUE ;
          SetEvent( events_.changeMessage ) ;
          break ;
      }
    }

    // Menu Key
    if( EvalEvent( events_.keyPressMenu ) ) {

      SetEvent( events_.changeMessage ) ;

      switch( machineState_ ) {

        case STATE_MEASURE:
          machineState_ = STATE_MENU ;
          break ;

        case STATE_MENU:
          machineState_ = STATE_MEASURE ;

          switch( menuState_.select ) {
            case MENU_VOLTAGE:
              measureMode_ = MEASURE_MODE_VOLTAGE ;
              break ;

            case MENU_BAR:
              measureMode_ = MEASURE_MODE_BAR ;
              break ;

            case MENU_AD_VALUE:
              measureMode_ = MEASURE_MODE_AD_VALUE ;
              break ;

            case MENU_VERSION:
              machineState_ = STATE_VERSION ;
              break ;
          }
          break ;

        case STATE_VERSION:
          machineState_ = STATE_MENU ;
          break ;
      }
    }

    // Up/Down Key
    switch( machineState_ ) {

      case STATE_MENU:
        if( EvalEvent( events_.keyPressUp ) ) {
          if( menuState_.select != 0 ) {
            menuState_.select-- ;
            if( menuState_.cursor != 0 ) menuState_.cursor-- ;
            SetEvent( events_.changeMessage ) ;
          }
        }
        if( EvalEvent( events_.keyPressDown ) ) {
          if( menuState_.select != ( MENU_SIZE - 1 ) ) {
            menuState_.select++ ;
            if( menuState_.cursor != 1 ) menuState_.cursor++ ;
            SetEvent( events_.changeMessage ) ;
          }
        }

        break ;
    }

    // Change Message
    if( EvalEvent( events_.changeMessage ) ) {
      switch( machineState_ ) {

        case STATE_MEASURE:
          switch( measureMode_ ) {

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
          if( isHold_ )
            _aqm0802_SendCharacter( ROW_SELECT_1 | 0x0 , 'H' ) ;

          SetEvent( events_.changeValue ) ;
          break ;

        case STATE_MENU:
          _aqm0802_SendStringClearing( ROW_SELECT_0 | 0x1 , MESSAGE_MENU[ menuState_.select - menuState_.cursor ] ) ;
          _aqm0802_SendStringClearing( ROW_SELECT_1 | 0x1 , MESSAGE_MENU[ menuState_.select - menuState_.cursor + 1 ] ) ;
          _aqm0802_SendCharacter( ROW_SELECT[ menuState_.cursor ] | 0x0 , CHAR_CURSOR ) ;
          break ;

        case STATE_VERSION:
          _aqm0802_SendStringClearing( ROW_SELECT_0 | 0x0 , MESSAGE.VERSION ) ;
          _aqm0802_SendStringClearing( ROW_SELECT_1 | 0x0 , SOFTWARE_VERSION ) ;
          break ;


        case STATE_ERROR:
          _aqm0802_SendStringClearing( ROW_SELECT_0 | 0x0 , MESSAGE.ERROR ) ;
          _aqm0802_ClearRow( ROW_SELECT_1 ) ;
          break ;
      }
    }

    // Change Value
    if( machineState_ == STATE_MEASURE && EvalEvent( events_.changeValue ) ) {

      // Display Averaged Value
      ADValue_t adValue ;
      uint08_t digit ;
      char string[8] = "  00000" ;
      const uint16_t compareUnits[] = { 10000 , 1000 , 100 , 10 , 1 } ;

      adValue = sumOfBuffer_ ;

      switch( measureMode_ ) {
        case MEASURE_MODE_VOLTAGE:
        case MEASURE_MODE_AD_VALUE:

          digit = 0 ;
          while( digit < 5 ) {

            uint16_t compareUnit = compareUnits[ digit ] ;

            while( adValue >= compareUnit ) {
              string[digit + 2]++ ;
              adValue -= compareUnit ;
            }

            digit++ ;

          }

          if( measureMode_ == MEASURE_MODE_VOLTAGE ) {
            string[1] = string[2] ;
            string[2] = string[3] ;
            string[3] = '.' ;
            string[6] = 'V' ;
          }

          _aqm0802_SendString( ROW_SELECT_1 | 0x1 , &string ) ;

          break ;

        case MEASURE_MODE_BAR:
          for( uint08_t i = 7 ; i != 1 ; i-- ) {
            if( adValue >= 1000 ) {
              _aqm0802_SendCharacter( ROW_SELECT_1 | i , 0x05 ) ;
              adValue -= 1000 ;
            }
            else {
              _aqm0802_SendCharacter( ROW_SELECT_1 | i , adValue / 200 ) ;
              adValue = 0 ;
            }
          }
          break ;
      }

    }

  }//for(;;)

}

#define INTERRUPT_FLUG INTCONbits.TMR0IF
// ----------------------------------------------------------------
// [Interrupt]
void interrupt _( void ) {
  if( !INTERRUPT_FLUG ) return ;
  INTERRUPT_FLUG = 0 ;

  // Read Key State
  static uint08_t interruptCount = 0 ;
  if( ! --interruptCount ) {
    interruptCount = 10 ;
    portAState_.byte = ReadKeyState( ) ;
  }

  // Get A/D Value
  if( !ADCON0bits.GO ) {

    static uint08_t bufferPostiion = 0 ;
    static ADValue_t adBuffer[ SIZE_OF_AD_BUFFER ] ;
    ADValue_t adValue = ( ( ( (uint16_t)ADRESH ) << 8 ) | ADRESL ) ;
    ADCON0bits.GO = 1 ;

    if( !isHold_ ) {
      sumOfBuffer_ -= adBuffer[ bufferPostiion ] ;
      sumOfBuffer_ += adValue ;
      adBuffer[bufferPostiion] = adValue ;

      if( ++bufferPostiion == SIZE_OF_AD_BUFFER ) bufferPostiion = 0 ;

    }

    SetEvent( events_.changeValue ) ;

  }

  if( INTERRUPT_FLUG ) SetEvent( events_.error ) ;

}


