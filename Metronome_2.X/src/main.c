// ################################################################
// Ignore Warning for "possible use of "=" instead of "=="
#pragma warning disable 758
// ################################################################

// ----------------------------------------------------------------
// Configuration Bits

// CONFIG1
#pragma config FOSC = INTOSC    // Oscillator Selection (INTOSC oscillator: I/O function on CLKIN pin)
#pragma config WDTE = ON        // Watchdog Timer Enable (WDT enabled)
#pragma config PWRTE = ON       // Power-up Timer Enable (PWRT enabled)
#pragma config MCLRE = OFF      // MCLR Pin Function Select (MCLR/VPP pin function is digital input)
#pragma config CP = OFF         // Flash Program Memory Code Protection (Program memory code protection is disabled)
#pragma config CPD = OFF        // Data Memory Code Protection (Data memory code protection is disabled)
#pragma config BOREN = ON       // Brown-out Reset Enable (Brown-out Reset enabled)
#pragma config CLKOUTEN = OFF   // Clock Out Enable (CLKOUT function is disabled. I/O or oscillator function on the CLKOUT pin)
#pragma config IESO = OFF       // Internal/External Switchover (Internal/External Switchover mode is disabled)
#pragma config FCMEN = OFF      // Fail-Safe Clock Monitor Enable (Fail-Safe Clock Monitor is disabled)

// CONFIG2
#pragma config WRT = OFF        // Flash Memory Self-Write Protection (Write protection off)
#pragma config PLLEN = OFF      // PLL Enable (4x PLL disabled)
#pragma config STVREN = ON      // Stack Overflow/Underflow Reset Enable (Stack Overflow or Underflow will cause a Reset)
#pragma config BORV = HI        // Brown-out Reset Voltage Selection (Brown-out Reset Voltage (Vbor), high trip point selected.)
#pragma config LVP = OFF        // Low-Voltage Programming Enable (High-voltage on MCLR/VPP must be used for programming)

// ----------------------------------------------------------------
// User ID ( Software Version )
#pragma config IDLOC0 = 0x0001 // Major Version
#pragma config IDLOC1 = 0x0001 // Minor Version
#pragma config IDLOC2 = 0x0000 // Debug Version
#pragma config IDLOC3 = 0x0000 // (Reserved)

// ----------------------------------------------------------------
// Include
#include "xc.h"
#include "pic16f1827_init.h"
#include "../../_Common/util.h"

// ----------------------------------------------------------------
// Definition for Parallel LCD
#define PARALLEL_LCD_WIDTH 16
#define PARALLEL_LCD_ROW_COUNT 2
#define PARALLEL_LCD_TriggerWrite()      {LATAbits.LATA0=1;NOP();LATAbits.LATA0=0;}
#define PARALLEL_LCD_SelectResister( r ) {if(r)LATAbits.LATA1=1;else LATAbits.LATA1=0;}
#define PARALLEL_LCD_SetData( data )     {LATB=data;}
#define PARALLEL_LCD_WaitTimer()         {while(!PIR3bits.TMR4IF);}
#define PARALLEL_LCD_ResetTimer()        {TMR4=0x00;PIR3bits.TMR4IF=0;}

// ----------------------------------------------------------------
// Include Original Header
#include "../../_Common/parallel_LCD.h"
#include "../../_Common/typedef.h"

#include "configuration.h"
#include "menu.h"
#include "message.h"

// ----------------------------------------------------------------
// Macros
#define EnableAllInterrupt()  INTCONbits.GIE=1
#define DisableAllInterrupt() INTCONbits.GIE=0
#define SoundOff() T2CONbits.TMR2ON = 0
#define SoundOn()  T2CONbits.TMR2ON = 1
#define SetOscillatorTune( tune ) OSCTUNE = tune
#define SetSoundTimerPeriod( period ) PR2 = period
#define SetSoundPulseWidth( pulseWidth ) {CCPR4L=(PR2>>pulseWidth);}

// ----------------------------------------------------------------
// State
typedef enum {
  STATE_BOOT ,
  STATE_METRONOME ,
  STATE_MENU_MAIN ,
  STATE_MENU_TONE ,
  STATE_ADJUST_BEAT_COUNT ,
  STATE_ADJUST_DURATION ,
  STATE_ADJUST_PULSE_WIDTH ,
  STATE_ADJUST_TONE ,
  STATE_ADJUST_OSCILLATOR_TUNE ,
  STATE_INFORMATION ,
  STATE_CONFIRM_SAVE ,
  STATE_SAVE ,
  STATE_CONFIRM_LOAD ,
  STATE_LOAD ,
  STATE_CONFIRM_RESET ,
  STATE_INITIALIZE ,
  STATE_RESET ,
  STATE_ERROR ,
} MachineState ;
MachineState machineState_ = STATE_BOOT ;
uint08 stateReturnCounter_ = 0 ;
uint08 isMute_ ;

