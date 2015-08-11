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
  uint08_t writeCount ;
  uint16_t tempo ;
  uint08_t beatCount ;
  uint08_t duration ;
  uint08_t pulseWidth ;
  uint08_t tone[3] ;
  sint08_t oscillatorTune ;
  uint08_t checkSum ;
} ConfigurationData ;
#  define CONFIG_DEFAULT { 0 , 120 , 4 , 32 , 1 , { 249 , 62 , 82 , }, 0 , 0 }
#  define CONFIG_DATA_SIZE sizeof(ConfigurationData)

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
void _configuration_WriteByte( uint08_t address , uint08_t data ) ;
uint08_t _configuration_ReadByte( uint08_t address , MemorySelect memorySelect ) ;

// ----------------------------------------------------------------
// [Function] Save
ReturnCode _configuration_Save( ConfigurationData* config ) {

  uint08_t* ptrConfig = (uint08_t*) config ;
  uint08_t romOffset ;
  ReturnCode returnCode = RETURN_CODE_NOERROR ;

  // Disable All Interrupt
  _private_DisableInterrupt( ) ;

  // Read ROM Position
  romOffset = _configuration_ReadByte( ADDRESS_OF_OFFSET , MEMORY_SELECT_EEPROM ) ;

  // Read Write Count
  config->writeCount = _configuration_ReadByte( romOffset , MEMORY_SELECT_EEPROM ) ;

  // Shift ROM Position ( for Wear Leveling )
  if ( ++config->writeCount == 1 ) {
    romOffset += CONFIG_DATA_SIZE ;
    config->writeCount = 1 ;
  }

  if ( romOffset >= ( SIZE_OF_EEPROM - CONFIG_DATA_SIZE ) )
    romOffset = 1 ;

  // Enable Writing
  _private_EnableWriting( ) ;

  config->checkSum = 0x00 ;

  // Write Each Byte of Config
  for ( uint08_t i = 0 ; i != CONFIG_DATA_SIZE ; i++ ) {
    _configuration_WriteByte( romOffset + i , ptrConfig[i] ) ;
    config->checkSum ^= ptrConfig[i] ;
    if ( _private_IsError( ) ) {
      returnCode = RETURN_CODE_WRITE_ERROR ;
      break ;
    }
  }

  // Write Position
  if ( ( config->writeCount == 1 ) && ( !_private_IsError( ) ) ) {
    _configuration_WriteByte( ADDRESS_OF_OFFSET , romOffset ) ;
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
ReturnCode _configuration_Load( ConfigurationData* config ) {

  uint08_t* ptrConfig = (uint08_t*) config ;
  uint08_t romOffset ;

  // Disable All Interrupt
  _private_DisableInterrupt( ) ;

  // Read ROM Position
  romOffset = _configuration_ReadByte( ADDRESS_OF_OFFSET , MEMORY_SELECT_EEPROM ) ;

  // Invalid Offser
  if ( romOffset >= ( SIZE_OF_EEPROM - CONFIG_DATA_SIZE ) ) return RETURN_CODE_INVALID_OFFSET ;

  config->checkSum = 0x00 ;

  // Read Each Byte of Config
  for ( uint08_t i = 0 ; i != CONFIG_DATA_SIZE ; i++ ) {
    ptrConfig[i] = _configuration_ReadByte( romOffset + i , MEMORY_SELECT_EEPROM ) ;
    config->checkSum ^= ptrConfig[i] ;
  }

  // Enable All Interrupt
  _private_EnableInterrupt( ) ;

  // Checksum
  if ( config->checkSum ) return RETURN_CODE_CHECKSUM_ERROR ;

  return RETURN_CODE_NOERROR ;
}

// ----------------------------------------------------------------
// [Function] Get Rom Offset
uint08_t _configuration_GetRomOffset( ) {
  return _configuration_ReadByte( ADDRESS_OF_OFFSET , MEMORY_SELECT_EEPROM ) ;
}

// ----------------------------------------------------------------
// [Function] Write Byte
void _configuration_WriteByte( uint08_t address , uint08_t data ) {
  _private_SetAddress( address ) ;
  _private_SetData( data ) ;
  _private_UnlockFlashProgram( ) ;
  _private_StartWrite( ) ;
  _private_WaitWriting( ) ;
}

// ----------------------------------------------------------------
// [Function] Read Byte
uint08_t _configuration_ReadByte( uint08_t address , MemorySelect memorySelect ) {
  _private_SetAddress( address ) ;
  _private_ConfugureReading( memorySelect ) ;
  _private_StartRead( ) ;
  NOP( ) ;
  NOP( ) ;
  return _private_Data( ) ;
}

#endif	/* CONFIGURATION_H */
