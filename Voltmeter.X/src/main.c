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

#include "../../_Common/Typedef.h"
#include "../../_Common/Util.h"
#include "../../_Common/AQM0802.h"

#define SOFTWARE_VERSION " 00.112 "

// --------------------------------
// Keys
typedef union {
  Uint08_t byte ;
  struct {
    unsigned down : 1 ;
    unsigned : 2 ;
    unsigned menu : 1 ;
    unsigned : 1 ;
    unsigned up : 1 ;
  } ;
} UniPortA ;
UniPortA portAState_ ;
#define ReadKeyState() (~PORTA&0b00101001);

#define CHAR_CURSOR 0x07

// --------------------------------
// State
typedef enum {
  STATE_BOOT ,
  STATE_MEASURE ,
  STATE_MENU ,
  STATE_VERSION ,
  STATE_SLEEP ,
  STATE_ERROR ,
} MachineState ;
MachineState machineState_ = STATE_BOOT ;
Bool_t isHold_ = BOOL_FALSE ;
Uint16_t sleepTimer_ = 0 ;

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
  " 2 4 6 8" ,
  "A/D Val" ,
  "Version" ,
  "Error" ,
} ;
const char* currentMessage_ = NULL ;

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
  Uint08_t select ;
  Uint08_t cursor ;
} menuState_ ;

// --------------------------------
// A/D Converter
#define SIZE_OF_AD_BUFFER 16
typedef Uint16_t ADValue_t ;
static ADValue_t sumOfBuffer_ = 0 ;

// --------------------------------
// Event
union {
  Uint08_t all ;
  struct {
    unsigned keyPressMenu : 1 ;
    unsigned keyPressUp : 1 ;
    unsigned keyPressDown : 1 ;
    unsigned keyPressUpDown : 1 ;
    unsigned changeValue : 1 ;
    unsigned changeMessage : 1 ;
    unsigned sleep : 1 ;
    unsigned error : 1 ;
  } ;
} events_ = { 0x00 } ;

// ----------------------------------------------------------------
// [Function] Main
int main( void ) {

  initialize( ) ;

  T2CONbits.TMR2ON = 1 ;

  WaitTimer( ) ;

  T2CONbits.T2OUTPS = 0b0000 ; // Postscaler 1/1
  PR2 = 7 ;

  ResetTimer( ) ;

  AQM0802_Initialize( ) ;
  AQM0802_SendStringClearing( ROW_SELECT_0 | 0x0 , MESSAGE.BOOT ) ;

  // Set CGRAM for Bar Indicator
  AQM0802_SendData( 0x00 , 0x00 ) ;
  AQM0802_SendData( 0x00 , 0x40 ) ;

  Uint08_t barImage = 0x40 ;
  for( Uint08_t i = 0 ; i < 48 ; i++ ) {
    if( !( i & 0x07 ) )
      barImage |= ( barImage >> 1 ) ;
    AQM0802_SendData( 0x40 , barImage ) ;
  }

  machineState_ = STATE_MEASURE ;
  currentMessage_ = MESSAGE.VOLTAGE ;
  SetEvent( events_.changeMessage ) ;

  // Start Timer Interrupt
  TMR0IF = 0 ;
  TMR0IE = 1 ;
  WDTCONbits.SWDTEN = 1 ;

  // Start Main Loop
  for( ; ; ) {

    // Clear Watchdog-Timer
    CLRWDT( ) ;

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
              currentMessage_ = MESSAGE.VOLTAGE ;
              break ;

            case MENU_BAR:
              measureMode_ = MEASURE_MODE_BAR ;
              currentMessage_ = MESSAGE.BAR ;
              break ;

            case MENU_AD_VALUE:
              measureMode_ = MEASURE_MODE_AD_VALUE ;
              currentMessage_ = MESSAGE.AD_VALUE ;
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
          AQM0802_SendStringClearing( ROW_SELECT_0 | 0x0 , currentMessage_ ) ;
          AQM0802_ClearRow( ROW_SELECT_1 ) ;
          //          if( isHold_ )
          //            AQM0802_SendCharacter( ROW_SELECT_0 | 0x7 , CHAR_HOLD ) ;

          SetEvent( events_.changeValue ) ;
          break ;

        case STATE_MENU:
          AQM0802_SendStringClearing( ROW_SELECT_0 | 0x1 , MESSAGE_MENU[ menuState_.select - menuState_.cursor ] ) ;
          AQM0802_SendStringClearing( ROW_SELECT_1 | 0x1 , MESSAGE_MENU[ menuState_.select - menuState_.cursor + 1 ] ) ;
          AQM0802_SendCharacter( ROW_SELECT[ menuState_.cursor ] | 0x0 , CHAR_CURSOR ) ;
          break ;

        case STATE_VERSION:
          AQM0802_SendStringClearing( ROW_SELECT_0 | 0x0 , MESSAGE.VERSION ) ;
          AQM0802_SendStringClearing( ROW_SELECT_1 | 0x0 , SOFTWARE_VERSION ) ;
          break ;


        case STATE_ERROR:
          AQM0802_SendStringClearing( ROW_SELECT_0 | 0x0 , MESSAGE.ERROR ) ;
          AQM0802_ClearRow( ROW_SELECT_1 ) ;
          break ;
      }
    }

    // Change Value
    if( machineState_ == STATE_MEASURE && EvalEvent( events_.changeValue ) ) {

      // Display Averaged Value
      ADValue_t currentAdValue ;
      Uint08_t digit ;
      char string[8] = "  00000" ;
      const Uint16_t COMPARE_UNITS[] = { 10000 , 1000 , 100 , 10 , 1 } ;

      currentAdValue = sumOfBuffer_ ;

      switch( measureMode_ ) {
        case MEASURE_MODE_VOLTAGE:
        case MEASURE_MODE_AD_VALUE:

          digit = 0 ;
          while( digit < 5 ) {

            Uint16_t compareUnit = COMPARE_UNITS[ digit ] ;

            while( currentAdValue >= compareUnit ) {
              string[digit + 2]++ ;
              currentAdValue -= compareUnit ;
            }

            digit++ ;

          }

          if( measureMode_ == MEASURE_MODE_VOLTAGE ) {
            string[1] = string[2] ;
            string[2] = string[3] ;
            string[3] = '.' ;
            string[6] = 'V' ;
          }

          AQM0802_SendString( ROW_SELECT_1 | 0x1 , &string ) ;

          break ;

        case MEASURE_MODE_BAR:
        {
          Char_t string[9] ;
          for( Uint08_t i = 0 ; i < 8 ; i++ ) {
            if( currentAdValue >= 1000 ) {
              string[i] = '\x05' ;
              currentAdValue -= 1000 ;
            }
            else {
              string[i] = currentAdValue / 200 ;
              string[ i + 1 ] = '\0' ;
              break ;
            }

          }
          AQM0802_SendStringClearing( ROW_SELECT_1 | 0x0 , string ) ;
        }
          break ;

      }

    }

    // Sleep
    if( EvalEvent( events_.sleep ) ) {
      AQM0802_ClearRow( ROW_SELECT_0 ) ;
      AQM0802_ClearRow( ROW_SELECT_1 ) ;
      WDTCONbits.SWDTEN = 0 ;
      INTCONbits.TMR0IE = 0 ;
      INTCONbits.IOCIF = 0 ;
      INTCONbits.IOCIE = 1 ;
      sleepTimer_ = 0 ;
      machineState_ = STATE_SLEEP ;
      SLEEP( ) ;
    }

  }//for(;;)

}

