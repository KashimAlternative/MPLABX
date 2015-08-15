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
#include "../../_Common/Typedef.h"
#include "../../_Common/Util.h"
#include "../../_Common/ParallelLCD.h"

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

// ----------------------------------------------------------------
// Error
typedef enum {
  ERROR_NONE = 0 ,
  ERROR_EEPROM ,
  ERROR_INTERRUPT ,
} EnErrorType ;
EnErrorType machineError_ = ERROR_NONE ;

// ----------------------------------------------------------------
// Other
Uint08_t stateReturnCounter_ = 0 ;
Bool_t isMute_ = BOOL_FALSE ;
#define RETURN_STATE_INTERVAL 100

// ----------------------------------------------------------------
// Configuration
ConfigurationData configration_ = CONFIG_DEFAULT ;

// ----------------------------------------------------------------
// Metronome Counter
#define _XTAL_FREQ 32000000UL
#define TOTAL_TEMOPO_COUNT ( _XTAL_FREQ * 3 / 20 )
#if TOTAL_TEMOPO_COUNT > 0xFFFFFF
#  error [ERROR] Too Large Tempo Count !!
#endif
Uint24_t tempoCounter_ = 0 ;
Uint08_t beatCounter_ = 0 ;

// ----------------------------------------------------------------
// Keys
typedef union {
  Uint08_t byte ;
  struct {
    unsigned : 5 ;
    unsigned keyMenu : 1 ;
    unsigned keyDown : 1 ;
    unsigned keyUp : 1 ;
  } ;
} UniPortAState ;
#define ReadKeyState() (~PORTA&0xE0)
UniPortAState sampledPortAState_ = { 0x00 } ;

// ----------------------------------------------------------------
// Key Count
#define KEY_COUNT_LOOP_START 0x3C
#define KEY_COUNT_LOOP_END 0x40

// ----------------------------------------------------------------
// Menu State
typedef struct {
  Uint08_t select ;
  Uint08_t cursorPosition ;
  const Uint08_t limit ;
  const Char_t** menuMessage ;
  const Char_t* singleMessage ;
} StMenuInfo ;
StMenuInfo menuInfoMain_ = { 0 , 0 , MENU_SIZE_MAIN - 1 , &MESSAGE_MENU_ITEM_MAIN , NULL , } ;
StMenuInfo menuInfoTone_ = { 0 , 0 , MENU_SIZE_TONE - 1 , &MESSAGE_MENU_ITEM_TONE , NULL , } ;
StMenuInfo menuInfoDuration_ = { 0 , 0 , MENU_SIZE_DURATION - 1 , &MESSAGE_MENU_ITEM_DURATION , NULL , } ;
StMenuInfo menuInfoConfirmLoad_ = { 0 , 0 , MENU_SIZE_CONFIRM - 1 , NULL , "Load ?" , } ;
StMenuInfo menuInfoConfirmSave_ = { 0 , 0 , MENU_SIZE_CONFIRM - 1 , NULL , "Save ?" , } ;
StMenuInfo menuInfoConfirmReset_ = { 0 , 0 , MENU_SIZE_CONFIRM - 1 , NULL , "Reset ?" , } ;
StMenuInfo menuInfoInformation_ = { 0 , 0 , MENU_SIZE_INFORMATION - 2 , NULL , NULL , } ;
StMenuInfo* currentMenuInfoPtr_ = NULL ;

// ----------------------------------------------------------------
// Single Message
const Char_t* currentSingleMessage_ = NULL ;

// ----------------------------------------------------------------
// Value
typedef struct {
  Uint08_t* valuePtr ;
  const struct {
    const Uint08_t upper ;
    const Uint08_t lower ;
  } limit ;
  const struct {
    const Char_t* title ;
    const Char_t* value ;
  } message ;
} StValueInfo ;
StValueInfo* currentValueInfoPtr_ = NULL ;
StValueInfo valueInfoTempo_ = { NULL ,
  { 0 , 0 } ,
  "Metronome" ,
  "Tempo" , } ;
StValueInfo valueInfoBeatCount_ = { NULL ,
  { 64 , 0 } ,
  "Configuration" ,
  "Beat Count" , } ;
