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
  STATE_ADJUST_VALUE ,
  STATE_ADJUST_OSCILLATOR_TUNE ,
  STATE_INFORMATION ,
  STATE_CONFIRM ,
  STATE_SAVE ,
  STATE_LOAD ,
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
union {
  Uint08_t byte ;
  struct {
    unsigned isUserMute : 1 ;
    unsigned isSystemMute : 1 ;
    unsigned isBusy : 1 ;
  } ;
} soundState_ = { 0x00 , } ;
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
#define KEY_COUNT_LOOP_INTERVAL 4
#define KEY_COUNT_LOOP_START 0x40

// ----------------------------------------------------------------
// Menu
typedef struct {
  Uint08_t select ;
  Uint08_t cursorPosition ;
  const Uint08_t limit ;
  const Char_t** menuMessage ;
} StMenuInfo ;
StMenuInfo menuInfoMain_ = { 0 , 0 , MENU_SIZE_MAIN - 1 , &MESSAGE_MENU_ITEM_MAIN , } ;
StMenuInfo menuInfoTone_ = { 0 , 0 , MENU_SIZE_TONE - 1 , &MESSAGE_MENU_ITEM_TONE , } ;
StMenuInfo menuInfoDuration_ = { 0 , 0 , MENU_SIZE_DURATION - 1 , &MESSAGE_MENU_ITEM_DURATION , } ;
StMenuInfo menuInfoInformation_ = { 0 , 0 , MENU_SIZE_INFORMATION - 2 , NULL , } ;
StMenuInfo* currentMenuInfoPtr_ = NULL ;

// ----------------------------------------------------------------
// Confirmation
typedef struct {
  Bool_t isSelectYes ;
  EnMachineState stateYes ;
  EnMachineState stateNo ;
  const Char_t* message ;
} StConfirmationInfo ;
StConfirmationInfo confirmationLoad_ = { BOOL_FALSE , STATE_LOAD , STATE_MENU_MAIN , "Load ?" , } ;
StConfirmationInfo ConfirmationSave_ = { BOOL_FALSE , STATE_SAVE , STATE_MENU_MAIN , "Save ?" , } ;
StConfirmationInfo confirmationReset_ = { BOOL_FALSE , STATE_RESET , STATE_MENU_MAIN , "Reset ?" , } ;
StConfirmationInfo* currentConfirmationInfoPtr_ = NULL ;

// ----------------------------------------------------------------
// Single Message
const Char_t* currentSingleMessage_ = NULL ;

// ----------------------------------------------------------------
// Value
#define MAX_TEMPO 999
#define MIN_TEMPO 1
typedef struct {
  Uint08_t* valuePtr ;
  EnMachineState parentState ;
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
  STATE_METRONOME ,
  { 0 , 0 } ,
  "Metronome" ,
  "Tempo" , } ;
StValueInfo valueInfoBeatCount_ = { NULL ,
  STATE_MENU_MAIN ,
  { 64 , 0 } ,
  "Configuration" ,
  "Beat Count" , } ;
StValueInfo valueInfoTone_[3] = {
  { NULL ,
    STATE_MENU_TONE ,
    { 0xFF , 0x00 } ,
    "Tone" ,
    "Tone0" , } ,
  { NULL ,
    STATE_MENU_TONE ,
    { 0xFF , 0x00 } ,
    "Tone" ,
    "Tone1" , } ,
  { NULL ,
    STATE_MENU_TONE ,
    { 0xFF , 0x00 } ,
    "Tone" ,
    "Tone2" , }
} ;
StValueInfo valueInfoDurationClick_ = { NULL ,
  STATE_MENU_DURATION ,
  { 0xFF , 0x00 } ,
  "Duration" ,
  "Click" , } ;
StValueInfo valueInfoDurationKey_ = { NULL ,
  STATE_MENU_DURATION ,
  { 0xFF , 0x00 } ,
  "Duration" ,
  "Key Beep" , } ;
StValueInfo valueInfoPulseWidth_ = { NULL ,
  STATE_MENU_MAIN ,
  { 7 , 1 } ,
  "Configuration" ,
  "Pulse Width" , } ;
StValueInfo valueInfoOscillatorTune_ = { NULL ,
  STATE_MENU_MAIN ,
  { (Uint08_t)30 , (Uint08_t)( -30 ) } ,
  "Configuration" ,
  "Osc. Tune" , } ;

