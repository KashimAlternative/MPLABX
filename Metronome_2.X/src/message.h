#ifndef MESSAGE_H
#  define MESSAGE_H

//--------------------------------
// Message
const struct {
  const struct {
    const Char_t* MAIN_TILE ;
    const Char_t* MUTE ;
  } METRONOME ;
  const struct {
    const Char_t* NO ;
    const Char_t* YES ;
  } CONFIRM ;
  const struct {
    const Char_t* LOAD ;
    const Char_t* SAVE ;
    const Char_t* INITIALIZE ;
  } MEMORY ;
  const struct {
    const Char_t* TITLE ;
    const Char_t* EEPROM ;
    const Char_t* INTERRUPT ;
  } ERROR ;
}
MESSAGE = {
  {
    "Metronome" ,
    "#Mute#" ,
  } ,
  {
    "No " ,
    "Yes" ,
  } ,
  {
    "Loaded" ,
    "Saved" ,
    "Initialized" ,
  } ,
  {
    "Error on ..." ,
    "EEPROM" ,
    "Interrupt" ,
  } ,
} ;

//--------------------------------
// Character
const struct {
  const Char_t CURSOR_RIGHT ;
  const Char_t CURSOR_UP ;
  const Char_t CURSOR_DOWN ;
}
CHAR_CODE = {
  0x00 ,
  0x01 ,
  0x02 ,
} ;
const struct {
  const Char_t CURSOR_RIGHT[8] ;
  const Char_t CURSOR_UP[8] ;
  const Char_t CURSOR_DOWN[8] ;
}
BITMAP = {
  {
    0b10000 ,
    0b11000 ,
    0b11100 ,
    0b11110 ,
    0b11100 ,
    0b11000 ,
    0b10000 ,
    0b00000 ,
  } ,
  {
    0b00000 ,
    0b00100 ,
    0b01110 ,
    0b11111 ,
    0b00000 ,
    0b00000 ,
    0b00000 ,
    0b00000 ,
  } ,
  {
    0b00000 ,
    0b00000 ,
    0b00000 ,
    0b00000 ,
    0b11111 ,
    0b01110 ,
    0b00100 ,
    0b00000 ,
  } ,

} ;

#endif	/* MESSAGE_H */

