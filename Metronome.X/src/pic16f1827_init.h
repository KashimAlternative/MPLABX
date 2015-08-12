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
#pragma config FCMEN = ON       // Fail-Safe Clock Monitor Enable (Fail-Safe Clock Monitor is enabled)

// CONFIG2
#pragma config WRT = OFF        // Flash Memory Self-Write Protection (Write protection off)
#pragma config PLLEN = ON       // PLL Enable (4x PLL enabled)
#pragma config STVREN = ON      // Stack Overflow/Underflow Reset Enable (Stack Overflow or Underflow will cause a Reset)
#pragma config BORV = HI        // Brown-out Reset Voltage Selection (Brown-out Reset Voltage (Vbor), high trip point selected.)
#pragma config LVP = OFF        // Low-Voltage Programming Enable (High-voltage on MCLR/VPP must be used for programming)


// ----------------------------------------------------------------
// [Function] Initialize
void initialize( ) {

  // Oscillator config
  OSCCONbits.SPLLEN = 1 ; // PLL Enable
  OSCCONbits.IRCF = 0b1110 ; // 32MHz ( with PLL )
  OSCCONbits.SCS = 0b00 ; // Determined by FOSC
  OSCTUNE = 0b00000000 ; // Factory-calibrated

  // Timer config
  INTCONbits.GIE = 0 ;
  INTCONbits.PEIE = 0 ;

  // Timer0 config
  PSA = 1 ;
  TMR0CS = 0 ;
  TMR0SE = 1 ;
  TMR0 = 0 ;
  TMR0IF = 0 ;
  TMR0IE = 0 ;

  // Timer2 config
  TMR2ON = 1 ;
  T2CONbits.T2CKPS = 0b11; //Prescaler 1:64
  T2CONbits.T2OUTPS =  0b0000; //Postscaler 1:1
  PR2 = 124 ;
  TMR2 = 0 ;
  TMR2IF = 0 ;
  TMR2IE = 0 ;

  // Timer4
  TMR4ON = 0 ;
  T4CON = 0b00000010 ; //Postscaler 1:1, Prescaler 1/16
  PR4 = 124 ;
  TMR4 = 0 ;
  TMR4IF = 0 ;
  TMR4IE = 0 ;

  // Timer6
  TMR6ON = 0 ;
  T6CONbits.T6CKPS = 0b00; //Prescaler 1:16
  T6CONbits.T6OUTPS = 0b0000 ; //Postscaler 1:1
  PR6 = 124 ;
  TMR6 = 0 ;
  TMR6IF = 0 ;
  TMR6IE = 0 ;

  // PWM config
  CCP2CON = 0B00001100 ; //Single PWM Mode
  CCP2SEL = 1 ; // Select RA7
  PSTR2CON = 0b00000001 ;
  CCPTMRS = 0b00000000 ; //CCP2 Select Timer2
  CCPR2L = 0 ;
  CCPR2H = 0 ;

  // A/D converter config
  ANSELA = 0b00000000 ; // Set AN0-AN4 as Digital I/O
  ANSELB = 0b00000000 ; // Set AN1-AN7 as Digital I/O

  TRISA = 0b01110000 ; // Set A4,A5,A6 as Input , Other as Output
  TRISB = 0b00000000 ; // Set B0-B7 as Output

}
