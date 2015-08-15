// ################################################################
// Ignore Warning for "possible use of "=" instead of "=="
#pragma warning disable 758
// ################################################################

// CONFIG1
#pragma config FOSC = INTOSC    // Oscillator Selection Bits (INTOSC oscillator: I/O function on CLKIN pin)
#pragma config WDTE = ON        // Watchdog Timer Enable (WDT enabled)
#pragma config PWRTE = ON       // Power-up Timer Enable (PWRT enabled)
#pragma config MCLRE = OFF      // MCLR Pin Function Select (MCLR/VPP pin function is digital input)
#pragma config CP = OFF         // Flash Program Memory Code Protection (Program memory code protection is disabled)
#pragma config BOREN = ON       // Brown-out Reset Enable (Brown-out Reset enabled)
#pragma config CLKOUTEN = OFF   // Clock Out Enable (CLKOUT function is disabled. I/O or oscillator function on the CLKOUT pin)
#pragma config IESO = OFF       // Internal/External Switchover Mode (Internal/External Switchover Mode is disabled)
#pragma config FCMEN = ON       // Fail-Safe Clock Monitor Enable (Fail-Safe Clock Monitor is enabled)

// CONFIG2
#pragma config WRT = OFF        // Flash Memory Self-Write Protection (Write protection off)
#pragma config STVREN = ON      // Stack Overflow/Underflow Reset Enable (Stack Overflow or Underflow will cause a Reset)
#pragma config BORV = HI        // Brown-out Reset Voltage Selection (Brown-out Reset Voltage (Vbor), high trip point selected.)
#pragma config LPBOR = ON       // Low-Power Brown Out Reset (Low-Power BOR is enabled)
#pragma config LVP = OFF        // Low-Voltage Programming Enable (High-voltage on MCLR/VPP must be used for programming)

#include <xc.h>
#include <stdbool.h>
#include "pic16f1508_init.h"

#define _XTAL_FREQ 1000000L

#define DisableAllInterrupt() INTCONbits.GIE=0;
#define EnableAllInterrupt() INTCONbits.GIE=1;

#define parallel_lcd_EnableWrite() LATBbits.LATB5=1;NOP();LATBbits.LATB5=0;
#define parallel_lcd_SelectResister( r ) if(r){LATBbits.LATB7=1;}else{LATBbits.LATB7=0;};
#define parallel_lcd_SetData( data ) LATC=data;

// ----------------------------------------------------------------
// Definition for Parallel LCD
#define PARALLEL_LCD_WIDTH 16
#define PARALLEL_LCD_ROW_COUNT 2
#define PARALLEL_LCD_TriggerWrite()      LATBbits.LATB5=1;NOP();LATBbits.LATB5=0;
#define PARALLEL_LCD_SelectResister( r ) LATBbits.LATB7=r&0x01;
#define PARALLEL_LCD_SetData( data )     LATC=data;
#define PARALLEL_LCD_WaitTimer()         __delay_us(40);
#define PARALLEL_LCD_ResetTimer()        ;

#include "../../_Common/typedef.h"
#include "../../_Common/ParallelLCD.h"
#include "../../_Common/DS1307.h"

#define SOFTWARE_VERSION "00.100"
typedef union {
  Uint08_t byte ;
  struct {
    unsigned keyDown : 1 ;
    unsigned keyUp : 1 ;
    unsigned : 2 ;
    unsigned keyRight : 1 ;
    unsigned keyLeft : 1 ;
  } ;
} UniPortA ;
UniPortA sampledPortAState_ = { 0x00 } ;
#define ReadKeyState() (~PORTA&0x33)

// Key Counter
union {
  struct {
    Uint08_t keyUp ;
    Uint08_t keyUD ;
    Uint08_t keyDown ;
    Uint08_t keyL ;
    Uint08_t keyLR ;
    Uint08_t keyR ;
  } ;
  struct {
    Uint16_t exceptDown ;
    Uint08_t _ ;
    Uint16_t exceptRight ;
  } ;
  struct {
    Uint08_t _0 ;
    Uint16_t exceptUp ;
    Uint08_t _1 ;
    Uint16_t exceptLeft ;
  } ;
} keyCounter_ ;
#define KEY_COUNT_LOOP_END   0x40
#define KEY_COUNT_LOOP_START 0x3C

#define CHAR_CURSOR 0x00

