// ################################################################
// Ignore Warning for "possible use of "=" instead of "=="
#pragma warning disable 758
// ################################################################

#include <xc.h>
#include "pic16f1827_init.h"

#include "../../_Common/typedef.h"
#include "configuration.h"

#define DisableAllInterrupt() INTCONbits.GIE=0
#define EnableAllInterrupt() INTCONbits.GIE=1

// Key
typedef union {
  Uint08_t byte ;
  struct {
    Uint08_t _ : 4 ;
    Uint08_t keyDown : 1 ;
    Uint08_t keyCenter : 1 ;
    Uint08_t keyUp : 1 ;
  } ;
} UniPortA ;
UniPortA portAState_ ;
#define ReadKeyState() (~PORTA&0x70);

#define KEY_PRESS_LOOP_START 0x3C
#define KEY_PRESS_LOOP_END 0x40
struct {
  Uint08_t up ;
  Uint08_t down ;
  Uint08_t center ;
} keyCount ;

#define TONE_OFF 0
#define TONE_BOOT 125 // B
#define TONE_TUNE 141 // A
#define TONE_TONE0 251
#define TONE_TONE1 125
#define TONE_TONE2 167
#define SoundOff() T2CONbits.TMR2ON=0
#define SoundOn() T2CONbits.TMR2ON=1

const Uint08_t SEGMENT_BIT[] = {
  0b11111100 , // 0
  0b01100000 , // 1
  0b11011010 , // 2
  0b11110010 , // 3
  0b01100110 , // 4
  0b10110110 , // 5
  0b10111110 , // 6
  0b11100100 , // 7
  0b11111110 , // 8
  0b11110110 , // 9
  0b00000000 , // [Space]
} ;
const Uint08_t SEGMET_BIT2[] = {
  0b00001110 ,
  0b00001101 ,
  0b00001011 ,
  0b00000111 ,
} ;

// Display
Uint08_t displayBuffer[4] = { 10 , 10 , 10 , 10 } ;
Uint08_t displayDigit ;

// Mode
typedef enum {
  STATE_METRONOME = 0 ,
  STATE_TUNE ,
} MachineState ;
MachineState machineState = STATE_METRONOME ;

// Values
ConfigurationData config = CONFIGURATION_INITIAL ;

// Metronome Counter
#define _XTAL_FREQ 32000000L
#define TOTAL_TEMOPO_COUNT ( _XTAL_FREQ * 3 / 25 )
Uint24_t tempoCounter = 0 ;
union {
  Uint08_t all ;
  struct {
    Uint08_t beat : 2 ;
    Uint08_t measure : 2 ;
  } ;
} beatCounter = 0 ;
struct {
  Uint08_t click ;
  Uint08_t key ;
} durationCounter_ = { 0 , 0 } ;

Uint16_t autoHideCounter_ = 0 ;

// Event
#define ClearEvent( event ) event = 0;
#define SetEvent( event )   event = 1;
#define EvalEvent( event )  (event&&!(event=0))
union {
  Uint08_t all ;
  struct {
    unsigned changeValue : 1 ;
    unsigned resetMetronome : 1 ;
    unsigned soundOn : 1 ;
    unsigned soundOnKey : 1 ;
    unsigned soundOff : 1 ;
  } ;
} events_ ;
union {
  Uint08_t all ;
  struct {
    unsigned keyPressUp : 1 ;
    unsigned keyPressHeldUp : 1 ;
    unsigned keyPressDown : 1 ;
    unsigned keyPressHeldDown : 1 ;
    unsigned keyPressCenter : 1 ;
    unsigned keyPressHeldCenter : 1 ;
  } ;
} inputEvents_ ;

