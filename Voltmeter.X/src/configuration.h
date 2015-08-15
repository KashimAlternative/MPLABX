#ifndef CONFIGURATION_H
#  define CONFIGURATION_H

#  include "../../_Common/typedef.h"

#  define _private_EnableInterrupt() INTCONbits.GIE=1;
#  define _private_DisableInterrupt() INTCONbits.GIE=0;

#  define _private_IsError() EECON1bits.WRERR

#  define _private_SetData( data ) EEDATH=0x00;EEDATL=(data);
#  define _private_SetAddress( address ) EEADRH=0x00;EEADRL=(address);

#  define _private_EnableWriting() EECON1bits.WREN=1;
#  define _private_DisableWriting() EECON1bits.WREN=0;
#  define _private_UnlockFlashProgram() EECON2=0x55;EECON2=0xAA;
#  define _private_StartWrite() EECON1bits.WR=1;
#  define _private_WaitWriting() while(!PIR2bits.EEIF);PIR2bits.EEIF=0;

#  define _private_ConfugureReading() EECON1bits.EEPGD=0;EECON1bits.CFGS=0;
#  define _private_StartRead() EECON1bits.RD=1;
#  define _private_Data() EEDATL

// ----------------------------------------------------------------
// [Struct] Configuration
#  define SIZE_OF_EEPROM 0x100
typedef struct {
  Uint08_t sleepTime ;
} StConfigurationData ;
#  define CONFIG_DEFAULT { 60 }
#  define CONFIG_DATA_SIZE sizeof(StConfigurationData)

// ----------------------------------------------------------------
// [Type-Def]
typedef Uint08_t EepromData_t ;
typedef Uint08_t EepromAddress_t ;

// ----------------------------------------------------------------
// [Prototypes]
PRIVATE void Configuration_WriteByte( EepromAddress_t address , EepromData_t data ) ;
PRIVATE EepromData_t Configuration_ReadByte( EepromAddress_t address ) ;

// ----------------------------------------------------------------
// [Function] Save
void Configuration_Save( StConfigurationData* config ) {

  EepromData_t* ptrConfig = (EepromData_t*) config ;

  // Disable All Interrupt
  _private_DisableInterrupt( ) ;

  // Enable Writing
  _private_EnableWriting( ) ;

  // Write Each Byte of Config
  for ( Uint08_t i = 0 ; i < CONFIG_DATA_SIZE ; i++ ) {
    Configuration_WriteByte( i , ptrConfig[i] ) ;
    if ( _private_IsError( ) ) break ;
  }

  //Disable Writing
  _private_DisableWriting( ) ;

  // Enable All Interrupt
  _private_EnableInterrupt( ) ;

}

// ----------------------------------------------------------------
// [Function] Load
void Configuration_Load( StConfigurationData* config ) {

  EepromData_t* ptrConfig = (EepromData_t*) config ;

  // Disable All Interrupt
  _private_DisableInterrupt( ) ;

  // Read Each Byte of Config
  for ( Uint08_t i = 0 ; i < CONFIG_DATA_SIZE ; i++ ) {
    ptrConfig[i] = Configuration_ReadByte( i ) ;
  }

  // Enable All Interrupt
  _private_EnableInterrupt( ) ;

}

// ----------------------------------------------------------------
// [Function] Write Byte
PRIVATE void Configuration_WriteByte( EepromAddress_t address , EepromData_t data ) {
  _private_SetAddress( address ) ;
  _private_SetData( data ) ;
  _private_UnlockFlashProgram( ) ;
  _private_StartWrite( ) ;
  _private_WaitWriting( ) ;
}

// ----------------------------------------------------------------
// [Function] Read Byte
PRIVATE EepromData_t Configuration_ReadByte( EepromAddress_t address ) {
  _private_SetAddress( address ) ;
  _private_ConfugureReading( ) ;
  _private_StartRead( ) ;
  NOP( ) ;
  NOP( ) ;
  return _private_Data( ) ;
}

#endif	/* CONFIGURATION_H */