// Date and Time Data
StDateTime dateCurrent ;
StDateTime dateTimer ;
StDateTime* datePtr ;

// Prescaler
Uint08_t blinkPrescaler = 0 ;
Uint08_t keyBeepCounter_ = 0 ;
typedef struct {
  Uint08_t position ;
  Uint08_t length ;
  Uint08_t max ;
  Uint08_t min ;
} ValueInfo ;
const struct {
  ValueInfo year ;
  ValueInfo month ;
  ValueInfo date ;
  ValueInfo dayOfWeek ;
  ValueInfo hour ;
  ValueInfo minute ;
  ValueInfo second ;
}
VALUE_INFORMATIONS = {
  { PARALLEL_LCD_ROW_SELECT_0 | 0x2 , 2 , 0x99 , 0x00 } , //Year
  { PARALLEL_LCD_ROW_SELECT_0 | 0x5 , 2 , 0x12 , 0x01 } , //Month
  { PARALLEL_LCD_ROW_SELECT_0 | 0x8 , 2 , 0x31 , 0x01 } , //Date
  { PARALLEL_LCD_ROW_SELECT_0 | 0xC , 3 , 0x7 , 0x01 } , //Day-of-Week
  { PARALLEL_LCD_ROW_SELECT_1 | 0x0 , 2 , 0x23 , 0x00 } , //Hour
  { PARALLEL_LCD_ROW_SELECT_1 | 0x3 , 2 , 0x59 , 0x00 } , //Minute
  { PARALLEL_LCD_ROW_SELECT_1 | 0x6 , 2 , 0x59 , 0x00 } , //Second
} ;
const ValueInfo* currentValueInfo ;
Uint08_t* currentEditValue ;

//State
typedef enum {
  STATE_BOOT ,
  STATE_CLOCK ,
  STATE_MENU ,
  STATE_ADJUST_CLOCK ,
  STATE_ALERM ,
  STATE_SET_TIMER ,
  STATE_BUZZER_TEST ,
  STATE_VERSION ,
  STATE_ERROR ,
} MachineState ;
MachineState machineState_ = STATE_BOOT ;

// Menu
#define MENU_SIZE ( sizeof( MESSAGE_MENU ) / sizeof( MESSAGE_MENU[0] ) )
typedef enum {
  MENU_ADJUST = 0 ,
  MENU_TIMER ,
  MENU_BUZZER_TEST ,
  MENU_VERSION ,
} MenuItem ;
const char* MESSAGE_MENU[] = {
  "Adjust Clock" ,
  "Set Timer" ,
  "Buzzer Test" ,
  "Version" ,
} ;
MenuItem menuSelect = MENU_ADJUST ;
Uint08_t cursorPosition = 0 ;

const Uint08_t CURSOR_BITMAP[8] = {
  0b10000 ,
  0b11000 ,
  0b11100 ,
  0b11110 ,
  0b11100 ,
  0b11000 ,
  0b10000 ,
  0b00000 ,
} ;

Uint08_t alerm = 0 ;

EnDateItem editSelect = DATE_ITEM_YEAR ;

// Sound
#define SoundOn()   PWM3CONbits.PWM3OE=1
#define SoundOff()  PWM3CONbits.PWM3OE=0
#define IsSoundOn() PWM3CONbits.PWM3OE


// Event
#define SetEvent(event)   event=1
#define ClearEvent(event) event=0
#define EvalEvent(event)  (event&&!(event=0))
struct {
  union {
    Uint08_t byte ;
    struct {
      unsigned up : 1 ;
      unsigned upHold : 1 ;
      unsigned down : 1 ;
      unsigned downHold : 1 ;
      unsigned upDown : 1 ;
      unsigned left : 1 ;
      unsigned right : 1 ;
      unsigned leftRightHold : 1 ;
    } ;
  } keyPress ;
  union {
    Uint08_t byte ;
    struct {
      unsigned right : 1 ;
    } ;
  } keyRelease ;
  union {
    Uint08_t byte ;
    struct {
      unsigned updateClock : 1 ;
      unsigned changeMessage : 1 ;
      unsigned changeValue : 1 ;
      unsigned soundOn : 1 ;
      unsigned soundOff : 1 ;
    } ;
  } output ;
} events_ = { 0x00 , 0x00 } ;