StValueInfo valueInfoTone_ = { NULL ,
  { 0xFF , 0x00 } ,
  "Tone" ,
  "Tone" , } ;
StValueInfo valueInfoDurationClick_ = { NULL ,
  { 0xFF , 0x00 } ,
  "Duration" ,
  "Click" , } ;
StValueInfo valueInfoDurationKey_ = { NULL ,
  { 0xFF , 0x00 } ,
  "Duration" ,
  "Key Beep" , } ;
StValueInfo valueInfoPulseWidth_ = { NULL ,
  { 7 , 1 } ,
  "Configuration" ,
  "Pulse Width" , } ;
StValueInfo valueInfoOscillatorTune_ = { NULL ,
  { (Uint08_t)30 , (Uint08_t)( -30 ) } ,
  "Configuration" ,
  "Osc. Tune" , } ;

// ----------------------------------------------------------------
// Events
struct {
  union {
    Uint08_t byte ;
    struct {
      unsigned keyPressUp : 1 ;
      unsigned keyPressDown : 1 ;
      unsigned keyPressMenu : 1 ;
      unsigned keyPressUpDown : 1 ;
      unsigned keyPressHeldUp : 1 ;
      unsigned keyPressHeldDown : 1 ;
    } ;
  } input ;
  union {
    Uint08_t byte ;
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
  } output ;
  //  union {
  //    Uint08_t byte ;
  //    struct {
  //      unsigned onEeprom : 1 ;
  //      unsigned onInterrupt : 1 ;
  //    } ;
  //  } error ;
} events_ = { 0x00 , 0x00 , /*0x00*/ } ;
// ----------------------------------------------------------------
// Tone
#define TONE_OFF 0
#define TONE_SYSTEM 124
#define TONE_TUNE 141 // A = 880 Hz
struct {
  Uint08_t click ;
  Uint08_t key ;
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
  SetEvent( events_.output.changeState ) ;

  // Boot Beep
  SetSoundTimerPeriod( TONE_SYSTEM ) ;
  SetSoundPulseWidth( 1 ) ;

  // Timer for LCD Command Wait
  T4CONbits.TMR4ON = 1 ;

  // Timer for Boot Sequence
  T1CONbits.TMR1ON = 1 ;

  // Execute Boot Sequence
  for( Uint08_t phase = 0 ; phase < 0xE ; phase++ ) {

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
        Uint08_t userId ;
        userId = Configuration_ReadByte( 0 , MEMORY_SELECT_CONFIGURATION ) ;
        informationValueBuffer[ INFORMATION_ITEM_VERSION ][1] = ( ( userId >> 4 ) | '0' ) ;
        informationValueBuffer[ INFORMATION_ITEM_VERSION ][2] = ( ( userId & 0x0F ) | '0' ) ;
        userId = Configuration_ReadByte( 1 , MEMORY_SELECT_CONFIGURATION ) ;
        informationValueBuffer[ INFORMATION_ITEM_VERSION ][4] = ( ( userId >> 4 ) | '0' ) ;
        informationValueBuffer[ INFORMATION_ITEM_VERSION ][5] = ( ( userId & 0x0F ) | '0' ) ;
      }
        break ;

      case 0x4:
        // Initialize Display
        ParallelLCD_Initialize(
                                PARALLEL_LCD_CONFIG_8BIT_MODE | PARALLEL_LCD_CONFIG_2LINE_MODE ,
                                PARALLEL_LCD_CONFIG_DISPLAY_ON ,
                                PARALLEL_LCD_CONFIG_CURSOR_NONE ,
                                PARALLEL_LCD_CONFIG_INCREMENTAL
                                ) ;

        // Indicate Title and Version
        ParallelLCD_WriteStringClearing( PARALLEL_LCD_ROW_SELECT_0 | 0x0 , MESSAGE.METRONOME.MAIN_TILE ) ;
        ParallelLCD_WriteStringClearing( PARALLEL_LCD_ROW_SELECT_1 | 0x0 , MESSAGE_INFORMATION[ INFORMATION_ITEM_VERSION ] ) ;
        ParallelLCD_WriteString( PARALLEL_LCD_ROW_SELECT_1 | 0xA , &informationValueBuffer[ INFORMATION_ITEM_VERSION ] ) ;

