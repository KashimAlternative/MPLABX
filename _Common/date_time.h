#ifndef DATE_TIME_H
#  define DATE_TIME_H

// Date Select
typedef enum {
  CLOCK_SELECT_YEAR = 0 ,
  CLOCK_SELECT_MONTH ,
  CLOCK_SELECT_DATE ,
  CLOCK_SELECT_DAY_OF_WEEK ,
  CLOCK_SELECT_HOUR ,
  CLOCK_SELECT_MINUTE ,
  CLOCK_SELECT_SECOND ,
} DateSelect ;

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
    uint08 second ;
    uint08 minute ;
    uint08 hour ;
    uint08 dayOfWeek ;
    uint08 day ;
    uint08 month ;
    uint08 year ;
  } ;
  struct {
    uint08 _ : 7 ;
    uint08 clockHalt : 1 ;
  } ;
  uint08 array[7] ;
} DateTime ;

// [Function] Convert Byte to Date Time ----------------
void _date_time_ConvertByteToDateTime( DateTime* dateTime , char* string ) {

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
void _date_time_ConvertByteToDate( DateTime* dateTime , char* stinrg ) {

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
void _date_time_ConvertByteToTime( DateTime* dateTime , char* string ) {

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
void _date_time_ConvertByteToDiscrete( DateTime* dateTime , char* string , DateSelect select ) {

  string[2] = 0 ;

  switch ( select ) {
    case CLOCK_SELECT_YEAR:
      string[1] = ( dateTime->year & 0x0F ) | '0' ;
      string[0] = ( dateTime->year >> 4 ) | '0' ;
      break ;
    case CLOCK_SELECT_MONTH:
      string[1] = ( dateTime->month & 0x0F ) | '0' ;
      string[0] = ( dateTime->month >> 4 ) | '0' ;
      break ;
    case CLOCK_SELECT_DATE:
      string[1] = ( dateTime->day & 0x0F ) | '0' ;
      string[0] = ( dateTime->day >> 4 ) | '0' ;
      break ;
    case CLOCK_SELECT_DAY_OF_WEEK:
      string[0] = STR_DAY_OF_WEEK[dateTime->dayOfWeek][0] ;
      string[1] = STR_DAY_OF_WEEK[dateTime->dayOfWeek][1] ;
      string[2] = STR_DAY_OF_WEEK[dateTime->dayOfWeek][2] ;
      string[3] = 0 ;
      break ;
    case CLOCK_SELECT_HOUR:
      string[1] = ( dateTime->hour & 0x0F ) | '0' ;
      string[0] = ( dateTime->hour >> 4 ) | '0' ;
      break ;
    case CLOCK_SELECT_MINUTE:
      string[1] = ( dateTime->minute & 0x0F ) | '0' ;
      string[0] = ( dateTime->minute >> 4 ) | '0' ;
      break ;
    case CLOCK_SELECT_SECOND:
      string[1] = ( dateTime->second & 0x0F ) | '0' ;
      string[0] = ( dateTime->second >> 4 ) | '0' ;
      break ;
  }

}

#endif	/* DATE_TIME_H */