// [Function] Main ----------------
int main( void ) {

  initialize( ) ;

  PWM3DCH = ( PR2 >> 2 ) ;
  PWM3DCL = ( ( PR2 & 0b11 ) << 6 ) ;
  PWM3OE = 0 ;

  // Read Timer From RAM of RTC
  DS1307_GetData( &dateTimer , 0x10 , 7 ) ;

  // Initialize parallel_lcd
  ParallelLCD_Initialize(
                          PARALLEL_LCD_CONFIG_8BIT_MODE | PARALLEL_LCD_CONFIG_2LINE_MODE ,
                          PARALLEL_LCD_CONFIG_DISPLAY_ON ,
                          PARALLEL_LCD_CONFIG_CURSOR_NONE ,
                          PARALLEL_LCD_CONFIG_INCREMENTAL
                          ) ;

  // Write CGRAM
  ParallelLCD_SetCgram( CHAR_CURSOR , CURSOR_BITMAP ) ;

  SetEvent( events_.output.updateClock ) ;
  machineState_ = STATE_CLOCK ;
  datePtr = &dateCurrent ;

  INTCONbits.TMR0IF = 0 ;
  INTCONbits.TMR0IE = 1 ;
  IOCIE = 0 ;

  for( ; ; ) {

    // Clear Watchdog-timer
    CLRWDT( ) ;

    if( EvalEvent( events_.output.updateClock ) ) {

      // Get Data from RTC
      if( DS1307_GetData( &dateCurrent , 0x00 , 7 ) ) {
        machineState_ = STATE_ERROR ;
      }
      else {

        // Check Alerm Time
        Bool_t isTimeToAlerm = BOOL_FALSE ;
        if( dateCurrent.date == dateTimer.date && dateCurrent.time == dateTimer.time ) {
          if( !dateTimer.dayOfWeek || dateCurrent.dayOfWeek == dateTimer.dayOfWeek )
            isTimeToAlerm = BOOL_TRUE ;
          else
            isTimeToAlerm = BOOL_FALSE ;
        }
        SetEvent( events_.output.changeMessage ) ;

        if( isTimeToAlerm ) machineState_ = STATE_ALERM ;
        if( dateCurrent.clockHalt ) machineState_ = STATE_ERROR ;
      }

    }

    static UniPortA prevPortAState = { 0x00 } ;
    UniPortA portAState , keyChange , keyPressed , keyReleased ;

    // Set Key Event
    portAState.byte = sampledPortAState_.byte ;
    keyChange.byte = portAState.byte ^ prevPortAState.byte ;
    keyPressed.byte = keyChange.byte & portAState.byte ;
    keyReleased.byte = keyChange.byte & ~portAState.byte ;
    prevPortAState.byte = portAState.byte ;

    // Up
    if( keyPressed.keyUp || EvalEvent( events_.keyPress.upHold ) ) {
      if( portAState.keyDown )
        SetEvent( events_.keyPress.upDown ) ;
      else
        SetEvent( events_.keyPress.up ) ;
    }

    // Down
    if( keyPressed.keyDown || EvalEvent( events_.keyPress.downHold ) ) {
      if( portAState.keyUp )
        SetEvent( events_.keyPress.upDown ) ;
      else
        SetEvent( events_.keyPress.down ) ;
    }

    // Left
    if( keyPressed.keyLeft ) {
      SetEvent( events_.keyPress.left ) ;
    }

    // Right
    if( keyPressed.keyRight ) {
      SetEvent( events_.keyPress.right ) ;
    }
    if( keyReleased.keyRight ) {
      SetEvent( events_.keyRelease.right ) ;
    }

    // Start Beep
    if( events_.keyPress.byte ) {
      keyBeepCounter_ = 20 ;
      SetEvent( events_.output.soundOn ) ;
    }

    // Left and Right
    if( EvalEvent( events_.keyPress.leftRightHold ) ) {

      SetEvent( events_.output.changeMessage ) ;

      switch( machineState_ ) {

        case STATE_ADJUST_CLOCK:
          DS1307_SetClock( &dateCurrent ) ;
          machineState_ = STATE_CLOCK ;
          break ;

        case STATE_SET_TIMER:
          machineState_ = STATE_CLOCK ;
          break ;

      }
    }

    // Left Key
    if( EvalEvent( events_.keyPress.left ) ) {

      switch( machineState_ ) {

        case STATE_MENU:
          machineState_ = STATE_CLOCK ;
          SetEvent( events_.output.changeMessage ) ;
          break ;

        case STATE_ADJUST_CLOCK:
        case STATE_SET_TIMER:
          if( editSelect == DATE_ITEM_YEAR ) {
            editSelect = DATE_ITEM_SECOND ;
            currentValueInfo = &VALUE_INFORMATIONS.second ;
            currentEditValue = &datePtr->second ;
          }
          else {
            editSelect-- ;
            currentValueInfo-- ;
            currentEditValue++ ;
          }

          blinkPrescaler = 0 ;
          break ;

        case STATE_BUZZER_TEST:
        case STATE_VERSION:
          machineState_ = STATE_MENU ;
          SetEvent( events_.output.changeMessage ) ;
          break ;

      }

    }

    // Right Key
    if( EvalEvent( events_.keyPress.right ) ) {

      switch( machineState_ ) {

        case STATE_CLOCK:
          machineState_ = STATE_MENU ;
          datePtr = &dateCurrent ;
          menuSelect = 0 ;
          cursorPosition = 0 ;
          SetEvent( events_.output.changeMessage ) ;
          break ;


        case STATE_ADJUST_CLOCK:
        case STATE_SET_TIMER:
          if( editSelect == DATE_ITEM_SECOND ) {
            editSelect = DATE_ITEM_YEAR ;
            currentValueInfo = &VALUE_INFORMATIONS.year ;
            currentEditValue = &datePtr->year ;
          }
          else {
            editSelect++ ;
            currentValueInfo++ ;
            currentEditValue-- ;
          }

          blinkPrescaler = 0 ;
          break ;

        case STATE_MENU:
          switch( menuSelect ) {
            case MENU_ADJUST:
              machineState_ = STATE_ADJUST_CLOCK ;
              datePtr = &dateCurrent ;
              currentValueInfo = &VALUE_INFORMATIONS.year ;
              currentEditValue = &datePtr->year ;
              break ;

            case MENU_TIMER:
              machineState_ = STATE_SET_TIMER ;
              datePtr = &dateTimer ;
              currentValueInfo = &VALUE_INFORMATIONS.year ;
              currentEditValue = &datePtr->year ;
              break ;

            case MENU_BUZZER_TEST:
              machineState_ = STATE_BUZZER_TEST ;
              break ;

            case MENU_VERSION:
              machineState_ = STATE_VERSION ;
              break ;
          }
          SetEvent( events_.output.changeMessage ) ;

          break ;

        case STATE_BUZZER_TEST:
          keyBeepCounter_ = 0 ;
          break ;
      }

    }

    // Right Key Release
    if( EvalEvent( events_.keyRelease.right ) ) {
      switch( machineState_ ) {
        case STATE_BUZZER_TEST:
          SetEvent( events_.output.soundOff ) ;
          break ;
      }
    }

    // Both Up and Down Key
    if( EvalEvent( events_.keyPress.upDown ) ) {
      switch( machineState_ ) {

        case STATE_ALERM:
          machineState_ = STATE_CLOCK ;
          SetEvent( events_.output.changeMessage ) ;
          break ;

      }
    }

    // Up or Down Key
    switch( machineState_ ) {

      case STATE_MENU:
        if( EvalEvent( events_.keyPress.up ) ) {
          if( menuSelect ) {
            menuSelect-- ;
            SetEvent( events_.output.changeMessage ) ;
          }
          if( cursorPosition != 0 ) {
            cursorPosition-- ;
            SetEvent( events_.output.changeMessage ) ;
          }

        }
        if( EvalEvent( events_.keyPress.down ) ) {
          if( menuSelect != ( MENU_SIZE - 1 ) ) {
            menuSelect++ ;
            SetEvent( events_.output.changeMessage ) ;
          }
          if( cursorPosition != 1 ) {
            cursorPosition++ ;
            SetEvent( events_.output.changeMessage ) ;
          }
        }
        break ;

      case STATE_ADJUST_CLOCK:
      case STATE_SET_TIMER:
        if( EvalEvent( events_.keyPress.up ) ) {
          if( *currentEditValue == currentValueInfo->max )
            *currentEditValue = currentValueInfo->min ;
          else if( ( *currentEditValue & 0x0F ) == 0x09 )
            *currentEditValue += 7 ;
          else
            ( *currentEditValue )++ ;
          SetEvent( events_.output.changeValue ) ;
        }
        if( EvalEvent( events_.keyPress.down ) ) {
          if( *currentEditValue == currentValueInfo->min )
            *currentEditValue = currentValueInfo->max ;
          else if( ( *currentEditValue & 0x0F ) == 0x00 )
            *currentEditValue -= 7 ;
          else
            ( *currentEditValue )-- ;
          SetEvent( events_.output.changeValue ) ;
        }
        break ;

      case STATE_BUZZER_TEST:
        if( EvalEvent( events_.keyPress.up ) ) {
          if( PR2 != 0xFF ) PR2++ ;
          SetEvent( events_.output.changeValue ) ;
        }
        if( EvalEvent( events_.keyPress.down ) ) {
          if( PR2 != 0 ) PR2-- ;
          SetEvent( events_.output.changeValue ) ;
        }
        break ;

      case STATE_CLOCK:
        if( EvalEvent( events_.keyPress.up ) || EvalEvent( events_.keyPress.down ) ) {
          machineState_ = STATE_MENU ;
          SetEvent( events_.output.changeMessage ) ;
        }
        break ;

    }

    if( EvalEvent( events_.output.soundOn ) )
      SoundOn( ) ;

    if( EvalEvent( events_.output.soundOff ) )
      SoundOff( ) ;


    //    // Buzzer
    //    switch( machineState ) {
    //
    //      case STATE_BUZZER_TEST:
    //        if( portAState_.keyRight )
    //          SoundOn( ) ;
    //        else
    //          SoundOff( ) ;
    //        break ;
    //
    //      case STATE_ALERM:
    //        alerm++ ;
    //        if( ( alerm & 0b00000100 ) && !( alerm & 0b00100000 ) )
    //          SoundOn( ) ;
    //        else
    //          SoundOff( ) ;
    //        break ;
    //
    //      default:
    //        SoundOff( ) ;
    //        break ;
    //
    //    }

    // Change Message
    if( EvalEvent( events_.output.changeMessage ) ) {

      switch( machineState_ ) {
        case STATE_CLOCK:
        case STATE_ALERM:
        case STATE_ADJUST_CLOCK:
        case STATE_SET_TIMER:

          ParallelLCD_ClearRow( PARALLEL_LCD_ROW_SELECT_0 ) ;

          switch( machineState_ ) {
            case STATE_CLOCK:
              ParallelLCD_ClearRow( PARALLEL_LCD_ROW_SELECT_1 ) ;
              break ;
            case STATE_ALERM:
              ParallelLCD_WriteStringClearing( PARALLEL_LCD_ROW_SELECT_1 | 0x9 , "ALERM!!" ) ;
              break ;
            case STATE_ADJUST_CLOCK:
              ParallelLCD_WriteStringClearing( PARALLEL_LCD_ROW_SELECT_1 | 0xA , "adjust" ) ;
              break ;
            case STATE_SET_TIMER:
              ParallelLCD_WriteStringClearing( PARALLEL_LCD_ROW_SELECT_1 | 0xB , "timer" ) ;
              break ;
          }
          SetEvent( events_.output.changeValue ) ;

          break ;

        case STATE_MENU:
          ParallelLCD_WriteStringClearing( PARALLEL_LCD_ROW_SELECT_0 | 0x1 , MESSAGE_MENU[ menuSelect - cursorPosition ] ) ;
          ParallelLCD_WriteStringClearing( PARALLEL_LCD_ROW_SELECT_1 | 0x1 , MESSAGE_MENU[ menuSelect - cursorPosition + 1] ) ;
          ParallelLCD_WriteCharacter( PARALLEL_LCD_ROW_SELECT[cursorPosition] | 0x0 , CHAR_CURSOR ) ;
          break ;

        case STATE_BUZZER_TEST:
          ParallelLCD_WriteStringClearing( PARALLEL_LCD_ROW_SELECT_0 | 0x0 , "Buzzer Test" ) ;
          ParallelLCD_WriteStringClearing( PARALLEL_LCD_ROW_SELECT_1 | 0x0 , "Period =" ) ;
          SetEvent( events_.output.changeValue ) ;
          break ;

        case STATE_VERSION:
          ParallelLCD_WriteStringClearing( PARALLEL_LCD_ROW_SELECT_0 | 0x0 , "Version" ) ;
          ParallelLCD_WriteStringClearing( PARALLEL_LCD_ROW_SELECT_1 | 0x8 , SOFTWARE_VERSION ) ;
          break ;

        case STATE_ERROR:
          ParallelLCD_WriteStringClearing( PARALLEL_LCD_ROW_SELECT_0 | 0x0 , "Receive Error !!" ) ;
          ParallelLCD_ClearRow( PARALLEL_LCD_ROW_SELECT_1 ) ;
          break ;
      }

    }

    // Change Value
    if( EvalEvent( events_.output.changeValue ) ) {
      switch( machineState_ ) {
        case STATE_CLOCK:
        case STATE_ALERM:
        case STATE_ADJUST_CLOCK:
        case STATE_SET_TIMER:
        {
          char string[17] ;
          _date_time_ConvertByteToDate( datePtr , &string ) ;
          ParallelLCD_WriteString( PARALLEL_LCD_ROW_SELECT_0 | 0x0 , &string ) ;
          _date_time_ConvertByteToTime( datePtr , &string ) ;
          ParallelLCD_WriteString( PARALLEL_LCD_ROW_SELECT_1 | 0x0 , &string ) ;
          blinkPrescaler = 0 ;
        }
          break ;

        case STATE_BUZZER_TEST:
        {
          char valueString[4] = "000" ;
          PWM3DCH = PR2 >> 2 ;
          Uint08_t tmpValue = PR2 ;
          Uint08_t isNonZero = 0 ;
          static const Uint08_t COMPARE_UNITS[] = { 100 , 10 , 1 } ;

          for( Uint08_t i = 0 ; i != 3 ; i++ ) {
            Uint08_t compareUnit = COMPARE_UNITS[i] ;
            while( tmpValue >= compareUnit ) {
              tmpValue -= compareUnit ;
              valueString[i]++ ;
              isNonZero = 1 ;
            }

            if( i == 2 ) break ;
            if( isNonZero ) continue ;

            valueString[i] = ' ' ;
          }

          ParallelLCD_WriteString( PARALLEL_LCD_ROW_SELECT_1 | 0xD , &valueString ) ;
        }
          break ;
      }

    }

    if( machineState_ == STATE_ADJUST_CLOCK || machineState_ == STATE_SET_TIMER ) {
      char string[4] ;
      if( blinkPrescaler == 0x00 ) {
        _date_time_ConvertByteToDiscrete( datePtr , &string , editSelect ) ;
        ParallelLCD_WriteString( currentValueInfo->position , &string ) ;
      }
      if( blinkPrescaler == 0xC0 ) {
        ParallelLCD_ClearPartial( currentValueInfo->position , currentValueInfo->length ) ;
      }
    }
  }// for(;;)

}

