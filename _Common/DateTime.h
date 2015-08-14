#ifndef DATE_TIME_H
#  define DATE_TIME_H

// Date Select
typedef enum {
  DATE_ITEM_YEAR = 0 ,
  DATE_ITEM_MONTH ,
  DATE_ITEM_DATE ,
  DATE_ITEM_DAY_OF_WEEK ,
  DATE_ITEM_HOUR ,
  DATE_ITEM_MINUTE ,
  DATE_ITEM_SECOND ,
} EnDateItem ;

#  define CHAR_SEPARATE_DATE '-'
#  define CHAR_SEPARATE_TIME ':'

// Day of Week
const char* STR_DAY_OF_WEEK[] = {
  "SUN" ,
  "MON" ,
  "TUE" ,
  "WED" ,
  "THU" ,
  "FRI" ,
  "SAT" ,
} ;

// Date Time
typedef union {
  struct {
    Uint08_t second ;
    Uint08_t minute ;
    Uint08_t hour ;
    Uint08_t dayOfWeek ;
    Uint08_t day ;
    Uint08_t month ;
    Uint08_t year ;
  } ;
  struct {
    unsigned : 7 ;
    unsigned clockHalt : 1 ;
  } ;
  Uint08_t array[7] ;
} StDateTime ;

// [Function] Convert Byte to Date Time ----------------
void _date_time_ConvertByteToDateTime( StDateTime* dateTime , char* string ) {

  string[16] = 0 ;

  string[13] = CHAR_SEPARATE_TIME ;
  string[10] = CHAR_SEPARATE_TIME ;
  string[5] = CHAR_SEPARATE_DATE ;
  string[2] = CHAR_SEPARATE_DATE ;

  // Second
  string[15] = ( dateTime->second & 0x0F ) | '0' ;
  string[14] = ( dateTime->second >> 4 ) | '0' ;
  // Minute
  string[12] = ( dateTime->minute & 0x0F ) | '0' ;
  string[11] = ( dateTime->minute >> 4 ) | '0' ;
  // Hour
  string[9] = ( dateTime->hour & 0x0F ) | '0' ;
  string[8] = ( dateTime->hour >> 4 ) | '0' ;

  // Date
  string[7] = ( dateTime->day & 0x0F ) | '0' ;
  string[6] = ( dateTime->day >> 4 ) | '0' ;
  // Month
  string[4] = ( dateTime->month & 0x0F ) | '0' ;
  string[3] = ( dateTime->month >> 4 ) | '0' ;
  // Year
  string[1] = ( dateTime->year & 0x0F ) | '0' ;
  string[0] = ( dateTime->year >> 4 ) | '0' ;

}

// [Function] Convert Byte to Date ----------------
void _date_time_ConvertByteToDate( StDateTime* dateTime , char* stinrg ) {

  stinrg[16] = 0 ;
  stinrg[15] = ']' ;
  stinrg[11] = '[' ;
  stinrg[10] = ' ' ;
  stinrg[7] = CHAR_SEPARATE_DATE ;
  stinrg[4] = CHAR_SEPARATE_DATE ;

  // Date of Week
  stinrg[14] = STR_DAY_OF_WEEK[dateTime->dayOfWeek][2] ;
  stinrg[13] = STR_DAY_OF_WEEK[dateTime->dayOfWeek][1] ;
  stinrg[12] = STR_DAY_OF_WEEK[dateTime->dayOfWeek][0] ;
  // Date
  stinrg[9] = ( dateTime->day & 0x0F ) | '0' ;
  stinrg[8] = ( dateTime->day >> 4 ) | '0' ;
  // Month
  stinrg[6] = ( dateTime->month & 0x0F ) | '0' ;
  stinrg[5] = ( dateTime->month >> 4 ) | '0' ;
  // Year
  stinrg[3] = ( dateTime->year & 0x0F ) | '0' ;
  stinrg[2] = ( dateTime->year >> 4 ) | '0' ;
  stinrg[1] = '0' ;
  stinrg[0] = '2' ;

}

// [Function] Convert Byte to Date Time ----------------
void _date_time_ConvertByteToTime( StDateTime* dateTime , char* string ) {

  string[8] = 0 ;

  string[5] = CHAR_SEPARATE_TIME ;
  string[2] = CHAR_SEPARATE_TIME ;

  // Second
  string[7] = ( dateTime->second & 0x0F ) | '0' ;
  string[6] = ( dateTime->second >> 4 ) | '0' ;
  // Minute
  string[4] = ( dateTime->minute & 0x0F ) | '0' ;
  string[3] = ( dateTime->minute >> 4 ) | '0' ;
  // Hour
  string[1] = ( dateTime->hour & 0x0F ) | '0' ;
  string[0] = ( dateTime->hour >> 4 ) | '0' ;

}

// [Function] Convert Byte to Discrete ----------------
void _date_time_ConvertByteToDiscrete( StDateTime* dateTime , char* string , EnDateItem select ) {

  string[2] = 0 ;

  switch ( select ) {
    case DATE_ITEM_YEAR:
      string[1] = ( dateTime->year & 0x0F ) | '0' ;
      string[0] = ( dateTime->year >> 4 ) | '0' ;
      break ;
    case DATE_ITEM_MONTH:
      string[1] = ( dateTime->month & 0x0F ) | '0' ;
      string[0] = ( dateTime->month >> 4 ) | '0' ;
      break ;
    case DATE_ITEM_DATE:
      string[1] = ( dateTime->day & 0x0F ) | '0' ;
      string[0] = ( dateTime->day >> 4 ) | '0' ;
      break ;
    case DATE_ITEM_DAY_OF_WEEK:
      string[0] = STR_DAY_OF_WEEK[dateTime->dayOfWeek][0] ;
      string[1] = STR_DAY_OF_WEEK[dateTime->dayOfWeek][1] ;
      string[2] = STR_DAY_OF_WEEK[dateTime->dayOfWeek][2] ;
      string[3] = 0 ;
      break ;
    case DATE_ITEM_HOUR:
      string[1] = ( dateTime->hour & 0x0F ) | '0' ;
      string[0] = ( dateTime->hour >> 4 ) | '0' ;
      break ;
    case DATE_ITEM_MINUTE:
      string[1] = ( dateTime->minute & 0x0F ) | '0' ;
      string[0] = ( dateTime->minute >> 4 ) | '0' ;
      break ;
    case DATE_ITEM_SECOND:
      string[1] = ( dateTime->second & 0x0F ) | '0' ;
      string[0] = ( dateTime->second >> 4 ) | '0' ;
      break ;
  }

}

#endif	/* DATE_TIME_H */

