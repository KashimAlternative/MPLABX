// ----------------------------------------------------------------
// Type Def

#ifndef TYPEDEF_H
#  define TYPEDEF_H

// ----------------------------------------------------------------
// [define] NULL
#  define NULL 0

// ----------------------------------------------------------------
// [Define] Private
#define PRIVATE static

// ----------------------------------------------------------------
// [Type-Def] Unsigned Integer
typedef unsigned char Uint08_t ;
typedef unsigned int Uint16_t ;
typedef unsigned short long Uint24_t ;
typedef unsigned long Uint32_t ;

// ----------------------------------------------------------------
// [Type-Def] Signed Integer
typedef signed char Sint08_t ;
typedef signed int Sint16_t ;
typedef signed short long Sint24_t ;
typedef signed long Sint32_t ;

// ----------------------------------------------------------------
// [Type-Def] Character
typedef char Char_t;

// ----------------------------------------------------------------
// [Type-Def] Bool
typedef enum {
  BOOL_FALSE = 0 ,
  BOOL_TRUE = 1 ,
} EnBool ;
typedef EnBool Bool_t;

#endif	/* TYPEDEF_H */

