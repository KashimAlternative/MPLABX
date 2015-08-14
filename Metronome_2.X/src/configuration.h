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

#  define _private_ConfugureReading( memorySelect ) EECON1bits.EEPGD=0;if(memorySelect)EECON1bits.CFGS=1;else EECON1bits.CFGS=0;
#  define _private_StartRead() EECON1bits.RD=1;
#  define _private_Data() EEDATL

// ----------------------------------------------------------------
// [Struct] Configuration
#  define SIZE_OF_EEPROM 0x100
#  define ADDRESS_OF_OFFSET 0x00
typedef struct {
  Uint08_t writeCount ; // must be first
  Uint08_t romOffset ;
  Uint16_t tempo ;
  Uint08_t beatCount ;
  struct {
    Uint08_t click ;
    Uint08_t key ;
  } duration ;
  Uint08_t tone[3] ;
  Uint08_t pulseWidth ;
  Sint08_t oscillatorTune ;
  Uint08_t checkSum ; // must be last
} ConfigurationData ;
#  define CONFIG_DEFAULT { 0 , 0 , 120 , 4 , { 16 , 16 , } , { 249 , 62 , 82 , } , 1 , 0 , 0 }
#  define CONFIG_DATA_SIZE sizeof(ConfigurationData)

// ----------------------------------------------------------------
// [Type-Def]
typedef Uint08_t EepromData_t ;
typedef Uint08_t EepromAddress_t ;

// ----------------------------------------------------------------
// [Enum] Return Code
typedef enum {
  RETURN_CODE_NOERROR = 0x00 ,
  RETURN_CODE_WRITE_ERROR = 0x10 ,
  RETURN_CODE_CHECKSUM_ERROR = 0x20 ,
  RETURN_CODE_INVALID_OFFSET = 0x30 ,
} ReturnCode ;
typedef enum {
  MEMORY_SELECT_EEPROM = 0 ,
  MEMORY_SELECT_CONFIGURATION ,
} MemorySelect ;

// ----------------------------------------------------------------
// [Prototypes]
PRIVATE void Configuration_WriteByte( EepromAddress_t address , EepromData_t data ) ;
PRIVATE EepromData_t Configuration_ReadByte( EepromAddress_t address , MemorySelect memorySelect ) ;

// ----------------------------------------------------------------
// [Function] Save
ReturnCode Configuration_Save( ConfigurationData* config ) {

  EepromData_t* ptrConfig = (EepromData_t*) config ;
  ReturnCode returnCode = RETURN_CODE_NOERROR ;

  // Disable All Interrupt
  _private_DisableInterrupt( ) ;

  // Read ROM Position
  config->romOffset = Configuration_ReadByte( ADDRESS_OF_OFFSET , MEMORY_SELECT_EEPROM ) ;

  // Read Write Count
  config->writeCount = Configuration_ReadByte( config->romOffset , MEMORY_SELECT_EEPROM ) ;

  // Shift ROM Position ( for Wear Leveling )
  if ( ++config->writeCount == 1 ) {
    config->romOffset += CONFIG_DATA_SIZE ;
    config->writeCount = 1 ;
  }

  if ( config->romOffset >= ( SIZE_OF_EEPROM - CONFIG_DATA_SIZE ) )
    config->romOffset = 1 ;

  // Enable Writing
  _private_EnableWriting( ) ;

  config->checkSum = 0x00 ;

  // Write Each Byte of Config
  for ( Uint08_t i = 0 ; i < CONFIG_DATA_SIZE ; i++ ) {
    Configuration_WriteByte( config->romOffset + i , ptrConfig[i] ) ;
    config->checkSum ^= ptrConfig[i] ;
    if ( _private_IsError( ) ) {
      returnCode = RETURN_CODE_WRITE_ERROR ;
      break ;
    }
  }

  // Write Position
  if ( ( config->writeCount == 1 ) && ( !_private_IsError( ) ) ) {
    Configuration_WriteByte( ADDRESS_OF_OFFSET , config->romOffset ) ;
    if ( _private_IsError( ) ) returnCode = RETURN_CODE_WRITE_ERROR ;
  }

  //Disable Writing
  _private_DisableWriting( ) ;

  // Enable All Interrupt
  _private_EnableInterrupt( ) ;

  return RETURN_CODE_NOERROR ;

}

// ----------------------------------------------------------------
// [Function] Load
ReturnCode Configuration_Load( ConfigurationData* config ) {

  EepromData_t* ptrConfig = (EepromData_t*) config ;

  // Disable All Interrupt
  _private_DisableInterrupt( ) ;

  // Read ROM Position
  config->romOffset = Configuration_ReadByte( ADDRESS_OF_OFFSET , MEMORY_SELECT_EEPROM ) ;

  // Invalid Offser
  if ( config->romOffset >= ( SIZE_OF_EEPROM - CONFIG_DATA_SIZE ) ) return RETURN_CODE_INVALID_OFFSET ;

  config->checkSum = 0x00 ;

  // Read Each Byte of Config
  for ( Uint08_t i = 0 ; i < CONFIG_DATA_SIZE ; i++ ) {
    ptrConfig[i] = Configuration_ReadByte( config->romOffset + i , MEMORY_SELECT_EEPROM ) ;
    config->checkSum ^= ptrConfig[i] ;
  }

  // Enable All Interrupt
  _private_EnableInterrupt( ) ;

  // Checksum
  if ( config->checkSum ) return RETURN_CODE_CHECKSUM_ERROR ;

  return RETURN_CODE_NOERROR ;
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
PRIVATE EepromData_t Configuration_ReadByte( EepromAddress_t address , MemorySelect memorySelect ) {
  _private_SetAddress( address ) ;
  _private_ConfugureReading( memorySelect ) ;
  _private_StartRead( ) ;
  NOP( ) ;
  NOP( ) ;
  return _private_Data( ) ;
}

#endif	/* CONFIGURATION_H */