#define INTERRUPT_FLAG INTCONbits.TMR0IF
#define INTERRUPT_FLAG_MODE_KEY IOCAFbits.IOCAF3
// ----------------------------------------------------------------
// [Interrupt]
void interrupt _( void ) {

  if( INTERRUPT_FLAG_MODE_KEY ) {
    INTERRUPT_FLAG_MODE_KEY = 0 ;
    WDTCONbits.SWDTEN = 1 ;
    INTCONbits.TMR0IF = 0 ;
    INTCONbits.TMR0IE = 1 ;
    INTCONbits.IOCIE = 0 ;
  }

  if( !INTERRUPT_FLAG ) return ;
  INTERRUPT_FLAG = 0 ;

  // Read Key State
  static Uint08_t interruptCount = 0 ;
  if( ! --interruptCount ) {
    interruptCount = 10 ;
    portAState_.byte = ReadKeyState( ) ;

    if( machineState_ == STATE_SLEEP ) {
      if( portAState_.menu ) {
        if( ++sleepTimer_ == 100 ) {
          sleepTimer_ = 0 ;
          machineState_ = STATE_MEASURE ;
          SetEvent( events_.changeMessage ) ;
        }
      }
      else {
        SetEvent( events_.sleep ) ;
      }
    }
    else {
      if( portAState_.byte ) {
        sleepTimer_ = 0 ;
      }
      else if( ++sleepTimer_ == 60 * 100 )
        SetEvent( events_.sleep ) ;

    }

  }

  // Get A/D Value
  if( !ADCON0bits.GO ) {

    static Uint08_t bufferPostiion = 0 ;
    static ADValue_t adBuffer[ SIZE_OF_AD_BUFFER ] ;
    ADValue_t adValue = ( ( ( (Uint16_t)ADRESH ) << 8 ) | ADRESL ) ;
    ADCON0bits.GO = 1 ;

    if( !isHold_ ) {
      sumOfBuffer_ -= adBuffer[ bufferPostiion ] ;
      sumOfBuffer_ += adValue ;
      adBuffer[ bufferPostiion ] = adValue ;

      if( ++bufferPostiion == SIZE_OF_AD_BUFFER ) bufferPostiion = 0 ;

    }

    SetEvent( events_.changeValue ) ;

  }

  if( INTERRUPT_FLAG ) SetEvent( events_.error ) ;

}


