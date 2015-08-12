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
#include <xc.h>
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
  STATE_MENU_DURATION ,
  STATE_ADJUST_BEAT_COUNT ,
  STATE_ADJUST_DURATION_CLICK ,
  STATE_ADJUST_DURATION_KEY ,
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
} EnMachineState ;
EnMachineState machineState_ = STATE_BOOT ;
uint08_t stateReturnCounter_ = 0 ;
Bool_t isMute_ = BOOL_FALSE ;
#define RETURN_STATE_INTERVAL 0x40

// ----------------------------------------------------------------
// Configuration
ConfigurationData configration_ = CONFIG_DEFAULT ;

// ----------------------------------------------------------------
// Metronome Counter
#define _XTAL_FREQ 32000000UL
#define TOTAL_TEMOPO_COUNT ( _XTAL_FREQ * 3 / 25 )
#if TOTAL_TEMOPO_COUNT > 0xFFFFFF
#  error [ERROR] Too Large Tempo Count !!
#endif
uint24_t tempoCounter_ = 0 ;
uint08_t beatCounter_ = 0 ;

// ----------------------------------------------------------------
// Keys
typedef union {
  uint08_t byte ;
  struct {
    unsigned : 5 ;
    unsigned keyMenu : 1 ;
    unsigned keyDown : 1 ;
    unsigned keyUp : 1 ;
  } ;
} UniPortAState ;
#define ReadKeyState() (~PORTA&0xE0)
UniPortAState portAState_ = { 0x00 } ;

// ----------------------------------------------------------------
// Key Count
#define KEY_COUNT_LOOP_START 0x3C
#define KEY_COUNT_LOOP_END 0x40
struct {
  uint08_t Up ;
  uint08_t Down ;
} keyHoldCount_ = { 0 , 0 } ;

// ----------------------------------------------------------------
// Menu State
typedef struct {
  uint08_t select ;
  uint08_t cursorPosition ;
  const uint08_t limit ;
  const char** menuMessage ;
} StMenuInfo ;
StMenuInfo menuInfoMain_ = { 0 , 0 , MENU_SIZE_MAIN - 1 , &MESSAGE_MENU_ITEM_MAIN } ;
StMenuInfo menuInfoTone_ = { 0 , 0 , MENU_SIZE_TONE - 1 , &MESSAGE_MENU_ITEM_TONE } ;
StMenuInfo menuInfoDuration_ = { 0 , 0 , MENU_SIZE_DURATION - 1 , &MESSAGE_MENU_ITEM_DURATION } ;
StMenuInfo menuInfoConfirm_ = { 0 , 0 , MENU_SIZE_CONFIRM - 1 , NULL } ;
StMenuInfo menuInfoInformation_ = { 0 , 0 , MENU_SIZE_INFORMATION - 2 , NULL } ;
StMenuInfo* currentMenuInfoPtr_ = NULL ;

// ----------------------------------------------------------------
// Value
typedef struct {
  uint08_t* valuePtr ;
  uint08_t limitUpper ;
  uint08_t limitLower ;
  const char* messageTitle ;
  const char* messageValue ;
} StValueInfo ;
StValueInfo currentValueInfo_ = { NULL , NULL } ;

// ----------------------------------------------------------------
// Events
#define ClearEvent( event ) event = 0
#define SetEvent( event )   event = 1
#define EvalEvent( event )  (event&&!(event=0))
union {
  uint08_t all ;
  struct {
    unsigned keyPressUp : 1 ;
    unsigned keyPressDown : 1 ;
    unsigned keyPressMenu : 1 ;
    unsigned keyPressUpDown : 1 ;
    unsigned keyPressHeldUp : 1 ;
    unsigned keyPressHeldDown : 1 ;
  } ;
} inputEvent_ = { 0x00 } ;
union {
  uint08_t all ;
  struct {
    unsigned changeState : 1 ;
    unsigned changeMessage : 1 ;
    unsigned changeValue : 1 ;
    unsigned soundOnClick : 1 ;
    unsigned soundOnKey : 1 ;
    unsigned soundOff : 1 ;
    unsigned resetMetronome : 1 ;
    unsigned accessEeprom : 1 ;
  } ;
} outputEvent_ = { 0x00 } ;

// ----------------------------------------------------------------
// Tone
#define TONE_OFF 0
#define TONE_SYSTEM 124
#define TONE_TUNE 141 // A = 880 Hz
struct {
  uint08_t click ;
  uint08_t key ;
} soundDurationCount_ = { 0 , 0 } ;

