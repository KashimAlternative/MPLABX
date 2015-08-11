#include <xc.h>

// ----------------------------------------------------------------
// [Function] Initialize
void initialize( ) {

  // Oscillator config
  OSCCONbits.IRCF = 0b1110 ; // 32MHz ( with PLL )
  OSCCONbits.SCS = 0b00 ; // According to Configuration Word 1
  OSCCONbits.SPLLEN = 1 ; // PLL Enabled
  OSCTUNEbits.TUN = 0b000000 ; // Factory-calibrated

  // Watchdog-TImer config
  WDTCONbits.WDTPS = 0b01111 ; // 1:2^20

  // Timer config
  INTCONbits.GIE = 0 ;
  INTCONbits.PEIE = 0 ;

  // Timer0 config
  OPTION_REGbits.PSA = 1 ;
  OPTION_REGbits.TMR0CS = 0 ;
  OPTION_REGbits.TMR0SE = 1 ;
  TMR0 = 0x00 ;
  INTCONbits.TMR0IE = 0 ;
  INTCONbits.TMR0IF = 0 ;

  // Timer1 config
  T1CONbits.TMR1ON = 0 ;
  T1CONbits.TMR1CS = 0b00 ; // Fosc/4
  T1CONbits.T1CKPS = 0b11 ; //Prescaler 1/4
  T1CONbits.nT1SYNC = 0 ; // Synchronize : Disable
  TMR1 = 0x0000 ;
  PIR1bits.TMR1IF = 0 ;
  PIE1bits.TMR1IE = 0 ;

  // Timer2 config
  T2CONbits.TMR2ON = 0 ;
  T2CONbits.T2OUTPS = 0b0000 ; // Postscaler 1:1
  T2CONbits.T2CKPS = 0b11 ; // Prescaler 1/64
  PR2 = 124 ;
  TMR2 = 0x00 ;
  PIR1bits.TMR2IF = 0 ;
  PIE1bits.TMR2IE = 0 ;

  // Timer4 config
  T4CONbits.TMR4ON = 0 ;
  T4CONbits.T4OUTPS = 0b0000 ;
  T4CONbits.T4CKPS = 0b01 ;
  PR4 = 79 ;
  TMR4 = 0x00 ;
  PIR3bits.TMR4IF = 0 ;
  PIE3bits.TMR4IE = 0 ;

  // Timer6 config
  T6CONbits.TMR6ON = 0 ;
  T6CONbits.T6OUTPS = 0b0000 ;
  T6CONbits.T6CKPS = 0b00 ;
  PR6 = 124 ;
  TMR6 = 0x00 ;
  PIR3bits.TMR6IF = 0 ;
  PIE3bits.TMR6IE = 0 ;

  // CCP config
  CCPTMRS = 0b00000000 ; // All Select Timer2
  CCP1CONbits.CCP1M = 0b0000 ; // Off
  CCP2CONbits.CCP2M = 0b0000 ; // Off
  CCP3CONbits.CCP3M = 0b0000 ; // Off

  // CCP4 config
  CCP4CONbits.CCP4M = 0b1100 ; // Set to PWM mode
  CCP4CONbits.DC4B = 0b00 ; // Clear LSbs of PWM duty cycle
  CCPTMRSbits.C4TSEL = 0b00 ; //Select Timer2
  CCPR4L = 0 ;
  CCPR4H = 0 ;

  // I/O config
  ANSELA = 0b00000000 ; // Set AN0-AN4 as Digital I/O
  ANSELB = 0b00000000 ; // Set AN1-AN7 as Digital I/O

  TRISA = 0b11100000 ; // Set A7,A6,A5 as Input , Other as Output
  TRISB = 0b00000000 ; // Set B0-B7 as Output

}
