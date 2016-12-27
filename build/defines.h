/*
 * defines.h
 *
 *  Created on: 2016-05-21
 *      Author: Felix
 */

#ifndef DEFINES_H_
#define DEFINES_H_

#define WRAP_FFMPEG 1
#define PRINT_PROCESSED 1
#define ENABLE_QUIT 1
#define FIX_PIXEL_FORMAT 1
#define DIGIT_ONLY_OPTION 1
#define OUTPUT_FILENAME_LEN 2048

#define MAP_SUPPORT_STREAMTYPE 1 // support like -map 0:p:1:v
#define DEBUG_LOGS 1

//#ifdef HAVE_ARC4RANDOM
//#undef HAVE_ARC4RANDOM
//#define HAVE_ARC4RANDOM 0
//#endif
//
//#ifdef HAVE_GETTIMEOFDAY
//#undef HAVE_GETTIMEOFDAY
//#define HAVE_GETTIMEOFDAY 0
//#endif
//
//#ifdef HAVE_GLOB
//#undef HAVE_GLOB
//#define HAVE_GLOB 0
//#endif
//
//disable docs
#ifdef CONFIG_DOC
#undef CONFIG_DOC
#define CONFIG_DOC 0
#endif

//fix the terminal problem: when error occurs the terminal will not display the command lines
#ifdef HAVE_TERMIOS_H
#undef HAVE_TERMIOS_H
#define HAVE_TERMIOS_H 0
#endif

#endif /* DEFINES_H_ */
