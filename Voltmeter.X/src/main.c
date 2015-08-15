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

#include "configuration.h"

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
struct {
  Uint08_t up ;
  Uint08_t down ;
} keyCount_ ;

#define KEY_COUNT_LOOP_START 0x3C
#define KEY_COUNT_LOOP_END 0x40

#define CHAR_CURSOR 0x07

// --------------------------------
// State
typedef enum {
  STATE_BOOT ,
  STATE_MEASURE ,
  STATE_MENU ,
  STATE_VERSION ,
  STATE_ADJUST_SLEEP_TIME ,
  STATE_SLEEP ,
  STATE_ERROR ,
} MachineState ;
MachineState machineState_ = STATE_BOOT ;
//Bool_t isHold_ = BOOL_FALSE ;
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
  struct {
    const char* TITLE ;
    const char* VALUE ;
  } SLEEP ;
  const char* VERSION ;
  const char* ERROR ;
}
MESSAGE = {
  "Boot..." ,
  "Voltage" ,
  " 2 4 6 8" ,
  "A/D Val" ,
  {
    "Sleep" ,
    "     sec" ,
  } ,
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
  MENU_SLEEP ,
  MENU_VERSION ,
} ;
const char* MESSAGE_MENU[] = {
  "<Return" ,
  "Voltage" ,
  "Bar" ,
  "A/D Val" ,
  "Sleep" ,
  "Version" ,
} ;
struct {
  unsigned select : 7 ;
  unsigned cursor : 1 ;
} menuState_ ;

// --------------------------------
// A/D Converter
#define SIZE_OF_AD_BUFFER 16
typedef Uint16_t ADValue_t ;
static ADValue_t adBuffer_[ SIZE_OF_AD_BUFFER ] ;
static ADValue_t sumOfBuffer_ = 0 ;

// --------------------------------
// Configuration
StConfigurationData configuration_ = CONFIG_DEFAULT ;

// --------------------------------
// Event
struct {
  union {
    Uint08_t byte ;
    struct {
      unsigned keyPressMenu : 1 ;
      unsigned keyPressUp : 1 ;
      unsigned keyPressDown : 1 ;
      unsigned : 5 ;
    } ;
  } input ;
  union {
    Uint08_t byte ;
    struct {
      unsigned : 3 ;
      unsigned changeValue : 1 ;
      unsigned changeMessage : 1 ;
      unsigned sleep : 1 ;
      unsigned wake : 1 ;
      unsigned error : 1 ;
    } ;
  } output ;
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

  Configuration_Load( &configuration_ ) ;

  machineState_ = STATE_MEASURE ;
  currentMessage_ = MESSAGE.VOLTAGE ;
  SetEvent( events_.output.changeMessage ) ;

  // Start Timer Interrupt
  TMR0IF = 0 ;
  TMR0IE = 1 ;
  WDTCONbits.SWDTEN = 1 ;

