// ################################################################
// Ignore Warning for "possible use of "=" instead of "=="
#pragma warning disable 758
// ################################################################

#include <xc.h>
#include "pic16f1827_init.h"

#include "../../_Common/typedef.h"
#include "configuration.h"
union {
  uint08_t byte ;
  struct {
    uint08_t _ : 4 ;
    uint08_t keyDown : 1 ;
    uint08_t keyMode : 1 ;
    uint08_t keyUp : 1 ;
  } ;
  struct {
    uint08_t _ : 4 ;
    uint08_t keys : 3 ;
  } ;
} porta ;
#define ReadKeyState() porta.byte=~PORTA;

#define PRESS_COUNT 0x10
#define PRESS_LOOP_END 0xFF
#define PRESS_LOOP_START 0xF0
#define PRESS_BEEP_MASK 0b00000110

#define TONE_OFF 0
#define TONE_BOOT 125 // B
#define TONE_TUNE 141 // A
#define TONE_TONE0 251
#define TONE_TONE1 125
#define TONE_TONE2 167

const uint08_t SEGMENT_BIT[] = {
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
const uint08_t SEGMET_BIT2[] = {
  0b00000111 ,
  0b00001011 ,
  0b00001101 ,
  0b00001110 ,
} ;

// Display
uint08_t displayBuffer[4] = { 10 , 10 , 10 , 10 } ;
uint08_t displayDigit ;

// Pres-scale
uint08_t prescaleEvent = 0 ;

// Button
union {
  struct {
    uint08_t up ;
    uint08_t upDown ;
    uint08_t down ;
    uint08_t mode ;
  } ;
  struct {
    uint16_t exceptDown ;
  } ;
  struct {
    uint08_t _ ;
    uint16_t exceptUp ;
  } ;
} keyCount ;

// Mode
typedef enum {
  STATE_METRONOME = 0 ,
  STATE_CONFIG_DURATION ,
  STATE_TUNE ,
  STATE_VOID ,
} MachineState ;
MachineState machineState = STATE_METRONOME ;

// Values
ConfigurationData config = CONFIGURATION_INITIAL ;

// Metronome Counter
#define _XTAL_FREQ 32000000L
#define TOTAL_TEMOPO_COUNT ( _XTAL_FREQ * 3 / 800 )
uint24_t tempoCounter = 0 ;
union {
  uint08_t all ;
  struct {
    uint08_t duration : 4 ;
    uint08_t beat : 2 ;
    uint08_t measure : 2 ;
  } ;
} beatCounter = 0 ;

// Event
#define ClearEvent( event ) event = 0;
#define SetEvent( event )   event = 1;
#define EvalEvent( event )  (event&&!(event=0))
union {
  uint08_t all ;
  struct {
    uint08_t valuePlus : 1 ;
    uint08_t valueMinus : 1 ;
    uint08_t changeState : 1 ;
    uint08_t displayValue : 1 ;
    uint08_t saveConfig : 1 ;
  } ;
  struct {
    uint08_t valueChange : 2 ;
  } ;
} events_ ;

// [Function] Main ----------------
int main( void ) {

  initialize( ) ;

  ReadKeyState( ) ;

  _configuration_Load( &config ) ;

  machineState = STATE_METRONOME ;
  tempoCounter = 0 ;
  beatCounter.all = 0 ;

  SetEvent( events_.displayValue ) ;

  // Enable Timer interrupt
  TMR6IE = 1 ;

  for( ; ; ) ;

}

// [Interrupt] ----------------
void interrupt _( void ) {
  if( !TMR6IF ) return ;
  TMR6IF = 0 ;

  // Tempo Count
  tempoCounter += config.tempo ;
  if( ++tempoCounter >= TOTAL_TEMOPO_COUNT ) {
    tempoCounter -= TOTAL_TEMOPO_COUNT ;
    beatCounter.all++ ;
  }

  // Pre-scale 1/32
  if( ++prescaleEvent & 0x3F ) return ;

  // Clear Watchdog-Timer
  CLRWDT( ) ;

  // Read Key State
  ReadKeyState( ) ;

  // Mode Button
  if( porta.keyMode ) {
    if( ++keyCount.mode == PRESS_LOOP_END ) keyCount.mode = PRESS_LOOP_START ;
    if( keyCount.mode == PRESS_COUNT ) SetEvent( events_.changeState ) ;
  }
  else
    keyCount.mode = 0 ;

  // Up Button
  if( porta.keyUp && !keyCount.exceptUp ) {
    if( ++keyCount.up == PRESS_LOOP_END ) keyCount.up = PRESS_LOOP_START ;
    if( keyCount.up == PRESS_COUNT || keyCount.up == PRESS_LOOP_START ) SetEvent( events_.valuePlus ) ;
    //    if( machineState == STATE_METRONOME && ( keyCount.up >= PRESS_LOOP_START ) )
    //      tone = ( ( keyCount.up & PRESS_BEEP_MASK ) ? config.tone[2] : TONE_OFF ) ;
  }
  else
    keyCount.up = 0 ;

  // Down Button
  if( porta.keyDown && !keyCount.exceptDown ) {
    if( ++keyCount.down == PRESS_LOOP_END ) keyCount.down = PRESS_LOOP_START ;
    if( keyCount.down == PRESS_COUNT || keyCount.down == PRESS_LOOP_START ) SetEvent( events_.valueMinus ) ;
    //    if( machineState == STATE_METRONOME && ( keyCount.down >= PRESS_LOOP_START ) )
    //      tone = ( ( keyCount.down & PRESS_BEEP_MASK ) ? config.tone[2] : TONE_OFF ) ;
  }
  else
    keyCount.down = 0 ;

  // Both Up & Down Key
  if( porta.keyDown && porta.keyUp ) {
    if( keyCount.upDown != PRESS_LOOP_END ) keyCount.upDown++ ;
    if( keyCount.upDown == PRESS_LOOP_START ) SetEvent( events_.saveConfig ) ;
  }
  else if( !( porta.keyDown && porta.keyUp ) )
    keyCount.upDown = 0 ;

  // Change State
  if( EvalEvent( events_.changeState ) ) {
    SetEvent( events_.displayValue ) ;

    if( ++machineState == STATE_VOID ) {
      machineState = STATE_METRONOME ;
      tempoCounter = 0 ;
      beatCounter.all = 0 ;
    }

  }

  // Save Config
  if( EvalEvent( events_.saveConfig ) ) {
    _configuration_Save( &config ) ;
    RESET( ) ;
  }

  // Round Value
  if( EvalEvent( events_.valueChange ) ) {

    SetEvent( events_.displayValue ) ;

    switch( machineState ) {

      case STATE_METRONOME:
        if( EvalEvent( events_.valuePlus ) && config.tempo != 999 ) config.tempo++ ;
        if( EvalEvent( events_.valueMinus ) && config.tempo != 0 ) config.tempo-- ;
        tempoCounter = 0 ;
        beatCounter.all = 0 ;
        break ;

      case STATE_CONFIG_DURATION:
        if( EvalEvent( events_.valuePlus ) && config.duration != 0x10 ) config.duration++ ;
        if( EvalEvent( events_.valueMinus ) && config.duration != 0x00 ) config.duration-- ;
        break ;

      case STATE_TUNE:
        if( EvalEvent( events_.valuePlus ) && config.oscillatorTune != 63 ) config.oscillatorTune++ ;
        if( EvalEvent( events_.valueMinus ) && config.oscillatorTune != 0 ) config.oscillatorTune-- ;
        break ;
    }

  }

  // Apply value Change
  if( EvalEvent( events_.displayValue ) ) {

    displayBuffer[0] = 0 ;
    displayBuffer[1] = 0 ;
    displayBuffer[2] = 0 ;
    displayBuffer[3] = 10 ;

    uint16_t tmpValue ;
    switch( machineState ) {
      case STATE_METRONOME:
        tmpValue = config.tempo ;
        break ;

      case STATE_CONFIG_DURATION:
        tmpValue = config.duration ;
        break ;

      case STATE_TUNE:
        tmpValue = config.oscillatorTune ;
        break ;

    }

    static const uint08_t COMPARE_UNITS[] = { 1 , 10 , 100 } ;

    uint08_t isNonZero = 0 ;
    for( uint08_t i = 2 ; i != 0xFF ; i-- ) {
      uint16_t compareUnit = COMPARE_UNITS[i] ;
      while( tmpValue >= compareUnit ) {
        tmpValue -= compareUnit ;
        displayBuffer[i]++ ;
        isNonZero = 1 ;
      }

      if( i == 0 ) break ;
      if( isNonZero ) continue ;

      displayBuffer[i] = 10 ;
    }

    for( uint08_t i = 0 ; i != 4 ; i++ )
      displayBuffer[i] = SEGMENT_BIT[ displayBuffer[ i ] ] ;

  }

  // Each Mode
  switch( machineState ) {

    case STATE_TUNE: // Tuning Mode
      OSCTUNE = config.oscillatorTune - 32 ;
      PR2 = TONE_TUNE ;
      for( uint08_t i = 0 ; i != 4 ; i++ )
        displayBuffer[i] &= 0xFE ;
      break ;

    default: // Metronome
      displayBuffer[3] = SEGMENT_BIT[ ( beatCounter.beat ) + 1 ] ;

      if( beatCounter.duration < config.duration ) {
        if( !beatCounter.beat ) {
          if( beatCounter.measure & 0x01 )
            PR2 = TONE_TONE2 ;
          else
            PR2 = TONE_TONE1 ;
        }
        else {
          PR2 = TONE_TONE0 ;
        }
        displayBuffer[3] |= 0x01 ;
      }
      else {
        PR2 = TONE_OFF ;
      }

      break ;
  }
  CCPR2L = ( PR2 >> 1 ) ;

  // Pre-scale 1/256
  if( prescaleEvent & 0x7F ) return ;

  // Shift Display Digit
  displayDigit = ( displayDigit + 1 ) & 0x03 ;
  //LATA = 0x0F ;
  //LATB = displayBuffer[ displayDigit ] ;
  //LATA = SEGMET_BIT2[ displayDigit ] ;
  LATB = displayBuffer[ 3 ] ;
  LATA = SEGMET_BIT2[ 3 ] ;

}