      case 0x5:
        valueInfoBeatCount_.valuePtr = &configration_.beatCount ;
        valueInfoDurationClick_.valuePtr = &configration_.duration.click ;
        valueInfoDurationKey_.valuePtr = &configration_.duration.key ;
        valueInfoPulseWidth_.valuePtr = &configration_.pulseWidth ;
        valueInfoOscillatorTune_.valuePtr = ( Uint08_t* ) & configration_.oscillatorTune ;
        break ;

      case 0x6:
        // Set CGRAM to LCD
        ParallelLCD_SetCgram( CHAR_CODE.CURSOR_UP , & BITMAP.CURSOR_UP ) ;
        ParallelLCD_SetCgram( CHAR_CODE.CURSOR_DOWN , & BITMAP.CURSOR_DOWN ) ;
        ParallelLCD_SetCgram( CHAR_CODE.CURSOR_RIGHT , & BITMAP.CURSOR_RIGHT ) ;
        break ;

    }

    while( !PIR1bits.TMR1IF ) ;
    PIR1bits.TMR1IF = 0 ;

  }

  // Start Timer Interrupt
  //INTCONbits.GIE = 1 ;
  INTCONbits.PEIE = 1 ;
  PIR3bits.TMR6IF = 0 ;
  PIE3bits.TMR6IE = 1 ;
  T6CONbits.TMR6ON = 1 ;

  // Main Loop
  for( ; ; ) {

    // Clear Watchdog-Timer ----------------
    CLRWDT( ) ;

    // Set Input Event ----------------
    static UniPortAState prevPortAState = { 0x00 } ;
    UniPortAState portAState ;
    UniPortAState keyPressed ;

    portAState.byte = sampledPortAState_.byte ;

    keyPressed.byte = ( portAState.byte ^ prevPortAState.byte ) & portAState.byte ;
    prevPortAState.byte = portAState.byte ;

    if( keyPressed.keyMenu ) {
      SetEvent( events_.input.keyPressMenu ) ;
    }

    if( keyPressed.keyUp ) {
      if( portAState.keyDown )
        SetEvent( events_.input.keyPressUpDown ) ;
      else
        SetEvent( events_.input.keyPressUp ) ;
    }

    if( keyPressed.keyDown ) {
      if( portAState.keyUp )
        SetEvent( events_.input.keyPressUpDown ) ;
      else
        SetEvent( events_.input.keyPressDown ) ;
    }

    if( EvalEvent( events_.input.keyPressHeldUp ) )
      SetEvent( events_.input.keyPressUp ) ;

    if( EvalEvent( events_.input.keyPressHeldDown ) )
      SetEvent( events_.input.keyPressDown ) ;

    if( events_.input.byte ) {
      SetEvent( events_.output.soundOnKey ) ;
    }

    // Both Up & Donw ----------------
    if( EvalEvent( events_.input.keyPressUpDown ) ) {
      if( machineState_ == STATE_METRONOME ) {
        ToggleBool( isMute_ ) ;
        SetEvent( events_.output.changeMessage ) ;
      }
    }

    // Apply Menu Key Event (Action Before State Change) ----------------
    if( EvalEvent( events_.input.keyPressMenu ) ) {
      SetEvent( events_.output.changeState ) ;

      switch( machineState_ ) {

        case STATE_METRONOME:
          machineState_ = STATE_MENU_MAIN ;
          menuInfoMain_.select = 0 ;
          menuInfoMain_.cursorPosition = 0 ;
          break ;

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

            case MENU_ITEM_MAIN_INFORMATION:
              machineState_ = STATE_INFORMATION ;
              break ;

            case MENU_ITEM_MAIN_LOAD_CONFIGURATION:
              machineState_ = STATE_CONFIRM_LOAD ;
              menuInfoConfirmLoad_.select = 0 ;
              menuInfoConfirmLoad_.cursorPosition = 0 ;
              break ;

            case MENU_ITEM_MAIN_SAVE_CONFIGURATION:
              machineState_ = STATE_CONFIRM_SAVE ;
              menuInfoConfirmSave_.select = 0 ;
              menuInfoConfirmSave_.cursorPosition = 0 ;
              break ;

            case MENU_ITEM_MAIN_RESET:
              machineState_ = STATE_CONFIRM_RESET ;
              menuInfoConfirmReset_.select = 0 ;
              menuInfoConfirmReset_.cursorPosition = 0 ;
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
          if( menuInfoConfirmLoad_.select )
            machineState_ = STATE_LOAD ;
          else
            machineState_ = STATE_MENU_MAIN ;
          break ;

        case STATE_CONFIRM_SAVE:
          if( menuInfoConfirmSave_.select )
            machineState_ = STATE_SAVE ;
          else
            machineState_ = STATE_MENU_MAIN ;
          break ;

        case STATE_CONFIRM_RESET:
          if( menuInfoConfirmReset_.select )
            machineState_ = STATE_RESET ;
          else
            machineState_ = STATE_MENU_MAIN ;
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
          machineState_ = STATE_MENU_MAIN ;
          SetEvent( events_.output.soundOff ) ;
          break ;

        case STATE_ADJUST_TONE:
          machineState_ = STATE_MENU_TONE ;
          SetEvent( events_.output.soundOff ) ;
          break ;

        case STATE_ERROR:
          machineError_ = ERROR_NONE ;
        case STATE_INITIALIZE:
          machineState_ = STATE_METRONOME ;
          SetEvent( events_.output.resetMetronome ) ;
          break ;

      }

    }

    // Error----------------
    if( machineError_ && machineState_ != STATE_ERROR ) {
      machineState_ = STATE_ERROR ;
      events_.output.byte = 0x00 ;
      SetEvent( events_.output.changeState ) ;
    }

    // Change State (Action After State Change)----------------
    if( EvalEvent( events_.output.changeState ) ) {

      SetEvent( events_.output.changeMessage ) ;

      switch( machineState_ ) {
        case STATE_METRONOME:
          currentValueInfoPtr_ = &valueInfoTempo_ ;
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
          currentMenuInfoPtr_ = &menuInfoConfirmLoad_ ;
          break ;

        case STATE_CONFIRM_SAVE:
          currentMenuInfoPtr_ = &menuInfoConfirmSave_ ;
          break ;

        case STATE_CONFIRM_RESET:
          currentMenuInfoPtr_ = &menuInfoConfirmReset_ ;
          break ;

        case STATE_ADJUST_BEAT_COUNT:
          currentValueInfoPtr_ = &valueInfoBeatCount_ ;
          break ;

        case STATE_ADJUST_DURATION_CLICK:
          currentValueInfoPtr_ = &valueInfoDurationClick_ ;
          break ;

        case STATE_ADJUST_DURATION_KEY:
          currentValueInfoPtr_ = &valueInfoDurationKey_ ;
          break ;

        case STATE_ADJUST_PULSE_WIDTH:
          currentValueInfoPtr_ = &valueInfoPulseWidth_ ;
          break ;

        case STATE_ADJUST_OSCILLATOR_TUNE:
          currentValueInfoPtr_ = &valueInfoOscillatorTune_ ;
          SetSoundTimerPeriod( TONE_TUNE ) ;
          SetSoundPulseWidth( 1 ) ;
          SoundOn( ) ;
          break ;

        case STATE_ADJUST_TONE:
          valueInfoTone_.valuePtr = &configration_.tone[ menuInfoTone_.select - MENU_ITEM_TONE_ADJUST_TONE0 ] ;
          currentValueInfoPtr_ = &valueInfoTone_ ;
          SetSoundTimerPeriod( *currentValueInfoPtr_->valuePtr ) ;
          SetSoundPulseWidth( configration_.pulseWidth ) ;
          SoundOn( ) ;
          break ;

        case STATE_INFORMATION:
          menuInfoInformation_.select = 0 ;
          menuInfoInformation_.cursorPosition = 0 ;
          currentMenuInfoPtr_ = &menuInfoInformation_ ;
          break ;

        case STATE_BOOT:
        case STATE_INITIALIZE:
        case STATE_LOAD:
        case STATE_SAVE:
          SetEvent( events_.output.accessEeprom ) ;
          break ;

        case STATE_RESET:
          ParallelLCD_ClearDisplay( ) ;
          RESET( ) ;

        case STATE_ERROR:
          switch( machineError_ ) {
            case ERROR_EEPROM:
              currentSingleMessage_ = MESSAGE.ERROR.EEPROM ;
              break ;
            case ERROR_INTERRUPT:
              currentSingleMessage_ = MESSAGE.ERROR.INTERRUPT ;
              break ;
          }
          break ;

      }

    }

    // EEPROM ----------------
    if( EvalEvent( events_.output.accessEeprom ) ) {

      DisableAllInterrupt( ) ;

      ReturnCode returnCode = RETURN_CODE_NOERROR ;

      SetEvent( events_.output.resetMetronome ) ;
      SetEvent( events_.output.changeMessage ) ;
      stateReturnCounter_ = RETURN_STATE_INTERVAL ;

      switch( machineState_ ) {

        case STATE_BOOT:
          returnCode = Configuration_Load( &configration_ ) ;
          stateReturnCounter_ = 1 ;
          break ;

        case STATE_LOAD:
          returnCode = Configuration_Load( &configration_ ) ;
          currentSingleMessage_ = MESSAGE.MEMORY.LOAD ;
          break ;

        case STATE_INITIALIZE:
          returnCode = Configuration_Save( &configration_ ) ;
          currentSingleMessage_ = MESSAGE.MEMORY.INITIALIZE ;
          break ;

        case STATE_SAVE:
          returnCode = Configuration_Save( &configration_ ) ;
          currentSingleMessage_ = MESSAGE.MEMORY.SAVE ;
          break ;

      }

      informationValueBuffer[ INFORMATION_ITEM_ROM_OFFSET ][4] = HEX_TABLE[ configration_.romOffset >> 4 ] ;
      informationValueBuffer[ INFORMATION_ITEM_ROM_OFFSET ][5] = HEX_TABLE[ configration_.romOffset & 0x0F ] ;
      informationValueBuffer[ INFORMATION_ITEM_WRITE_COUNT ][4] = HEX_TABLE[ configration_.writeCount >> 4 ] ;
      informationValueBuffer[ INFORMATION_ITEM_WRITE_COUNT ][5] = HEX_TABLE[ configration_.writeCount & 0x0F ] ;
      informationValueBuffer[ INFORMATION_ITEM_ERROR_CODE ][4] = HEX_TABLE[ returnCode >> 4 ] ;
      informationValueBuffer[ INFORMATION_ITEM_ERROR_CODE ][5] = HEX_TABLE[ returnCode & 0x0F ] ;

      if( returnCode )
        machineError_ = ERROR_EEPROM ;

      EnableAllInterrupt( ) ;

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
        if( EvalEvent( events_.input.keyPressDown ) ) {
          if( currentMenuInfoPtr_->select != currentMenuInfoPtr_->limit ) {
            currentMenuInfoPtr_->select++ ;
            if( !currentMenuInfoPtr_->cursorPosition ) currentMenuInfoPtr_->cursorPosition++ ;
            SetEvent( events_.output.changeMessage ) ;
          }
        }
        if( EvalEvent( events_.input.keyPressUp ) ) {
          if( currentMenuInfoPtr_->select ) {
            currentMenuInfoPtr_->select-- ;
            if( currentMenuInfoPtr_->cursorPosition ) currentMenuInfoPtr_->cursorPosition-- ;
            SetEvent( events_.output.changeMessage ) ;
          }
        }
        break ;

      case STATE_METRONOME:
        if( EvalEvent( events_.input.keyPressUp ) ) {
          if( configration_.tempo < 999 ) {
            configration_.tempo++ ;
            SetEvent( events_.output.changeValue ) ;
          }
          SetEvent( events_.output.resetMetronome ) ;
        }
        if( EvalEvent( events_.input.keyPressDown ) ) {
          if( configration_.tempo > 1 ) {
            configration_.tempo-- ;
            SetEvent( events_.output.changeValue ) ;
          }
          SetEvent( events_.output.resetMetronome ) ;
        }
        break ;

      case STATE_ADJUST_BEAT_COUNT:
      case STATE_ADJUST_PULSE_WIDTH:
      case STATE_ADJUST_DURATION_CLICK:
      case STATE_ADJUST_DURATION_KEY:
      case STATE_ADJUST_TONE:
      case STATE_ADJUST_OSCILLATOR_TUNE:
        if( EvalEvent( events_.input.keyPressUp ) ) {
          if( *currentValueInfoPtr_->valuePtr != currentValueInfoPtr_->limit.upper ) {
            ( *currentValueInfoPtr_->valuePtr )++ ;
            SetEvent( events_.output.changeValue ) ;
          }
        }
        if( EvalEvent( events_.input.keyPressDown ) ) {
          if( *currentValueInfoPtr_->valuePtr != currentValueInfoPtr_->limit.lower ) {
            ( *currentValueInfoPtr_->valuePtr )-- ;
            SetEvent( events_.output.changeValue ) ;
          }
        }
        break ;

    }

    // Reset Metronome ----------------
    DisableAllInterrupt( ) ;
    if( EvalEvent( events_.output.resetMetronome ) ) {
      tempoCounter_ = 0 ;
      beatCounter_ = 0 ;
      SetEvent( events_.output.soundOnClick ) ;
    }
    else {
      if( tempoCounter_ >= TOTAL_TEMOPO_COUNT ) {
        tempoCounter_ -= TOTAL_TEMOPO_COUNT ;
        if( ++beatCounter_ >= ( configration_.beatCount << 1 ) )
          beatCounter_ = 0 ;

        SetEvent( events_.output.soundOnClick ) ;
      }
    }
    EnableAllInterrupt( ) ;

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
        if( EvalEvent( events_.output.soundOnKey ) ) {
          soundDurationCount_.key = configration_.duration.key ;
          SetSoundTimerPeriod( TONE_SYSTEM ) ;
          SetSoundPulseWidth( 1 ) ;
          SoundOn( ) ;
        }
        if( EvalEvent( events_.output.soundOnClick ) && !isMute_ && !soundDurationCount_.key ) {

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
        if( EvalEvent( events_.output.soundOff ) )
          SoundOff( ) ;

        break ;
    }

    // Message ----------------
    if( EvalEvent( events_.output.changeMessage ) ) {

      switch( machineState_ ) {

        case STATE_MENU_MAIN:
        case STATE_MENU_TONE:
        case STATE_MENU_DURATION:
          ParallelLCD_WriteStringClearing( PARALLEL_LCD_ROW_SELECT_0 | 0x1 , currentMenuInfoPtr_->menuMessage[ currentMenuInfoPtr_->select - currentMenuInfoPtr_->cursorPosition ] ) ;
          ParallelLCD_WriteStringClearing( PARALLEL_LCD_ROW_SELECT_1 | 0x1 , currentMenuInfoPtr_->menuMessage[ currentMenuInfoPtr_->select - currentMenuInfoPtr_->cursorPosition + 1 ] ) ;

          if( currentMenuInfoPtr_->select != currentMenuInfoPtr_->cursorPosition )
            ParallelLCD_WriteCharacter( PARALLEL_LCD_ROW_SELECT_0 | 0xF , CHAR_CODE.CURSOR_UP ) ;
          if( currentMenuInfoPtr_->select != ( currentMenuInfoPtr_->limit + currentMenuInfoPtr_->cursorPosition - 1 ) )
            ParallelLCD_WriteCharacter( PARALLEL_LCD_ROW_SELECT_1 | 0xF , CHAR_CODE.CURSOR_DOWN ) ;

          ParallelLCD_WriteCharacter( PARALLEL_LCD_ROW_SELECT[currentMenuInfoPtr_->cursorPosition] | 0x0 , CHAR_CODE.CURSOR_RIGHT ) ;

          break ;

        case STATE_METRONOME:
        case STATE_ADJUST_BEAT_COUNT:
        case STATE_ADJUST_TONE:
        case STATE_ADJUST_DURATION_CLICK:
        case STATE_ADJUST_DURATION_KEY:
        case STATE_ADJUST_PULSE_WIDTH:
        case STATE_ADJUST_OSCILLATOR_TUNE:
          ParallelLCD_WriteStringClearing( PARALLEL_LCD_ROW_SELECT_0 | 0x0 , currentValueInfoPtr_->message.title ) ;
          ParallelLCD_WriteStringClearing( PARALLEL_LCD_ROW_SELECT_1 | 0x0 , currentValueInfoPtr_->message.value ) ;
          if( machineState_ == STATE_METRONOME ) {
            if( isMute_ )
              ParallelLCD_WriteString( PARALLEL_LCD_ROW_SELECT_0 | 0xA , MESSAGE.METRONOME.MUTE ) ;
          }
          else if( machineState_ == STATE_ADJUST_TONE )
            ParallelLCD_WriteCharacter( PARALLEL_LCD_ROW_SELECT_1 | 0x5 , menuInfoTone_.select - MENU_ITEM_TONE_ADJUST_TONE0 + '0' ) ;

          SetEvent( events_.output.changeValue ) ;
          break ;

        case STATE_INFORMATION:
          ParallelLCD_WriteStringClearing( PARALLEL_LCD_ROW_SELECT_0 | 0x0 , MESSAGE_INFORMATION[ menuInfoInformation_.select ] ) ;
          ParallelLCD_WriteString( PARALLEL_LCD_ROW_SELECT_0 | 0xA , &informationValueBuffer[ menuInfoInformation_.select ] ) ;
          ParallelLCD_WriteStringClearing( PARALLEL_LCD_ROW_SELECT_1 | 0x0 , MESSAGE_INFORMATION[ menuInfoInformation_.select + 1 ] ) ;
          ParallelLCD_WriteString( PARALLEL_LCD_ROW_SELECT_1 | 0xA , &informationValueBuffer[ menuInfoInformation_.select + 1 ] ) ;
          break ;


        case STATE_CONFIRM_LOAD:
        case STATE_CONFIRM_SAVE:
        case STATE_CONFIRM_RESET:
          ParallelLCD_WriteStringClearing( PARALLEL_LCD_ROW_SELECT_0 | 0xD , MESSAGE.CONFIRM.NO ) ;
          ParallelLCD_WriteStringClearing( PARALLEL_LCD_ROW_SELECT_1 | 0xD , MESSAGE.CONFIRM.YES ) ;
          ParallelLCD_WriteString( PARALLEL_LCD_ROW_SELECT_0 | 0x0 , currentMenuInfoPtr_->singleMessage ) ;
          ParallelLCD_WriteCharacter( PARALLEL_LCD_ROW_SELECT[ currentMenuInfoPtr_->cursorPosition ] | 0xC , CHAR_CODE.CURSOR_RIGHT ) ;
          break ;

        case STATE_LOAD:
        case STATE_SAVE:
        case STATE_INITIALIZE:
          ParallelLCD_WriteStringClearing( PARALLEL_LCD_ROW_SELECT_0 | 0x0 , currentSingleMessage_ ) ;
          ParallelLCD_ClearRow( PARALLEL_LCD_ROW_SELECT_1 ) ;
          break ;

        case STATE_ERROR:
          ParallelLCD_WriteStringClearing( PARALLEL_LCD_ROW_SELECT_0 | 0x0 , MESSAGE.ERROR.TITLE ) ;
          ParallelLCD_WriteStringClearing( PARALLEL_LCD_ROW_SELECT_1 | 0x0 , currentSingleMessage_ ) ;
          break ;

      }

    }

    // Display Value ----------------
    if( EvalEvent( events_.output.changeValue ) ) {

      Uint16_t tmpValue ;
      Char_t valueString[6] = "= 000" ;

      switch( machineState_ ) {

        case STATE_METRONOME:
          tmpValue = configration_.tempo ;
          break ;

        case STATE_ADJUST_OSCILLATOR_TUNE:
          if( (Uint08_t)configration_.oscillatorTune & 0x80 ) {
            tmpValue = -configration_.oscillatorTune ;
            valueString[1] = '-' ;
          }
          else {
            tmpValue = (Uint16_t)configration_.oscillatorTune ;
          }
          break ;

        default:
          tmpValue = *currentValueInfoPtr_->valuePtr ;
          break ;
      }

      Bool_t isNonZero = BOOL_FALSE ;
      const Uint08_t COMPARE_UNITS[] = { 100 , 10 , 1 , } ;
      for( Uint08_t i = 0 ; i < 3 ; i++ ) {
        Char_t chr = '0' ;
        while( tmpValue >= COMPARE_UNITS[i] ) {
          tmpValue -= COMPARE_UNITS[i] ;
          chr++ ;
        }

        if( isNonZero || chr > '0' || i == 2 ) {
          valueString[ i + 2 ] = chr ;
          isNonZero = BOOL_TRUE ;
        }
        else {
          valueString[ i + 2 ] = valueString[ i + 1 ] ;
          valueString[ i + 1 ] = ' ' ;
        }
      }

      ParallelLCD_WriteString( PARALLEL_LCD_ROW_SELECT_1 | 0xB , &valueString ) ;

      switch( machineState_ ) {
        case STATE_ADJUST_OSCILLATOR_TUNE:
          SetOscillatorTune( configration_.oscillatorTune ) ;
          break ;
        case STATE_ADJUST_TONE:
          SetSoundTimerPeriod( *currentValueInfoPtr_->valuePtr ) ;
          SetSoundPulseWidth( configration_.pulseWidth ) ;
          break ;
      }
    }

  } //for(;;)

}//main()