  // Start Main Loop
  for( ; ; ) {

    // Clear Watchdog-Timer
    CLRWDT( ) ;

    // Sleep
    if( EvalEvent( events_.output.sleep ) ) {
      if( machineState_ != STATE_SLEEP ) {
        AQM0802_ClearRow( ROW_SELECT_0 ) ;
        AQM0802_ClearRow( ROW_SELECT_1 ) ;
      }
      WDTCONbits.SWDTEN = 0 ;
      INTCONbits.TMR0IE = 0 ;
      INTCONbits.IOCIF = 0 ;
      INTCONbits.IOCIE = 1 ;
      sleepTimer_ = 100 ;
      machineState_ = STATE_SLEEP ;
      SLEEP( ) ;
    }

    // Wake
    if( EvalEvent( events_.output.wake ) ) {
      if( machineState_ == STATE_SLEEP ) {
        machineState_ = STATE_MEASURE ;
        SetEvent( events_.output.changeMessage ) ;
      }
      sleepTimer_ = configuration_.sleepTime * 100 ;
    }

    // Set Key Event
    static UniPortA prevPortAState = { 0x00 } ;
    UniPortA keyPressed ;

    keyPressed.byte = ( portAState_.byte ^ prevPortAState.byte ) & portAState_.byte ;
    prevPortAState.byte = portAState_.byte ;

    if( keyPressed.menu )
      SetEvent( events_.input.keyPressMenu ) ;

    if( keyPressed.up ) {
      //      if( portAState_.down )
      //        SetEvent( events_.input.keyPressUpDown ) ;
      //      else
      SetEvent( events_.input.keyPressUp ) ;
    }
    if( keyPressed.down ) {
      //      if( portAState_.up )
      //        SetEvent( events_.input.keyPressUpDown ) ;
      //      else
      SetEvent( events_.input.keyPressDown ) ;
    }

    if( EvalEvent( events_.output.error ) ) {
      machineState_ = STATE_ERROR ;
      SetEvent( events_.output.changeMessage ) ;
    }

    // Menu Key
    if( EvalEvent( events_.input.keyPressMenu ) ) {

      SetEvent( events_.output.changeMessage ) ;

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

            case MENU_SLEEP:
              machineState_ = STATE_ADJUST_SLEEP_TIME ;
              break ;

            case MENU_VERSION:
              machineState_ = STATE_VERSION ;
              break ;
          }
          break ;

        case STATE_ADJUST_SLEEP_TIME:
          Configuration_Save( &configuration_ ) ;
        case STATE_VERSION:
          machineState_ = STATE_MENU ;
          break ;
      }
    }

    // Up/Down Key
    switch( machineState_ ) {

      case STATE_MENU:
        if( EvalEvent( events_.input.keyPressUp ) ) {
          if( menuState_.select != 0 ) {
            menuState_.select-- ;
            if( menuState_.cursor != 0 ) menuState_.cursor-- ;
            SetEvent( events_.output.changeMessage ) ;
          }
        }
        if( EvalEvent( events_.input.keyPressDown ) ) {
          if( menuState_.select != ( MENU_SIZE - 1 ) ) {
            menuState_.select++ ;
            if( menuState_.cursor != 1 ) menuState_.cursor++ ;
            SetEvent( events_.output.changeMessage ) ;
          }
        }
        break ;

      case STATE_ADJUST_SLEEP_TIME:
        if( EvalEvent( events_.input.keyPressUp ) ) {
          if( configuration_.sleepTime != 255 ) {
            configuration_.sleepTime++ ;
            SetEvent( events_.output.changeValue ) ;
          }
        }
        if( EvalEvent( events_.input.keyPressDown ) ) {
          if( configuration_.sleepTime != 1 ) {
            configuration_.sleepTime-- ;
            SetEvent( events_.output.changeValue ) ;
          }
        }

        break ;
    }

    // Change Message
    if( EvalEvent( events_.output.changeMessage ) ) {
      switch( machineState_ ) {

        case STATE_MEASURE:
          AQM0802_SendStringClearing( ROW_SELECT_0 | 0x0 , currentMessage_ ) ;
          AQM0802_ClearRow( ROW_SELECT_1 ) ;

          SetEvent( events_.output.changeValue ) ;
          break ;

        case STATE_MENU:
          AQM0802_SendStringClearing( ROW_SELECT_0 | 0x1 , MESSAGE_MENU[ menuState_.select - menuState_.cursor ] ) ;
          AQM0802_SendStringClearing( ROW_SELECT_1 | 0x1 , MESSAGE_MENU[ menuState_.select - menuState_.cursor + 1 ] ) ;
          AQM0802_SendCharacter( ROW_SELECT[ menuState_.cursor ] | 0x0 , CHAR_CURSOR ) ;
          break ;

        case STATE_ADJUST_SLEEP_TIME:
          AQM0802_SendStringClearing( ROW_SELECT_0 | 0x0 , MESSAGE.SLEEP.TITLE ) ;
          AQM0802_SendStringClearing( ROW_SELECT_1 | 0x0 , MESSAGE.SLEEP.VALUE ) ;
          SetEvent( events_.output.changeValue ) ;
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
    if( EvalEvent( events_.output.changeValue ) ) {

      Char_t string[9] ;

      switch( machineState_ ) {
        case STATE_MEASURE:
        {

          // Display Averaged Value
          ADValue_t currentAdValue = sumOfBuffer_ ;

          switch( measureMode_ ) {
            case MEASURE_MODE_VOLTAGE:
            case MEASURE_MODE_AD_VALUE:
            {
              const Uint16_t COMPARE_UNITS[] = { 10000 , 1000 , 100 , 10 , 1 } ;

              for( Uint08_t i = 0 ; i < 5 ; i++ ) {
                string[i + 1] = '0' ;
                Uint16_t compareUnit = COMPARE_UNITS[ i ] ;
                while( currentAdValue >= compareUnit ) {
                  string[i + 1]++ ;
                  currentAdValue -= compareUnit ;
                }
              }

              string[0] = ' ' ;
              if( measureMode_ == MEASURE_MODE_VOLTAGE ) {
                string[0] = string[1] ;
                string[1] = string[2] ;
                string[2] = '.' ;
                string[5] = 'V' ;
              }
              string[6] = 0 ;

              AQM0802_SendString( ROW_SELECT_1 | 0x2 , &string ) ;
            }
              break ;

            case MEASURE_MODE_BAR:
            {
              for( Uint08_t i = 0 ; i < 8 ; i++ ) {
                if( currentAdValue >= 1000 ) {
                  string[i] = '\x05' ;
                  currentAdValue -= 1000 ;
                }
                else {
                  string[i] = 0 ;
                  while( currentAdValue >= 200 ) {
                    string[i]++ ;
                    currentAdValue -= 200 ;
                  }
                  string[ i + 1 ] = 0 ;
                  break ;
                }

              }
              string[8] = 0 ;
              AQM0802_SendStringClearing( ROW_SELECT_1 | 0x0 , string ) ;
            }
              break ;

          }
        }
          break ;

        case STATE_ADJUST_SLEEP_TIME:
        {
          Uint08_t tmpValue = configuration_.sleepTime ;

          const Uint08_t COMPARE_UNITS[] = { 100 , 10 , 1 } ;
          Bool_t isNonZero = BOOL_FALSE ;

          for( Uint08_t i = 0 ; i < 3 ; i++ ) {
            string[i] = '0' ;
            Uint08_t compareUnit = COMPARE_UNITS[ i ] ;
            while( tmpValue >= compareUnit ) {
              string[i]++ ;
              tmpValue -= compareUnit ;
            }

            if( isNonZero || string[i] != '0' || i == 2 ) {
              isNonZero = BOOL_TRUE ;
            }
            else {
              string[i] = ' ' ;
            }
          }

          string[3] = 0 ;
          AQM0802_SendString( ROW_SELECT_1 | 0x2 , &string ) ;
        }
          break ;
      }



    }

  }//for(;;)

}

