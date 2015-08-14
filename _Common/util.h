/* 
 * File:   util.h
 * Author: kshimada
 *
 * Created on 2015/08/09, 11:22
 */

#ifndef UTIL_H
#  define	UTIL_H

// ----------------------------------------------------------------
// [Define] Event
#define SetEvent(event) event=1
#define ClearEvent(event) event=0
#define EvalEvent(event) (event&&!(event=0))

// ----------------------------------------------------------------
// [Define] Toggle
#define ToggleBool( bit ) {if(bit){bit=0;}else{bit=1;}}

#endif	/* UTIL_H */