// ----------------------------------------------------------------
// Metronome Counter
#define _XTAL_FREQ 32000000UL
#define TOTAL_TEMOPO_COUNT ( _XTAL_FREQ * 3 / 25 )
#if TOTAL_TEMOPO_COUNT > 0xFFFFFF
#  error [ERROR] Too Large Tempo Count !!
#endif
static uint24 tempoCounter_ = 0 ;
static uint08 beatCounter_ = 0 ;
static uint08 duration_ = 0 ;

// ----------------------------------------------------------------
// Keys
typedef union {
  uint08 all ;
  struct {
    uint08 _ : 5 ;
    uint08 keyMenu : 1 ;
    uint08 keyDown : 1 ;
    uint08 keyUp : 1 ;
  } ;
} UniPortAState ;
#define ReadKeyState() (~PORTA&0xE0)
UniPortAState portAState_ = { 0x00 } ;
uint08 keyBeepCounter_ = 0 ;
#define KEY_BEEP_DURATION 0x20

// ----------------------------------------------------------------
// Key Count
#define KEY_COUNT_LOOP_START 0x3C
#define KEY_COUNT_LOOP_END 0x40
union {
  uint32 all ;
  struct {
    uint08 Up ;
    uint08 UpDown ;
    uint08 Down ;
    uint08 Mode ;
  } ;
  struct {
    uint16 exceptDown ;
  } ;
  struct {
    uint08 ;
    uint16 exceptUp ;
  } ;
} keyCount_ = 0 ;

// ----------------------------------------------------------------
// Menu State
typedef struct {
  uint08 select ;
  uint08 cursorPosition ;
  const uint08 limit ;
  const char** menuMessage ;
} MenuState ;
MenuState menuStateMain_ = { 0 , 0 , MENU_SIZE_MAIN - 1 , &MESSAGE_MENU_ITEM_MAIN } ;
MenuState menuStateTone_ = { 0 , 0 , MENU_SIZE_TONE - 1 , &MESSAGE_MENU_ITEM_TONE } ;
MenuState menuStateConfirm_ = { 0 , 0 , 1 , 0 } ;
MenuState menuStateInformation_ = { 0 , 0 , ( sizeof ( MESSAGE_INFORMATION ) / sizeof ( MESSAGE_INFORMATION[0] ) ) - 2 , 0 } ;
MenuState* currentMenuStatePtr_ ;

// ----------------------------------------------------------------
// Configuration
ConfigurationData configration_ = CONFIG_DEFAULT ;

// ----------------------------------------------------------------
// Value
uint08* currentValuePtr ;
struct {
  uint08 upper ;
  uint08 lower ;
} valueLimit_ ;

// ----------------------------------------------------------------
// Events
#define ClearEvent( event ) event = 0
#define SetEvent( event )   event = 1
#define EvalEvent( event )  (event&&!(event=0))
union {
  uint08 all ;
  struct {
    uint08 keyPressUp : 1 ;
    uint08 keyPressDown : 1 ;
    uint08 keyPressMenu : 1 ;
    uint08 keyPressUpDown : 1 ;
    uint08 keyPressHeldUp : 1 ;
    uint08 keyPressHeldDown : 1 ;
  } ;
} inputEvent_ ;
union {
  uint08 all ;
  struct {
    uint08 changeState : 1 ;
    uint08 changeMessage : 1 ;
    uint08 changeValue : 1 ;
    uint08 resetMetronome : 1 ;
    uint08 soundClickOn : 1 ;
    uint08 soundOn : 1 ;
    uint08 soundOff : 1 ;
    uint08 accessEeprom : 1 ;
  } ;
} outputEvent_ ;

// ----------------------------------------------------------------
// Tone
#define TONE_OFF 0
#define TONE_SYSTEM 124
#define TONE_TUNE 141 // A = 880 Hz