#define INTERRUPT_FLAG_TIMER INTCONbits.TMR0IF

// [Interrupt] ----------------
void interrupt _( void ) {

  // Reload Clock
  if( IOCIF ) {
    IOCAF3 = 0 ;
    SetEvent( events_.output.updateClock ) ;
  }

  if( INTERRUPT_FLAG_TIMER ) {
    INTERRUPT_FLAG_TIMER = 0 ;

    if( keyBeepCounter_ && !--keyBeepCounter_ )
      SetEvent( events_.output.soundOff ) ;

    static Uint08_t prescaler10ms = 0 ;

    // Prescale to 10ms
    if( ! --prescaler10ms ) {
      prescaler10ms = 10 ;

      // Read Key State
      sampledPortAState_.byte = ReadKeyState( ) ;

      if( sampledPortAState_.keyUp && !sampledPortAState_.keyDown ) {
        if( ++keyCounter_.keyUp == KEY_COUNT_LOOP_END ) {
          keyCounter_.keyUp = KEY_COUNT_LOOP_START ;
          SetEvent( events_.keyPress.upHold ) ;
        }
      }
      else
        keyCounter_.keyUp = 0 ;

      if( sampledPortAState_.keyDown && !sampledPortAState_.keyUp ) {
        if( ++keyCounter_.keyDown == KEY_COUNT_LOOP_END ) {
          keyCounter_.keyDown = KEY_COUNT_LOOP_START ;
          SetEvent( events_.keyPress.downHold ) ;
        }
      }
      else
        keyCounter_.keyDown = 0 ;

    }
  }

}

