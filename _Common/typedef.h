// ----------------------------------------------------------------
// Type Def
// Revision 2015/03/04

#ifndef TYPEDEF_H
#  define TYPEDEF_H

// ----------------------------------------------------------------
// [define] NULL
#  define NULL 0

// ----------------------------------------------------------------
// [Type-Def] Bool
typedef enum {
  BOOL_FALSE = 0 ,
  BOOL_TRUE = 1 ,
} Bool ;

// ----------------------------------------------------------------
// [Type-Def] Bit
typedef enum {
  BIT_CLEAR = 0 ,
  BIT_SET = 1 ,
} Bit ;

// ----------------------------------------------------------------
// [Type-Def] Unsigned Integer
typedef unsigned char uint08 ;
typedef unsigned int uint16 ;
typedef unsigned short long uint24 ;
typedef unsigned long uint32 ;

// ----------------------------------------------------------------
// [Type-Def] Signed Integer
typedef signed char sint08 ;
typedef signed int sint16 ;
typedef signed short long uint24 ;
typedef signed long sint32 ;

#endif	/* TYPEDEF_H */