// --------------------------------------------------------------------------------------------------------------------------------
// [Function] Main
void main( void ) {

  // Initialize
  initialize( ) ;

  // Read or Write Configuration from EEPROM
  if( ( ReadKeyState( ) & 0xE0 ) == 0xC0 )
    machineState_ = STATE_INITIALIZE ;
  else
    machineState_ = STATE_BOOT ;

  SetEvent( outputEvent_.accessEeprom ) ;

  // Boot Beep
  SetSoundTimerPeriod( TONE_SYSTEM ) ;
  SetSoundPulseWidth( 1 ) ;

  // Timer for LCD Command Wait
  T4CONbits.TMR4ON = 1 ;

  // Timer for Boot Sequence
  T1CONbits.TMR1ON = 1 ;

  // Execute Boot Sequence
  for( uint08 phase = 0 ; phase < 0xE ; phase++ ) {

    CLRWDT( ) ;

    TMR1 = 0x0000 ;

    switch( phase ) {
      case 0x0:
      case 0x2:
        // Start Sound
        SoundOn( ) ;
        break ;

      case 0x1:
      case 0xC:
        // Stop Sound
        SoundOff( ) ;
        break ;

      case 0x3:
        // Read User ID
      {
        uint08 userId ;
        userId = _configuration_ReadByte( 0 , MEMORY_SELECT_CONFIGURATION ) ;
        informationValueBuffer[ INFORMATION_ITEM_VERSION ][1] = ( ( userId >> 4 ) | '0' ) ;
        informationValueBuffer[ INFORMATION_ITEM_VERSION ][2] = ( ( userId & 0x0F ) | '0' ) ;
        userId = _configuration_ReadByte( 1 , MEMORY_SELECT_CONFIGURATION ) ;
        informationValueBuffer[ INFORMATION_ITEM_VERSION ][4] = ( ( userId >> 4 ) | '0' ) ;
        informationValueBuffer[ INFORMATION_ITEM_VERSION ][5] = ( ( userId & 0x0F ) | '0' ) ;
      }
        break ;

      case 0x4:
        // Initialize Display
        _parallel_lcd_Initialize( ) ;

        // Indicate Title and Version
        _parallel_lcd_WriteStringClearing( PARALLEL_LCD_ROW_SELECT_0 | 0x0 , MESSAGE.METRONOME.TILE ) ;
        _parallel_lcd_WriteStringClearing( PARALLEL_LCD_ROW_SELECT_1 | 0x0 , MESSAGE_INFORMATION[ INFORMATION_ITEM_VERSION ] ) ;
        _parallel_lcd_WriteString( PARALLEL_LCD_ROW_SELECT_1 | 0xA , &informationValueBuffer[ INFORMATION_ITEM_VERSION ] ) ;

      case 0x6:
        // Set CGRAM to LCD
        _parallel_lcd_SetCgram( CHAR_CODE.CURSOR_UP , & BITMAP.CURSOR_UP ) ;
        _parallel_lcd_SetCgram( CHAR_CODE.CURSOR_DOWN , & BITMAP.CURSOR_DOWN ) ;
        _parallel_lcd_SetCgram( CHAR_CODE.CURSOR_RIGHT , & BITMAP.CURSOR_RIGHT ) ;
        break ;

    }

    while( !PIR1bits.TMR1IF ) ;
    PIR1bits.TMR1IF = 0 ;

  }

  // Start Timer Interrupt
  //INTCONbits.GIE = 1 ;
  INTCONbits.PEIE = 1 ;
  T6CONbits.TMR6ON = 1 ;
  PIE3bits.TMR6IE = 1 ;

  // Main Loop
  for( ; ; ) {

    // Clear Watchdog-Timer ----------------
    CLRWDT( ) ;

    // Set Input Event ----------------
    static UniPortAState prevPortAState = { 0x00 } ;
    UniPortAState keyPressed ;

    keyPressed.all = ( portAState_.all ^ prevPortAState.all ) & portAState_.all ;
    prevPortAState.all = portAState_.all ;

    if( keyPressed.keyMenu ) {
      SetEvent( inputEvent_.keyPressMenu ) ;
    }

    if( keyPressed.keyUp ) {
      if( portAState_.keyDown )
        SetEvent( inputEvent_.keyPressUpDown ) ;
      else
        SetEvent( inputEvent_.keyPressUp ) ;
    }

    if( keyPressed.keyDown ) {
      if( portAState_.keyUp )
        SetEvent( inputEvent_.keyPressUpDown ) ;
      else
        SetEvent( inputEvent_.keyPressDown ) ;
    }

    if( EvalEvent( inputEvent_.keyPressHeldUp ) )
      SetEvent( inputEvent_.keyPressUp ) ;

    if( EvalEvent( inputEvent_.keyPressHeldDown ) )
      SetEvent( inputEvent_.keyPressDown ) ;

    if( inputEvent_.all ) {
      switch( machineState_ ) {
        case STATE_ADJUST_TONE:
        case STATE_ADJUST_OSCILLATOR_TUNE:
          // Do Nothing
          break ;

        default:
          keyBeepCounter_ = KEY_BEEP_DURATION ;
          SetSoundTimerPeriod( TONE_SYSTEM ) ;
          SetSoundPulseWidth( 1 ) ;
          SoundOn( ) ;
          break ;
      }

    }

    // EEPROM ----------------
    if( EvalEvent( outputEvent_.accessEeprom ) ) {

      DisableAllInterrupt( ) ;

      ReturnCode returnCode = RETURN_CODE_NOERROR ;

      switch( machineState_ ) {

        case STATE_BOOT:
          machineState_ = STATE_METRONOME ;
          SetEvent( outputEvent_.changeState ) ;
        case STATE_LOAD:
          returnCode = _configuration_Load( &configration_ ) ;
          break ;

        case STATE_INITIALIZE:
          SetEvent( outputEvent_.changeState ) ;
        case STATE_SAVE:
          returnCode = _configuration_Save( &configration_ ) ;
          break ;

      }
      SetEvent( outputEvent_.resetMetronome ) ;

      uint08 romOffset = _configuration_GetRomOffset( ) ;
      informationValueBuffer[ INFORMATION_ITEM_ROM_OFFSET ][3] = HEX_TABLE[ romOffset >> 4 ] ;
      informationValueBuffer[ INFORMATION_ITEM_ROM_OFFSET ][4] = HEX_TABLE[ romOffset & 0x0F ] ;
      informationValueBuffer[ INFORMATION_ITEM_WRITE_COUNT ][3] = HEX_TABLE[ configration_.writeCount >> 4 ] ;
      informationValueBuffer[ INFORMATION_ITEM_WRITE_COUNT ][4] = HEX_TABLE[ configration_.writeCount & 0x0F ] ;
      informationValueBuffer[ INFORMATION_ITEM_ERROR_CODE ][3] = HEX_TABLE[ returnCode >> 4 ] ;
      informationValueBuffer[ INFORMATION_ITEM_ERROR_CODE ][4] = HEX_TABLE[ returnCode & 0x0F ] ;

      if( returnCode )
        machineState_ = STATE_ERROR ;

      EnableAllInterrupt( ) ;

    }

    // Both Up & Donw ----------------
    if( EvalEvent( inputEvent_.keyPressUpDown ) ) {
      if( machineState_ == STATE_METRONOME ) {
        Toggle( isMute_ ) ;
        SetEvent( outputEvent_.changeMessage ) ;
      }
    }

    // Apply Menu Key Event ----------------
    if( EvalEvent( inputEvent_.keyPressMenu ) ) {
      SetEvent( outputEvent_.changeState ) ;

      switch( machineState_ ) {

        case STATE_MENU_MAIN:
          switch( menuStateMain_.select ) {
            case MENU_ITEM_MAIN_RETURN:
              machineState_ = STATE_METRONOME ;
              break ;

            case MENU_ITEM_MAIN_BEAT_COUNT:
              machineState_ = STATE_ADJUST_BEAT_COUNT ;
              break ;

            case MENU_ITEM_MAIN_TONE_MENU:
              machineState_ = STATE_MENU_TONE ;
              menuStateTone_.select = 0 ;
              menuStateTone_.cursorPosition = 0 ;
              break ;

            case MENU_ITEM_MAIN_ADJUST_DURATION:
              machineState_ = STATE_ADJUST_DURATION ;
              break ;

            case MENU_ITEM_MAIN_PULSE_WIDTH:
              machineState_ = STATE_ADJUST_PULSE_WIDTH ;
              break ;

            case MENU_ITEM_MAIN_ADJUST_OSCILLATOR_TUNE:
              machineState_ = STATE_ADJUST_OSCILLATOR_TUNE ;
              break ;

            case MENU_ITEM_MAIN_LOAD_CONFIGURATION:
              machineState_ = STATE_CONFIRM_LOAD ;
              break ;

            case MENU_ITEM_MAIN_SAVE_CONFIGURATION:
              machineState_ = STATE_CONFIRM_SAVE ;
              break ;

            case MENU_ITEM_MAIN_INFORMATION:
              machineState_ = STATE_INFORMATION ;
              break ;

            case MENU_ITEM_MAIN_RESET:
              machineState_ = STATE_CONFIRM_RESET ;
              break ;

          }
          break ;

        case STATE_MENU_TONE:
          machineState_ = ( menuStateTone_.select == MENU_ITEM_TONE_RETURN ) ? STATE_MENU_MAIN : STATE_ADJUST_TONE ;
          break ;

        case STATE_CONFIRM_LOAD:
          machineState_ = menuStateConfirm_.select ? STATE_LOAD : STATE_MENU_MAIN ;
          break ;

        case STATE_CONFIRM_SAVE:
          machineState_ = menuStateConfirm_.select ? STATE_SAVE : STATE_MENU_MAIN ;
          break ;

        case STATE_CONFIRM_RESET:
          machineState_ = menuStateConfirm_.select ? STATE_RESET : STATE_MENU_MAIN ;
          break ;

        case STATE_METRONOME:
          machineState_ = STATE_MENU_MAIN ;
          menuStateMain_.select = 0 ;
          menuStateMain_.cursorPosition = 0 ;
          break ;

        case STATE_ADJUST_BEAT_COUNT:
        case STATE_ADJUST_DURATION:
        case STATE_ADJUST_PULSE_WIDTH:
        case STATE_INFORMATION:
          machineState_ = STATE_MENU_MAIN ;
          break ;

        case STATE_ADJUST_OSCILLATOR_TUNE:
          SetEvent( outputEvent_.soundOff ) ;
          machineState_ = STATE_MENU_MAIN ;
          break ;

        case STATE_ADJUST_TONE:
          SetEvent( outputEvent_.soundOff ) ;
          machineState_ = STATE_MENU_TONE ;
          break ;

        case STATE_INITIALIZE:
        case STATE_ERROR:
          machineState_ = STATE_METRONOME ;
          SetEvent( outputEvent_.resetMetronome ) ;
          break ;

      }

    }

    // Change State ----------------
    if( EvalEvent( outputEvent_.changeState ) ) {

      SetEvent( outputEvent_.changeMessage ) ;

      switch( machineState_ ) {
        case STATE_METRONOME:
          break ;

        case STATE_MENU_MAIN:
          currentMenuStatePtr_ = &menuStateMain_ ;
          break ;

        case STATE_MENU_TONE:
          currentMenuStatePtr_ = &menuStateTone_ ;
          break ;

        case STATE_CONFIRM_LOAD:
        case STATE_CONFIRM_SAVE:
        case STATE_CONFIRM_RESET:
          menuStateConfirm_.select = 0 ;
          menuStateConfirm_.cursorPosition = 0 ;
          currentMenuStatePtr_ = &menuStateConfirm_ ;
          break ;

        case STATE_ADJUST_BEAT_COUNT:
          SetEvent( outputEvent_.changeValue ) ;
          currentValuePtr = &configration_.beatCount ;
          valueLimit_.upper = 64 ;
          valueLimit_.lower = 0 ;
          break ;

        case STATE_ADJUST_DURATION:
          SetEvent( outputEvent_.changeValue ) ;
          currentValuePtr = &configration_.duration ;
          valueLimit_.upper = 0xFF ;
          valueLimit_.lower = 0x00 ;
          break ;

        case STATE_ADJUST_PULSE_WIDTH:
          SetEvent( outputEvent_.changeValue ) ;
          currentValuePtr = &configration_.pulseWidth ;
          valueLimit_.upper = 0x07 ;
          valueLimit_.lower = 0x00 ;
          break ;

        case STATE_ADJUST_OSCILLATOR_TUNE:
          SetEvent( outputEvent_.changeValue ) ;
          currentValuePtr = ( uint08* ) & configration_.oscillatorTune ;
          valueLimit_.upper = (uint08)30 ;
          valueLimit_.lower = ( uint08 ) - 30 ;
          SetEvent( outputEvent_.soundOn ) ;
          break ;

        case STATE_ADJUST_TONE:
          SetEvent( outputEvent_.changeValue ) ;
          currentValuePtr = &configration_.tone[ menuStateTone_.select - MENU_ITEM_TONE_ADJUST_TONE0 ] ;
          valueLimit_.upper = 0xFF ;
          valueLimit_.lower = 0x00 ;
          SoundOn( ) ;
          break ;

        case STATE_INFORMATION:
          menuStateInformation_.select = 0 ;
          menuStateInformation_.cursorPosition = 0 ;
          currentMenuStatePtr_ = &menuStateInformation_ ;
          break ;

        case STATE_LOAD:
          SetEvent( outputEvent_.resetMetronome ) ;
          SetEvent( outputEvent_.accessEeprom ) ;
          stateReturnCounter_ = 0xFF ;
          break ;

        case STATE_SAVE:
          SetEvent( outputEvent_.accessEeprom ) ;
          stateReturnCounter_ = 0xFF ;
          break ;

        case STATE_RESET:
          _parallel_lcd_ClearRow( PARALLEL_LCD_ROW_SELECT_0 ) ;
          _parallel_lcd_ClearRow( PARALLEL_LCD_ROW_SELECT_1 ) ;
          RESET( ) ;

      }

    }

    // Apply Up/Down Key Event ----------------
    switch( machineState_ ) {

      case STATE_MENU_MAIN:
      case STATE_MENU_TONE:
      case STATE_CONFIRM_LOAD:
      case STATE_CONFIRM_SAVE:
      case STATE_CONFIRM_RESET:
      case STATE_INFORMATION:
        if( EvalEvent( inputEvent_.keyPressDown ) ) {
          if( currentMenuStatePtr_->select != currentMenuStatePtr_->limit ) {
            currentMenuStatePtr_->select++ ;
            if( !currentMenuStatePtr_->cursorPosition ) currentMenuStatePtr_->cursorPosition++ ;
            SetEvent( outputEvent_.changeMessage ) ;
          }
        }
        if( EvalEvent( inputEvent_.keyPressUp ) ) {
          if( currentMenuStatePtr_->select ) {
            currentMenuStatePtr_->select-- ;
            if( currentMenuStatePtr_->cursorPosition ) currentMenuStatePtr_->cursorPosition-- ;
            SetEvent( outputEvent_.changeMessage ) ;
          }
        }
        break ;

      case STATE_METRONOME:
        if( EvalEvent( inputEvent_.keyPressUp ) ) {
          if( configration_.tempo < 999 ) {
            configration_.tempo++ ;
            SetEvent( outputEvent_.changeValue ) ;
          }
          SetEvent( outputEvent_.resetMetronome ) ;
        }
        if( EvalEvent( inputEvent_.keyPressDown ) ) {
          if( configration_.tempo > 1 ) {
            configration_.tempo-- ;
            SetEvent( outputEvent_.changeValue ) ;
          }
          SetEvent( outputEvent_.resetMetronome ) ;
        }
        break ;

      case STATE_ADJUST_BEAT_COUNT:
      case STATE_ADJUST_PULSE_WIDTH:
      case STATE_ADJUST_DURATION:
      case STATE_ADJUST_TONE:
      case STATE_ADJUST_OSCILLATOR_TUNE:
        if( EvalEvent( inputEvent_.keyPressUp ) ) {
          if( *currentValuePtr != valueLimit_.upper ) {
            ( *currentValuePtr )++ ;
            SetEvent( outputEvent_.changeValue ) ;
          }
        }
        if( EvalEvent( inputEvent_.keyPressDown ) ) {
          if( *currentValuePtr != valueLimit_.lower ) {
            ( *currentValuePtr )-- ;
            SetEvent( outputEvent_.changeValue ) ;
          }
        }
        break ;

    }

    // Reset Metronome ----------------
    if( EvalEvent( outputEvent_.resetMetronome ) ) {
      DisableAllInterrupt( ) ;
      tempoCounter_ = 0 ;
      beatCounter_ = 0 ;
      duration_ = configration_.duration ;
      SetEvent( outputEvent_.soundClickOn ) ;
      EnableAllInterrupt( ) ;
    }

    // Sound On ----------------
    switch( machineState_ ) {

      case STATE_BOOT:
      case STATE_INITIALIZE:
      case STATE_LOAD:
      case STATE_SAVE:
      case STATE_ERROR:
        // Do Noting
        break ;

      case STATE_ADJUST_TONE:
        if( EvalEvent( outputEvent_.soundOn ) ) {
          SetSoundTimerPeriod( *currentValuePtr ) ;
          SetSoundPulseWidth( 1 ) ;
          SoundOn( ) ;
        }
        break ;
      case STATE_ADJUST_OSCILLATOR_TUNE:
        if( EvalEvent( outputEvent_.soundOn ) ) {
          SetSoundTimerPeriod( TONE_TUNE ) ;
          SetSoundPulseWidth( 1 ) ;
          SoundOn( ) ;
        }
        break ;

      default:
        if( EvalEvent( outputEvent_.soundClickOn ) ) {
          if( !isMute_ ) {
            if( beatCounter_ == 0 )
              SetSoundTimerPeriod( configration_.tone[ 1 ] ) ;
            else if( beatCounter_ == configration_.beatCount )
              SetSoundTimerPeriod( configration_.tone[ 2 ] ) ;
            else
              SetSoundTimerPeriod( configration_.tone[ 0 ] ) ;

            SetSoundPulseWidth( configration_.pulseWidth ) ;
            SoundOn( ) ;
          }
        }

        break ;
    }

    // Sound Off ----------------
    switch( machineState_ ) {

      case STATE_BOOT:
      case STATE_INITIALIZE:
      case STATE_LOAD:
      case STATE_SAVE:
      case STATE_ERROR:
        SoundOff( ) ;
        break ;

      case STATE_ADJUST_TONE:
      case STATE_ADJUST_OSCILLATOR_TUNE:
        // Do Nothing
        break ;

      default:
        if( EvalEvent( outputEvent_.soundOff ) )
          SoundOff( ) ;

        break ;
    }

    // Message ----------------
    if( EvalEvent( outputEvent_.changeMessage ) ) {

      switch( machineState_ ) {

        case STATE_MENU_MAIN:
        case STATE_MENU_TONE:
          _parallel_lcd_WriteStringClearing( PARALLEL_LCD_ROW_SELECT_0 | 0x1 , currentMenuStatePtr_->menuMessage[ currentMenuStatePtr_->select - currentMenuStatePtr_->cursorPosition ] ) ;
          _parallel_lcd_WriteStringClearing( PARALLEL_LCD_ROW_SELECT_1 | 0x1 , currentMenuStatePtr_->menuMessage[ currentMenuStatePtr_->select - currentMenuStatePtr_->cursorPosition + 1 ] ) ;

          if( currentMenuStatePtr_->select != currentMenuStatePtr_->cursorPosition )
            _parallel_lcd_WriteCharacter( PARALLEL_LCD_ROW_SELECT_0 | 0xF , CHAR_CODE.CURSOR_UP ) ;
          if( currentMenuStatePtr_->select != ( currentMenuStatePtr_->limit + currentMenuStatePtr_->cursorPosition - 1 ) )
            _parallel_lcd_WriteCharacter( PARALLEL_LCD_ROW_SELECT_1 | 0xF , CHAR_CODE.CURSOR_DOWN ) ;

          _parallel_lcd_WriteCharacter( PARALLEL_LCD_ROW_SELECT[currentMenuStatePtr_->cursorPosition] | 0x0 , CHAR_CODE.CURSOR_RIGHT ) ;

          break ;

        case STATE_CONFIRM_LOAD:
        case STATE_CONFIRM_SAVE:
        case STATE_CONFIRM_RESET:

          _parallel_lcd_WriteStringClearing( PARALLEL_LCD_ROW_SELECT_0 | 0xD , MESSAGE.CONFIRM.NO ) ;
          _parallel_lcd_WriteStringClearing( PARALLEL_LCD_ROW_SELECT_1 | 0xD , MESSAGE.CONFIRM.YES ) ;

          switch( machineState_ ) {
            case STATE_CONFIRM_LOAD:
              _parallel_lcd_WriteString( PARALLEL_LCD_ROW_SELECT_0 | 0x0 , MESSAGE.CONFIRM.LOAD ) ;
              break ;
            case STATE_CONFIRM_SAVE:
              _parallel_lcd_WriteString( PARALLEL_LCD_ROW_SELECT_0 | 0x0 , MESSAGE.CONFIRM.SAVE ) ;
              break ;
            case STATE_CONFIRM_RESET:
              _parallel_lcd_WriteString( PARALLEL_LCD_ROW_SELECT_0 | 0x0 , MESSAGE.CONFIRM.RESET ) ;
              break ;

          }

          _parallel_lcd_WriteCharacter( PARALLEL_LCD_ROW_SELECT[ currentMenuStatePtr_->cursorPosition ] | 0xC , CHAR_CODE.CURSOR_RIGHT ) ;

          break ;

        case STATE_METRONOME:
          _parallel_lcd_WriteStringClearing( PARALLEL_LCD_ROW_SELECT_0 | 0x0 , MESSAGE.METRONOME.TILE ) ;
          _parallel_lcd_WriteStringClearing( PARALLEL_LCD_ROW_SELECT_1 | 0x0 , MESSAGE.METRONOME.TEMPO ) ;
          if( isMute_ ) _parallel_lcd_WriteString( PARALLEL_LCD_ROW_SELECT_0 | 0xA , MESSAGE.METRONOME.MUTE ) ;
          SetEvent( outputEvent_.changeValue ) ;
          break ;

        case STATE_ADJUST_BEAT_COUNT:
          _parallel_lcd_WriteStringClearing( PARALLEL_LCD_ROW_SELECT_0 | 0x0 , MESSAGE.CONFIGURATION.TITLE ) ;
          _parallel_lcd_WriteStringClearing( PARALLEL_LCD_ROW_SELECT_1 | 0x0 , MESSAGE.CONFIGURATION.BEAT_COUNT ) ;
          SetEvent( outputEvent_.changeValue ) ;
          break ;

        case STATE_ADJUST_DURATION:
          _parallel_lcd_WriteStringClearing( PARALLEL_LCD_ROW_SELECT_0 | 0x0 , MESSAGE.CONFIGURATION.TITLE ) ;
          _parallel_lcd_WriteStringClearing( PARALLEL_LCD_ROW_SELECT_1 | 0x0 , MESSAGE.CONFIGURATION.DURATION ) ;
          SetEvent( outputEvent_.changeValue ) ;
          break ;

        case STATE_ADJUST_PULSE_WIDTH:
          _parallel_lcd_WriteStringClearing( PARALLEL_LCD_ROW_SELECT_0 | 0x0 , MESSAGE.CONFIGURATION.TITLE ) ;
          _parallel_lcd_WriteStringClearing( PARALLEL_LCD_ROW_SELECT_1 | 0x0 , MESSAGE.CONFIGURATION.PULSE_WIDTH ) ;
          SetEvent( outputEvent_.changeValue ) ;
          break ;

        case STATE_ADJUST_OSCILLATOR_TUNE:
          _parallel_lcd_WriteStringClearing( PARALLEL_LCD_ROW_SELECT_0 | 0x0 , MESSAGE.CONFIGURATION.TITLE ) ;
          _parallel_lcd_WriteStringClearing( PARALLEL_LCD_ROW_SELECT_1 | 0x0 , MESSAGE.CONFIGURATION.OSCILLATOR_TUNE ) ;
          SetEvent( outputEvent_.changeValue ) ;
          break ;

        case STATE_ADJUST_TONE:
          _parallel_lcd_WriteStringClearing( PARALLEL_LCD_ROW_SELECT_0 | 0x0 , MESSAGE.CONFIGURATION.TITLE ) ;
          _parallel_lcd_WriteStringClearing( PARALLEL_LCD_ROW_SELECT_1 | 0x0 , MESSAGE.CONFIGURATION.TONE ) ;
          _parallel_lcd_WriteCharacter( PARALLEL_LCD_ROW_SELECT_1 | 0x5 , menuStateTone_.select - MENU_ITEM_TONE_ADJUST_TONE0 + '0' ) ;
          SetEvent( outputEvent_.changeValue ) ;
          break ;

        case STATE_INFORMATION:
          _parallel_lcd_WriteStringClearing( PARALLEL_LCD_ROW_SELECT_0 | 0x0 , MESSAGE_INFORMATION[ menuStateInformation_.select ] ) ;
          _parallel_lcd_WriteString( PARALLEL_LCD_ROW_SELECT_0 | 0xA , &informationValueBuffer[ menuStateInformation_.select ] ) ;
          _parallel_lcd_WriteStringClearing( PARALLEL_LCD_ROW_SELECT_1 | 0x0 , MESSAGE_INFORMATION[ menuStateInformation_.select + 1 ] ) ;
          _parallel_lcd_WriteString( PARALLEL_LCD_ROW_SELECT_1 | 0xA , &informationValueBuffer[ menuStateInformation_.select + 1 ] ) ;
          break ;

        case STATE_LOAD:
          _parallel_lcd_WriteStringClearing( PARALLEL_LCD_ROW_SELECT_0 | 0x0 , MESSAGE.MEMORY.LOAD ) ;
          _parallel_lcd_ClearRow( PARALLEL_LCD_ROW_SELECT_1 ) ;
          break ;

        case STATE_SAVE:
          _parallel_lcd_WriteStringClearing( PARALLEL_LCD_ROW_SELECT_0 | 0x0 , MESSAGE.MEMORY.SAVE ) ;
          _parallel_lcd_ClearRow( PARALLEL_LCD_ROW_SELECT_1 ) ;
          break ;

        case STATE_INITIALIZE:
          _parallel_lcd_WriteStringClearing( PARALLEL_LCD_ROW_SELECT_0 | 0x0 , MESSAGE.MEMORY.LOAD_DEFAULT ) ;
          _parallel_lcd_ClearRow( PARALLEL_LCD_ROW_SELECT_1 ) ;
          break ;

        case STATE_ERROR:
          _parallel_lcd_WriteStringClearing( PARALLEL_LCD_ROW_SELECT_0 | 0x0 , MESSAGE.ERROR.MESSAGE ) ;
          _parallel_lcd_ClearRow( PARALLEL_LCD_ROW_SELECT_1 ) ;
          break ;
      }

    }

    // Display Value ----------------
    if( EvalEvent( outputEvent_.changeValue ) ) {

      uint16 tmpValue ;
      char valueString[6] = "= 000" ;

      switch( machineState_ ) {

        case STATE_METRONOME:
          tmpValue = configration_.tempo ;
          break ;

        case STATE_ADJUST_OSCILLATOR_TUNE:
          if( (uint08)configration_.oscillatorTune & 0x80 ) {
            tmpValue = -configration_.oscillatorTune ;
            valueString[1] = '-' ;
          }
          else {
            tmpValue = (uint16)configration_.oscillatorTune ;
          }
          break ;

        default:
          tmpValue = *currentValuePtr ;
          break ;
      }

      uint08 isNonZero = BOOL_FALSE ;
      for( uint08 i = 2 ; i != 5 ; i++ ) {
        uint08 compareUnit ;
        switch( i ) {
          case 2: compareUnit = 100 ;
            break ;
          case 3: compareUnit = 10 ;
            break ;
          case 4: compareUnit = 1 ;
            break ;
        }
        while( tmpValue >= compareUnit ) {
          tmpValue -= compareUnit ;
          valueString[i]++ ;
          isNonZero = BOOL_TRUE ;
        }

        if( i == 4 ) break ;

        if( isNonZero ) continue ;

        valueString[i] = valueString[ i - 1 ] ;
        valueString[ i - 1 ] = ' ' ;

      }

      _parallel_lcd_WriteString( PARALLEL_LCD_ROW_SELECT_1 | 0xB , &valueString ) ;

      switch( machineState_ ) {
        case STATE_ADJUST_OSCILLATOR_TUNE:
          SetOscillatorTune( configration_.oscillatorTune ) ;
          break ;
        case STATE_ADJUST_TONE:
          SetSoundTimerPeriod( *currentValuePtr ) ;
          SetSoundPulseWidth( 1 ) ;
          break ;
      }
    }

  } //for(;;)

}//main()

