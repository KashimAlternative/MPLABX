#ifndef UTIL_H
#  define	UTIL_H

// ----------------------------------------------------------------
// [Define] Event
#  define SetEvent(event) event=1
#  define ClearEvent(event) event=0
#  define EvalEvent(event) (event&&!(event=0))
// ################################################################
// Ignore Warning for "possible use of "=" instead of "=="
#  pragma warning disable 758
// ################################################################

// ----------------------------------------------------------------
// [Define] Toggle
#  define ToggleBool( bit ) {if(bit){bit=0;}else{bit=1;}}

#endif	/* UTIL_H */

