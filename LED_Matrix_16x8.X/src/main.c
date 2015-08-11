// CONFIG
#pragma config FOSC = HS        // Oscillator Selection bits (HS oscillator: High-speed crystal/resonator on RA6/OSC2/CLKOUT and RA7/OSC1/CLKIN)
#pragma config WDTE = OFF       // Watchdog Timer Enable bit (WDT disabled)
#pragma config PWRTE = ON       // Power-up Timer Enable bit (PWRT enabled)
#pragma config MCLRE = ON       // RA5/MCLR/VPP Pin Function Select bit (RA5/MCLR/VPP pin function is MCLR)
#pragma config BOREN = ON       // Brown-out Detect Enable bit (BOD enabled)
#pragma config LVP = OFF        // Low-Voltage Programming Enable bit (RB4/PGM pin has digital I/O function, HV on MCLR must be used for programming)
#pragma config CPD = OFF        // Data EE Memory Code Protection bit (Data memory code protection off)
#pragma config CP = OFF         // Flash Program Memory Code Protection bit (Code protection off)

#include <xc.h>

#include "font7x8.h"

typedef unsigned char byte ;

// Strings
#define ROW_COUNT ( sizeof ( STRINGS ) / sizeof ( STRINGS[0] ) )
const char* STRINGS[] = {
  "0: 0123456789" ,
  "1: ABCDEFGHIJKLMNOPQRSTUVWXYZ" ,
  "2: abcdefghijklmnopqrstuvwxyz" ,
  "3: sefkalhslfaglzygdkfjhsvdnhagflZKvlawhevp'oc'qlc aodwiwkdj ;oiqxlhvpwiajlzbd"
} ;

// Declarations
union {
  byte all ;
  struct {
    byte scanPosition : 4 ;
    byte scanCount : 4 ;
  } ;
} prescaler_ = 0 ;

byte displayBuffer_[16] ;

byte stringSelect_ = 0 ;
int currentPosition_ = -16 ;
byte nullCount_ = 0 ;

// [Function] Main ----------------
void main( void ) {

  // Timer0 config
  OPTION_REGbits.T0CS = 0 ;
  OPTION_REGbits.PSA = 0 ;
  OPTION_REGbits.PS = 0b000 ; // Prescaler 1:2
  TMR0 = 0 ;
  INTCONbits.T0IE = 0 ;
  INTCONbits.T0IF = 0 ;

  // Timer2 config
  T2CONbits.TMR2ON = 1 ;
  T2CONbits.T2CKPS = 0b11 ;
  T2CONbits.TOUTPS = 0b0000 ;
  PR2 = 0xFF ;
  TMR2 = 0x00 ;
  TMR2IF = 0 ;
  TMR2IE = 0 ;

  TRISA = 0b00100000 ;
  TRISB = 0b00000000 ;

  PORTA = 0b00000000 ;
  PORTB = 0b00000000 ;

  INTCONbits.T0IE = 1 ;

  // Interrupt config
  INTCONbits.GIE = 1 ;
  INTCONbits.PEIE = 0 ;
  INTCONbits.RBIE = 0 ;

  for( ; ; ) ;

}
#define INTERRUPT_FLAG INTCONbits.T0IF
// [Interrupt] ----------------
void interrupt _( void ) {
  if( !INTERRUPT_FLAG ) return ;
  INTERRUPT_FLAG = 0 ;

  if( !prescaler_.scanCount ) {
    if( prescaler_.all == 0x0F ) {
      displayBuffer_[ prescaler_.scanPosition ] = 0x00 ;
      if( !nullCount_ ) {
        int linePosition = currentPosition_ + prescaler_.scanPosition ;
        if( linePosition >= 0 ) {

          byte characterPosition = 0 ;
          while( linePosition >= FONT_WIDTH ) {
            linePosition -= FONT_WIDTH ;
            characterPosition++ ;
          }

          char chr = STRINGS[ stringSelect_ ][ characterPosition ] ;

          if( chr ) 
            displayBuffer_[ prescaler_.scanPosition ] = FONT_IMAGE[ chr - CODE_OFFSET ][ linePosition ] ;
          else
            nullCount_++ ;
        }
      }
    }
    else {
      displayBuffer_[ prescaler_.scanPosition ] = displayBuffer_[ prescaler_.scanPosition + 1 ] ;
    }
  }

  PORTA = 0x10 ;
  PORTB = displayBuffer_[ prescaler_.scanPosition ] ;
  PORTA = prescaler_.scanPosition ;

  if( ++prescaler_.all ) return ;

  if( nullCount_ && ++nullCount_ & 0x10 ) {
    nullCount_ = 0 ;
    currentPosition_ = -16 ;
    if( ++stringSelect_ == ROW_COUNT ) stringSelect_ = 0 ;
  }
  else {
    currentPosition_++ ;
  }

  //if( INTERRUPT_FLAG ) RESET();

}

