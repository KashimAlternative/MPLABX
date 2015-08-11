// CONFIG
#pragma config OSC = HS         // Oscillator selection bits (HS oscillator)
#pragma config WDT = OFF        // Watchdog timer enable bit (WDT disabled)
#pragma config CP = OFF         // Code protection bit (Code protection off)

//#pragma config IDLOC0 = 0x0000
//#pragma config IDLOC1 = 0x0000
//#pragma config IDLOC2 = 0x0000
//#pragma config IDLOC3 = 0x0000

#include <xc.h>

typedef unsigned char byte ;
byte prescaler_ ;
byte bitMask_ ;
union {
  byte all ;
  struct {
    unsigned x : 4 ;
    unsigned y : 4 ;
  } ;
} position_ ;

byte displayBuffer_[8] ;
byte calcBuffer_[8]
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
byte count_ = 0 ;

// [Function] Count Neighbors ----------------
void countNeighbors( byte add ) {
  position_.all += add ;
  position_.all &= 0x77 ;
  if( ( displayBuffer_[ position_.x ] >> ( position_.y ) ) & 0x01 )
    count_++ ;
}

// [Function] Main ----------------
int main( void ) {

  // Initialize
  OPTION = 0b00011111 ;
  TRISB = 0x00 ;
  TRISA = 0x00 ;

  for( ; ; ) {

    PORTA = prescaler_ & 0x07 ;
    PORTB = displayBuffer_[ PORTA ] = calcBuffer_[ PORTA ] ;
    PORTA |= 0x08 ;

    TMR0 = 0xC0 ;
    while( TMR0 ) NOP( ) ;

    if( ++prescaler_ ) continue ;

    PORTA = 0x00 ;

    for( position_.all = 0x00 ; !( position_.all & 0x08 ) ; position_.all++ ) {
      calcBuffer_[ position_.all & 0x0F ] = 0x00 ;
      bitMask_ = 0x01 ;
      for( position_.all &= 0x0F ; !( position_.all & 0x80 ) ; position_.all += 0x10 ) {

        count_ = 0 ;

        countNeighbors( 0x01 ) ;
        countNeighbors( 0x16 ) ;
        countNeighbors( 0x01 ) ;
        countNeighbors( 0x01 ) ;
        countNeighbors( 0x66 ) ;
        countNeighbors( 0x01 ) ;
        countNeighbors( 0x01 ) ;
        countNeighbors( 0x16 ) ;
        countNeighbors( 0x01 ) ;

        if( ( count_ == 3 ) || ( displayBuffer_[ position_.x ] & bitMask_ ) && ( count_ == 4 ) )
          calcBuffer_[ position_.x ] |= bitMask_ ;

        bitMask_ <<= 1 ;

      }
    }

  }

}

