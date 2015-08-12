// ----------------------------------------------------------------
// [Function] Initialize
void initialize( ) {

  // Oscillator config
  OSCCONbits.IRCF = 0b1011 ; // 1MHz
  OSCCONbits.SCS = 0b10 ; // Internal Oscillator Block

  // Timer config
  INTCONbits.GIE = 1 ;
  INTCONbits.PEIE = 0 ;
  INTCONbits.INTE = 0 ;

  // Interrupt-on-Change config
  INTCONbits.IOCIE = 0 ;
  INTCONbits.IOCIE = 0 ;
  IOCANbits.IOCAN3 = 1 ;

  // Timer0 config
  OPTION_REGbits.PSA = 0 ; // Assign Prescaler
  OPTION_REGbits.PS = 0b010 ; // Prescaler 1:8
  OPTION_REGbits.TMR0CS = 0 ;
  OPTION_REGbits.TMR0SE = 1 ;
  TMR0 = 0 ;
  INTCONbits.TMR0IE = 0 ;
  INTCONbits.TMR0IF = 0 ;

  //I2C config
  SSP1STAT = 0b10000000 ;
  SSP1CON1 = 0b00101000 ;
  SSP1ADD = 9 ;

  // Timer2 config
  T2CONbits.T2CKPS = 0b00 ; //Prescaler 1/4
  T2CONbits.T2OUTPS = 0b0000 ; //Postscaler 1:1
  PIE1bits.TMR2IE = 0 ;
  PIR1bits.TMR2IF = 0 ;
  PR2 = 60 ;
  TMR2 = 0 ;
  T2CONbits.TMR2ON = 1 ;

  // PWM config
  PWM3DCH = 0 ;
  PWM3DCL = 0 ;
  PWM3CONbits.PWM3OE = 0 ;
  PWM3CONbits.PWM3POL = 0 ;
  PWM3CONbits.PWM3EN = 1 ;

  ANSELA = 0b00000000 ;
  ANSELB = 0b00000000 ;
  ANSELC = 0b00000000 ;

  TRISA = 0b00110011 ;
  TRISB = 0b01010000 ;
  TRISC = 0b00000000 ;

}
