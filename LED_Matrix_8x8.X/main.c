// CONFIG
#pragma config OSC = HS         // Oscillator selection bits (HS oscillator)
#pragma config WDT = OFF        // Watchdog timer enable bit (WDT disabled)
#pragma config CP = OFF         // Code protection bit (Code protection off)

//#pragma config IDLOC0 = 0x0000
//#pragma config IDLOC1 = 0x0000
//#pragma config IDLOC2 = 0x0000
//#pragma config IDLOC3 = 0x0000

#include "xc.h"
#include <pic16f54.h>

typedef unsigned char byte ;
union {
  byte prescaler ;
  byte bitMask ;
} Union ;
//byte count ;
union {
  byte all ;
  struct {
    unsigned x : 4 ;
    unsigned y : 4 ;
  } ;
} Position ;

byte displayBuffer[8] ;
byte calcBuffer[8]
        = {
  0b00011100 ,
  0b00010000 ,
  0b00001000 ,
  0b00000000 ,
  0b00000000 ,
  0b00000000 ,
  0b00000000 ,
  0b00000000 ,
} ;
//        = {
//  0b00000011 ,
//  0b10000000 ,
//  0b00001000 ,
//  0b00101000 ,
//  0b00010100 ,
//  0b00010000 ,
//  0b00000001 ,
//  0b11000000 ,
//} ;
byte count = 0 ;
void countNeighbors( byte add ) {
  Position.all += add ;
  Position.all &= 0x77 ;
  if( ( displayBuffer[ Position.x ] >> ( Position.y ) ) & 0x01 )
    count++ ;
}
// [Function] Main ----------------
int main( void ) {

  // Initialize
  OPTION = 0b00011111 ;
  TRISB = 0x00 ;
  TRISA = 0x00 ;

  for( ; ; ) {

    PORTA = Union.prescaler & 0x07 ;
    PORTB = displayBuffer[ PORTA ] = calcBuffer[ PORTA ] ;
    PORTA |= 0x08 ;

    TMR0 = 0xC0 ;
    while( TMR0 ) NOP( ) ;

    if( ++Union.prescaler ) continue ;

    PORTA = 0x00 ;

    for( Position.all = 0x00 ; !( Position.all & 0x08 ) ; Position.all++ ) {
      calcBuffer[ Position.all & 0x0F ] = 0x00 ;
      Union.bitMask = 0x01 ;
      for( Position.all &= 0x0F ; !( Position.all & 0x80 ) ; Position.all += 0x10 ) {

        count = 0 ;

        countNeighbors( 0x01 ) ;
        countNeighbors( 0x16 ) ;
        countNeighbors( 0x01 ) ;
        countNeighbors( 0x01 ) ;
        countNeighbors( 0x66 ) ;
        countNeighbors( 0x01 ) ;
        countNeighbors( 0x01 ) ;
        countNeighbors( 0x16 ) ;
        countNeighbors( 0x01 ) ;

        if( ( count == 3 ) || ( displayBuffer[ Position.x ] & Union.bitMask ) && ( count == 4 ) )
          calcBuffer[ Position.x ] |= Union.bitMask ;

        Union.bitMask <<= 1 ;

      }
    }

  }

}

