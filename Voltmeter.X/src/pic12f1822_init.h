// ----------------------------------------------------------------
// Configuration Word 1
#pragma config MCLRE=0,WDTE=OFF,FOSC=INTOSC
// Configuration Word 2
#pragma config LVP=0,PLLEN=1

// ----------------------------------------------------------------
// [Function] Initialize
void initialize( ) {

  // Oscillator config
  OSCCONbits.SPLLEN = 0 ; // PLL Disable
  OSCCONbits.IRCF = 0b1011 ; // 1MHz
  OSCCONbits.SCS = 0b10 ; // Internal Oscillator Block

  // Interrupt
  INTCONbits.GIE = 1 ;
  INTCONbits.PEIE = 0 ;

  // Timer0 config
  OPTION_REGbits.TMR0SE = 1 ;
  OPTION_REGbits.TMR0CS = 0 ;
  OPTION_REGbits.PSA = 1 ; // Assign No Prescaler
  OPTION_REGbits.PS = 0b010 ; // Prescaler 1:8 ;
  TMR0 = 0x00 ;
  INTCONbits.TMR0IF = 0 ;
  INTCONbits.TMR0IE = 0 ;

  // Timer1 config
  T1CONbits.TMR1ON = 0 ;
  //  T1CONbits.T1CKPS = 0b00 ; // 1:1 Prescaler
  //  T1CONbits.TMR1CS = 0b00 ; // System Instruction Clock
  //  TMR1 = 0x0000 ;
  //  PIR1bits.TMR1IF = 0 ;
  //  PIE1bits.TMR1IE = 0 ;

  // Timer2 config
  T2CONbits.TMR2ON = 0 ;
  T2CONbits.T2CKPS = 0b00 ; // Prescaler 1:1
  T2CONbits.T2OUTPS = 0b1001 ; // Postscaler 1:10
  PR2 = 249 ;
  TMR2 = 0x00 ;
  PIR1bits.TMR2IF = 0 ;
  PIE1bits.TMR2IE = 0 ;

  // I2C config
  SSPSTAT = 0x00 ;
  SSP1CON1bits.WCOL = 0 ;
  SSP1CON1bits.SSPOV = 0 ;
  SSP1CON1bits.SSPEN = 1 ;
  SSP1CON1bits.SSPM = 0b1000 ;
  SSP1ADD = 0 ;

  // A/D converter config
  FVRCONbits.ADFVR = 0b10 ; // 2.048V
  FVRCONbits.FVREN = 1 ;

  ADCON0bits.CHS = 3 ; // AN3
  ADCON0bits.ADON = 1 ; // ADC Enable
  ADCON1bits.ADFM = 1 ; // Right Justify
  ADCON1bits.ADCS = 0b000 ; // Fosc/2
  ADCON1bits.ADPREF = 0b11 ; // Vref is connected to FVR

  ANSELA = 0b00010000 ; // Set AN3 as A/D Input
  TRISA = 0b00101111 | ANSELA ; // Set A4,A5,A6 as Input , Other as Output

}
