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
// [Type-Def] Bool
typedef enum {
  BOOL_FALSE = 0 ,
  BOOL_TRUE = 1 ,
} EnBool ;
typedef EnBool Bool_t;

// ----------------------------------------------------------------
// [Type-Def] Unsigned Integer
typedef unsigned char uint08_t ;
typedef unsigned int uint16_t ;
typedef unsigned short long uint24_t ;
typedef unsigned long uint32_t ;

// ----------------------------------------------------------------
// [Type-Def] Signed Integer
typedef signed char sint08_t ;
typedef signed int sint16_t ;
typedef signed short long sint24_t ;
typedef signed long sint32_t ;

#endif	/* TYPEDEF_H */

