#ifndef MESSAGE_H
#  define MESSAGE_H

//--------------------------------
// Message
const struct {
  struct {
    const char* TILE ;
    const char* MUTE ;
    const char* TEMPO ;
  } METRONOME ;
  struct {
    const char* TITLE ;
    const char* BEAT_COUNT ;
    const char* TONE ;
    const char* DURATION ;
    const char* PULSE_WIDTH ;
    const char* OSCILLATOR_TUNE ;
  } CONFIGURATION ;
  struct {
    const char* LOAD ;
    const char* SAVE ;
    const char* RESET ;
    const char* NO ;
    const char* YES ;
  } CONFIRM ;
  struct {
    const char* LOAD ;
    const char* SAVE ;
    const char* LOAD_DEFAULT ;
  } MEMORY ;
  struct {
    const char* MESSAGE ;
  } ERROR ;
}
MESSAGE = {
  {
    "Metronome" ,
    "#Mute#" ,
    "Tempo" ,
  } ,
  {
    "Config" ,
    "Beat Count" ,
    "Tone" ,
    "Duration" ,
    "Pulse Width" ,
    "Osc. Tune" ,
  } ,
  {
    "Load ?" ,
    "Save ?" ,
    "Reset ?" ,
    "No " ,
    "Yes" ,
  } ,
  {
    "Loaded" ,
    "Saved" ,
    "Initialized" ,
  } ,
  {
    "ERROR !!" ,
  } ,
} ;

//--------------------------------
// Character
const struct {
  const char CURSOR_RIGHT ;
  const char CURSOR_UP ;
  const char CURSOR_DOWN ;
}
CHAR_CODE = {
  0x00 ,
  0x01 ,
  0x02 ,
} ;
const struct {
  const char CURSOR_RIGHT[8] ;
  const char CURSOR_UP[8] ;
  const char CURSOR_DOWN[8] ;
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