// --------------------------------------------------------------------------------------------------------------------------------
// [Function] Main
void main( void ) {

  // Initialize
  initialize( ) ;

  // Read or Write Configuration from EEPROM
  if( ReadKeyState( ) == 0xC0 )
    machineState_ = STATE_INITIALIZE ;
  else
    machineState_ = STATE_BOOT ;
  SetEvent( outputEvent_.changeState ) ;

  // Boot Beep
  SetSoundTimerPeriod( TONE_SYSTEM ) ;
  SetSoundPulseWidth( 1 ) ;

  // Timer for LCD Command Wait
  T4CONbits.TMR4ON = 1 ;

  // Timer for Boot Sequence
  T1CONbits.TMR1ON = 1 ;

  // Execute Boot Sequence
  for( uint08_t phase = 0 ; phase < 0xE ; phase++ ) {

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
        uint08_t userId ;
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

    keyPressed.byte = ( portAState_.byte ^ prevPortAState.byte ) & portAState_.byte ;
    prevPortAState.byte = portAState_.byte ;

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
      SetEvent( outputEvent_.soundOnKey ) ;
    }

    // EEPROM ----------------
    if( EvalEvent( outputEvent_.accessEeprom ) ) {

      DisableAllInterrupt( ) ;

      ReturnCode returnCode = RETURN_CODE_NOERROR ;

      switch( machineState_ ) {

        case STATE_BOOT:
          machineState_ = STATE_METRONOME ;
          SetEvent( outputEvent_.changeState ) ;
          SetEvent( outputEvent_.resetMetronome ) ;
        case STATE_LOAD:
          returnCode = _configuration_Load( &configration_ ) ;
          break ;

        case STATE_INITIALIZE:
        case STATE_SAVE:
          returnCode = _configuration_Save( &configration_ ) ;
          break ;

      }

      uint08_t romOffset = _configuration_GetRomOffset( ) ;
      informationValueBuffer[ INFORMATION_ITEM_ROM_OFFSET ][3] = HEX_TABLE[ romOffset >> 4 ] ;
      informationValueBuffer[ INFORMATION_ITEM_ROM_OFFSET ][4] = HEX_TABLE[ romOffset & 0x0F ] ;
      informationValueBuffer[ INFORMATION_ITEM_WRITE_COUNT ][3] = HEX_TABLE[ configration_.writeCount >> 4 ] ;
      informationValueBuffer[ INFORMATION_ITEM_WRITE_COUNT ][4] = HEX_TABLE[ configration_.writeCount & 0x0F ] ;
      informationValueBuffer[ INFORMATION_ITEM_ERROR_CODE ][3] = HEX_TABLE[ returnCode >> 4 ] ;
      informationValueBuffer[ INFORMATION_ITEM_ERROR_CODE ][4] = HEX_TABLE[ returnCode & 0x0F ] ;

      if( returnCode ) {
        machineState_ = STATE_ERROR ;
        outputEvent_.all = 0x00 ;
        SetEvent( outputEvent_.changeState ) ;
      }

      EnableAllInterrupt( ) ;

    }

    // Both Up & Donw ----------------
    if( EvalEvent( inputEvent_.keyPressUpDown ) ) {
      if( machineState_ == STATE_METRONOME ) {
        ToggleBool( isMute_ ) ;
        SetEvent( outputEvent_.changeMessage ) ;
      }
    }

    // Apply Menu Key Event (Action Before State Change) ----------------
    if( EvalEvent( inputEvent_.keyPressMenu ) ) {
      SetEvent( outputEvent_.changeState ) ;

      switch( machineState_ ) {

        case STATE_MENU_MAIN:
          switch( menuInfoMain_.select ) {
            case MENU_ITEM_MAIN_BEAT_COUNT:
              machineState_ = STATE_ADJUST_BEAT_COUNT ;
              break ;

            case MENU_ITEM_MAIN_TONE_MENU:
              machineState_ = STATE_MENU_TONE ;
              menuInfoTone_.select = 0 ;
              menuInfoTone_.cursorPosition = 0 ;
              break ;

            case MENU_ITEM_MAIN_ADJUST_DURATION:
              machineState_ = STATE_MENU_DURATION ;
              menuInfoDuration_.select = 0 ;
              menuInfoDuration_.cursorPosition = 0 ;
              break ;

            case MENU_ITEM_MAIN_PULSE_WIDTH:
              machineState_ = STATE_ADJUST_PULSE_WIDTH ;
              break ;

            case MENU_ITEM_MAIN_ADJUST_OSCILLATOR_TUNE:
              machineState_ = STATE_ADJUST_OSCILLATOR_TUNE ;
              break ;

            case MENU_ITEM_MAIN_LOAD_CONFIGURATION:
              machineState_ = STATE_CONFIRM_LOAD ;
              currentValueInfo_.messageTitle = MESSAGE.CONFIRM.LOAD ;
              break ;

            case MENU_ITEM_MAIN_SAVE_CONFIGURATION:
              machineState_ = STATE_CONFIRM_SAVE ;
              currentValueInfo_.messageTitle = MESSAGE.CONFIRM.SAVE ;
              break ;

            case MENU_ITEM_MAIN_INFORMATION:
              machineState_ = STATE_INFORMATION ;
              break ;

            case MENU_ITEM_MAIN_RESET:
              machineState_ = STATE_CONFIRM_RESET ;
              currentValueInfo_.messageTitle = MESSAGE.CONFIRM.RESET ;
              break ;

            case MENU_ITEM_MAIN_RETURN:
            default:
              machineState_ = STATE_METRONOME ;
              break ;

          }
          break ;

        case STATE_MENU_TONE:
          if( menuInfoTone_.select == MENU_ITEM_TONE_RETURN )
            machineState_ = STATE_MENU_MAIN ;
          else
            machineState_ = STATE_ADJUST_TONE ;
          break ;

        case STATE_MENU_DURATION:
          switch( currentMenuInfoPtr_->select ) {
            case MENU_ITEM_DURATION_ADJUST_CLICK:
              machineState_ = STATE_ADJUST_DURATION_CLICK ;
              break ;

            case MENU_ITEM_DURATION_ADJUST_KEY:
              machineState_ = STATE_ADJUST_DURATION_KEY ;
              break ;

            case MENU_ITEM_DURATION_RETURN:
            default:
              machineState_ = STATE_MENU_MAIN ;
              break ;

          }
          break ;

        case STATE_CONFIRM_LOAD:
          if( menuInfoConfirm_.select )
            machineState_ = STATE_LOAD ;
          else
            machineState_ = STATE_MENU_MAIN ;
          break ;

        case STATE_CONFIRM_SAVE:
          if( menuInfoConfirm_.select )
            machineState_ = STATE_SAVE ;
          else
            machineState_ = STATE_MENU_MAIN ;
          break ;

        case STATE_CONFIRM_RESET:
          if( menuInfoConfirm_.select )
            machineState_ = STATE_RESET ;
          else
            machineState_ = STATE_MENU_MAIN ;
          break ;

        case STATE_METRONOME:
          machineState_ = STATE_MENU_MAIN ;
          menuInfoMain_.select = 0 ;
          menuInfoMain_.cursorPosition = 0 ;
          break ;

        case STATE_ADJUST_BEAT_COUNT:
        case STATE_ADJUST_PULSE_WIDTH:
        case STATE_INFORMATION:
          machineState_ = STATE_MENU_MAIN ;
          break ;

        case STATE_ADJUST_DURATION_CLICK:
        case STATE_ADJUST_DURATION_KEY:
          machineState_ = STATE_MENU_DURATION ;
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

    // Change State (Action After State Change)----------------
    if( EvalEvent( outputEvent_.changeState ) ) {

      SetEvent( outputEvent_.changeMessage ) ;

      switch( machineState_ ) {
        case STATE_METRONOME:
          currentValueInfo_.messageTitle = MESSAGE.METRONOME.TILE ;
          currentValueInfo_.messageValue = MESSAGE.METRONOME.TEMPO ;
          break ;

        case STATE_MENU_MAIN:
          currentMenuInfoPtr_ = &menuInfoMain_ ;
          break ;

        case STATE_MENU_TONE:
          currentMenuInfoPtr_ = &menuInfoTone_ ;
          break ;

        case STATE_MENU_DURATION:
          currentMenuInfoPtr_ = &menuInfoDuration_ ;
          break ;

        case STATE_CONFIRM_LOAD:
        case STATE_CONFIRM_SAVE:
        case STATE_CONFIRM_RESET:
          menuInfoConfirm_.select = 0 ;
          menuInfoConfirm_.cursorPosition = 0 ;
          currentMenuInfoPtr_ = &menuInfoConfirm_ ;
          break ;

        case STATE_ADJUST_BEAT_COUNT:
          SetEvent( outputEvent_.changeValue ) ;
          currentValueInfo_.valuePtr = &configration_.beatCount ;
          currentValueInfo_.limitUpper = 64 ;
          currentValueInfo_.limitLower = 0 ;
          currentValueInfo_.messageTitle = MESSAGE.CONFIGURATION.TITLE ;
          currentValueInfo_.messageValue = MESSAGE.CONFIGURATION.BEAT_COUNT ;
          break ;

        case STATE_ADJUST_DURATION_CLICK:
          SetEvent( outputEvent_.changeValue ) ;
          currentValueInfo_.valuePtr = &configration_.duration.click ;
          currentValueInfo_.limitUpper = 0xFF ;
          currentValueInfo_.limitLower = 0x00 ;
          currentValueInfo_.messageTitle = MESSAGE.CONFIGURATION.TITLE_DURATION ;
          currentValueInfo_.messageValue = MESSAGE.CONFIGURATION.DURATION_CLICK ;
          break ;

        case STATE_ADJUST_DURATION_KEY:
          SetEvent( outputEvent_.changeValue ) ;
          currentValueInfo_.valuePtr = &configration_.duration.key ;
          currentValueInfo_.limitUpper = 0xFF ;
          currentValueInfo_.limitLower = 0x00 ;
          currentValueInfo_.messageTitle = MESSAGE.CONFIGURATION.TITLE_DURATION ;
          currentValueInfo_.messageValue = MESSAGE.CONFIGURATION.DURATION_KEY ;
          break ;

        case STATE_ADJUST_PULSE_WIDTH:
          SetEvent( outputEvent_.changeValue ) ;
          currentValueInfo_.valuePtr = &configration_.pulseWidth ;
          currentValueInfo_.limitUpper = 0x07 ;
          currentValueInfo_.limitLower = 0x00 ;
          currentValueInfo_.messageTitle = MESSAGE.CONFIGURATION.TITLE ;
          currentValueInfo_.messageValue = MESSAGE.CONFIGURATION.PULSE_WIDTH ;
          break ;

        case STATE_ADJUST_OSCILLATOR_TUNE:
          SetEvent( outputEvent_.changeValue ) ;
          currentValueInfo_.valuePtr = ( uint08_t* ) & configration_.oscillatorTune ;
          currentValueInfo_.limitUpper = (uint08_t)30 ;
          currentValueInfo_.limitLower = (uint08_t)30 ;
          currentValueInfo_.messageTitle = MESSAGE.CONFIGURATION.TITLE ;
          currentValueInfo_.messageValue = MESSAGE.CONFIGURATION.OSCILLATOR_TUNE ;
          SetSoundTimerPeriod( TONE_TUNE ) ;
          SetSoundPulseWidth( 1 ) ;
          SoundOn( ) ;
          break ;

        case STATE_ADJUST_TONE:
          SetEvent( outputEvent_.changeValue ) ;
          currentValueInfo_.valuePtr = &configration_.tone[ menuInfoTone_.select - MENU_ITEM_TONE_ADJUST_TONE0 ] ;
          currentValueInfo_.limitUpper = 0xFF ;
          currentValueInfo_.limitLower = 0x00 ;
          currentValueInfo_.messageTitle = MESSAGE.CONFIGURATION.TITLE_TONE ;
          currentValueInfo_.messageValue = MESSAGE.CONFIGURATION.TONE ;
          SoundOn( ) ;
          break ;

        case STATE_INFORMATION:
          menuInfoInformation_.select = 0 ;
          menuInfoInformation_.cursorPosition = 0 ;
          currentMenuInfoPtr_ = &menuInfoInformation_ ;
          break ;

        case STATE_LOAD:
          SetEvent( outputEvent_.resetMetronome ) ;
          SetEvent( outputEvent_.accessEeprom ) ;
          currentValueInfo_.messageTitle = MESSAGE.MEMORY.LOAD ;
          stateReturnCounter_ = RETURN_STATE_INTERVAL ;
          break ;

        case STATE_SAVE:
          SetEvent( outputEvent_.accessEeprom ) ;
          currentValueInfo_.messageTitle = MESSAGE.MEMORY.SAVE ;
          stateReturnCounter_ = RETURN_STATE_INTERVAL ;
          break ;

        case STATE_BOOT:
          SetEvent( outputEvent_.accessEeprom ) ;
          break ;

        case STATE_INITIALIZE:
          SetEvent( outputEvent_.accessEeprom ) ;
          currentValueInfo_.messageTitle = MESSAGE.MEMORY.INITIALIZE ;
          break ;

        case STATE_RESET:
          _parallel_lcd_ClearRow( PARALLEL_LCD_ROW_SELECT_0 ) ;
          _parallel_lcd_ClearRow( PARALLEL_LCD_ROW_SELECT_1 ) ;
          RESET( ) ;

        case STATE_ERROR:
          currentValueInfo_.messageTitle = MESSAGE.ERROR.MESSAGE ;
          break ;

      }

    }

    // Apply Up/Down Key Event ----------------
    switch( machineState_ ) {

      case STATE_MENU_MAIN:
      case STATE_MENU_TONE:
      case STATE_MENU_DURATION:
      case STATE_CONFIRM_LOAD:
      case STATE_CONFIRM_SAVE:
      case STATE_CONFIRM_RESET:
      case STATE_INFORMATION:
        if( EvalEvent( inputEvent_.keyPressDown ) ) {
          if( currentMenuInfoPtr_->select != currentMenuInfoPtr_->limit ) {
            currentMenuInfoPtr_->select++ ;
            if( !currentMenuInfoPtr_->cursorPosition ) currentMenuInfoPtr_->cursorPosition++ ;
            SetEvent( outputEvent_.changeMessage ) ;
          }
        }
        if( EvalEvent( inputEvent_.keyPressUp ) ) {
          if( currentMenuInfoPtr_->select ) {
            currentMenuInfoPtr_->select-- ;
            if( currentMenuInfoPtr_->cursorPosition ) currentMenuInfoPtr_->cursorPosition-- ;
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
      case STATE_ADJUST_DURATION_CLICK:
      case STATE_ADJUST_DURATION_KEY:
      case STATE_ADJUST_TONE:
      case STATE_ADJUST_OSCILLATOR_TUNE:
        if( EvalEvent( inputEvent_.keyPressUp ) ) {
          if( *currentValueInfo_.valuePtr != currentValueInfo_.limitUpper ) {
            ( *currentValueInfo_.valuePtr )++ ;
            SetEvent( outputEvent_.changeValue ) ;
          }
        }
        if( EvalEvent( inputEvent_.keyPressDown ) ) {
          if( *currentValueInfo_.valuePtr != currentValueInfo_.limitLower ) {
            ( *currentValueInfo_.valuePtr )++ ;
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
      SetEvent( outputEvent_.soundOnClick ) ;
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
        break ;
      case STATE_ADJUST_OSCILLATOR_TUNE:
        break ;

      default:
        if( EvalEvent( outputEvent_.soundOnKey ) ) {
          soundDurationCount_.key = configration_.duration.key ;
          SetSoundTimerPeriod( TONE_SYSTEM ) ;
          SetSoundPulseWidth( 1 ) ;
          SoundOn( ) ;
        }
        if( EvalEvent( outputEvent_.soundOnClick ) && !isMute_ && !soundDurationCount_.key ) {

          soundDurationCount_.click = configration_.duration.click ;

          if( beatCounter_ == 0 )
            SetSoundTimerPeriod( configration_.tone[ 1 ] ) ;
          else if( beatCounter_ == configration_.beatCount )
            SetSoundTimerPeriod( configration_.tone[ 2 ] ) ;
          else
            SetSoundTimerPeriod( configration_.tone[ 0 ] ) ;

          SetSoundPulseWidth( configration_.pulseWidth ) ;
          SoundOn( ) ;
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
        case STATE_MENU_DURATION:
          _parallel_lcd_WriteStringClearing( PARALLEL_LCD_ROW_SELECT_0 | 0x1 , currentMenuInfoPtr_->menuMessage[ currentMenuInfoPtr_->select - currentMenuInfoPtr_->cursorPosition ] ) ;
          _parallel_lcd_WriteStringClearing( PARALLEL_LCD_ROW_SELECT_1 | 0x1 , currentMenuInfoPtr_->menuMessage[ currentMenuInfoPtr_->select - currentMenuInfoPtr_->cursorPosition + 1 ] ) ;

          if( currentMenuInfoPtr_->select != currentMenuInfoPtr_->cursorPosition )
            _parallel_lcd_WriteCharacter( PARALLEL_LCD_ROW_SELECT_0 | 0xF , CHAR_CODE.CURSOR_UP ) ;
          if( currentMenuInfoPtr_->select != ( currentMenuInfoPtr_->limit + currentMenuInfoPtr_->cursorPosition - 1 ) )
            _parallel_lcd_WriteCharacter( PARALLEL_LCD_ROW_SELECT_1 | 0xF , CHAR_CODE.CURSOR_DOWN ) ;

          _parallel_lcd_WriteCharacter( PARALLEL_LCD_ROW_SELECT[currentMenuInfoPtr_->cursorPosition] | 0x0 , CHAR_CODE.CURSOR_RIGHT ) ;

          break ;

        case STATE_CONFIRM_LOAD:
        case STATE_CONFIRM_SAVE:
        case STATE_CONFIRM_RESET:
          _parallel_lcd_WriteStringClearing( PARALLEL_LCD_ROW_SELECT_0 | 0xD , MESSAGE.CONFIRM.NO ) ;
          _parallel_lcd_WriteStringClearing( PARALLEL_LCD_ROW_SELECT_1 | 0xD , MESSAGE.CONFIRM.YES ) ;
          _parallel_lcd_WriteString( PARALLEL_LCD_ROW_SELECT_0 | 0x0 , currentValueInfo_.messageTitle ) ;
          _parallel_lcd_WriteCharacter( PARALLEL_LCD_ROW_SELECT[ currentMenuInfoPtr_->cursorPosition ] | 0xC , CHAR_CODE.CURSOR_RIGHT ) ;

          break ;

        case STATE_METRONOME:
        case STATE_ADJUST_BEAT_COUNT:
        case STATE_ADJUST_TONE:
        case STATE_ADJUST_DURATION_CLICK:
        case STATE_ADJUST_DURATION_KEY:
        case STATE_ADJUST_PULSE_WIDTH:
        case STATE_ADJUST_OSCILLATOR_TUNE:
          _parallel_lcd_WriteStringClearing( PARALLEL_LCD_ROW_SELECT_0 | 0x0 , currentValueInfo_.messageTitle ) ;
          _parallel_lcd_WriteStringClearing( PARALLEL_LCD_ROW_SELECT_1 | 0x0 , currentValueInfo_.messageValue ) ;
          if( machineState_ == STATE_METRONOME && isMute_ )
            _parallel_lcd_WriteString( PARALLEL_LCD_ROW_SELECT_0 | 0xA , MESSAGE.METRONOME.MUTE ) ;
          else if( machineState_ == STATE_ADJUST_TONE )
            _parallel_lcd_WriteCharacter( PARALLEL_LCD_ROW_SELECT_1 | 0x5 , menuInfoTone_.select - MENU_ITEM_TONE_ADJUST_TONE0 + '0' ) ;

          SetEvent( outputEvent_.changeValue ) ;
          break ;

        case STATE_INFORMATION:
          _parallel_lcd_WriteStringClearing( PARALLEL_LCD_ROW_SELECT_0 | 0x0 , MESSAGE_INFORMATION[ menuInfoInformation_.select ] ) ;
          _parallel_lcd_WriteString( PARALLEL_LCD_ROW_SELECT_0 | 0xA , &informationValueBuffer[ menuInfoInformation_.select ] ) ;
          _parallel_lcd_WriteStringClearing( PARALLEL_LCD_ROW_SELECT_1 | 0x0 , MESSAGE_INFORMATION[ menuInfoInformation_.select + 1 ] ) ;
          _parallel_lcd_WriteString( PARALLEL_LCD_ROW_SELECT_1 | 0xA , &informationValueBuffer[ menuInfoInformation_.select + 1 ] ) ;
          break ;

        case STATE_LOAD:
        case STATE_SAVE:
        case STATE_INITIALIZE:
        case STATE_ERROR:
          _parallel_lcd_WriteStringClearing( PARALLEL_LCD_ROW_SELECT_0 | 0x0 , currentValueInfo_.messageTitle ) ;
          _parallel_lcd_ClearRow( PARALLEL_LCD_ROW_SELECT_1 ) ;
          break ;

      }

    }

    // Display Value ----------------
    if( EvalEvent( outputEvent_.changeValue ) ) {

      uint16_t tmpValue ;
      char valueString[6] = "= 000" ;

      switch( machineState_ ) {

        case STATE_METRONOME:
          tmpValue = configration_.tempo ;
          break ;

        case STATE_ADJUST_OSCILLATOR_TUNE:
          if( (uint08_t)configration_.oscillatorTune & 0x80 ) {
            tmpValue = -configration_.oscillatorTune ;
            valueString[1] = '-' ;
          }
          else {
            tmpValue = (uint16_t)configration_.oscillatorTune ;
          }
          break ;

        default:
          tmpValue = *currentValueInfo_.valuePtr ;
          break ;
      }

      uint08_t isNonZero = BOOL_FALSE ;
      const uint08_t COMPARE_UNITS[] = { 100 , 10 , 1 , } ;
      for( uint08_t i = 0 ; i < 3 ; i++ ) {

        while( tmpValue >= COMPARE_UNITS[i] ) {
          tmpValue -= COMPARE_UNITS[i] ;
          valueString[ i + 2 ]++ ;
          isNonZero = BOOL_TRUE ;
        }

        if( i == 2 ) break ;

        if( isNonZero ) continue ;

        valueString[ i + 2 ] = valueString[ i + 1 ] ;
        valueString[ i + 1 ] = ' ' ;

      }

      _parallel_lcd_WriteString( PARALLEL_LCD_ROW_SELECT_1 | 0xB , &valueString ) ;

      switch( machineState_ ) {
        case STATE_ADJUST_OSCILLATOR_TUNE:
          SetOscillatorTune( configration_.oscillatorTune ) ;
          break ;
        case STATE_ADJUST_TONE:
          SetSoundTimerPeriod( *currentValueInfo_.valuePtr ) ;
          SetSoundPulseWidth( 1 ) ;
          break ;
      }
    }

  } //for(;;)

}//main()

// --------------------------------------------------------------------------------------------------------------------------------
// [Interrupt Service Routine]
#define INTERRUPT_FLAG PIR3bits.TMR6IF
void interrupt isr( void ) {
  if( !INTERRUPT_FLAG ) return ;
  INTERRUPT_FLAG = 0 ;

  static uint16_t eventPrescaler = 0 ;

  // Increment Metronome Count ----------------
  tempoCounter_ += configration_.tempo ;

  // Set Click Sound Event ----------------
  if( tempoCounter_ >= TOTAL_TEMOPO_COUNT ) {
    tempoCounter_ -= TOTAL_TEMOPO_COUNT ;
    if( ++beatCounter_ >= ( configration_.beatCount << 1 ) )
      beatCounter_ = 0 ;

    SetEvent( outputEvent_.soundOnClick ) ;
  }

  // Check Sound Duration ----------------
  if( !( eventPrescaler & 0x3F ) ) {
    if( soundDurationCount_.click && !--soundDurationCount_.click && !soundDurationCount_.key )
      SetEvent( outputEvent_.soundOff ) ;
    if( soundDurationCount_.key && ! --soundDurationCount_.key )
      SetEvent( outputEvent_.soundOff ) ;
  }

  // Prescale ----------------
  if( ++eventPrescaler != 640 ) return ;
  eventPrescaler = 0 ;

  // Count for State Return ----------------
  if( stateReturnCounter_ &&! --stateReturnCounter_ ) {
    SetEvent( outputEvent_.changeState ) ;
    SetEvent( outputEvent_.resetMetronome ) ;
    machineState_ = STATE_METRONOME ;
  }

  // Read Key State ----------------
  portAState_.byte = ReadKeyState( ) ;

  if( portAState_.keyUp && !portAState_.keyDown ) {
    if( ++keyHoldCount_.Up == KEY_COUNT_LOOP_END ) {
      keyHoldCount_.Up = KEY_COUNT_LOOP_START ;
      SetEvent( inputEvent_.keyPressHeldUp ) ;
    }
  }
  else
    keyHoldCount_.Up = 0 ;

  if( portAState_.keyDown && !portAState_.keyUp ) {
    if( ++keyHoldCount_.Down == KEY_COUNT_LOOP_END ) {
      keyHoldCount_.Down = KEY_COUNT_LOOP_START ;
      SetEvent( inputEvent_.keyPressHeldDown ) ;
    }
  }
  else
    keyHoldCount_.Down = 0 ;

  if( INTERRUPT_FLAG ) RESET( ) ;

}//isr

