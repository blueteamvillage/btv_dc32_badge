#ifndef _IMAGE_H_
#define _IMAGE_H_
//#include <avr/pgmspace.h>
#ifdef ARDUINO_ARCH_AVR
#include <avr/pgmspace.h>
#else
#include <pgmspace.h>
#endif

extern PROGMEM const unsigned char gImage_70X70[];


#endif