// --------------------------------------------------------------------------------------------------------------------------------
// [Interrupt Service Routine]
#define INTERRUPT_FLAG PIR3bits.TMR6IF
void interrupt isr( void ) {
  static struct {
    Uint08_t count1ms ;
    Uint08_t count10ms ;
  } prescaler = { 0 , 0 } ;

  if( !INTERRUPT_FLAG ) return ;
  INTERRUPT_FLAG = 0 ;

  // Increment Metronome Count ----------------
  tempoCounter_ += configration_.tempo ;

  // Prescale 1ms ----------------
  if( --prescaler.count1ms ) return ;
  prescaler.count1ms = 80 ;

  // Decrement Sound Duration ----------------
  if( soundDurationCount_.click && !--soundDurationCount_.click && !soundDurationCount_.key )
    SetEvent( events_.output.soundOff ) ;
  if( soundDurationCount_.key && ! --soundDurationCount_.key )
    SetEvent( events_.output.soundOff ) ;

  // Prescale 10ms ----------------
  if( --prescaler.count10ms ) return ;
  prescaler.count10ms = 10 ;

  // Count for State Return ----------------
  if( stateReturnCounter_ && ! --stateReturnCounter_ ) {
    SetEvent( events_.output.changeState ) ;
    SetEvent( events_.output.resetMetronome ) ;
    machineState_ = STATE_METRONOME ;
  }

  // Read Key State ----------------
  static struct {
    Uint08_t Up ;
    Uint08_t Down ;
  } keyHoldCount = { 0 , 0 } ;

  sampledPortAState_.byte = ReadKeyState( ) ;

  if( sampledPortAState_.keyUp && !sampledPortAState_.keyDown ) {
    if( ++keyHoldCount.Up == KEY_COUNT_LOOP_END ) {
      keyHoldCount.Up = KEY_COUNT_LOOP_START ;
      SetEvent( events_.input.keyPressHeldUp ) ;
    }
  }
  else
    keyHoldCount.Up = 0 ;

  if( sampledPortAState_.keyDown && !sampledPortAState_.keyUp ) {
    if( ++keyHoldCount.Down == KEY_COUNT_LOOP_END ) {
      keyHoldCount.Down = KEY_COUNT_LOOP_START ;
      SetEvent( events_.input.keyPressHeldDown ) ;
    }
  }
  else
    keyHoldCount.Down = 0 ;

  if( INTERRUPT_FLAG ) machineError_ = ERROR_INTERRUPT ;

}//isr