// ----------------------------------------------------------------
// ISR Interrupt Flag
#define ISR_FLAG PIR3bits.TMR6IF

// --------------------------------------------------------------------------------------------------------------------------------
// [Interrupt Service Routine]
void interrupt isr( void ) {
  if( !ISR_FLAG ) return ;
  ISR_FLAG = 0 ;

  static uint16 eventPrescaler = 0 ;

  // Metronome Count ----------------
  tempoCounter_ += configration_.tempo ;
  if( tempoCounter_ >= TOTAL_TEMOPO_COUNT ) {
    tempoCounter_ -= TOTAL_TEMOPO_COUNT ;
    duration_ = configration_.duration ;
    if( ++beatCounter_ >= ( configration_.beatCount << 1 ) )
      beatCounter_ = 0 ;

    if( !keyBeepCounter_ )
      SetEvent( outputEvent_.soundClickOn ) ;
  }

  // Check Sound Duration ----------------
  if( !( eventPrescaler & 0x3F ) ) {
    if( duration_ && !--duration_ && !keyBeepCounter_ )
      SetEvent( outputEvent_.soundOff ) ;
    if( keyBeepCounter_ && ! --keyBeepCounter_ )
      SetEvent( outputEvent_.soundOff ) ;
  }

  // Prescale ----------------
  if( ++eventPrescaler != 640 ) return ;
  eventPrescaler = 0 ;

  // Count for State Return ----------------
  if( stateReturnCounter_ ) {
    if( --stateReturnCounter_ ) return ;

    SetEvent( outputEvent_.changeState ) ;
    SetEvent( outputEvent_.resetMetronome ) ;
    machineState_ = STATE_METRONOME ;
  }

  // Read Key State ----------------
  portAState_.all = ReadKeyState( ) ;

  if( portAState_.keyUp && !portAState_.keyDown ) {
    if( ++keyCount_.Up == KEY_COUNT_LOOP_END ) {
      keyCount_.Up = KEY_COUNT_LOOP_START ;
      SetEvent( inputEvent_.keyPressHeldUp ) ;
    }
  }
  else
    keyCount_.Up = 0 ;

  if( portAState_.keyDown && !portAState_.keyUp ) {
    if( ++keyCount_.Down == KEY_COUNT_LOOP_END ) {
      keyCount_.Down = KEY_COUNT_LOOP_START ;
      SetEvent( inputEvent_.keyPressHeldDown ) ;
    }
  }
  else
    keyCount_.Down = 0 ;

  if( ISR_FLAG ) RESET( ) ;

}//isr