#define INTERRUPT_FLAG_TIMER INTCONbits.TMR0IF
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

  if( !INTERRUPT_FLAG_TIMER ) return ;
  INTERRUPT_FLAG_TIMER = 0 ;

  // Read Key State
  static Uint08_t interruptCount = 0 ;
  if( ! --interruptCount ) {
    interruptCount = 10 ;

    portAState_.byte = ReadKeyState( ) ;

    if( portAState_.up ) {
      if( ++keyCount_.up == KEY_COUNT_LOOP_END ) {
        keyCount_.up = KEY_COUNT_LOOP_START ;
        SetEvent( events_.input.keyPressUp ) ;
      }
    }
    else
      keyCount_.up = 0 ;

    if( portAState_.down ) {
      if( ++keyCount_.down == KEY_COUNT_LOOP_END ) {
        keyCount_.down = KEY_COUNT_LOOP_START ;
        SetEvent( events_.input.keyPressDown ) ;
      }
    }
    else
      keyCount_.down = 0 ;


    // Count Sleep Timer
    if( machineState_ == STATE_SLEEP ) {
      if( portAState_.menu ) {
        if( ! --sleepTimer_ )
          SetEvent( events_.output.wake ) ;
      }
      else {
        SetEvent( events_.output.sleep ) ;
      }
    }
    else {
      if( portAState_.byte ) {
        SetEvent( events_.output.wake ) ;
      }
      else {
        if( ! --sleepTimer_ )
          SetEvent( events_.output.sleep ) ;
      }
    }

  }

  // Get A/D Value
  if( !ADCON0bits.GO ) {

    static Uint08_t bufferPostiion = 0 ;
    ADValue_t adValue = ( ( ( (Uint16_t)ADRESH ) << 8 ) | ADRESL ) ;
    ADCON0bits.GO = 1 ;

    sumOfBuffer_ -= adBuffer_[ bufferPostiion ] ;
    sumOfBuffer_ += adValue ;
    adBuffer_[ bufferPostiion ] = adValue ;

    if( ++bufferPostiion == SIZE_OF_AD_BUFFER ) bufferPostiion = 0 ;

    SetEvent( events_.output.changeValue ) ;

  }

  if( INTERRUPT_FLAG_TIMER ) SetEvent( events_.output.error ) ;

}