// [Function] Main ----------------
int main( void ) {

  initialize( ) ;

  _configuration_Load( &config ) ;

  machineState = STATE_METRONOME ;

  SetEvent( events_.resetMetronome ) ;

  // Enable Timer interrupt
  PIE3bits.TMR6IE = 1 ;
  PIR3bits.TMR6IF = 0 ;
  T6CONbits.TMR6ON = 1 ;
  INTCONbits.PEIE = 1 ;
  INTCONbits.GIE = 1 ;

  for( ; ; ) {

    // Clear Watchdog-Timer
    CLRWDT( ) ;

    // Set Key Event
    static UniPortA prevPortAState = { 0x00 } ;
    UniPortA keyPressed ;

    keyPressed.byte = ( portAState_.byte ^ prevPortAState.byte ) & portAState_.byte ;
    prevPortAState.byte = portAState_.byte ;

    if( keyPressed.keyCenter ) {
      SetEvent( inputEvents_.keyPressCenter ) ;
    }

    if( keyPressed.keyUp || EvalEvent( inputEvents_.keyPressHeldUp ) ) {
      SetEvent( inputEvents_.keyPressUp ) ;
    }

    if( keyPressed.keyDown || EvalEvent( inputEvents_.keyPressHeldDown ) ) {
      SetEvent( inputEvents_.keyPressDown ) ;
    }

    if( inputEvents_.all ) {
      SetEvent( events_.soundOnKey ) ;
    }

    // Change State
    if( EvalEvent( inputEvents_.keyPressCenter ) ) {

    }

    // Save Config
    if( EvalEvent( inputEvents_.keyPressHeldCenter ) ) {
      DisableAllInterrupt( ) ;
      _configuration_Save( &config ) ;
      RESET( ) ;
    }

    // Round Value
    switch( machineState ) {

      case STATE_METRONOME:
        if( EvalEvent( inputEvents_.keyPressUp ) && config.tempo != 9999 ) {
          config.tempo++ ;
          SetEvent( events_.changeValue ) ;
          SetEvent( events_.resetMetronome ) ;
        }
        if( EvalEvent( inputEvents_.keyPressDown ) && config.tempo != 0 ) {
          config.tempo-- ;
          SetEvent( events_.changeValue ) ;
          SetEvent( events_.resetMetronome ) ;
        }
        break ;


      case STATE_TUNE:
        if( EvalEvent( inputEvents_.keyPressUp ) && config.oscillatorTune != 63 ) config.oscillatorTune++ ;
        if( EvalEvent( inputEvents_.keyPressDown ) && config.oscillatorTune != 0 ) config.oscillatorTune-- ;
        break ;
    }

    // Reset Metronome
    if( EvalEvent( events_.resetMetronome ) ) {
      tempoCounter = 0 ;
      beatCounter.all = 0 ;
      SetEvent( events_.soundOn ) ;
      SetEvent( events_.changeValue ) ;
    }

    // Apply value Change
    if( EvalEvent( events_.changeValue ) ) {

      autoHideCounter_ = 0x1000 ;


      Uint16_t tmpValue ;
      switch( machineState ) {
        case STATE_METRONOME:
          tmpValue = config.tempo ;
          break ;


        case STATE_TUNE:
          tmpValue = config.oscillatorTune ;
          break ;

      }

      static const Uint16_t COMPARE_UNITS[] = { 1000 , 100 , 10 , 1 } ;

      Uint08_t displayNumber = 0 ;

      Bool_t isNonZero = BOOL_FALSE ;
      for( Uint08_t i = 0 ; i < 4 ; i++ ) {
        displayNumber = 0 ;
        Uint16_t compareUnit = COMPARE_UNITS[i] ;
        while( tmpValue >= compareUnit ) {
          tmpValue -= compareUnit ;
          displayNumber++ ;
          isNonZero = BOOL_TRUE ;
        }

        if( isNonZero )
          displayBuffer[i] = SEGMENT_BIT[ displayNumber ] ;
        else
          displayBuffer[i] = 0x00 ;
      }

    }

    // Each Mode
    switch( machineState ) {

      case STATE_TUNE: // Tuning Mode
        OSCTUNE = config.oscillatorTune - 32 ;
        PR2 = TONE_TUNE ;
        for( Uint08_t i = 0 ; i != 4 ; i++ )
          displayBuffer[i] &= 0xFE ;
        break ;

      default: // Metronome
        if( EvalEvent( events_.soundOnKey ) ) {
          durationCounter_.key = 0x40 ;
          PR2 = TONE_BOOT ;
          CCPR2L = ( PR2 >> 1 ) ;
          SoundOn( ) ;
        }
        if( EvalEvent( events_.soundOn ) && !durationCounter_.key ) {
          durationCounter_.click = 0x80 ;

          if( !beatCounter.beat ) {
            if( beatCounter.measure & 0x01 )
              PR2 = TONE_TONE2 ;
            else
              PR2 = TONE_TONE1 ;
          }
          else {
            PR2 = TONE_TONE0 ;
          }

          CCPR2L = ( PR2 >> 1 ) ;
          SoundOn( ) ;
        }
        break ;
    }

    if( EvalEvent( events_.soundOff ) )
      SoundOff( ) ;

  }

}
#define INTERRUPT_FLAG PIR3bits.TMR6IF
// [Interrupt] ----------------
void interrupt _( void ) {
  if( !INTERRUPT_FLAG ) return ;
  INTERRUPT_FLAG = 0 ;

  // Tempo Count
  tempoCounter += config.tempo ;
  if( tempoCounter >= TOTAL_TEMOPO_COUNT ) {
    tempoCounter -= TOTAL_TEMOPO_COUNT ;
    beatCounter.all++ ;

    SetEvent( events_.soundOn ) ;
  }

  // Prescaler
  static Uint16_t interruptCount = 0 ;
  if( ++interruptCount == 640 ) interruptCount = 0 ;

  if( !( interruptCount & 0x0F ) ) {
    if( autoHideCounter_ )
      autoHideCounter_-- ;

    if( durationCounter_.click && ! --durationCounter_.click && !durationCounter_.key )
      SetEvent( events_.soundOff ) ;
    if( durationCounter_.key && ! --durationCounter_.key )
      SetEvent( events_.soundOff ) ;
  }

  // Read Key State
  if( !interruptCount ) {
    portAState_.byte = ReadKeyState( ) ;

    if( portAState_.keyUp ) {
      if( ++keyCount.up == KEY_PRESS_LOOP_END ) {
        keyCount.up = KEY_PRESS_LOOP_START ;
        SetEvent( inputEvents_.keyPressHeldUp ) ;
      }
    }
    else
      keyCount.up = 0 ;

    if( portAState_.keyDown ) {
      if( ++keyCount.down == KEY_PRESS_LOOP_END ) {
        keyCount.down = KEY_PRESS_LOOP_START ;
        SetEvent( inputEvents_.keyPressHeldDown ) ;
      }
    }
    else
      keyCount.down = 0 ;

    if( portAState_.keyCenter ) {
      if( ++keyCount.center == KEY_PRESS_LOOP_END ) {
        keyCount.center = KEY_PRESS_LOOP_START ;
        SetEvent( inputEvents_.keyPressHeldCenter ) ;
      }
    }
    else
      keyCount.center = 0 ;
  }

  // Shift Display Digit
  if( !( interruptCount & 0x001F ) ) {
    displayDigit = ( displayDigit + 1 ) & 0x03 ;
    if( autoHideCounter_ ) {
      LATA = 0x0F ;
      LATB = displayBuffer[ displayDigit ] ;
      LATA = SEGMET_BIT2[ displayDigit ] ;
    }
    else {
      LATB = SEGMENT_BIT[beatCounter.beat + 1] ;
      LATA = SEGMET_BIT2[ beatCounter.beat ] ;
    }
  }

}
