#ifndef MENU_H
#  define MENU_H

#  include "../../_Common/typedef.h"

//--------------------------------
// Main Menu
#  define MENU_SIZE_MAIN ( sizeof( MESSAGE_MENU_ITEM_MAIN ) / sizeof( MESSAGE_MENU_ITEM_MAIN[0] ) )
const Char_t* MESSAGE_MENU_ITEM_MAIN[] = {
  //23456789ABCDEF
  "< Return" ,
  "Beat Count" ,
  "Adj. Tone" ,
  "Adj. Duration" ,
  "Adj. P-Width" ,
  "Adj. Tune" ,
  "Load" ,
  "Save" ,
  "Info" ,
  "Reset" ,
} ;
enum {
  MENU_ITEM_MAIN_RETURN = 0 ,
  MENU_ITEM_MAIN_BEAT_COUNT ,
  MENU_ITEM_MAIN_TONE_MENU ,
  MENU_ITEM_MAIN_ADJUST_DURATION ,
  MENU_ITEM_MAIN_PULSE_WIDTH ,
  MENU_ITEM_MAIN_ADJUST_OSCILLATOR_TUNE ,
  MENU_ITEM_MAIN_LOAD_CONFIGURATION ,
  MENU_ITEM_MAIN_SAVE_CONFIGURATION ,
  MENU_ITEM_MAIN_INFORMATION ,
  MENU_ITEM_MAIN_RESET ,
} ;

//--------------------------------
// Tone Menu
#  define MENU_SIZE_TONE ( sizeof( MESSAGE_MENU_ITEM_TONE ) / sizeof( MESSAGE_MENU_ITEM_TONE[0] ) )
const Char_t* MESSAGE_MENU_ITEM_TONE[] = {
  //23456789ABCDEF
  "< Return" ,
  "Tone0" ,
  "Tone1" ,
  "Tone2" ,
} ;
enum {
  MENU_ITEM_TONE_RETURN = 0 ,
  MENU_ITEM_TONE_ADJUST_TONE0 ,
  MENU_ITEM_TONE_ADJUST_TONE1 ,
  MENU_ITEM_TONE_ADJUST_TONE2 ,
} ;

//--------------------------------
// Duration Menu
#  define MENU_SIZE_DURATION ( sizeof( MESSAGE_MENU_ITEM_DURATION ) / sizeof( MESSAGE_MENU_ITEM_DURATION[0] ) )
const Char_t* MESSAGE_MENU_ITEM_DURATION[] = {
  //23456789ABCDEF
  "< Return" ,
  "Click Sound" ,
  "Key Sound" ,
} ;
enum {
  MENU_ITEM_DURATION_RETURN = 0 ,
  MENU_ITEM_DURATION_ADJUST_CLICK ,
  MENU_ITEM_DURATION_ADJUST_KEY ,
} ;

//--------------------------------
// Information
#define MENU_SIZE_INFORMATION ( sizeof ( MESSAGE_INFORMATION ) / sizeof ( MESSAGE_INFORMATION[0] ) )
const Char_t* MESSAGE_INFORMATION[] = {
  //23456789ABCDEF
  "Version" ,
  "ROM Offset" ,
  "Save Count" ,
  "Error Code" ,
} ;
Char_t informationValueBuffer[][7] = {
  " --.--" ,
  "  0x--" ,
  "  0x--" ,
  "  0x00" ,
} ;
enum {
  INFORMATION_ITEM_VERSION = 0 ,
  INFORMATION_ITEM_ROM_OFFSET ,
  INFORMATION_ITEM_WRITE_COUNT ,
  INFORMATION_ITEM_ERROR_CODE ,
} ;

//--------------------------------
// Confirm Menu
#define MENU_SIZE_CONFIRM 2
enum {
  MENU_ITEM_CONFIRM_NO = 0 ,
  MENU_ITEM_CONFIRM_YES ,
} ;

#endif	/* MENU_H */
