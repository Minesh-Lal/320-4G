/*
	Progressive Resources LLC

			FlashFile

	Version : 	2.11
	Date: 		08/25/2005
	Author: 	Erick M. Higa

	Software License
	The use of Progressive Resources LLC FlashFile Source Package indicates
	your understanding and acceptance of the following terms and conditions.
	This license shall supersede any verbal or prior verbal or written, statement
	or agreement to the contrary. If you do not understand or accept these terms,
	or your local regulations prohibit "after sale" license agreements or limited
	disclaimers, you must cease and desist using this product immediately.
	This product is � Copyright 2003 by Progressive Resources LLC, all rights
	reserved. International copyright laws, international treaties and all other
	applicable national or international laws protect this product. This software
	product and documentation may not, in whole or in part, be copied, photocopied,
	translated, or reduced to any electronic medium or machine readable form, without
	prior consent in writing, from Progressive Resources LLC and according to all
	applicable laws. The sole owner of this product is Progressive Resources LLC.

	Operating License
	You have the non-exclusive right to use any enclosed product but have no right
	to distribute it as a source code product without the express written permission
	of Progressive Resources LLC. Use over a "local area network" (within the same
	locale) is permitted provided that only a single person, on a single computer
	uses the product at a time. Use over a "wide area network" (outside the same
	locale) is strictly prohibited under any and all circumstances.

	Liability Disclaimer
	This product and/or license is provided as is, without any representation or
	warranty of any kind, either express or implied, including without limitation
	any representations or endorsements regarding the use of, the results of, or
	performance of the product, Its appropriateness, accuracy, reliability, or
	correctness. The user and/or licensee assume the entire risk as to the use of
	this product. Progressive Resources LLC does not assume liability for the use
	of this product beyond the original purchase price of the software. In no event
	will Progressive Resources LLC be liable for additional direct or indirect
	damages including any lost profits, lost savings, or other incidental or
	consequential damages arising from any defects, or the use or inability to
	use these products, even if Progressive Resources LLC have been advised of
	the possibility of such damages.
*/

#ifndef _OPTIONS_H_
	#define _OPTIONS_H_
// Control Block //
	//#define _RTC_ON_
	#define _SECOND_FAT_ON_
//	#define _FAT12_ON_
//	#define _READ_ONLY_
//	#define _DEBUG_ON_				//SJL - CAVR2 - debug
	#define _DIRECTORIES_SUPPORTED_	//SJL - CAVR2 - compiler wasn't seeing this defined here from file_sys.h so
    								//have disabled all #ifdef in file_sys.h
	#define _NO_MALLOC_				//SJL - CAVR2 - debug moved to file_sys.h
	#define _BytsPerSec_512_
	#define	_LITTLE_ENDIAN_
//	#define _BIG_ENDIAN_

	// The settings below should be modified
	// to match your hardware/software settings
	#define _CVAVR_
//	#define _ICCAVR_
//	#define _IAR_EWAVR_

	#define _SD_MMC_MEDIA_
//	#define _CF_MEDIA_

//	#define _MEGA128DEV_
//	#define _MEGAAVRDEV_

  	#ifdef _NO_MALLOC_
		#define _FF_MAX_FILES_OPEN	1
  	#endif

	#define SD_CS_OFF()		PORTB |= 0x10
	#define SD_CS_ON()		PORTB &= 0xEF
	#define CS_DDR_SET()	DDRB |= 0x10

	#define _FF_MAX_FPRINTF		50
	//#define _FF_PATH_LENGTH		50	//SJL - CAVR2 - moved to sd_cmd.h

  #ifdef _DEBUG_ON_
	#include <stdio.h>
  #endif

	#include <mega128.h>
	#define CLI()	#asm("cli")
	#define SEI()	#asm("sei")
	#include <stdarg.h>



  //#ifndef _NO_MALLOC_
	#include <stdlib.h>
  //#endif
	#include <ctype.h>
	#include <string.h>
	#include <stdio.h>

	#include "flash\sd_cmd.h"
	#include "flash\file_sys.h"
  #ifdef _RTC_ON_
	#include "flash\twi.h"
  #endif
  /* Comment out including .c files as header files - the header files
  	 sd_cmd.h and file_sys.c  are now included individually

	#include "flash\sd_cmd.c"
	#include "flash\file_sys.c"
  	#ifdef _RTC_ON_
		#include "flash\twi.c"
  	#endif
  */
#endif