// ----------------------------------------------------------------
// Events
struct {
  union {
    Uint08_t byte ;
    struct {
      unsigned up : 1 ;
      unsigned down : 1 ;
      unsigned menu : 1 ;
      unsigned bothUpDown : 1 ;
      unsigned upHold : 1 ;
      unsigned downHold : 1 ;
    } ;
  } keyPress ;
  union {
    Uint08_t byte ;
    struct {
      unsigned changeState : 1 ;
      unsigned changeMessage : 1 ;
      unsigned changeValue : 1 ;
      unsigned resetMetronome : 1 ;
      unsigned accessEeprom : 1 ;
    } ;
  } output ;
  union {
    Uint08_t byte ;
    struct {
      unsigned off : 1 ;
      unsigned click : 1 ;
      unsigned key : 1 ;
      unsigned oscillatorTune : 1 ;
    } ;
  } sound ;
} events_ = { 0x00 , 0x00 , 0x00 , } ;

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
        valueInfoTone_[0].valuePtr = &configration_.tone[0] ;
        valueInfoTone_[1].valuePtr = &configration_.tone[1] ;
        valueInfoTone_[2].valuePtr = &configration_.tone[2] ;
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
      SetEvent( events_.keyPress.menu ) ;
    }

    if( keyPressed.keyUp ) {
      if( portAState.keyDown )
        SetEvent( events_.keyPress.bothUpDown ) ;
      else
        SetEvent( events_.keyPress.up ) ;
    }

    if( keyPressed.keyDown ) {
      if( portAState.keyUp )
        SetEvent( events_.keyPress.bothUpDown ) ;
      else
        SetEvent( events_.keyPress.down ) ;
    }

    if( EvalEvent( events_.keyPress.upHold ) )
      SetEvent( events_.keyPress.up ) ;

    if( EvalEvent( events_.keyPress.downHold ) )
      SetEvent( events_.keyPress.down ) ;

    if( events_.keyPress.byte ) {
      SetEvent( events_.sound.key ) ;
    }

    // Both Up & Donw ----------------
    if( EvalEvent( events_.keyPress.bothUpDown ) ) {
      if( machineState_ == STATE_METRONOME ) {
        ToggleBool( soundState_.isUserMute ) ;
        SetEvent( events_.output.changeMessage ) ;
      }
    }

    // Apply Menu Key Event (Action Before State Change) ----------------
    if( EvalEvent( events_.keyPress.menu ) ) {
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
              machineState_ = STATE_ADJUST_VALUE ;
              currentValueInfoPtr_ = &valueInfoBeatCount_ ;
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
              currentValueInfoPtr_ = &valueInfoPulseWidth_ ;
              machineState_ = STATE_ADJUST_VALUE ;
              break ;

            case MENU_ITEM_MAIN_ADJUST_OSCILLATOR_TUNE:
              machineState_ = STATE_ADJUST_OSCILLATOR_TUNE ;
              break ;

            case MENU_ITEM_MAIN_INFORMATION:
              machineState_ = STATE_INFORMATION ;
              break ;

            case MENU_ITEM_MAIN_LOAD_CONFIGURATION:
              machineState_ = STATE_CONFIRM ;
              confirmationLoad_.isSelectYes = BOOL_FALSE ;
              currentConfirmationInfoPtr_ = &confirmationLoad_ ;
              break ;

            case MENU_ITEM_MAIN_SAVE_CONFIGURATION:
              machineState_ = STATE_CONFIRM ;
              ConfirmationSave_.isSelectYes = BOOL_FALSE ;
              currentConfirmationInfoPtr_ = &ConfirmationSave_ ;
              break ;

            case MENU_ITEM_MAIN_RESET:
              machineState_ = STATE_CONFIRM ;
              confirmationReset_.isSelectYes = BOOL_FALSE ;
              currentConfirmationInfoPtr_ = &confirmationReset_ ;
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
          else {
            machineState_ = STATE_ADJUST_VALUE ;
            currentValueInfoPtr_ = &valueInfoTone_[ menuInfoTone_.select - MENU_ITEM_TONE_ADJUST_TONE0 ] ;
          }
          break ;

        case STATE_MENU_DURATION:
          switch( currentMenuInfoPtr_->select ) {
            case MENU_ITEM_DURATION_ADJUST_CLICK:
              machineState_ = STATE_ADJUST_VALUE ;
              currentValueInfoPtr_ = &valueInfoDurationClick_ ;
              break ;

            case MENU_ITEM_DURATION_ADJUST_KEY:
              machineState_ = STATE_ADJUST_VALUE ;
              currentValueInfoPtr_ = &valueInfoDurationKey_ ;
              break ;

            case MENU_ITEM_DURATION_RETURN:
            default:
              machineState_ = STATE_MENU_MAIN ;
              break ;

          }
          break ;

        case STATE_CONFIRM:
          if( currentConfirmationInfoPtr_->isSelectYes )
            machineState_ = currentConfirmationInfoPtr_->stateYes ;
          else
            machineState_ = currentConfirmationInfoPtr_->stateNo ;
          break ;

        case STATE_INFORMATION:
          machineError_ = STATE_MENU_MAIN ;
          break ;

        case STATE_ADJUST_VALUE:
          machineState_ = currentValueInfoPtr_->parentState ;
          break ;

        case STATE_ADJUST_OSCILLATOR_TUNE:
          machineState_ = STATE_MENU_MAIN ;
          soundState_.isBusy = BOOL_FALSE ;
          SetEvent( events_.sound.off ) ;
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
          soundState_.isSystemMute = BOOL_FALSE ;
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

        case STATE_ADJUST_OSCILLATOR_TUNE:
          currentValueInfoPtr_ = &valueInfoOscillatorTune_ ;
          soundState_.isBusy = BOOL_TRUE ;
          SetEvent( events_.sound.oscillatorTune ) ;
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
          soundState_.isSystemMute = BOOL_TRUE ;
          SetEvent( events_.sound.off ) ;
          SetEvent( events_.output.accessEeprom ) ;
          break ;

        case STATE_RESET:
          ParallelLCD_ClearDisplay( ) ;
          RESET( ) ;

        case STATE_ERROR:
          soundState_.isSystemMute = BOOL_TRUE ;
          SetEvent( events_.sound.off ) ;
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


    // Reset Metronome ----------------
    DisableAllInterrupt( ) ;
    if( EvalEvent( events_.output.resetMetronome ) ) {
      tempoCounter_ = 0 ;
      beatCounter_ = 0 ;
      SetEvent( events_.sound.click ) ;
    }
    else {
      if( tempoCounter_ >= TOTAL_TEMOPO_COUNT ) {
        tempoCounter_ -= TOTAL_TEMOPO_COUNT ;
        if( ++beatCounter_ >= ( configration_.beatCount << 1 ) )
          beatCounter_ = 0 ;

        SetEvent( events_.sound.click ) ;
      }
    }
    EnableAllInterrupt( ) ;

    // Key Sound ----------------
    if( EvalEvent( events_.sound.key ) ) {
      if( !soundState_.byte ) {
        soundDurationCount_.key = configration_.duration.key ;
        SetSoundTimerPeriod( TONE_SYSTEM ) ;
        SetSoundPulseWidth( 1 ) ;
        SoundOn( ) ;
      }
    }

    // Click Sound ----------------
    if( EvalEvent( events_.sound.click ) ) {

      if( !soundState_.byte && !soundDurationCount_.key ) {

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

    }

    // Oscillator Tune ----------------
    if( EvalEvent( events_.sound.oscillatorTune ) ) {
      SetSoundTimerPeriod( TONE_TUNE ) ;
      SetSoundPulseWidth( 1 ) ;
      SoundOn( ) ;
    }

    // Sound Off ----------------
    if( EvalEvent( events_.sound.off ) ) {
      soundDurationCount_.click = 0 ;
      soundDurationCount_.key = 0 ;
      SoundOff( ) ;
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

    // Apply Up Key Press Event ----------------
    if( EvalEvent( events_.keyPress.up ) ) {
      switch( machineState_ ) {
        case STATE_MENU_MAIN:
        case STATE_MENU_TONE:
        case STATE_MENU_DURATION:
        case STATE_INFORMATION:
          if( currentMenuInfoPtr_->select ) {
            currentMenuInfoPtr_->select-- ;
            if( currentMenuInfoPtr_->cursorPosition ) currentMenuInfoPtr_->cursorPosition-- ;
            SetEvent( events_.output.changeMessage ) ;
          }
          break ;

        case STATE_CONFIRM:
          if( currentConfirmationInfoPtr_->isSelectYes ) {
            currentConfirmationInfoPtr_->isSelectYes = BOOL_FALSE ;
            SetEvent( events_.output.changeMessage ) ;
          }
          break ;

        case STATE_ADJUST_VALUE:
        case STATE_ADJUST_OSCILLATOR_TUNE:
          if( *currentValueInfoPtr_->valuePtr != currentValueInfoPtr_->limit.upper ) {
            ( *currentValueInfoPtr_->valuePtr )++ ;
            SetEvent( events_.output.changeValue ) ;
          }
          break ;

        case STATE_METRONOME:
          if( configration_.tempo != MAX_TEMPO ) {
            configration_.tempo++ ;
            SetEvent( events_.output.changeValue ) ;
          }
          SetEvent( events_.output.resetMetronome ) ;
          break ;
      }
    }

    // Apply Down Key Press Event ----------------
    if( EvalEvent( events_.keyPress.down ) ) {
      switch( machineState_ ) {
        case STATE_MENU_MAIN:
        case STATE_MENU_TONE:
        case STATE_MENU_DURATION:
        case STATE_INFORMATION:
          if( currentMenuInfoPtr_->select != currentMenuInfoPtr_->limit ) {
            currentMenuInfoPtr_->select++ ;
            if( !currentMenuInfoPtr_->cursorPosition ) currentMenuInfoPtr_->cursorPosition++ ;
            SetEvent( events_.output.changeMessage ) ;
          }
          break ;

        case STATE_CONFIRM:
          if( !currentConfirmationInfoPtr_->isSelectYes ) {
            currentConfirmationInfoPtr_->isSelectYes = BOOL_TRUE ;
            SetEvent( events_.output.changeMessage ) ;
          }
          break ;

        case STATE_ADJUST_VALUE:
        case STATE_ADJUST_OSCILLATOR_TUNE:
          if( *currentValueInfoPtr_->valuePtr != currentValueInfoPtr_->limit.lower ) {
            ( *currentValueInfoPtr_->valuePtr )-- ;
            SetEvent( events_.output.changeValue ) ;
          }
          break ;

        case STATE_METRONOME:
          if( configration_.tempo != MIN_TEMPO ) {
            configration_.tempo-- ;
            SetEvent( events_.output.changeValue ) ;
          }
          SetEvent( events_.output.resetMetronome ) ;
          break ;
      }
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
        case STATE_ADJUST_VALUE:
        case STATE_ADJUST_OSCILLATOR_TUNE:
          ParallelLCD_WriteStringClearing( PARALLEL_LCD_ROW_SELECT_0 | 0x0 , currentValueInfoPtr_->message.title ) ;
          ParallelLCD_WriteStringClearing( PARALLEL_LCD_ROW_SELECT_1 | 0x0 , currentValueInfoPtr_->message.value ) ;
          if( machineState_ == STATE_METRONOME ) {
            if( soundState_.isUserMute )
              ParallelLCD_WriteString( PARALLEL_LCD_ROW_SELECT_0 | 0xA , MESSAGE.METRONOME.MUTE ) ;
          }

          SetEvent( events_.output.changeValue ) ;
          break ;

        case STATE_INFORMATION:
          ParallelLCD_WriteStringClearing( PARALLEL_LCD_ROW_SELECT_0 | 0x0 , MESSAGE_INFORMATION[ menuInfoInformation_.select ] ) ;
          ParallelLCD_WriteString( PARALLEL_LCD_ROW_SELECT_0 | 0xA , &informationValueBuffer[ menuInfoInformation_.select ] ) ;
          ParallelLCD_WriteStringClearing( PARALLEL_LCD_ROW_SELECT_1 | 0x0 , MESSAGE_INFORMATION[ menuInfoInformation_.select + 1 ] ) ;
          ParallelLCD_WriteString( PARALLEL_LCD_ROW_SELECT_1 | 0xA , &informationValueBuffer[ menuInfoInformation_.select + 1 ] ) ;
          break ;

        case STATE_CONFIRM:
          ParallelLCD_WriteStringClearing( PARALLEL_LCD_ROW_SELECT_0 | 0xD , MESSAGE.CONFIRM.NO ) ;
          ParallelLCD_WriteStringClearing( PARALLEL_LCD_ROW_SELECT_1 | 0xD , MESSAGE.CONFIRM.YES ) ;
          ParallelLCD_WriteString( PARALLEL_LCD_ROW_SELECT_0 | 0x0 , currentConfirmationInfoPtr_->message ) ;
          ParallelLCD_WriteCharacter( PARALLEL_LCD_ROW_SELECT[ currentConfirmationInfoPtr_->isSelectYes ] | 0xC , CHAR_CODE.CURSOR_RIGHT ) ;
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
      Char_t valueString[6] ;
      valueString[0] = '=' ;
      valueString[1] = ' ' ;

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
        Uint08_t compareUnit = COMPARE_UNITS[i] ;
        while( tmpValue >= compareUnit ) {
          tmpValue -= compareUnit ;
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
    SetEvent( events_.sound.off ) ;
  if( soundDurationCount_.key && ! --soundDurationCount_.key )
    SetEvent( events_.sound.off ) ;

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
    if( !--keyHoldCount.Up ) {
      keyHoldCount.Up = KEY_COUNT_LOOP_INTERVAL ;
      SetEvent( events_.keyPress.upHold ) ;
    }
  }
  else
    keyHoldCount.Up = KEY_COUNT_LOOP_START ;

  if( sampledPortAState_.keyDown && !sampledPortAState_.keyUp ) {
    if( !--keyHoldCount.Down ) {
      keyHoldCount.Down = KEY_COUNT_LOOP_INTERVAL ;
      SetEvent( events_.keyPress.downHold ) ;
    }
  }
  else
    keyHoldCount.Down = KEY_COUNT_LOOP_START ;

  if( INTERRUPT_FLAG ) machineError_ = ERROR_INTERRUPT ;

}//isr

