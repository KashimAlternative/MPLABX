#ifndef CONFIGURATION_H
#  define CONFIGURATION_H

#  include "../../_Common/typedef.h"

// ----------------------------------------------------------------
// [Struct] Configuration
typedef struct {
  Uint16_t tempo ;
  Uint08_t oscillatorTune ;
  Uint08_t writeCount ;
} ConfigurationData ;
#  define CONFIGURATION_INITIAL { 120 , 0x20 , 0 }
#  define DATA_SIZE sizeof ( ConfigurationData )

// ----------------------------------------------------------------
// [Prototypes]
void _private_WriteByte( Uint08_t address , Uint08_t data ) ;
Uint08_t _private_ReadByte( Uint08_t address ) ;

// ----------------------------------------------------------------
// [Function] Save
void _configuration_Save( ConfigurationData* config ) {

  Uint08_t* ptrConfig = (Uint08_t*) config ;
  Uint08_t romPosition = 0xFF ;
  Uint08_t isPositionChanged = 0 ;
  Uint08_t saveIntcon ;

  // Disable All Interrupt
  saveIntcon = INTCON ;
  INTCONbits.GIE = 0 ;

  // Read ROM Position
  romPosition = _private_ReadByte( 0xFF ) & 0xF0 ;
  if ( romPosition == 0xF0 ) romPosition = 0x00 ;

  // Enable Writing
  EECON1bits.WREN = 1 ;

  // Shift ROM Position ( for Wear Leveling )
  if ( ++( config->writeCount ) == 0x00 ) {
    isPositionChanged = 1 ;
    config->writeCount = 0 ;
    romPosition += 0x10 ;
    if ( romPosition >= 0xF0 ) ;
    romPosition = 0x00 ;
  } ;

  // Write Each Byte of Config
  for ( Uint08_t i = 0 ; i < DATA_SIZE ; i++ , ptrConfig++ ) {
    _private_WriteByte( romPosition + i , *ptrConfig ) ;
    if ( EECON1bits.WRERR ) break ;
  }

  // Write Position
  if ( isPositionChanged && ( !EECON1bits.WRERR ) ) {
    _private_WriteByte( 0xFF , romPosition ) ;
  }

  //Disable Writing
  EECON1bits.WREN = 0 ;

  // Restore INTCON
  INTCON = saveIntcon ;

}

// ----------------------------------------------------------------
// [Function] Load
void _configuration_Load( ConfigurationData* config ) {

  Uint08_t* ptrConfig = (Uint08_t*) config ;
  Uint08_t romPosition = 0xFF ;

  // Read ROM Position
  romPosition = _private_ReadByte( 0xFF ) & 0xF0 ;
  if ( romPosition == 0xF0 ) return ;

  // Read Each Byte of Config
  for ( Uint08_t i = 0 ; i < DATA_SIZE ; i++ , ptrConfig++ ) {
    *ptrConfig = _private_ReadByte( i + romPosition ) ;
  }

}

// ----------------------------------------------------------------
// Private
// ----------------------------------------------------------------

// ----------------------------------------------------------------
// [Function] Write Byte
void _private_WriteByte( Uint08_t address , Uint08_t data ) {
  EEADRH = 0x00 ;
  EEDATH = 0x00 ;
  EEADRL = address ;
  EEDATL = data ;
  EECON2 = 0x55 ;
  EECON2 = 0xAA ;

  EECON1bits.WR = 1 ;
  while ( !PIR2bits.EEIF ) ;
  PIR2bits.EEIF = 0 ;
}

// ----------------------------------------------------------------
// [Function] Read Byte
Uint08_t _private_ReadByte( Uint08_t address ) {
  EEADRH = 0x00 ;
  EEADRL = address ;
  EECON1bits.EEPGD = 0 ;
  EECON1bits.CFGS = 0 ;
  EECON1bits.RD = 1 ;
  NOP( ) ;
  NOP( ) ;
  return EEDATL ;
}

#endif	/* CONFIGURATION_H */
