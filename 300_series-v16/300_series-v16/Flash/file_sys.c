/*
	Progressive Resources LLC

			FlashFile

	Version : 	2.12
	Date: 		10/04/2005
	Author: 	Erick M. Higa

	Revision History:
	10/04/2005 - EMH - v2.12
					 - Fixed bug in fquickformat() that wouuld not
					 - Fixed bug in fcreate() (in v2.11) that would destroy a cluster chain even if it doesn't
					   exist.
	08/25/2005 - EMH - v2.11
					 - Fixed bug in fcreate() that caused lost clusters if the file previously existed.
	06/07/2005 - EMH - v2.10
					 - Major change in accessing directory entries using structures and unions to cut down on
					   code size.  Effects all functions that access the directory entries (scan_directory(),
					   read_directory(), append_toc(), mkdir(), rmdir(), chdir(), _FF_chdir(), fopen(), fcreate(),
					   remove(), and rename()).
					 - Changed internal functions so that most internal FlashFile functions will return a 0 upon
					   success, and an EOF on a failure.
					 - Fixed bug in sub directory functions that was not checking for directory entries after
					   the first cluster of entries.
	03/11/2005 - EMH - v2.03
					 - Updated "options.h" to define _FF_MAX_FILES_OPEN, for use when malloc() is not used
					   (_NO_MALLOC_ is #defined) so the FlashFile will automatically allocate the n FILE
					   structures into global variable space, where n is the number of files to be opened
					   simultaneously defined by _FF_MAX_FILES_OPEN.
					 - Fixed bug in fopen() that caused the first sector of a file to be wrongly updated when
					   opened in APPEND mode for IAR and ICC users.
	02/10/2005 - EMH - v2.02
					 - Fixed error in wrong files uploaded in v2.01
					 - Changed initialize_media() in "sd_cmd.c"/"cf_cmd.c" so that if an active partition is
					   not found in the partition table, it will assume that the first partition entry is
					   valid, and use that to calculate the Boot Sector of the disk.
					 - Changed "options.h" file to fix compile errors when using CodeVisionAVR v1.24.4 and
					   above.
	01/27/2005 - EMH - v2.01
					 - Fixed bug in scan_directory(), fopen(), fcreate(), rename(), and get_file_info() that
					   gave compile errors when _DIRECTORIES_SUPPORTED_ was not defined.
					 - Corrected return value of rename() to 0 when successful (previously returned a 1)
	11/01/2004 - EMH - v2.00
					 - Changing code to use structures and unions to optimize code to run faster
					   and use less code space.
					 - Fixed bug in fgets() that was reading too long if the line was >= n, and
					   it was returning the wrong pointer.
					 - Changing code to use sector addressing rather than direct memory addressing
					   to save code space and speed up time.
					 - Added _DIRECTORIES_SUPPORTED_ switch to options.h to enable or disable (less
					   code space) the use of directories.
					 - Added _NO_MALLOC_ switch to options.h to enable or disable the need of a
					   malloc() function.
					 - Added _BytsPerSec_512_ switch to options.h to enable or disable the BPB_BytsPerSec
					   variable.  In all current flash media, there are 512 bytes per sector, and there
					   really is no need to have it as a variable.  Enabling _BytsPerSec_512_ will hard
					   code all references to BPB_BytsPerSec as 0x200 or 512.  This will cut down on code
					   space and speed the functions up a bit since << 9 and >> 9 can replace * 512 and
					   / 512, and & 0x1FF can replace % 512.
					 - Fixed bug in addr_to_clust() that was checking if sector # was <= FirstDataSector
					   where it should just be < FirstDataSector.
					 - Added ability in initialize_media() for the calculations to look for the first
					   partition location in all four partition entry spaces rather than just the first.
					 - Fixed bug in scan_directory() that did not always 0xE5 as an empty file location.
					 - Fixed Directory support (scan_directory(), fcreate(), and mkdir()) so that multi
					   cluster sub-directories could be read.
					 - Fixed Initialization of the File Structure that was intermittantly causing errors in
					   fopen() and fcreate().
					 - Fixed bug in fcreatec(), fopenc(), and fget_file_infoc() that was not always handling
					   full length filenames of files correctly.
					 - Combined code so that CodeVision, ImageCraft, and IAR all use the same base code.
					 - Updated fprintf() in ImageCraft and IAR versions so that the use vsprintf and can be
					   used just like a _FF_printf().
					 - Added fread() and fwrite() functions.
	06/14/2004 - EMH - v1.41
					 - Fixed bug in "fflush()" that was incorrectly saving the last sector of a
					   cluster to the first if saving or closing a file at the end of file AND at
					   the end of a cluster.
	05/17/2004 - EMH - v1.40
					 - Added IAR EWAVR Support to the FlashFile
	05/17/2004 - EMH - v1.34
					 - Fixed bug "fopen()" that was incorrectly writing to 0xFE00 + _FF_DIR_ADDR
					   if opening a file in "WRITE" mode (bug since v1.22).
					 - Fixed bug in "append_toc()" that was incorrectly updating the time/date
					   stamp if _RTC_ON_ is NOT defined.
	05/07/2004 - EMH - v1.33
					 - Fixed bug in "fflush()" and "fopen()" that was incorrectly handling blank
					   files on certain MMC/SD cards.
	04/01/2004 - EMH - v1.32
					 - Fixed bug in "prev_cluster()" that didn't use updated FAT table address
					   calculations.  (effects XP formatted cards see v1.22 notes)
	03/26/2004 - EMH - v1.31
					 - Added define for easy MEGA128Dev board setup
					 - Changed demo projects so "option.h" is in the project directory
	03/10/2004 - EMH - v1.30
					 - Added support for a Compact Flash package
					 - Renamed read and write to flash function names for multiple media support
	03/02/2004 - EMH - v1.22 (unofficial release)
					 - Changed Directory Functions to allow for multi-cluster directory entries
					 - Added function addr_to_clust() to support long directories
					 - Fixed FAT table address calculation to support multiple reserved sectors
					   (previously) assumed one reserved sector, if XP formats card sometimes
					   multiple reserved sectors - thanks YW
	02/19/2004 - EMH - v1.21
					 - Replaced all "const" refrances to "flash" to support CodeVision 1.24.1b
	01/21/2004 - EMH - v1.20
			   	 	 - Added ICC Support to the FlashFileSD
					 - Fixed card initialization error for MMC/SD's that have only a boot
			   	 	   sector and no partition table
					 - Fixed intermittant error on fcreate when creating existing file
					 - Changed "options.h" to #include all required files
	01/19/2004 - EMH - v1.10
			   	 	 - Fixed FAT access errors by allowing both FAT tables to be updated
					 - Fixed erase_cluster chain to stop if chain goes to '0'
					 - Fixed #include's so other non m128 processors could be used
					 - Fixed fcreate to match 'C' standard for function "creat"
					 - Fixed fseek so it would not error when in "READ" mode
					 - Modified SPI interface to use _FF_spi() so it is more universal
					   (see the "sd_cmd.c" file for the function used)
					 - Redifined global variables and #defines for more unique names
					 - Added string functions fputs, fputsc, & fgets
					 - Added functions fquickformat, fgetfileinfo, & GetVolID()
					 - Added directory support
					 - Modified delays in "sd_cmd.c" to increase transfer speed to max
					 - Updated "options.h" to include additions, and to make #defines
					   more universal to multiple platforms
	12/31/2003 - EMH - v1.00
			   	 	 - Initial Release

	Software License
	The use of Progressive Resources LLC FlashFile Source Package indicates
	your understanding and acceptance of the following terms and conditions.
	This license shall supersede any verbal or prior verbal or written, statement
	or agreement to the contrary. If you do not understand or accept these terms,
	or your local regulations prohibit "after sale" license agreements or limited
	disclaimers, you must cease and desist using this product immediately.
	This product is © Copyright 2003 by Progressive Resources LLC, all rights
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

#if defined(_SD_MMC_MEDIA_)
 #include "flash\sd_cmd.h"
#elif defined(_CF_MEDIA_)
 #include "flash\cf_cmd.h"
#endif

#include "flash\file_sys.h"
#ifdef _RTC_ON_
  #include "flash\twi.h"
#endif

#include <stdlib.h>				//SJL - CAVR2 - required to access avr ports
#include <mega128.h>			//SJL - CAVR2 - required to access avr ports
#include "options.h"			//SJL - CAVR2 - required to access _FF_PATH_LENGTH
//#include "drivers\config.h"	//SJL - CAVR2 - required to access _FF_buff
//#include "drivers\uart.h"     //SJL - CAVR2 - required to access print
//#include "drivers\debug.h"	//SJL - CAVR2 - required to access DEBUG_printStr

//#ifdef _DEBUG_ON_
//  char flash _FF_VERSION_[] = "v2.11";
//#endif

unsigned char _FF_buff[512];	//config.c

#ifndef _BytsPerSec_512_
	uHILO16 BPB_BytsPerSec;		//SJL - CAVR2 - declared - sd_cmd.h
#endif
unsigned char BPB_SecPerClus;
uHILO16 BPB_RsvdSecCnt;			//SJL - CAVR2 - declared - sd_cmd.h
uHILO16 BPB_RootEntCnt;			//SJL - CAVR2 - declared - sd_cmd.h
uHILO16 BPB_FATSz16;			//SJL - CAVR2 - declared - sd_cmd.h
unsigned char BPB_FATType;
unsigned char BS_VolLab[12];
unsigned int _FF_PART_ADDR;
unsigned long _FF_ROOT_ADDR;
#ifndef _BIG_ENDIAN_
unsigned long BS_VolSerial;
#else
uHILO32 BS_VolSerial; 			//SJL - CAVR2 - declared - sd_cmd.h
#endif
#ifdef _DIRECTORIES_SUPPORTED_
unsigned long _FF_DIR_ADDR;
#endif
unsigned int _FF_FAT1_ADDR;
unsigned long _FF_FAT2_ADDR;
unsigned int FirstDataSector;
unsigned char _FF_error;
unsigned long _FF_buff_addr;
unsigned int DataClusTot;
#if defined(_CVAVR_)
bit NextClusterValidFlag;
#else
unsigned char NextClusterValidFlag;
#endif
#ifdef _RTC_ON_
unsigned char rtc_hour, rtc_min, rtc_sec;
unsigned char rtc_date, rtc_month;
unsigned int rtc_year;
#endif
unsigned char _FF_FULL_PATH[_FF_PATH_LENGTH];
unsigned int clus_0_counter;
unsigned long _FF_scan_addr;

#ifdef _DEBUG_ON_
  #define _DEBUG_FUNCTIONS_
#endif

#ifdef _NO_MALLOC_
FILE _FF_File_Space_Allocation[_FF_MAX_FILES_OPEN];

FILE *_FF_MALLOC(void)
{
	unsigned char c;

    //DEBUG_printStr("malloc disabled\r\n");

	for (c=0; c<_FF_MAX_FILES_OPEN; c++)
	{	// Look for the first unused area
		if (_FF_File_Space_Allocation[c].used_flag == 0)
		{	// If not in use, flag that it is in use and return pointer location
			_FF_File_Space_Allocation[c].used_flag = 1;
			return (&_FF_File_Space_Allocation[c]);
		}
	}

	return (NULL);	// All file allocated space is currently in use
}

#define _FF_free(qp)	qp->used_flag = 0

#else

#define _FF_MALLOC()	malloc(sizeof(FILE))
#define _FF_free		free

#endif

#ifndef _FF_STRING_DEFS_
  #define _FF_STRING_DEFS_

  #ifdef _CVAVR_
	#define		_FF_strcpyf		strcpyf
	#define		_FF_sprintf		sprintf
	#define		_FF_printf		printf
	#define		_FF_strlen		strlen
	#define		_FF_strncmp		strncmp
  #endif

  #ifdef _ICCAVR_
	#define		_FF_strcpyf		cstrcpy
	#define		_FF_sprintf		csprintf
	#define		_FF_printf		cprintf
	#define		_FF_strrchr		strrchr
	#define		_FF_strncmp		strncmp
	#define		_FF_strlen		strlen
  #endif

  #ifdef _IAR_EWAVR_
	#define		_FF_strcpyf		strcpy_P
	#define		_FF_sprintf		sprintf_P
	#define		_FF_printf		printf_P
  #endif
#endif

#ifdef _IAR_EWAVR_
int _FF_strncmp(unsigned char *data_buff1, unsigned char *data_buff2, int n)
{
    char c1, c2;

	for(; n !=0 ; --n)
	{
		c1 = *data_buff1;
		data_buff1++;
		c2 = *data_buff2;
		data_buff2++;
		if (c1 != c2)
			return c1 < c2 ? -1 : 1;
		else if (c1 == 0)
			return 0;
	}

	return n;
}

size_t _FF_strlen(unsigned char *s)
{
	unsigned char *p = s;

	while (*p)
		p++;

	return (p - s);
}
#endif

#if defined (_IAR_EWAVR_)
unsigned char *_FF_strrchr(unsigned char *s, int c)
{	/* find last occurrence of c in char s[] */
	const char ch = c;
	unsigned char *sc;

	for (sc = 0; ; ++s)
	{	/* check another char */
		if (*s == ch)
			sc = s;
		if (*s == '\0')
			return ((unsigned char *)sc);
	}
}
#endif

// Conversion file to change an ASCII valued character into the calculated value
unsigned char ascii_to_char(unsigned char ascii_char)
{
	unsigned char temp_char;

	if (ascii_char < 0x30)		// invalid, return error
		return (0xFF);
	else if (ascii_char < 0x3A)
	{	//number, subtract 0x30, retrun value
		temp_char = ascii_char - 0x30;
		return (temp_char);
	}
	else if (ascii_char < 0x41)	// invalid, return error
		return (0xFF);
	else if (ascii_char < 0x47)
	{	// lower case a-f, subtract 0x37, return value
		temp_char = ascii_char - 0x37;
		return (temp_char);
	}
	else if (ascii_char < 0x61)	// invalid, return error
		return (0xFF);
	else if (ascii_char < 0x67)
	{	// upper case A-F, subtract 0x57, return value
		temp_char = ascii_char - 0x57;
		return (temp_char);
	}
	else	// invalid, return error
		return (0xFF);
}

// Function to see if the character is a valid 8.3 FILENAME character
unsigned char valid_file_char(unsigned char file_char)
{
	if ((file_char < 0x20) || (file_char==0x22) || (file_char==0x2A) || (file_char==0x2B) ||
			(file_char==0x2C) || (file_char==0x2E) || (file_char==0x2F) ||
			((file_char>=0x3A)&&(file_char<=0x3F)) || ((file_char>=0x5B)&&(file_char<=0x5D)) ||
			(file_char==0x7C) || (file_char==0xE5) )
		return (0);
	else
		return (file_char);
}

#ifdef _DEBUG_FUNCTIONS_
 #define _DEBUG_SCANDIR_
 #ifdef _DEBUG_SCANDIR_
  flash char _scandir_start_str_[] = "\r\nSCAN_DIRECTORY(\"%s\", 0x%lX, Mode-%d) - ";
  flash char _scandir_debug_str_[] = "\t[ScanDirHere-%02d](%lX)";
  flash char _scandir_info_str_[] = "\t[%s=?%s]";
 #endif
#endif

// Function will scan the directory @ _FF_ROOT_ADDR or _FF_DIR_ADDR (which ever
// is applicable and will return a FILE_DIR_ENTRY pointer if successful (depends
// on the mode) or a NULL if an error occurs.
FILE_DIR_ENTRY *scan_directory(unsigned char *NAME, unsigned char mode_flag)
{
  #ifdef _DIRECTORIES_SUPPORTED_
	unsigned int dir_clus, c;
  #endif
	unsigned int ent_cntr, ent_max, n;
	unsigned long temp_addr;
	unsigned char aval_flag, name_store[14];
	FILE_DIR_ENTRY *dep, *dep_save;
  #ifndef _BytsPerSec_512_
	unsigned int EntPerSec;

	EntPerSec = BPB_BytsPerSec.ival >> 5;		// same as divide by 32
  #endif

  #ifdef _DEBUG_SCANDIR_
	_FF_printf(_scandir_start_str_, NAME, _FF_DIR_ADDR<<9, mode_flag);
  #endif

	aval_flag = 0;
	ent_cntr = 0;	// set to 0

	// error if name not set right
	if (file_name_conversion(NAME, name_store))
	{
	  #ifdef _DEBUG_SCANDIR_
		_FF_printf(_scandir_debug_str_, 1);
	  #endif
		return (0);
	}

  #ifdef _DIRECTORIES_SUPPORTED_
	if (_FF_DIR_ADDR == _FF_ROOT_ADDR)
		ent_max = BPB_RootEntCnt.ival;		// if this is the root directory, RootEntCnt is the maximum entries
	else
	{	// sub-directory
		dir_clus = addr_to_clust(_FF_DIR_ADDR);		// find the cluster number of this address
		if (dir_clus == 0)
		{
	 	  #ifdef _DEBUG_SCANDIR_
			_FF_printf(_scandir_debug_str_, 2, _FF_DIR_ADDR);
		  #endif
			return (0);
		}
	  #ifdef _BytsPerSec_512_ // ent_max should be (BytsPerSec/BytsPerEntry)*(SecPerClus)
		ent_max = (unsigned int) BPB_SecPerClus << 4;		// fast way to multip
	  #else
		ent_max = (BPB_BytsPerSec.ival / EntPerSec) * (unsigned int) BPB_SecPerClus;
	  #endif
	}
	_FF_scan_addr = _FF_DIR_ADDR;
  #else
	_FF_scan_addr = _FF_ROOT_ADDR;
	ent_max = BPB_RootEntCnt.ival;
 #endif
  #ifdef _DEBUG_SCANDIR_
	_FF_printf(_scandir_debug_str_, 99, _FF_scan_addr<<9);
  #endif
	while (ent_cntr < ent_max)
	{
		if (_FF_read(_FF_scan_addr, _FF_buff))
		{	// read error
	 	  #ifdef _DEBUG_SCANDIR_
			_FF_printf(_scandir_debug_str_, 3, _FF_scan_addr<<9);
		  #endif
			return (0);
		}
		dep = (FILE_DIR_ENTRY *) _FF_buff;			// set directory entry pointer to the beginning of the entry buffer

	  #ifdef _BytsPerSec_512_
		for (n=0; n<16; n++)
	  #else
		for (n=0; n<EntPerSec; n++)
	  #endif
		{
			if (dep->name_entry[0]==0)
			{	// There are no more entries in this folder
				if (aval_flag == 0)
				{
					dep_save = dep;
					temp_addr = _FF_scan_addr;
					aval_flag = 1;
				}
		 	  #ifdef _DEBUG_SCANDIR_
				_FF_printf(_scandir_debug_str_, 4, _FF_scan_addr<<9);
			  #endif
				goto NoMoreEntries;
			}
			else if ((dep->name_entry[0]==0xE5) && (aval_flag==0))
			{	// found a deleted entry space set temp_addr
				dep_save = dep;
				temp_addr = _FF_scan_addr;
				aval_flag = 1;
			}
			else
			{
		 	  #ifdef _DEBUG_SCANDIR_
				_FF_printf(_scandir_info_str_,name_store,dep->name_entry);
			  #endif
				if (_FF_strncmp(name_store, dep->name_entry, 11)==0)
				{	// found entry set VALID_ADDR to the exact entry location (Full Address)
			 	  #ifdef _DEBUG_SCANDIR_
					_FF_printf(_scandir_debug_str_, 5, (_FF_scan_addr<<9) & (n<<4));
				  #endif
 					switch (mode_flag)
 					{
					  #ifdef _DIRECTORIES_SUPPORTED_
 						case SCAN_DIR_W:
							if (dep->attr & ATTR_DIRECTORY)
							{
						 	  #ifdef _DEBUG_SCANDIR_
								_FF_printf(_scandir_debug_str_, 51, 0);
							  #endif
								return (0);
							}
	 						break;
						case SCAN_DIR_R:
  							if (dep->attr & ATTR_DIRECTORY)
							{
			 			 	  #ifdef _DEBUG_SCANDIR_
								_FF_printf(_scandir_debug_str_, 52, 0);
							  #endif
								return (dep);
							}
	 						break;
					  #endif
						case SCAN_FILE_R:
 						case SCAN_FILE_W:
 							if ((dep->attr & ATTR_DIRECTORY) == 0)
							{
								return (dep);
							}
	 						break;
 					}
				}
			}
			dep++;		// increment the directory entry pointer
			ent_cntr++;
		}
		_FF_scan_addr++;
	  #ifdef _DIRECTORIES_SUPPORTED_
		if (ent_cntr == ent_max)
		{
			if (_FF_DIR_ADDR != _FF_ROOT_ADDR)		// are we in a folder @ a valid cluster
			{
				c = next_cluster(dir_clus, SINGLE);		// find the next cluster of this folder
				if (c==0)
				{	// this cluster should never point to cluster 0
		 	 	  #ifdef _DEBUG_SCANDIR_
					_FF_printf(_scandir_debug_str_, 6, (((unsigned long)c)<<16) & dir_clus);
				  #endif
					return (0);
				}
				else if (c >= 0xFFF8)
				{	// points to EOF
				  #ifdef _DEBUG_SCANDIR_
					_FF_printf(_scandir_debug_str_, 7, _FF_scan_addr<<9);
				  #endif
					goto NoMoreEntries;
				}
				else
				{	// another dir cluster exists, set address to it and
					_FF_scan_addr = clust_to_addr(c);
 					if (_FF_scan_addr == 0)
					{	// this should never be 0
			 	 	  #ifdef _DEBUG_SCANDIR_
						_FF_printf(_scandir_debug_str_, 8, c);
					  #endif
						return (0);
					}
					dir_clus = c;		// set new dir_clus
					ent_cntr = 0;		// reset entry count
					c = 0;				// reset sector count
				}
			}
		}
	  #endif
	}

NoMoreEntries:
    switch (mode_flag)
    {
      #ifndef _READ_ONLY_
	   #ifdef _DIRECTORIES_SUPPORTED_
		case SCAN_DIR_W:
	   #endif
		case SCAN_FILE_W:
			if (aval_flag)
			{	// If there was a previous avaliable entry
				if (temp_addr != _FF_buff_addr)
				{
				    if (_FF_read(temp_addr, _FF_buff))
					{
				 	  #ifdef _DEBUG_SCANDIR_
						_FF_printf(_scandir_debug_str_, 9, _FF_scan_addr<<9);
					  #endif
						return (0);
					}
				}
			  #ifdef _DEBUG_SCANDIR_
				_FF_printf(_scandir_debug_str_, 10, temp_addr<<9);
			  #endif
				_FF_scan_addr = temp_addr;
				return (dep_save);
			}
		  #ifdef _DIRECTORIES_SUPPORTED_
			else if (_FF_DIR_ADDR != _FF_ROOT_ADDR)
			{
 				c = prev_cluster(0);	// find the next avaliable cluster
                // Point current cluster to the next avaliable
                if ((c == 0) || (write_clus_table(dir_clus, c, START_CHAIN)))
				{
			 	  #ifdef _DEBUG_SCANDIR_
					_FF_printf(_scandir_debug_str_, 11, c);
				  #endif
					return (0);
				}
                // Point next avaliable to a EOF marker
                if (write_clus_table(c, (unsigned int)EOF, END_CHAIN))
				{
			 	  #ifdef _DEBUG_SCANDIR_
					_FF_printf(_scandir_debug_str_, 12, c);
				  #endif
					return (0);
				}
                // Set the path address to the new cluster location
                _FF_scan_addr = clust_to_addr(c);
				if (_FF_scan_addr == 0)
				{
				  #ifdef _DEBUG_SCANDIR_
					_FF_printf(_scandir_debug_str_, 13, (_FF_scan_addr+c)<<9);
				  #endif
					return (0);
				}
			  #ifdef _BytsPerSec_512_
				memset(_FF_buff, 0, 512);
			  #else
				memset(_FF_buff, 0, BPB_BytsPerSec.ival);
			  #endif
				for (c=0; c<BPB_SecPerClus; c++)
 				{	// clear out the whole cluster for entries
					if (_FF_write(_FF_scan_addr + c, _FF_buff))
					{
					  #ifdef _DEBUG_SCANDIR_
						_FF_printf(_scandir_debug_str_, 14, (_FF_scan_addr+c)<<9);
					  #endif
						return (0);
					}
				}
		 	  #ifdef _DEBUG_SCANDIR_
				_FF_printf(_scandir_debug_str_, 15, _FF_scan_addr<<9);
			  #endif
				dep = (FILE_DIR_ENTRY *) _FF_buff;
                return (dep);
			}
		  #endif
			else
			{	// No more entries
		 	  #ifdef _DEBUG_SCANDIR_
				_FF_printf(_scandir_debug_str_, 16, 0);
			  #endif
				return (0);
			}
  			break;
	  #endif
	  #ifdef _DIRECTORIES_SUPPORTED_
		case SCAN_DIR_R:
	  #endif
		case SCAN_FILE_R:
 	 	  #ifdef _DEBUG_SCANDIR_
			_FF_printf(_scandir_debug_str_, 17, 0);
		  #endif
		default:
			return (0);
 			break;
	}
}


#ifdef _DEBUG_ON_
char flash FileList_str[] = "\r\nFile Listing for:  ROOT\x5c";
char flash SC_str[] = "]\t(%X)";
char flash Entry_str[] = "\t[%ld] bytes\t(%X)\t";

// Function to display all files and folders in the root directory,
// with the size of the file in bytes within the [brakets]
void read_directory(void)
{
	unsigned char valid_flag;
	unsigned int c, n, d, m, ent_max;
	FILE_DIR_ENTRY *dep;
  #ifdef _DIRECTORIES_SUPPORTED_
	unsigned long dir_addr;
	unsigned int dir_clus;

	if (_FF_DIR_ADDR != _FF_ROOT_ADDR)
	{
		dir_clus = addr_to_clust(_FF_DIR_ADDR);
		if (dir_clus < 2)
			return;
	}
  #endif

	_FF_printf(FileList_str);
  #ifdef _DIRECTORIES_SUPPORTED_
	for (d=0; ((d<_FF_PATH_LENGTH)&&(_FF_FULL_PATH[d])); d++)
		putchar(_FF_FULL_PATH[d]);
	if (_FF_DIR_ADDR == _FF_ROOT_ADDR)
		ent_max = BPB_RootEntCnt.ival;
	else
	  #ifdef _BytsPerSec_512_
		ent_max = ((unsigned int) BPB_SecPerClus) << 4;
	  #else
		ent_max = BPB_BytsPerSec.ival / sizeof(FILE_DIR_ENTRY) * BPB_SecPerClus;
	  #endif
	dir_addr = _FF_DIR_ADDR;
  #else
	ent_max = BPB_RootEntCnt.ival;
  #endif
	d = 0;
	m = 0;
	while (d<ent_max)
	{
	  #ifdef _DIRECTORIES_SUPPORTED_
		if (_FF_read(dir_addr+m, _FF_buff))
	  #else
		if (_FF_read(_FF_ROOT_ADDR+m, _FF_buff))
	  #endif
    		break;
		dep = (FILE_DIR_ENTRY *) _FF_buff;
	  #ifdef _BytsPerSec_512_
		for (n=0; n<16; n++)
	  #else
		for (n=0; n<(BPB_BytsPerSec.ival/sizeof(FILE_DIR_ENTRY)); n++)
	  #endif
		{
			valid_flag = 1;
			if (dep->name_entry[0]==0)
			{	// a '\0' in this location means no more entries avaliable
				return;					// clear entries
			}
			for (c=0; ((c<11)&&(valid_flag)); c++)
			{	// make sure all the characters in the file/folder name are valid
				if (valid_file_char(dep->name_entry[c]) == 0)
					valid_flag = 0;
		    }
		    if (valid_flag)
	  		{
		  		putchar('\r');
				putchar('\n');
				putchar('\t');
				if ((dep->attr) & ATTR_DIRECTORY)
					putchar('[');
				for (c=0; c<8; c++)
				{
					if (dep->name_entry[c]==0x20)
						c=8;
					else
						putchar(dep->name_entry[c]);
				}
				if ((dep->attr) & ATTR_DIRECTORY)
				{
					_FF_printf(SC_str, dep->start_clus_lo);
				}
				else
				{
					putchar('.');
					for (c=8; c<11; c++)
					{
						if ((dep->name_entry[c])==0x20)
							c=11;
						else
							putchar(dep->name_entry[c]);
					}
			  		_FF_printf(Entry_str, dep->file_size, dep->start_clus_lo);
				}
		  	}
		  	dep++;
		  	d++;
		}
		m++;
	  #ifdef _DIRECTORIES_SUPPORTED_
		if (_FF_ROOT_ADDR!=_FF_DIR_ADDR)
		{
		   	if (m==BPB_SecPerClus)
		   	{

				m = next_cluster(dir_clus, SINGLE);
				if ((m < 0xFFF8) && (NextClusterValidFlag))
				{	// another dir cluster exists
					dir_addr = clust_to_addr(m);
					dir_clus = m;
					d = 0;
					m = 0;
				}
				else
					break;

		   	}
		}
	  #endif
	}
	putchar('\r');
	putchar('\n');
}
#endif

char flash VolSerialStr[] = "\r\n\tVolume Serial:\t[0x%08lX]";
char flash VolLabelStr[] = "\r\n\tVolume Label:\t[%s]\r\n";

void GetVolID(void)
{
  #ifndef _BIG_ENDIAN_
	_FF_printf(VolSerialStr, BS_VolSerial);
  #else
	_FF_printf(VolSerialStr, BS_VolSerial.lval);
  #endif
	_FF_printf(VolLabelStr, BS_VolLab);
}

#ifdef _DEBUG_FUNCTIONS_
 #define _DEBUG_C2A_
 #ifdef _DEBUG_C2A_
  flash char _c2a_err_str_[] = "\r\n[Clus2AddrErr]";
 #endif
#endif

// Convert a cluster number into a read address
unsigned long clust_to_addr(unsigned int clust_no)
{
	unsigned long FirstSectorofCluster;

	if (clust_no < 2)
	{
	  #ifdef _DEBUG_C2A_
		_FF_printf(_c2a_err_str_);
	  #endif
		return (0);
	}

	FirstSectorofCluster = ((clust_no - 2) * (unsigned long) BPB_SecPerClus) + (unsigned long) FirstDataSector;

	return (FirstSectorofCluster + _FF_PART_ADDR);
}


#ifdef _DEBUG_FUNCTIONS_
 #define _DEBUG_A2C_
 #ifdef _DEBUG_A2C_
  flash char _a2c_err_str_[] = "\r\n[Addr2ClusErr-%d]";
 #endif
#endif

// Converts an address into a cluster number
unsigned int addr_to_clust(unsigned long clus_addr)
{
	if (clus_addr <= _FF_PART_ADDR)
	{	// invalid file address entry
	  #ifdef _DEBUG_A2C_
		_FF_printf(_a2c_err_str_, 1);
	  #endif
		return (0);
	}
	clus_addr -= _FF_PART_ADDR;
	if (clus_addr < (unsigned long) FirstDataSector)
	{	// input addr - part addr should be < the first data sector
	  #ifdef _DEBUG_A2C_
		_FF_printf(_a2c_err_str_, 2);
	  #endif
		return (0);
	}
	clus_addr -= FirstDataSector;
	clus_addr /= BPB_SecPerClus;
	clus_addr += 2;
	if (clus_addr > 0xFFFF)
	{
	  #ifdef _DEBUG_A2C_
		_FF_printf(_a2c_err_str_, 3);
	  #endif
		return (0);
    }
    // this is now the cluster number of the address
	return ((unsigned int) clus_addr);
}


#ifdef _DEBUG_FUNCTIONS_
  #define _DEBUG_NEXTCLUS_
#endif
#ifdef _DEBUG_NEXTCLUS_
  flash char _nextclus_debug_str_[] = "\tNextClusHere-%02d";
  flash char _nextclus_info_str_[] = "\r\nNextClus(%X,%d)";
#endif
// Find the cluster that the current cluster is pointing to
unsigned int next_cluster(unsigned int current_cluster, unsigned char mode)
{
	unsigned int calc_sec, calc_offset;
  #ifdef _FAT12_ON_
	unsigned char calc_remainder;
  #endif
	uHILO16 next_clust;
	unsigned long addr_temp;

  #ifdef _DEBUG_NEXTCLUS_
	_FF_printf(_nextclus_info_str_, current_cluster, mode);
  #endif

	if (current_cluster<=1)		// If cluster is 0 or 1, its the wrong cluster
	{
		NextClusterValidFlag = 0;
	  #ifdef _DEBUG_NEXTCLUS_
		_FF_printf(_nextclus_debug_str_, 1);
	  #endif
		return (0);
	}

	if (BPB_FATType == 0x36)		// if FAT16
	{
		// FAT16 table address calculations
	  #ifdef _BytsPerSec_512_
		calc_sec = (current_cluster >> 8) + BPB_RsvdSecCnt.ival;
		calc_offset = (current_cluster & 0xFF) << 1;
	  #else
		calc_sec = current_cluster / (BPB_BytsPerSec.ival >> 1) + BPB_RsvdSecCnt.ival;
		calc_offset = (current_cluster % (BPB_BytsPerSec.ival >> 1)) << 1;
	  #endif
	}
  #ifdef _FAT12_ON_
	else if (BPB_FATType == 0x32)	// if FAT12
	{
		// FAT12 table address calculations
		calc_offset = (current_cluster * 3) >> 1;
		calc_remainder = current_cluster & 1;
	  #ifdef _BytsPerSec_512_
		calc_sec = (calc_offset >> 9) + BPB_RsvdSecCnt.ival;
		calc_offset &= 0x1FF;
	  #else
		calc_sec = (calc_offset / BPB_BytsPerSec.ival) + BPB_RsvdSecCnt.ival;
		calc_offset %= BPB_BytsPerSec.ival;
	  #endif
	}
   #endif
	else		// not FAT12 or FAT16, return EOF
	{
		NextClusterValidFlag = 0;
	  #ifdef _DEBUG_NEXTCLUS_
		_FF_printf(_nextclus_debug_str_, 2);
	  #endif
		return (0);
	}

 	addr_temp = _FF_PART_ADDR+(calc_sec);
	switch (mode)
	{
		case SINGLE:	// This is a single cluster lookup
			if (_FF_read(addr_temp, _FF_buff))
			{
				NextClusterValidFlag = 0;
			  #ifdef _DEBUG_NEXTCLUS_
				_FF_printf(_nextclus_debug_str_, 3);
			  #endif
				return (0);
			}
			break;
	  #ifndef _READ_ONLY_
		case CHAIN_W:
			if (addr_temp!=_FF_buff_addr)
			{	// Is the address of lookup is different then the current buffere address
				if (_FF_buff_addr)	// if the buffer address is 0, don't write
				{
					if (_FF_buff_addr < _FF_FAT2_ADDR)
					{	// Only write data if we are dealing with the FAT table
					  #ifdef _SECOND_FAT_ON_
						if (_FF_write(_FF_buff_addr+BPB_FATSz16.ival, _FF_buff))
						{
							NextClusterValidFlag = 0;
						  #ifdef _DEBUG_NEXTCLUS_
							_FF_printf(_nextclus_debug_str_, 4);
						  #endif
							return (0);
						}
					  #endif
						if (_FF_write(_FF_buff_addr, _FF_buff))	// Save buffer data to card
						{
							NextClusterValidFlag = 0;
						  #ifdef _DEBUG_NEXTCLUS_
							_FF_printf(_nextclus_debug_str_, 5);
						  #endif
							return (0);
						}
					}
				}
				if (_FF_read(addr_temp, _FF_buff))	// Read new table info
				{
					NextClusterValidFlag = 0;
				  #ifdef _DEBUG_NEXTCLUS_
					_FF_printf(_nextclus_debug_str_, 6);
				  #endif
					return (0);
				}
			}
			break;
	  #endif
		case CHAIN_R:
			if (addr_temp!=_FF_buff_addr)
			{	// Is the address of lookup is different then the current buffere address
				if (_FF_read(addr_temp, _FF_buff))	// Read new table info
				{
					NextClusterValidFlag = 0;
				  #ifdef _DEBUG_NEXTCLUS_
					_FF_printf(_nextclus_debug_str_, 7);
				  #endif
					return (0);
				}
			}
			break;
	}

  #ifdef _FAT12_ON_
	if (BPB_FATType == 0x36)		// if FAT16
	{
  #endif
		next_clust.cval.HI = _FF_buff[calc_offset+1];
		next_clust.cval.LO = _FF_buff[calc_offset];
  #ifdef _FAT12_ON_
	}
	else
	{	// If we are here it has to be FAT12
		next_clust.cval.LO = _FF_buff[calc_offset];
	  #ifdef _BytsPerSec_512_
		if (calc_offset == 511)
	  #else
		if (calc_offset == (BPB_BytsPerSec.ival-1))
	  #endif
		{	// Is the FAT12 record accross more than one sector?
			addr_temp = _FF_PART_ADDR+(calc_sec+1);
			if (mode==CHAIN_W)
			{	// multiple chain lookup
			  #ifndef _READ_ONLY_
			   #ifdef _SECOND_FAT_ON_
				if (_FF_buff_addr < _FF_FAT2_ADDR)
				{
					if (_FF_write(_FF_buff_addr+BPB_FATSz16.ival, _FF_buff))
					{
						NextClusterValidFlag = 0;
				  #ifdef _DEBUG_NEXTCLUS_
					_FF_printf(_nextclus_debug_str_, 8);
				  #endif
						return (0);
					}
				}
			   #endif
				if (_FF_write(_FF_buff_addr, _FF_buff))	// Save buffer data to card
				{
					NextClusterValidFlag = 0;
				  #ifdef _DEBUG_NEXTCLUS_
					_FF_printf(_nextclus_debug_str_, 9);
				  #endif
					return (0);
				}
			  #endif
			}
			if (_FF_read(addr_temp, _FF_buff))
			{	// Need to find the second half of the
				NextClusterValidFlag = 0;
			  #ifdef _DEBUG_NEXTCLUS_
				_FF_printf(_nextclus_debug_str_, 10);
			  #endif
				return (0);
			}
			next_clust.cval.HI = _FF_buff[0];
		}
		else
			next_clust.cval.HI = _FF_buff[calc_offset+1];

		if (calc_remainder)
			next_clust.ival >>= 4;
		else
			next_clust.cval.HI &= 0x0F;

		if (next_clust.ival >= 0xFF8)
			next_clust.cval.HI = 0xFF;
	}
  #endif

	NextClusterValidFlag = 1;
	return (next_clust.ival);
}


// Function that tells you the number of available clusters left on the disk
unsigned int available_clusters(void)
{
	unsigned int i, cnt;

	cnt = 0;

	for (i=2; i<(DataClusTot); i++)
	{
		if (next_cluster(i, CHAIN_R) == 0)
			cnt++;
	}

	return (cnt);
}


// Convert a constant string file name into the proper 8.3 FAT format
char file_name_conversion(unsigned char *input_name, unsigned char *save_name)
{
	unsigned char n, c;

	c = 0;
	for (n=0; n<14; n++)
	{
		if (valid_file_char(input_name[n])!=0)
		{
			// If the character is valid, save in uppercase to file name buffer
			save_name[c] = toupper(input_name[n]);
			c++;
		}
		else if (input_name[n]=='.')
		{
			// If it is a period, back fill buffer with [spaces], till 8 characters deep
			while (c<8)
			{
				save_name[c] = 0x20;
				c++;
			}
		}
		else if (input_name[n]==0)
		{	// If it is NULL, back fill buffer with [spaces], till 11 characters deep
			while (c<11)
			{
				save_name[c] = 0x20;
				c++;
			}
			break;
		}
		else
		{
			_FF_error = NAME_ERR;
			return ((char)EOF);
		}
		if (c>=11)
			break;
	}
	save_name[c] = 0;

	return (0);
}

#ifdef _DEBUG_FUNCTIONS_
  #define _DEBUG_PREVCLUS_
#endif
#ifdef _DEBUG_PREVCLUS_
  flash char _prevclus_debug_str_[] = "\tPrevClusHere-%02d";
  flash char _prevclus_info_str_[] = "\r\nPrevClus(%X)";
#endif
// Find the first cluster that is pointing to clus_no
unsigned int prev_cluster(unsigned int clus_no)
{
	unsigned int calc_temp, calc_clus;

  #ifdef _DEBUG_PREVCLUS_
	_FF_printf(_prevclus_info_str_, clus_no);
  #endif

 	if (clus_no==0)
	{	// Start from the last previous known cluster pointing to 0
		calc_temp = clus_0_counter;
	}
	else
	{
		calc_temp = 2;
	}

	_FF_buff_addr = 0;	// Clear to make sure it reads the buffer the first time through
	while (calc_temp <= DataClusTot)
	{
		calc_clus = next_cluster(calc_temp, CHAIN_R);
		if (NextClusterValidFlag && (calc_clus == clus_no))
		{
			if (clus_no == 0)
				clus_0_counter = calc_temp;
			return (calc_temp);
		}
		else if (NextClusterValidFlag == 0)
		{
		  #ifdef _DEBUG_PREVCLUS_
			_FF_printf(_prevclus_debug_str_, 1);
		  #endif
			return (0);
		}
		calc_temp++;
	}

  #ifdef _DEBUG_PREVCLUS_
	_FF_printf(_prevclus_debug_str_, 2);
  #endif
	return (0);
}

#ifndef _READ_ONLY_
#ifdef _DEBUG_FUNCTIONS_
  #define _WRITE_CLUSTER_TABLE_DEBUG_

  char flash _write_cluster_debug_str_[] = "\tWriteClusERR-%02d";
  char flash _write_cluster_info_str_[] = "\r\nWriteClus(%X,%X,%d)";
#endif
// Update cluster table to point to new cluster
char write_clus_table(unsigned int current_cluster, unsigned int next_value, unsigned char mode)
{
	unsigned long addr_temp;
	unsigned int calc_sec, calc_offset;
  #ifdef _FAT12_ON_
	unsigned char calc_remainder;
  #endif
	uHILO16 temp_int;

  #ifdef _WRITE_CLUSTER_TABLE_DEBUG_
	_FF_printf(_write_cluster_info_str_, current_cluster, next_value, mode);
  #endif

	if (current_cluster <= 1)		// Should never be writing to cluster 0 or 1
	{
	  #ifdef _WRITE_CLUSTER_TABLE_DEBUG_
		_FF_printf(_write_cluster_debug_str_, 1);
	  #endif
		return ((char)EOF);
	}
	temp_int.ival = next_value;
	if (BPB_FATType == 0x36)			// if FAT16
	{
	  #ifdef _BytsPerSec_512_
		calc_sec = (current_cluster >> 8) + BPB_RsvdSecCnt.ival;
		calc_offset = (current_cluster & 0xFF) << 1;
	  #else
		calc_sec = current_cluster / (BPB_BytsPerSec.ival >> 1) + BPB_RsvdSecCnt.ival;
		calc_offset = (current_cluster % (BPB_BytsPerSec.ival >> 1)) << 1;
	  #endif
	}
  #ifdef _FAT12_ON_
  	else if (BPB_FATType == 0x32)		// if FAT12
  	{
  		calc_offset = (current_cluster * 3) >> 1;
  		calc_remainder = current_cluster & 1;
	  #ifdef _BytsPerSec_512_
  		calc_sec = (calc_offset >> 9) + BPB_RsvdSecCnt.ival;
  		calc_offset &= 0x1FF;
	  #else
  		calc_sec = calc_offset / BPB_BytsPerSec.ival + BPB_RsvdSecCnt.ival;
  		calc_offset %= BPB_BytsPerSec.ival;
	  #endif
	}
  #endif
	else		// not FAT12 or FAT16, return 0
	{
	  #ifdef _WRITE_CLUSTER_TABLE_DEBUG_
		_FF_printf(_write_cluster_debug_str_, 2);
	  #endif
		return ((char)EOF);
	}

	addr_temp = _FF_PART_ADDR + calc_sec;
	if ((mode==SINGLE) || (mode==START_CHAIN))
	{	// Updating a single cluster (like writing or saving a file)
		if (_FF_read(addr_temp, _FF_buff))
		{
		  #ifdef _WRITE_CLUSTER_TABLE_DEBUG_
			_FF_printf(_write_cluster_debug_str_, 3);
		  #endif
			return((char)EOF);
		}
	}
	else // Must be --> if ((mode==CHAIN_W) || (mode==END_CHAIN))
	{	// Multiple table access operation
		if (addr_temp!=_FF_buff_addr)
		{	// if the desired address is already in the buffer => skip loading buffer
			if (_FF_buff_addr)	// if new table address, write buffered, and load new
			{
				if (_FF_buff_addr < _FF_FAT2_ADDR)
				{
				  #ifdef _SECOND_FAT_ON_
					if (_FF_write(_FF_buff_addr+BPB_FATSz16.ival, _FF_buff))
					{
					  #ifdef _WRITE_CLUSTER_TABLE_DEBUG_
						_FF_printf(_write_cluster_debug_str_, 4);
					  #endif
						return((char)EOF);
					}
				  #endif
					if (_FF_write(_FF_buff_addr, _FF_buff))
					{
					  #ifdef _WRITE_CLUSTER_TABLE_DEBUG_
						_FF_printf(_write_cluster_debug_str_, 5);
					  #endif
						return((char)EOF);
					}
				}
			}
			if (_FF_read(addr_temp, _FF_buff))
			{
			  #ifdef _WRITE_CLUSTER_TABLE_DEBUG_
				_FF_printf(_write_cluster_debug_str_, 6);
			  #endif
				return((char)EOF);
			}
		}
	}

	if (BPB_FATType == 0x36)		// if FAT16
	{
		_FF_buff[calc_offset+1] = temp_int.cval.HI;
		_FF_buff[calc_offset] = temp_int.cval.LO;
		if ((mode==SINGLE) || (mode==END_CHAIN))
		{
		  #ifdef _SECOND_FAT_ON_
			if (_FF_write(addr_temp+BPB_FATSz16.ival, _FF_buff))
			{
			  #ifdef _WRITE_CLUSTER_TABLE_DEBUG_
				_FF_printf(_write_cluster_debug_str_, 7);
			  #endif
				return((char)EOF);
			}
		  #endif
			if (_FF_write(addr_temp, _FF_buff))
			{
			  #ifdef _WRITE_CLUSTER_TABLE_DEBUG_
				_FF_printf(_write_cluster_debug_str_, 8);
			  #endif
				return((char)EOF);
			}
		}
	}
  #ifdef _FAT12_ON_
  	else // Must be --> if (BPB_FATType == 0x32)		// if FAT12
  	{
	  #ifdef _BytsPerSec_512_
  		if (calc_offset == 511)
	  #else
  		if (calc_offset == (BPB_BytsPerSec.ival-1))
	  #endif
  		{	// Is the FAT12 record accross a sector?
  			if (calc_remainder)
  			{	// Record table uses 1 nibble of last byte
  				_FF_buff[calc_offset] &= 0x0F;	// Mask to add new value
  				_FF_buff[calc_offset] |= (((unsigned char) temp_int.nib.NIB_0) << 4);	// store nibble in correct location
  			  #ifdef _SECOND_FAT_ON_
  				if (_FF_write(addr_temp+BPB_FATSz16.ival, _FF_buff))
				{
				  #ifdef _WRITE_CLUSTER_TABLE_DEBUG_
					_FF_printf(_write_cluster_debug_str_, 9);
				  #endif
  					return((char)EOF);
				}
  			  #endif
  				if (_FF_write(addr_temp, _FF_buff))
				{
				  #ifdef _WRITE_CLUSTER_TABLE_DEBUG_
					_FF_printf(_write_cluster_debug_str_, 10);
				  #endif
  					return((char)EOF);
				}
  				addr_temp++;
  				if (_FF_read(addr_temp, _FF_buff))
				{
				  #ifdef _WRITE_CLUSTER_TABLE_DEBUG_
					_FF_printf(_write_cluster_debug_str_, 11);
				  #endif
  					return((char)EOF);	// if the read fails return 0
				}
  				_FF_buff[0] = (((unsigned char) temp_int.nib.NIB_2) << 4) | temp_int.nib.NIB_1;
  				if ((mode==SINGLE) || (mode==END_CHAIN))
  				{
  				  #ifdef _SECOND_FAT_ON_
  					if (_FF_write(addr_temp+BPB_FATSz16.ival, _FF_buff))
					{
					  #ifdef _WRITE_CLUSTER_TABLE_DEBUG_
						_FF_printf(_write_cluster_debug_str_, 12);
					  #endif
  						return((char)EOF);
					}
  				  #endif
  					if (_FF_write(addr_temp, _FF_buff))
					{
					  #ifdef _WRITE_CLUSTER_TABLE_DEBUG_
						_FF_printf(_write_cluster_debug_str_, 13);
					  #endif
						return((char)EOF);
					}
  				}
  			}
  			else
  			{	// Record table uses whole last byte + 1 nibble in the next buffer
  				_FF_buff[calc_offset] = temp_int.cval.LO;
  			  #ifdef _SECOND_FAT_ON_
  				if (_FF_write(addr_temp+BPB_FATSz16.ival, _FF_buff))
			 	{
				  #ifdef _WRITE_CLUSTER_TABLE_DEBUG_
					_FF_printf(_write_cluster_debug_str_, 14);
				  #endif
 					return((char)EOF);
				}
  			  #endif
  				if (_FF_write(addr_temp, _FF_buff))
				{
				  #ifdef _WRITE_CLUSTER_TABLE_DEBUG_
					_FF_printf(_write_cluster_debug_str_, 15);
				  #endif
  					return((char)EOF);
				}
  				addr_temp++;
  				if (_FF_read(addr_temp, _FF_buff))
				{
				  #ifdef _WRITE_CLUSTER_TABLE_DEBUG_
					_FF_printf(_write_cluster_debug_str_, 16);
				  #endif
  					return((char)EOF);	// if the read fails return 0
				}
  				_FF_buff[0] &= 0xF0;		// Mask to add new value
  				_FF_buff[0] |= temp_int.nib.NIB_2;	// store nibble in correct location
  				if ((mode==SINGLE) || (mode==END_CHAIN))
  				{
  				  #ifdef _SECOND_FAT_ON_
  					if (_FF_write(addr_temp+BPB_FATSz16.ival, _FF_buff))
					{
					  #ifdef _WRITE_CLUSTER_TABLE_DEBUG_
						_FF_printf(_write_cluster_debug_str_, 17);
					  #endif
  						return((char)EOF);
					}
  				  #endif
  					if (_FF_write(addr_temp, _FF_buff))
					{
					  #ifdef _WRITE_CLUSTER_TABLE_DEBUG_
						_FF_printf(_write_cluster_debug_str_, 18);
					  #endif
  						return((char)EOF);
					}
  				}
  			}
  		}
  		else
  		{
  			if (calc_remainder)
  			{	// Record table uses 1 nibble of current byte
  				_FF_buff[calc_offset] &= 0x0F;	// Mask to add new value
  				_FF_buff[calc_offset] |= (((unsigned char) temp_int.nib.NIB_0) << 4);	// store nibble in correct location
  				_FF_buff[calc_offset+1] = (((unsigned char) temp_int.nib.NIB_2) << 4) | temp_int.nib.NIB_1;
  				if ((mode==SINGLE) || (mode==END_CHAIN))
  				{
  				  #ifdef _SECOND_FAT_ON_
  					if (_FF_write(addr_temp+BPB_FATSz16.ival, _FF_buff))
					{
					  #ifdef _WRITE_CLUSTER_TABLE_DEBUG_
						_FF_printf(_write_cluster_debug_str_, 19);
					  #endif
  						return((char)EOF);
					}
  				  #endif
  					if (_FF_write(addr_temp, _FF_buff))
					{
					  #ifdef _WRITE_CLUSTER_TABLE_DEBUG_
						_FF_printf(_write_cluster_debug_str_, 20);
					  #endif
  						return((char)EOF);
					}
  				}
  			}
  			else
  			{	// Record table uses whole current byte
  				_FF_buff[calc_offset] = temp_int.cval.LO;
  				_FF_buff[calc_offset+1] &= 0xF0;		// Mask to add new value
  				_FF_buff[calc_offset+1] |= temp_int.nib.NIB_2;	// store nibble in correct location
  				if ((mode==SINGLE) || (mode==END_CHAIN))
  				{
  				  #ifdef _SECOND_FAT_ON_
  					if (_FF_write(addr_temp+BPB_FATSz16.ival, _FF_buff))
					{
					  #ifdef _WRITE_CLUSTER_TABLE_DEBUG_
						_FF_printf(_write_cluster_debug_str_, 21);
					  #endif
  						return((char)EOF);
					}
  				  #endif
  					if (_FF_write(addr_temp, _FF_buff))
					{
					  #ifdef _WRITE_CLUSTER_TABLE_DEBUG_
						_FF_printf(_write_cluster_debug_str_, 22);
					  #endif
  						return((char)EOF);
					}
  				}
  			}
  		}
  	}
  #endif

	return(0);
}
#endif

#ifndef _READ_ONLY_
#ifdef _DEBUG_FUNCTIONS_
  #define _DEBUG_APPRNDTOC_
  char flash _appendtoc_debug_str_[] = "\tAppendTOCERR-%02d";
  char flash _appendtoc_info_str_[] = "\r\nAppendTOCERR-%02d";
  char flash _appendtoc_addr_str_[] = "\r\nAddress Entry @ 0x%lX - APPENDTOC";
#endif
// Save new entry data to FAT entry
char append_toc(FILE *rp)
{
  #ifdef _BIG_ENDIAN_
	DIR_ENT_TIME ff_time;
	DIR_ENT_DATE ff_date;
	uHILO32 temp32;
  #endif

	if (rp==NULL)
	{
	  #ifdef _DEBUG_APPRNDTOC_
		_FF_printf(_appendtoc_debug_str_, 1);
	  #endif
		return ((char)EOF);
	}

	if (_FF_read(rp->entry_sec_addr, _FF_buff))
	{
	  #ifdef _DEBUG_APPRNDTOC_
		_FF_printf(_appendtoc_debug_str_, 2);
	  #endif
		return ((char)EOF);
	}

  #ifndef _BIG_ENDIAN_
	// Update Starting Cluster
	(rp->file_dep)->start_clus_lo = rp->clus_start;
	// Update the File Size
	(rp->file_dep)->file_size = rp->length;

   #ifdef _RTC_ON_ 	// Date/Time Stamp file w/ RTC
	rtc_get_timeNdate(&rtc_hour, &rtc_min, &rtc_sec, &rtc_date, &rtc_month, &rtc_year);
	(rp->file_dep)->mod_date.date.month = rtc_month;			// File modify month
	(rp->file_dep)->mod_date.date.day = rtc_date;				// File modify day
	(rp->file_dep)->mod_date.date.year = (rtc_year - 1980);		// File modify year	+ 1980
	(rp->file_dep)->mod_time.time.hour = rtc_hour;				// File modify hour
	(rp->file_dep)->mod_time.time.min = rtc_min;				// File modify minute
	(rp->file_dep)->mod_time.time.x2sec = (rtc_sec / 2);		// File modify seconds / 2
   #else		// Increment Date Code, no RTC used
	if (++((rp->file_dep)->mod_time.time.x2sec) > 29)
	{	// File modify seconds / 2
		(rp->file_dep)->mod_time.time.x2sec = 0;
		if ((++((rp->file_dep))->mod_time.time.min) > 59)
		{	// File modify minute
			(rp->file_dep)->mod_time.time.min = 0;
			if (++((rp->file_dep)->mod_time.time.hour) > 23)
			{	// File modify hour
				(rp->file_dep)->mod_time.time.hour = 0;
				if (++((rp->file_dep)->mod_date.date.day) > 28)
				{	// File modify day
					(rp->file_dep)->mod_date.date.day = 1;
					if (++((rp->file_dep)->mod_date.date.month) > 12)
					{	// File modify month
						(rp->file_dep)->mod_date.date.month = 1;
						(rp->file_dep)->mod_date.date.year++;
					}
				}
			}
		}
	}
   #endif
  #else
	// Update Starting Cluster
	temp32.ival.LO = rp->clus_start;
	(rp->file_dep)->start_clus_lo.cval.HI = temp32.cval.LO;
	(rp->file_dep)->start_clus_lo.cval.LO = temp32.cval.ML;
	// Update the File Size
	temp32.lval = rp->length;
    (rp->file_dep)->file_size.cval.HI = temp32.cval.LO;
    (rp->file_dep)->file_size.cval.MH = temp32.cval.ML;
    (rp->file_dep)->file_size.cval.ML = temp32.cval.MH;
    (rp->file_dep)->file_size.cval.LO = temp32.cval.HI;

   #ifdef _RTC_ON_ 	// Date/Time Stamp file w/ RTC
	rtc_get_timeNdate(&rtc_hour, &rtc_min, &rtc_sec, &rtc_date, &rtc_month, &rtc_year);
	ff_date.date.month = rtc_month;				// File modify month
	ff_date.date.day = rtc_date;				// File modify day
	ff_date.date.year = (rtc_year - 1980);		// File modify year	+ 1980
	ff_time.time.hour = rtc_hour;				// File modify hour
	ff_time.time.min = rtc_min;					// File modify minute
	ff_time.time.x2sec = (rtc_sec / 2);			// File modify seconds / 2
   #else		// Increment Date Code, no RTC used
	ff_date.dateHILO.HI = dep->mod_date.dateHILO.LO;		// File modify date
	ff_date.dateHILO.LO = dep->mod_date.dateHILO.HI;
	ff_time.timeHILO.HI = dep->mod_time.timeHILO.LO;		// File modify time
	ff_time.timeHILO.LO = dep->mod_time.timeHILO.HI;
	if (++(ff_time.time.x2sec) > 29)
	{	// File modify seconds / 2
		ff_time.time.x2sec = 0;
		if (++(ff_time.time.min) > 59)
		{	// File modify minute
			ff_time.time.min = 0;
			if (++(ff_time.time.hour) > 23)
			{	// File modify hour
				ff_time.time.hour = 0;
				if (++(ff_date.date.day) > 28)
				{	// File modify day
					ff_date.date.day = 1;
					if (++(ff_date.date.month) > 12)
					{	// File modify month
						ff_date.date.month = 1;
						(ff_date.date.year)++;
					}
				}
			}
		}
	}
   #endif
	(rp->file_dep)->mod_date.dateHILO.HI = ff_date.dateHILO.LO;		// File modify date
	(rp->file_dep)->mod_date.dateHILO.LO = ff_date.dateHILO.HI;
	(rp->file_dep)->mod_time.timeHILO.HI = ff_time.timeHILO.LO;		// File modify time
	(rp->file_dep)->mod_time.timeHILO.LO = ff_time.timeHILO.HI;
  #endif
	if (_FF_write(rp->entry_sec_addr, _FF_buff))
	{
	  #ifdef _DEBUG_APPRNDTOC_
		_FF_printf(_appendtoc_debug_str_, 1);
	  #endif
		return ((char)EOF);
	}

  #ifdef _DEBUG_APPRNDTOC_
	_FF_printf(_appendtoc_addr_str_, (rp->entry_sec_addr)<<9);
	dump_file_entry_hex(rp->file_dep);
  #endif
	return(0);
}
#endif

#ifndef _READ_ONLY_
// Erase a chain of clusters (set table entries to 0 for clusters in chain)
char erase_clus_chain(unsigned int start_clus)
{
	unsigned int clus_temp, clus_use;

	if (start_clus<2)
		return ((char)EOF);
	clus_use = start_clus;
	_FF_buff_addr = 0;
	while(1)
	{
		clus_temp = next_cluster(clus_use, CHAIN_W);
		if (NextClusterValidFlag == 0)
			return ((char)EOF);
		if ((clus_temp > DataClusTot) || (clus_temp == 0))
			break;	// No more in the chain, break
		if (write_clus_table(clus_use, 0, CHAIN_W))
			return ((char)EOF);
		// check for new low cluster 0's
		if (clus_use < clus_0_counter)
			clus_0_counter = clus_use;
		clus_use = clus_temp;
	}
	if (write_clus_table(clus_use, 0, END_CHAIN))
		return ((char)EOF);

	return (0);
}

// Quickformat of a card (erase cluster table and root directory
int fquickformat(void)
{
	unsigned long addr, addr_chk;

  #ifdef _BytsPerSec_512_
	memset(_FF_buff, 0, 512);
  #else
	memset(_FF_buff, 0, BPB_BytsPerSec.ival);
  #endif

	addr = (unsigned long) (_FF_FAT1_ADDR + 1);
  #ifdef _BytsPerSec_512_
	addr_chk = _FF_ROOT_ADDR + ((unsigned long) BPB_RootEntCnt.ival) << 4;
  #else
	addr_chk = _FF_ROOT_ADDR + ((unsigned long) BPB_RootEntCnt.ival) * BPB_BytsPerSec.ival / 32;
  #endif
	while (addr < addr_chk)
	{
		if (_FF_write(addr, _FF_buff))
		{
			_FF_error = WRITE_ERR;
			return ((int)EOF);
		}
		addr++;
	}
	_FF_buff[0] = 0xF8;
	_FF_buff[1] = 0xFF;
	_FF_buff[2] = 0xFF;
	if (BPB_FATType == 0x36)
		_FF_buff[3] = 0xFF;
	if ((_FF_write((unsigned long) _FF_FAT1_ADDR, _FF_buff)) || (_FF_write(_FF_FAT2_ADDR, _FF_buff)))
	{
		_FF_error = WRITE_ERR;
		return ((int)EOF);
	}
	return (0);
}
#endif

#ifdef _DIRECTORIES_SUPPORTED_
// Function that checks for directory changes then gets into a working form.
// given the working dir path, this function will return a -1 if it cannot
// understand the path entered or there is an error when trying to change the
// directories.  If everything is successful, a '0' is returned, and the char
// string path_temp is pointing to will be everything before the last '\'
int _FF_checkdir(unsigned char *F_PATH, unsigned char *path_temp)
{
	unsigned char *sp, *qp;

    qp = F_PATH;
    if (*qp=='\\')
    {	// If there is a \ that means it starts at the root
    	_FF_DIR_ADDR = _FF_ROOT_ADDR;
		qp++;
	}

	sp = path_temp;
	while(*qp)
	{
		if ((valid_file_char(*qp)!=0) || (*qp=='.'))
		{
		  #ifdef _IAR_EWAVR_
			*sp = toupper(*qp);
			sp++;
			qp++;
		  #else
			*sp++ = toupper(*qp++);
		  #endif
		}
		else if (*qp=='\\')
		{
			*sp = 0;	// terminate string
			if (_FF_chdir(path_temp))
			{	// couldn't get to the new dir, Return and Error
				return ((int)EOF);
			}
			sp = path_temp;
			qp++;
		}
		else
			return ((int)EOF);
	}

	*sp = 0;		// terminate string
	return (0);
}

#ifndef _READ_ONLY_

#ifdef _DEBUG_FUNCTIONS_
 #define _DEBUG_MKDIR_
 #ifdef _DEBUG_MKDIR_
  flash char _mkdir_debug_str_[] = "\tMakeDirHere-%02d";
 #endif
#endif

int mkdir(unsigned char *F_PATH)
{
	unsigned char c, c2;
	unsigned char fpath[14], fpath_new[14];
	uHILO16 clus_new, clus_old;
	FILE_DIR_ENTRY *dep;
	uHILO16 calc_time, calc_date;
	unsigned long addr_temp, path_addr_temp;
  #ifdef _BIG_ENDIAN_
	DIR_ENT_DATE ff_date;
	DIR_ENT_TIME ff_time;
  #endif

	addr_temp = _FF_DIR_ADDR;	// save local dir addr
    if (_FF_checkdir(F_PATH, fpath))
	{
		_FF_DIR_ADDR = addr_temp;
	  #ifdef _DEBUG_MKDIR_
		_FF_printf(_mkdir_debug_str_, 1);
	  #endif
		return ((int)EOF);
	}

	dep = scan_directory(fpath, SCAN_DIR_W);		// Find directory space
	if (dep==0)
	{
		_FF_DIR_ADDR = addr_temp;
	  #ifdef _DEBUG_MKDIR_
		_FF_printf(_mkdir_debug_str_, 2);
	  #endif
		return ((int)EOF);
	}

	// Get the filename into a form we can use to compare
	if (file_name_conversion(fpath, fpath_new))
	{
		_FF_error = NAME_ERR;
		_FF_DIR_ADDR = addr_temp;
	  #ifdef _DEBUG_MKDIR_
		_FF_printf(_mkdir_debug_str_, 3);
	  #endif
		return ((int)EOF);
	}

	clus_new.ival = prev_cluster(0);		// find the next avaliable cluster for new directory
	if (_FF_read(_FF_scan_addr, _FF_buff))	// refresh the buffer for making a dir
	{
		_FF_DIR_ADDR = addr_temp;
	  #ifdef _DEBUG_MKDIR_
		_FF_printf(_mkdir_debug_str_, 4);
	  #endif
		return ((int)EOF);
	}

	// Write folder information in current directory
	memcpy(dep->name_entry, fpath_new, 11);		// Write Filename
	dep->attr = ATTR_DIRECTORY;					// Attribute bit auto set to "DIRECTORY"
	dep->reserved = 0;							// Reserved for WinNT
	dep->crt_tenth_sec = 0;						// Mili-second stamp for create
  #ifndef _BIG_ENDIAN_
   #ifdef _RTC_ON_
	rtc_get_timeNdate(&rtc_hour, &rtc_min, &rtc_sec, &rtc_date, &rtc_month, &rtc_year);
	dep->crt_date.date.month = rtc_month;				// File create month
	dep->crt_date.date.day = rtc_date;					// File create day
	dep->crt_date.date.year = (rtc_year - 1980);		// File create year	+ 1980
	dep->crt_time.time.hour = rtc_hour;					// File create hour
	dep->crt_time.time.min = rtc_min;					// File create minute
	dep->crt_time.time.x2sec = (rtc_sec / 2);			// File create seconds / 2
	dep->acc_date.date_word = dep->crt_date.date_word;	// File access date
	dep->mod_date.date_word = dep->crt_date.date_word;	// File modify date
	dep->mod_time.time_word = dep->crt_time.time_word;	// File modify time
   #else
	// Set to 01/01/1980 00:00:00
	// date_word=0x0021		time_word=0x0000
	dep->crt_date.date_word = 0x21;			// File create date
	dep->crt_time.time_word = 0;			// File create time
	dep->acc_date.date_word = 0x21;			// File access date
	dep->mod_date.date_word = 0x21;			// File modify date
	dep->mod_time.time_word = 0;			// File modify time
   #endif
	dep->start_clus_hi = 0;					// 0 for FAT12/16 (2 bytes)
	dep->start_clus_lo = clus_new.ival;		// Starting cluster (2 bytes)
	dep->file_size = 0;						// File length (0 for new)
  #else
   #ifdef _RTC_ON_
	rtc_get_timeNdate(&rtc_hour, &rtc_min, &rtc_sec, &rtc_date, &rtc_month, &rtc_year);
	ff_date.date.month = rtc_month;						// File create month
	ff_date.date.day = rtc_date;						// File create day
	ff_date.date.year = (rtc_year - 1980);				// File create year	+ 1980
	ff_time.time.hour = rtc_hour;						// File create hour
	ff_time.time.min = rtc_min;							// File create minute
	ff_time.time.x2sec = (rtc_sec / 2);					// File create seconds / 2
	dep->crt_date.dateHILO.HI = ff_date.dateHILO.LO;	// File create date
	dep->crt_date.dateHILO.LO = ff_date.dateHILO.HI;
	dep->crt_time.timeHILO.HI = ff_time.timeHILO.LO;	// File create time
	dep->crt_time.timeHILO.LO = ff_time.timeHILO.HI;
	dep->acc_date.date_word = dep->crt_date.date_word;	// File access date
	dep->mod_date.date_word = dep->crt_date.date_word;	// File modify date
	dep->mod_time.time_word = dep->crt_time.time_word;	// File modify time
   #else
	// Set to 01/01/1980 00:00:00
	// date_word=0x2100		time_word=0x0000
	dep->crt_date.date_word = 0x2100;		// File create date
	dep->crt_time.time_word = 0;			// File create time
	dep->acc_date.date_word = 0x2100;		// File access date
	dep->mod_date.date_word = 0x2100;		// File modify date
	dep->mod_time.time_word = 0;			// File modify time
   #endif
	dep->start_clus_hi = 0;							// 0 for FAT12/16 (2 bytes)
	dep->start_clus_lo.cval.LO = clus_new.cval.HI;	// Starting cluster (2 bytes)
	dep->start_clus_lo.cval.HI = clus_new.cval.LO;
	dep->file_size.lval = 0;						// File length (0 for new)
  #endif

	calc_time.ival = dep->crt_time.time_word;		// Save for dir entry, OK backwards for BIG_ENDIAN
	calc_date.ival = dep->crt_date.date_word;		// Save for dir entry, OK backwards for BIG_ENDIAN

	if (_FF_write(_FF_scan_addr, _FF_buff))	// write entry to card
	{
		_FF_DIR_ADDR = addr_temp;
	  #ifdef _DEBUG_MKDIR_
		_FF_printf(_mkdir_debug_str_, 5);
	  #endif
		return ((int)EOF);
	}
	if (write_clus_table(clus_new.ival, 0xFFFF, SINGLE))
	{
		_FF_DIR_ADDR = addr_temp;
	  #ifdef _DEBUG_MKDIR_
		_FF_printf(_mkdir_debug_str_, 6);
	  #endif
		return ((int)EOF);
	}
	dep = (FILE_DIR_ENTRY *) _FF_buff;
	clus_old.ival = addr_to_clust(_FF_DIR_ADDR);		// find the cluster number of this address
	for (c2=1; c2<3; c2++)
	{
		for (c=0; c<c2; c++)
			dep->name_entry[c] = '.';				// set name
		for (c=c2; c<11; c++)
			dep->name_entry[c] = 0x20;				// set name
		dep->attr = ATTR_DIRECTORY;					// set to a directory
		dep->reserved = 0;							// set reserved to 0
		dep->crt_tenth_sec = 0;						// set milisec stamp to 0
		dep->crt_time.time_word = calc_time.ival;	// set create time
		dep->crt_date.date_word = calc_date.ival;	// set create date
		dep->acc_date.date_word = calc_date.ival;	// set access date
		dep->start_clus_hi = 0;						// 0 for FAT12/16
		dep->mod_time.time_word = calc_time.ival;	// set modify time
		dep->mod_date.date_word = calc_date.ival;	// set modify date
		if (c2 == 1)
		{
		  #ifndef _BIG_ENDIAN_
			dep->start_clus_lo = clus_new.ival;				// Starting cluster (2 bytes)
		  #else
			dep->start_clus_lo.cval.LO = clus_new.cval.HI;	// Starting cluster (2 bytes)
			dep->start_clus_lo.cval.HI = clus_new.cval.LO;
		  #endif
		}
		else
		{
		  #ifndef _BIG_ENDIAN_
			dep->start_clus_lo = clus_old.ival;				// Starting cluster (2 bytes)
		  #else
			dep->start_clus_lo.cval.LO = clus_old.cval.HI;	// Starting cluster (2 bytes)
			dep->start_clus_lo.cval.HI = clus_old.cval.LO;
		  #endif
		}
	  #ifndef _BIG_ENDIAN_
		dep->file_size = 0;								// File length (0 for new)
	  #else
		dep->file_size.lval = 0;						// File length (0 for new)
	  #endif
		dep++;
	}
  #ifdef _BytsPerSec_512_
	memset(&_FF_buff[0x40], 0, 448);
  #else
	memset(&_FF_buff[0x40], 0, (BPB_BytsPerSec.ival - 0x40));
  #endif

	path_addr_temp = clust_to_addr(clus_new.ival);

	_FF_DIR_ADDR = addr_temp;	// reset dir addr
	if (_FF_write(path_addr_temp, _FF_buff))
	{
	  #ifdef _DEBUG_MKDIR_
		_FF_printf(_mkdir_debug_str_, 7);
	  #endif
		return ((int)EOF);
	}

	memset(_FF_buff, 0, 0x40);

	for (c=1; c<BPB_SecPerClus; c++)
	{	// Clear the rest of the Directory
		if (_FF_write(path_addr_temp+c, _FF_buff))
		{
		  #ifdef _DEBUG_MKDIR_
			_FF_printf(_mkdir_debug_str_, 8);
		  #endif
			return ((int)EOF);
		}
	}
	return (0);
}

int rmdir(unsigned char *F_PATH)
{
	unsigned char *sp;
	unsigned char fpath[14];
	unsigned int n, i;
	uHILO16 clus_temp;
	FILE_DIR_ENTRY *dep;
	unsigned long addr_temp, path_addr_temp, scan_addr_temp;
	unsigned int ent_max, ent_cntr, dir_clus;
  #ifndef _BytsPerSec_512_
	unsigned int EntPerSec;

	EntPerSec = BPB_BytsPerSec.ival >> 5;		// same as divide by 32
  #endif

	addr_temp = _FF_DIR_ADDR;	// save local dir addr
    if (_FF_checkdir(F_PATH, fpath))
	{
		_FF_DIR_ADDR = addr_temp;
		return ((int)EOF);
	}
	if (fpath[0]==0)
	{
		_FF_DIR_ADDR = addr_temp;
		return ((int)EOF);
	}

    path_addr_temp = _FF_DIR_ADDR;	// save addr for later

	if (_FF_chdir(fpath))	// Change the dir_addr to dir to the one that we are trying to delete
	{	// if Failed, this is not a directory
		_FF_DIR_ADDR = addr_temp;
		return ((int)EOF);
	}
	if ((_FF_DIR_ADDR==_FF_ROOT_ADDR)||(_FF_DIR_ADDR==addr_temp))
	{	// if trying to delete root, or current dir error
		_FF_DIR_ADDR = addr_temp;
		return ((int)EOF);
	}

	if (_FF_DIR_ADDR == _FF_ROOT_ADDR)
		ent_max = BPB_RootEntCnt.ival;		// if this is the root directory, RootEntCnt is the maximum entries
	else
	{	// sub-directory
		dir_clus = addr_to_clust(_FF_DIR_ADDR);		// find the cluster number of this address
		if (dir_clus == 0)
		{
			_FF_DIR_ADDR = addr_temp;
			return ((int)EOF);
		}
	  #ifdef _BytsPerSec_512_ // ent_max should be (BytsPerSec/BytsPerEntry)*(SecPerClus)
		ent_max = (unsigned int) BPB_SecPerClus << 4;		// fast way to multip
	  #else
		ent_max = (BPB_BytsPerSec.ival / EntPerSec) * (unsigned int) BPB_SecPerClus;
	  #endif
	}
	scan_addr_temp = _FF_DIR_ADDR;

	ent_cntr = 0;
	while (ent_cntr < ent_max)
	{
		if (_FF_read(scan_addr_temp, _FF_buff))
		{	// read error
			_FF_DIR_ADDR = addr_temp;
			return (0);
		}
		dep = (FILE_DIR_ENTRY *) _FF_buff;			// set directory entry pointer to the beginning of the entry buffer

	  #ifdef _BytsPerSec_512_
		for (n=0; n<16; n++)
	  #else
		for (n=0; n<EntPerSec; n++)
	  #endif
		{
			sp = (dep->name_entry);
			if (*sp==0)
			{	//
				ent_cntr = ent_max;
				break;
   			}
   			while (valid_file_char(*sp)!=0)
   			{	// if it is a valid character
   				sp++;
   				if (sp == &(dep->attr))
   				{	// a valid file or folder found
   					_FF_DIR_ADDR = addr_temp;
   					return ((int)EOF);
   				}
   			}
    	}
		scan_addr_temp++;
		if (ent_cntr == ent_max)
		{
			i = next_cluster(dir_clus, SINGLE);		// find the next cluster of this folder
			if (i==0)
			{	// this cluster should never point to cluster 0
				_FF_DIR_ADDR = addr_temp;
				return (0);
			}
			else if (i >= 0xFFF5)
			{	// points to EOF
   				break;
			}
			else
			{	// another dir cluster exists, set address to it and
				scan_addr_temp = clust_to_addr(i);
				if (scan_addr_temp == 0)
					return ((int)EOF);
				dir_clus = i;		// set new dir_clus
				ent_cntr = 0;		// reset entry count
				i = 0;				// reset sector count
			}
		}
	}

	// directory empty, delete dir
	_FF_DIR_ADDR = path_addr_temp;	// go back to previous directory

	dep = scan_directory(fpath, SCAN_DIR_R);		// Find directory

	_FF_DIR_ADDR = addr_temp;	// reset address

	//     no more entries  // An Error Occurred            // No file found
	if (dep==0)
		return ((int)EOF);
	if (dep->attr & ATTR_READ_ONLY)
		return ((int)EOF);

  #ifndef _BIG_ENDIAN_
	clus_temp.ival = dep->start_clus_lo;
  #else
	clus_temp.cval.HI = dep->start_clus_lo.cval.LO;
	clus_temp.cval.LO = dep->start_clus_lo.cval.HI;
  #endif
	dep->name_entry[0] = 0xE5;

	if (_FF_write(path_addr_temp, _FF_buff))
		return ((int)EOF);
	if (erase_clus_chain(clus_temp.ival))
		return ((int)EOF);

    return (0);
}
#endif

 #if defined(_CVAVR_) || defined(_ICCAVR_)
int chdirc(unsigned char flash *F_PATH)
 #else
int chdirc(PGM_P F_PATH)
 #endif
{
	unsigned char temp_path[_FF_PATH_LENGTH];

	_FF_strcpyf(temp_path, F_PATH);

	return (chdir(temp_path));
}


#ifdef _DEBUG_FUNCTIONS_
 #define _CHDIR_DEBUG_
 #ifdef _CHDIR_DEBUG_
  char flash _chdir_debug_str_[] = "\tChDirHere-%02d";
  char flash _chdir_val_str_[] = " %02X";
 #endif
#endif

// This function changes the working directory path for the
// File System.  _FF_DIR_ADDR is changed to how the input
// string directs it.  A NULL is returned upon success, or
// an EOF with no changes for a Failure.
int chdir(unsigned char *F_PATH)
{
	unsigned char *qp, *sp, fpath[14];
  #ifdef _CVAVR_
	char c;
  #endif
  #ifdef _CHDIR_DEBUG_
	unsigned char cnt;
  #endif
	unsigned long addr_temp;

	addr_temp = _FF_DIR_ADDR;	// save initial dir addr

	if (_FF_checkdir(F_PATH, fpath))
	{
		_FF_DIR_ADDR = addr_temp;
	  #ifdef _CHDIR_DEBUG_
		_FF_printf(_chdir_debug_str_, 1);
	  #endif
		return ((int)EOF);
	}
	if (fpath[0])
	{
		if (_FF_chdir(fpath))	// Change the dir_addr to dir to the one that we are trying to delete
		{	// if Failed, this is not a directory
			_FF_DIR_ADDR = addr_temp;
		  #ifdef _CHDIR_DEBUG_
			_FF_printf(_chdir_debug_str_, 2);
		  #endif
			return ((int)EOF);
		}
	}

	sp = F_PATH;
	if (*sp ==  '\\')
	{
		_FF_FULL_PATH[1] = 0;
		sp++;
	}

  #ifdef _CHDIR_DEBUG_
	cnt = 0;
  #endif
	while (*sp)
	{
	  #ifdef _CHDIR_DEBUG_
		_FF_printf(_chdir_debug_str_, 5+(cnt++));
	  #endif
		qp = &_FF_FULL_PATH[_FF_strlen(_FF_FULL_PATH)];
		if ((sp[0] == '.') && (sp[1] == '.'))
		{	// go back a directory
		  #if defined(_ICCAVR_) || defined(_IAR_EWAVR_)
			qp = _FF_strrchr(_FF_FULL_PATH, '\\');
			if (qp == NULL)
			{
				_FF_DIR_ADDR = addr_temp;
			  #ifdef _CHDIR_DEBUG_
				_FF_printf(_chdir_debug_str_, 3);
			  #endif
				return ((int)EOF);
			}
			*qp = 0;		// take off the '\' so nex reverse compare works
			qp = _FF_strrchr(_FF_FULL_PATH, '\\') + 1;
			if (qp == NULL)
			{
				_FF_DIR_ADDR = addr_temp;
			  #ifdef _CHDIR_DEBUG_
				_FF_printf(_chdir_debug_str_, 4);
			  #endif
				return ((int)EOF);
			}
			*qp = 0;		// new termination
		  #endif
		  #ifdef _CVAVR_
		   #pragma warn-
			c = strrpos(_FF_FULL_PATH, '\\');
			if (c<0)
			{
				_FF_DIR_ADDR = addr_temp;
			  #ifdef _CHDIR_DEBUG_
				_FF_printf(_chdir_debug_str_, 5);
			  #endif
				return ((int)EOF);
			}
			_FF_FULL_PATH[c] = 0;
			c = strrpos(_FF_FULL_PATH, '\\');
			if (c<0)
			{
				_FF_DIR_ADDR = addr_temp;
			  #ifdef _CHDIR_DEBUG_
				_FF_printf(_chdir_debug_str_, 6);
			  #endif
				return ((int)EOF);
			}
			_FF_FULL_PATH[c+1] = 0;
		   #pragma warn+
		  #endif
			sp += 2;
		}
		else
		{
			while ((*sp != '\\') && (*sp != 0))
			{
			  #ifdef _CHDIR_DEBUG_
				_FF_printf(_chdir_val_str_, toupper(*sp));
			  #endif
			  #if defined(_IAR_EWAVR_)
				*qp = toupper(*sp);
				qp++;	sp++;
			  #else
				*qp++ = toupper(*sp++);
			  #endif
			}

		  #if defined(_IAR_EWAVR_)
			*qp = '\\';
			qp++;
			*qp = 0;
			if ((sp[0]==0) || (sp[1]==0))
		  #else
			*qp++ = '\\';
			*qp = 0;
			if ((sp[0]==0) || (sp[1]==0))
		  #endif
			{
				break;
			}
		}
		if (*sp)		// if there is more increment
			sp++;
	}
	return (0);
}

#ifdef _DEBUG_FUNCTIONS_
 #define _FF_CHDIR_DEBUG_
 #ifdef _FF_CHDIR_DEBUG_
  char flash _ff_chdir_debug_str_[] = "\tFFChDir-%02d";
 #endif
#endif
// Function to change directories one at a time, not effecting the working dir string
char _FF_chdir(unsigned char *F_PATH)
{
	uHILO16 m;
	unsigned long addr_temp;
	FILE_DIR_ENTRY *dep;

	if ((F_PATH[0]=='.') && (F_PATH[1]=='.') && (F_PATH[2]==0))
	{	// trying to get back to prev dir
		if (_FF_DIR_ADDR == _FF_ROOT_ADDR)		// already as far back as can go
		{
		  #ifdef _FF_CHDIR_DEBUG_
			_FF_printf(_ff_chdir_debug_str_, 1);
		  #endif
			return ((char)EOF);
		}
		if (_FF_read(_FF_DIR_ADDR, _FF_buff))
		{
		  #ifdef _FF_CHDIR_DEBUG_
			_FF_printf(_ff_chdir_debug_str_, 2);
		  #endif
			return ((char)EOF);
		}
		m.cval.HI = _FF_buff[0x3B];
		m.cval.LO = _FF_buff[0x3A];
		if (m.ival > 1)
		{
			addr_temp = clust_to_addr(m.ival);
			if (addr_temp == 0)
			{
			  #ifdef _FF_CHDIR_DEBUG_
				_FF_printf(_ff_chdir_debug_str_, 3);
			  #endif
				return ((char)EOF);
			}
			_FF_DIR_ADDR = addr_temp;
		}
		else
			_FF_DIR_ADDR = _FF_ROOT_ADDR;
		return (0);
	}

	// Find the dir and change locations
	addr_temp = _FF_DIR_ADDR;
	dep = scan_directory(F_PATH, SCAN_DIR_R);		// Find directory
	if (dep==0)
	{	// An Error Occurred		// No folder found
	  #ifdef _DIRECTORIES_SUPPORTED_
		_FF_DIR_ADDR = addr_temp;
	  #endif
	  #ifdef _FF_CHDIR_DEBUG_
		_FF_printf(_ff_chdir_debug_str_, 4);
	  #endif
		return ((char)EOF);
	}

	// found the location of the new file/dir entry
  #ifndef _BIG_ENDIAN_
	m.ival = dep->start_clus_lo;
  #else
	m.cval.HI = dep->start_clus_lo.cval.LO;
	m.cval.LO = dep->start_clus_lo.cval.HI;
  #endif
	if (m.ival < 2)								// not a valid directory entry
	{
	  #ifdef _FF_CHDIR_DEBUG_
		_FF_printf(_ff_chdir_debug_str_, 5);
	  #endif
		return ((char)EOF);
	}

	_FF_DIR_ADDR = clust_to_addr(m.ival);		// Set DIR_ADDR to the new location
	if (_FF_DIR_ADDR == 0)
	{	// An Error Occurred		// No folder found
		_FF_DIR_ADDR = addr_temp;
	  #ifdef _FF_CHDIR_DEBUG_
		_FF_printf(_ff_chdir_debug_str_, 6);
	  #endif
		return ((char)EOF);
	}
	return (0);
}
#endif


#if !defined(_SECOND_FAT_ON_) && !defined(_READ_ONLY_)
// Function that clears the secondary FAT table
int clear_second_FAT(void)
{
	unsigned int c, d;
	unsigned long n;

  #ifdef _BytsPerSec_512_
	memset(_FF_buff, 0, 512);
  #else
	memset(_FF_buff, 0, BPB_BytsPerSec.ival);
  #endif
	for (n=1; n<BPB_FATSz16.ival; n++)
	{
		if (_FF_write(_FF_FAT2_ADDR+n, _FF_buff))
			return ((int)EOF);
	}

	_FF_buff[0] = 0xF8;
	_FF_buff[1] = 0xFF;
	_FF_buff[2] = 0xFF;
	if (BPB_FATType == 0x36)
		_FF_buff[3] = 0xFF;
	if (_FF_write(_FF_FAT2_ADDR, _FF_buff))
		return ((int)EOF);

	return (1);
}
#endif

// Open a file, name stored in string fileopen
 #if defined(_CVAVR_) || defined(_ICCAVR_)
FILE *fopenc(unsigned char flash *NAMEC, unsigned char MODEC)
 #else
FILE *fopenc(PGM_P NAMEC, unsigned char MODEC)
 #endif
{
	unsigned char temp_path[_FF_PATH_LENGTH];

	_FF_strcpyf(temp_path, NAMEC);

    //print("directories supported disabled 0\r\n");

	return(fopen(temp_path, MODEC));
}

#ifdef _DEBUG_FUNCTIONS_
 #define _DEBUG_FOPEN_
 #ifdef _DEBUG_FOPEN_
  unsigned char flash _fopen_debug_str_[] = "\r\nFopenERR-%02d";
  unsigned char flash _fopen_details_str_[] = "\r\nOpened [mode = %d]:\r\n ";
 #endif
#endif

FILE *fopen(unsigned char *NAME, unsigned char MODE)
{
  //#ifdef _DIRECTORIES_SUPPORTED_
	unsigned char fpath[14];
 	unsigned long addr_temp;
  //#endif
//	unsigned char fpath_new[14];
	FILE *rp;

    //print("directories supported disabled 1\r\n"); //p

  #ifdef _DEBUG_FOPEN_
	unsigned char c, *p;
  #endif
  #ifdef _BIG_ENDIAN_
	uHILO32 temp32;
  #endif

  #ifdef _READ_ONLY_
	if (MODE!=READ)
	{
	  #ifdef _DEBUG_FOPEN_
		_FF_printf(_fopen_debug_str_, 1);
	  #endif
		return (NULL);
	}
  #endif

    rp = _FF_MALLOC(); //IT'S THIS THAT ISN'T WORKING

	if (rp == 0)
	{	// Could not allocate requested memory
        //print("directories supported disabled 1\r\n");
		_FF_error = ALLOC_ERR;
	  #ifdef _DEBUG_FOPEN_
		_FF_printf(_fopen_debug_str_, 2);
	  #endif
		return (NULL);
	}

  //#ifdef _DIRECTORIES_SUPPORTED_
  //print("directories supported disabled 2\r\n");
	addr_temp = _FF_DIR_ADDR;	// save local dir addr
    if (_FF_checkdir(NAME, fpath))
	{
		_FF_DIR_ADDR = addr_temp;
		_FF_free(rp);
	  #ifdef _DEBUG_FOPEN_
		_FF_printf(_fopen_debug_str_, 3);
	  #endif
		return (NULL);
	}
	if (fpath[0]==0)
	{
		_FF_DIR_ADDR = addr_temp;
		_FF_free(rp);
	  #ifdef _DEBUG_FOPEN_
		_FF_printf(_fopen_debug_str_, 4);
	  #endif
		return (NULL);
	}
	(rp->file_dep) = scan_directory(fpath, SCAN_FILE_R);		// Find File
  //#else
	//(rp->file_dep) = scan_directory(NAME, SCAN_FILE_R);		// Find File
  //#endif

	if ((rp->file_dep) == NULL)
	{	// An Error Occurred            // No file found
	  //#ifdef _DIRECTORIES_SUPPORTED_
		_FF_DIR_ADDR = addr_temp;
	  //#endif
		_FF_free(rp);
	  #ifdef _DEBUG_FOPEN_
		_FF_printf(_fopen_debug_str_, 5);
	  #endif
		return (NULL);
	}

  #ifndef _READ_ONLY_
	if ((!(MODE==READ)) && (((rp->file_dep)->attr) & ATTR_READ_ONLY))
	{	// if writing to file verify it is not "READ ONLY"
		_FF_error = MODE_ERR;
		_FF_free(rp);
	  //#ifdef _DIRECTORIES_SUPPORTED_
		_FF_DIR_ADDR = addr_temp;
	  //#endif
	  #ifdef _DEBUG_FOPEN_
		_FF_printf(_fopen_debug_str_, 7);
	  #endif
		return (NULL);
	}
  #endif

	// Save Starting Cluster
  #ifndef _BIG_ENDIAN_
	rp->clus_start = (rp->file_dep)->start_clus_lo;
  #else
	temp32.cval.ML = (rp->file_dep)->start_clus_lo.cval.LO;
	temp32.cval.LO = (rp->file_dep)->start_clus_lo.cval.HI;
	rp->clus_start = temp32.ival.LO;
  #endif
	// Set Current Cluster
	rp->clus_current = rp->clus_start;
	// Set Previous Cluster to 0 (indicating @start)
	rp->clus_prev = 0;
	// Save file length
  #ifndef _BIG_ENDIAN_
	rp->length = (rp->file_dep)->file_size;
  #else
	temp32.cval.HI = (rp->file_dep)->file_size.cval.LO;
	temp32.cval.MH = (rp->file_dep)->file_size.cval.ML;
	temp32.cval.ML = (rp->file_dep)->file_size.cval.MH;
	temp32.cval.LO = (rp->file_dep)->file_size.cval.HI;
	rp->length = temp32.lval;
  #endif
	// Set Current Position to 0
	rp->position = 0;
  #ifndef _READ_ONLY_
	if ((rp->length==0) && (rp->clus_start==0))
	{	// Check for Blank File
	  #ifndef _READ_ONLY_
		if (MODE==READ)
	  #endif
		{	// IF trying to open a blank file to read, ERROR
			_FF_error = MODE_ERR;
			_FF_free(rp);
		  //#ifdef _DIRECTORIES_SUPPORTED_
			_FF_DIR_ADDR = addr_temp;
		  //#endif
		  #ifdef _DEBUG_FOPEN_
			_FF_printf(_fopen_debug_str_, 11);
		  #endif
			return (NULL);
		}
		//Setup blank FILE characteristics
	  #ifndef _READ_ONLY_
		MODE = WRITE;
	  #endif
	}
	if (MODE==WRITE)
	{	// Change file to blank
		if (rp->clus_start)
		{
			// Write new directory info so that the file entry is now blank
		  #ifndef _BIG_ENDIAN_
			(rp->file_dep)->file_size = 0;
			(rp->file_dep)->start_clus_lo = 0;
		  #else
			(rp->file_dep)->file_size.lval = 0;
			(rp->file_dep)->start_clus_lo.ival = 0;
		  #endif
			if (_FF_write(_FF_scan_addr, _FF_buff))
			{
				_FF_free(rp);
			  //#ifdef _DIRECTORIES_SUPPORTED_
				_FF_DIR_ADDR = addr_temp;
			  //#endif
			  #ifdef _DEBUG_FOPEN_
				_FF_printf(_fopen_debug_str_, 8);
			  #endif
				return (NULL);
			}
			rp->length = 0;			// Clear length in pointer
			// Erase the cluster chain of the current file
			if (erase_clus_chain(rp->clus_start))
			{
				_FF_free(rp);
			  //#ifdef _DIRECTORIES_SUPPORTED_
				_FF_DIR_ADDR = addr_temp;
			  //#endif
			  #ifdef _DEBUG_FOPEN_
				_FF_printf(_fopen_debug_str_, 9);
			  #endif
				return (NULL);
			}
			rp->clus_start = 0;		// Clear starting cluster in pointer
			rp->clus_current = 0;
		}
	}
	else
  #endif
	{
		// Set and save next cluster #
		rp->clus_next = next_cluster(rp->clus_current, SINGLE);
		if (NextClusterValidFlag == 0)
		{	// Not valid cluster
			_FF_free(rp);
		  //#ifdef _DIRECTORIES_SUPPORTED_
			_FF_DIR_ADDR = addr_temp;
		  //#endif
		  #ifdef _DEBUG_FOPEN_
			_FF_printf(_fopen_debug_str_, 10);
		  #endif
			return (NULL);
		}
	}
	// Save the file offset to read entry
	rp->entry_sec_addr = _FF_scan_addr;
	// Set sector offset to 0
	rp->sec_offset = 0;
  #ifndef _READ_ONLY_
	if (MODE==APPEND)
	{
		rp->EOF_flag = 1;
		if (fseek(rp, 0,SEEK_END)==(int)EOF)
		{
			_FF_free(rp);
		  //#ifdef _DIRECTORIES_SUPPORTED_
			_FF_DIR_ADDR = addr_temp;
		  //#endif
		  #ifdef _DEBUG_FOPEN_
			_FF_printf(_fopen_debug_str_, 12);
		  #endif
			return (NULL);
		}
	}
	else
	{	// Set pointer to the begining of the file
		if (MODE==READ)
  #endif
		{
		  #ifndef _NO_FILE_DATA_BUFFER_
			if (_FF_read(clust_to_addr(rp->clus_current), rp->buff))
		  #else
			if (_FF_read(clust_to_addr(rp->clus_current), _FF_buff))
		  #endif
	 		{
				_FF_free(rp);
			  //#ifdef _DIRECTORIES_SUPPORTED_
				_FF_DIR_ADDR = addr_temp;
			  //#endif
			  #ifdef _DEBUG_FOPEN_
				_FF_printf(_fopen_debug_str_, 13);
			  #endif
				return (NULL);
			}
		}
	  #ifndef _NO_FILE_DATA_BUFFER_
		rp->pntr = rp->buff;
	  #else
		rp->pntr = _FF_buff;
	  #endif
  #ifndef _READ_ONLY_
	}
  #endif
	rp->mode = MODE;
	_FF_error = NO_ERR;
  //#ifdef _DIRECTORIES_SUPPORTED_
	_FF_DIR_ADDR = addr_temp;
  //#endif
  #ifdef _DEBUG_FCREATE_
	_FF_printf("\r\nAddress Entry @ 0x%lX - FOPEN", _FF_scan_addr<<9);
	dump_file_entry_hex(rp->file_dep);
 	_FF_printf("\r\nfopen success mode-%d:", rp->mode);
 	_FF_printf("\r\n  clus_start - %X", rp->clus_start);
 	_FF_printf("\r\n  size", rp->);
 	_FF_printf("\r\n ", rp->);
  #endif

	return(rp);
}


#ifndef _READ_ONLY_
// Create a file
  #if defined(_CVAVR_) || defined(_ICCAVR_)
FILE *fcreatec(unsigned char flash *NAMEC, unsigned char MODE)
  #else
FILE *fcreatec(PGM_P NAMEC, unsigned char MODE)
  #endif
{
	unsigned char temp_path[_FF_PATH_LENGTH];

    _FF_strcpyf(temp_path, NAMEC);
	return (fcreate(temp_path, MODE));
}

#ifdef _DEBUG_FUNCTIONS_
 #define _DEBUG_FCREATE_
 #ifdef _DEBUG_FCREATE_
  char flash _fcreate_debug_str_[] = "\tFcreateERR-%02d";
  char flash _fcreate_info_str_[] = "\r\nFCREATE(\"%s\", %d";
  char flash _fcreate_addr_str_[] = "\r\nAddress Entry @ 0x%lX - FCREATE";
 #endif
#endif

FILE *fcreate(unsigned char *NAME, unsigned char MODE)
{
  #ifdef _DIRECTORIES_SUPPORTED_
	unsigned char fpath[14];
	unsigned long addr_temp;
  #endif
	DIR_ENT_TIME ff_time;
	DIR_ENT_DATE ff_date;
	unsigned int clus_temp = 0;
	unsigned char fpath_new[14];
	FILE *rp;

    //SJL - CAVR2 - debug
    //print("Attempting to create a file on the memory card...");
    //print("\r\nFCREATE");
    //print(NAME);
    //print("\r\n");

  #ifdef _DEBUG_FCREATE_
	_FF_printf(_fcreate_info_str_, NAME, MODE);
  #endif
	rp = _FF_MALLOC();	//SJL - CAVR2 - malloc(sizeof(FILE))
    //rp = malloc(sizeof(FILE));

	if (rp == NULL)
	{	// make sure we have enough SRAM to open the file
	  #ifdef _DEBUG_FCREATE_
		_FF_printf(_fcreate_debug_str_, 1);
	  #endif
      	//print("file still hasn't been created and returning null");
		return (NULL);
	}

  #ifdef _DIRECTORIES_SUPPORTED_
  	// Get the Proper path set for the file (if nessisary)
	addr_temp = _FF_DIR_ADDR;	// save local dir addr
	if (_FF_checkdir(NAME, fpath))
	{
		_FF_error = PATH_ERR;
		_FF_DIR_ADDR = addr_temp;
	  #ifdef _DEBUG_FCREATE_
		_FF_printf(_fcreate_debug_str_, 2);
	  #endif
		return (NULL);
	}
	if (fpath[0]==0)
	{
		_FF_error = NAME_ERR;
		_FF_DIR_ADDR = addr_temp;
	  #ifdef _DEBUG_FCREATE_
		_FF_printf(_fcreate_debug_str_, 3);
	  #endif
		return (NULL);
	}

	(rp->file_dep) = scan_directory(fpath, SCAN_FILE_W);		// Find File

	// Set the working directory back to what it was
	_FF_DIR_ADDR = addr_temp;
  #else
	(rp->file_dep) = scan_directory(NAME, SCAN_FILE_W);			// Find File
  #endif

	if ((rp->file_dep)==0)
	{	// An error occurred
	  #ifdef _DEBUG_FCREATE_
		_FF_printf(_fcreate_debug_str_, 4);
	  #endif
		_FF_free(rp);
		return (NULL);
	}

  #ifdef _DIRECTORIES_SUPPORTED_
	// Get the filename into a form we can use to compare
	if (file_name_conversion(fpath, fpath_new))
  #else
	// Get the filename into a form we can use to compare
	if (file_name_conversion(NAME, fpath_new))
  #endif
	{
		_FF_error = NAME_ERR;
	  #ifdef _DIRECTORIES_SUPPORTED_
		_FF_DIR_ADDR = addr_temp;
	  #endif
	  #ifdef _DEBUG_FCREATE_
		_FF_printf(_fcreate_debug_str_, 5);
	  #endif
		return (NULL);
	}

	// Setup the Time/Date stamps
  #ifdef _RTC_ON_
	rtc_get_timeNdate(&rtc_hour, &rtc_min, &rtc_sec, &rtc_date, &rtc_month, &rtc_year);
	ff_date.date.month = rtc_month;						// Current month
	ff_date.date.day = rtc_date;						// Current day
	ff_date.date.year = (rtc_year - 1980);				// Current year	+ 1980
	ff_time.time.hour = rtc_hour;						// Current hour
	ff_time.time.min = rtc_min;							// Current minute
	ff_time.time.x2sec = (rtc_sec / 2);					// Current seconds / 2
  #else
	// Set to 01/01/1980 00:00:00
	// date_word=0x0021		time_word=0x0000
	ff_date.date_word = 0x21;							// Make date
	ff_time.time_word = 0;								// Make time
  #endif

	if ((((rp->file_dep)->name_entry[0])!=0) && (((rp->file_dep)->name_entry[0])!=0xE5))
	{	// found old entry
		if (((rp->file_dep)->attr) & ATTR_READ_ONLY)		// is file read only
		{
			_FF_error = READONLY_ERR;
		  #ifdef _DEBUG_FCREATE_
			_FF_printf(_fcreate_debug_str_, 6);
		  #endif
			_FF_free(rp);
			return (NULL);
		}
		else
		{	// clear old file data
		  #ifndef _BIG_ENDIAN_
			clus_temp = rp->file_dep->start_clus_lo;
		  #else
			clus_temp = rp->file_dep->start_clus_lo.ival;
		  #endif
		}
	}
	else
	{
		memcpy((rp->file_dep)->name_entry, fpath_new, 11);		// Write Filename
		(rp->file_dep)->crt_tenth_sec = 0;						// Mili-second stamp for create

	  #ifndef _BIG_ENDIAN_
		(rp->file_dep)->crt_date.date_word = ff_date.date_word;		// File create date
		(rp->file_dep)->crt_time.time_word = ff_time.time_word;		// File create time
	  #else
		(rp->file_dep)->crt_date.dateHILO.HI = ff_date.dateHILO.LO;	// File create date
		(rp->file_dep)->crt_date.dateHILO.LO = ff_date.dateHILO.HI;
		(rp->file_dep)->crt_time.timeHILO.HI = ff_time.timeHILO.LO;	// File create time
		(rp->file_dep)->crt_time.timeHILO.LO = ff_time.timeHILO.HI;
	  #endif
	}

	(rp->file_dep)->attr = MODE;								// Attribute bit
	(rp->file_dep)->reserved = 0;								// Reserved for WinNT
  #ifndef _BIG_ENDIAN_
	(rp->file_dep)->acc_date.date_word = ff_date.date_word;	// File access date
	(rp->file_dep)->mod_date.date_word = ff_date.date_word;	// File modify date
	(rp->file_dep)->mod_time.time_word = ff_time.time_word;	// File modify time
	(rp->file_dep)->start_clus_hi = 0;							// 0 for FAT12/16 (2 bytes)
	(rp->file_dep)->start_clus_lo = 0;							// Starting cluster (2 bytes)
	(rp->file_dep)->file_size = 0;								// File length (0 for new)
  #else
	(rp->file_dep)->acc_date.dateHILO.HI = ff_date.dateHILO.LO;	// File access date
	(rp->file_dep)->acc_date.dateHILO.LO = ff_date.dateHILO.HI;
	(rp->file_dep)->mod_date.dateHILO.HI = ff_date.dateHILO.LO;	// File modify date
	(rp->file_dep)->mod_date.dateHILO.LO = ff_date.dateHILO.HI;
	(rp->file_dep)->mod_time.timeHILO.HI = ff_time.timeHILO.LO;	// File modify time
	(rp->file_dep)->mod_time.timeHILO.LO = ff_time.timeHILO.HI;
	(rp->file_dep)->start_clus_hi = 0;							// 0 for FAT12/16 (2 bytes)
	(rp->file_dep)->start_clus_lo.ival = 0;					// Starting cluster (2 bytes)
	(rp->file_dep)->file_size.lval = 0;						// File length (0 for new)
  #endif

	if (_FF_write(_FF_scan_addr, _FF_buff))
	{
		_FF_error = WRITE_ERR;
	  #ifdef _DEBUG_FCREATE_
		_FF_printf(_fcreate_debug_str_, 7);
	  #endif
		_FF_free(rp);
		return (NULL);
	}

  #ifdef _DEBUG_FCREATE_
	_FF_printf(_fcreate_addr_str_, _FF_scan_addr<<9);
	dump_file_entry_hex((rp->file_dep));
  #endif

	if (clus_temp)
	{	// a chain needs to be destroyed
		if (erase_clus_chain(clus_temp))
		{
		  #ifdef _DEBUG_FCREATE_
			_FF_printf(_fcreate_debug_str_, 8);
		  #endif
			_FF_free(rp);
			return (NULL);
		}
	}

 	// Setup the FILE pointer to new file conditions
	rp->clus_prev = 0;
	rp->clus_start = 0;
	rp->length = 0;
	rp->position = 0;
	rp->mode = WRITE;
	rp->sec_offset = 0;
	rp->entry_sec_addr = _FF_scan_addr;
	rp->EOF_flag = 0;
  #ifndef _NO_FILE_DATA_BUFFER_
	rp->pntr = rp->buff;
  #else
	rp->pntr = _FF_buff;
  #endif

	_FF_error = NO_ERR;

	return (rp);
}
#endif

#ifndef _READ_ONLY_
// Open a file, name stored in string fileopen
  #if defined(_CVAVR_) || defined(_ICCAVR_)
int removec(unsigned char flash *NAMEC)
  #else
int removec(PGM_P NAMEC)
  #endif
{
	unsigned char temp_path[_FF_PATH_LENGTH];

	_FF_strcpyf(temp_path, NAMEC);

	return (remove(temp_path));
}

// Remove a file from the root directory
int remove(unsigned char *NAME)
{
  #ifdef _DIRECTORIES_SUPPORTED_
	unsigned char fpath[14];
	unsigned long addr_temp;
  #endif
	FILE_DIR_ENTRY *dep;

  #ifdef _DIRECTORIES_SUPPORTED_
    addr_temp = _FF_DIR_ADDR;	// save local dir addr
    if (_FF_checkdir(NAME, fpath))
	{
		_FF_error = PATH_ERR;
		_FF_DIR_ADDR = addr_temp;
		return ((int)EOF);
	}
	if (fpath[0]==0)
	{
		_FF_error = NAME_ERR;
		_FF_DIR_ADDR = addr_temp;
		return ((int)EOF);
	}
	dep = scan_directory(fpath, SCAN_FILE_R);		// Find File
  #else
	dep = scan_directory(NAME, SCAN_FILE_R);		// Find File
  #endif

	if (dep==0)
	{	// No more space    // An Error Occurred
		_FF_error = NO_ENTRY_AVAL;
	  #ifdef _DIRECTORIES_SUPPORTED_
		_FF_DIR_ADDR = addr_temp;
	  #endif
		return ((int)EOF);
	}

	// Erase entry (put 0xE5 into start of the filename
	dep->name_entry[0] = 0xE5;
	if (_FF_write(_FF_scan_addr, _FF_buff))
	{
		_FF_error = WRITE_ERR;
		return ((int)EOF);
	}

	// Destroy cluster chain
	if (erase_clus_chain(dep->start_clus_lo))
		return ((int)EOF);

	return (0);
}
#endif

#ifndef _READ_ONLY_
// Rename a file in the Root Directory
int rename(unsigned char *NAME_OLD, unsigned char *NAME_NEW)
{
	FILE_DIR_ENTRY *dep;
	unsigned char fpath_new[14];
  #ifdef _DIRECTORIES_SUPPORTED_
	unsigned char fpath[14];
	unsigned long addr_temp;
  #endif

	// Get the filename into a form we can use to compare
	if (file_name_conversion(NAME_NEW, fpath_new))
	{
		_FF_error = NAME_ERR;
		return ((int)EOF);
	}

  #ifdef _DIRECTORIES_SUPPORTED_
    addr_temp = _FF_DIR_ADDR;	// save local dir addr
    if (_FF_checkdir(NAME_OLD, fpath))
	{
		_FF_error = PATH_ERR;
		_FF_DIR_ADDR = addr_temp;
		return ((int)EOF);
	}
	if (fpath[0]==0)
	{
		_FF_error = NAME_ERR;
		_FF_DIR_ADDR = addr_temp;
		return ((int)EOF);
	}
  #endif

    // see if the new filename already exists
	dep = scan_directory(NAME_NEW, SCAN_FILE_R);		// Find File

	if (dep != 0)
	{	// new filename alread exists or error
	  #ifdef _DIRECTORIES_SUPPORTED_
		_FF_DIR_ADDR = addr_temp;
	  #endif
		_FF_error = EXIST_ERR;
		return ((int)EOF);
	}

	// find the existing file entry
  #ifdef _DIRECTORIES_SUPPORTED_
	dep = scan_directory(fpath, SCAN_FILE_R);		// Find File
  #else
	dep = scan_directory(NAME_OLD, SCAN_FILE_R);		// Find File
  #endif

	if (dep==0)
	{	// An Error Occurred
	  #ifdef _DIRECTORIES_SUPPORTED_
		_FF_DIR_ADDR = addr_temp;
	  #endif
		return ((int)EOF);
	}

	// Rename entry
	memcpy(dep->name_entry, fpath_new, 11);
	if (_FF_write(_FF_scan_addr, _FF_buff))
		return ((int)EOF);

	return(0);
}
#endif

#ifndef _READ_ONLY_

#ifdef _DEBUG_FUNCTIONS_
 #define _DEBUG_FFLUSH_
 #ifdef _DEBUG_FFLUSH_
  char flash _debug_fflush_str_[] = "\tFFlushHere-%02d";
  char flash FFLUSH_STR[] = "\r\nFFLUSH Called";
 #endif
#endif

// Save Contents of file, w/o closing
int fflush(FILE *rp)
{
	unsigned long addr_temp;

  #ifdef _DEBUG_FFLUSH_
	_FF_printf(FFLUSH_STR);
  #endif

	if (rp==NULL)
	{
	  #ifdef _DEBUG_FFLUSH_
		_FF_printf(_debug_fflush_str_, 1);
	  #endif
		return ((int)EOF);
	}
	else if (rp->mode==READ)
	{
	  #ifdef _DEBUG_FFLUSH_
		_FF_printf(_debug_fflush_str_, 2);
	  #endif
		return ((int)EOF);
	}

	if (rp->clus_current == 0)
	{
		if (rp->length != 0)
		{
		  #ifdef _DEBUG_FFLUSH_
			_FF_printf(_debug_fflush_str_, 3);
		  #endif
			return ((int)EOF);
		}
	}
	else if ((rp->EOF_flag) == 0)	// Info already saved
	{
		addr_temp = clust_to_addr(rp->clus_current);
		if (addr_temp == 0)
		{
		  #ifdef _DEBUG_FFLUSH_
			_FF_printf(_debug_fflush_str_, 4);
		  #endif
			return((int)EOF);
		}
		addr_temp += (rp->sec_offset);

		if (_FF_write(addr_temp, rp->buff))	// Write SD buffer to disk
		{
		  #ifdef _DEBUG_FFLUSH_
			_FF_printf(_debug_fflush_str_, 5);
		  #endif
			return ((int)EOF);
		}
	  #ifndef _NO_FILE_DATA_BUFFER_
		if (_FF_write(addr_temp, rp->buff))	// Write SD buffer to disk
	  #else
		if (_FF_write(addr_temp, _FF_buff))	// Write SD buffer to disk
	  #endif
		{
		  #ifdef _DEBUG_FFLUSH_
			_FF_printf(_debug_fflush_str_, 4);
		  #endif
			return ((int)EOF);
		}
	}
	if (append_toc(rp))	// Update Entry or Error
	{
	  #ifdef _DEBUG_FFLUSH_
		_FF_printf(_debug_fflush_str_, 5);
	  #endif
		return ((int)EOF);
	}

  #ifdef _NO_FILE_DATA_BUFFER_
	if (_FF_read(addr_temp, _FF_buff))	// Read SD So file data is back in _FF_buff
	{
	  #ifdef _DEBUG_FFLUSH_
		_FF_printf(_debug_fflush_str_, 6);
	  #endif
		return ((int)EOF);
	}
  #endif

	return (0);
}
#endif


// Close an open file
int fclose(FILE *rp)
{
  #ifndef _READ_ONLY_
	if (rp->mode!=READ)
		if (fflush(rp)==(int)EOF)
			return ((int)EOF);
  #endif
	// Clear File Structure
	_FF_free(rp);

	return(0);
}

int ffreemem(FILE *rp)
{
	// Clear File Structure
	if (rp==0)
		return ((int)EOF);
	_FF_free(rp);
	return(0);
}

 #if defined(_CVAVR_) || defined(_ICCAVR_)
int fget_file_infoc(unsigned char flash *NAMEC, unsigned long *F_SIZE, unsigned char *F_CREATE,
				unsigned char *F_MODIFY, unsigned char *F_ATTRIBUTE, unsigned int *F_CLUS_START)
 #else
int fget_file_infoc(PGM_P NAMEC, unsigned long *F_SIZE, unsigned char *F_CREATE,
				unsigned char *F_MODIFY, unsigned char *F_ATTRIBUTE, unsigned int *F_CLUS_START)
 #endif
{
	unsigned char temp_path[_FF_PATH_LENGTH];

	_FF_strcpyf(temp_path, NAMEC);

	return (fget_file_info(temp_path, F_SIZE, F_CREATE, F_MODIFY, F_ATTRIBUTE, F_CLUS_START));
}

char flash _time_date_str[] = "%02d/%02d/%04d %02d:%02d:%02d";

int fget_file_info(unsigned char *NAME, unsigned long *F_SIZE, unsigned char *F_CREATE,
				unsigned char *F_MODIFY, unsigned char *F_ATTRIBUTE, unsigned int *F_CLUS_START)
{
    FILE_DIR_ENTRY *dep;
  #ifdef _BIG_ENDIAN_
	DIR_ENT_TIME ff_time;
	DIR_ENT_DATE ff_date;
	uHILO32 temp32;
	uHILO16 temp16;
  #endif
  #ifdef _DIRECTORIES_SUPPORTED_
	unsigned char fpath[14];
    unsigned long addr_temp;

    addr_temp = _FF_DIR_ADDR;			// save local dir addr
    if (_FF_checkdir(NAME, fpath))
	{
		_FF_DIR_ADDR = addr_temp;
		return ((int)EOF);
	}
	if (fpath[0]==0)
	{
		_FF_DIR_ADDR = addr_temp;
		return ((int)EOF);
	}
	dep = scan_directory(fpath, SCAN_FILE_R);		// Find File
  #else
	unsigned char *qp;

	qp = NAME;
	while (*qp !=0 )
	{
		if (valid_file_char(*qp++)==0)		// verify that this is a valid charater
	 		return ((int)EOF);
	}
	dep = scan_directory(NAME, SCAN_FILE_R);		// Find File
  #endif

	if (dep==0)
	{	// no entry found
	  #ifdef _DIRECTORIES_SUPPORTED_
		_FF_DIR_ADDR = addr_temp;
	  #endif
		return ((int)EOF);
	}

	// Save ATTRIBUTE Byte from location
	*F_ATTRIBUTE = dep->attr;
  #ifndef _BIG_ENDIAN_
	// Get the Starting cluster and save it to the location *F_CLUS_START
	*F_CLUS_START = dep->start_clus_lo;
	// Get File Size and save it to the location *F_SIZE
	*F_SIZE = dep->file_size;
	// Get the Create time/date and save it to the location *F_CREATE
	_FF_sprintf(F_CREATE, _time_date_str,
		dep->crt_date.date.month, dep->crt_date.date.day, (dep->crt_date.date.year+1980),
		dep->crt_time.time.hour, dep->crt_time.time.min, (dep->crt_time.time.x2sec) * 2);
	// Get the Modify time/date and save it to the location *F_MODIFY
	_FF_sprintf(F_MODIFY, _time_date_str,
		dep->mod_date.date.month, dep->mod_date.date.day, (dep->mod_date.date.year)+1980,
		dep->mod_time.time.hour, dep->mod_time.time.min, (dep->mod_time.time.x2sec) * 2);
  #else
	// Get the Starting cluster and save it to the location *F_CLUS_START
	temp16.cval.HI = dep->start_clus_lo.cval.LO;
	temp16.cval.LO = dep->start_clus_lo.cval.HI;
	*F_CLUS_START = temp16.ival;
	// Get File Size and save it to the location *F_SIZE
 	temp32.cval.HI = dep->file_size.cval.LO;
 	temp32.cval.MH = dep->file_size.cval.ML;
 	temp32.cval.ML = dep->file_size.cval.MH;
 	temp32.cval.LO = dep->file_size.cval.HI;
	*F_SIZE = temp32.lval;
	// Get the Create time/date and save it to the location *F_CREATE
	ff_time.timeHILO.HI = dep->crt_time.timeHILO.LO;
	ff_time.timeHILO.LO = dep->crt_time.timeHILO.HI;
	ff_date.dateHILO.HI = dep->crt_date.dateHILO.LO;
	ff_date.dateHILO.LO = dep->crt_date.dateHILO.HI;
	sprintf(F_CREATE, "%02d/%02d/%04d %02d:%02d:%02d",
		ff_date.date.month, ff_date.date.day, ff_date.date.year+1980,
		ff_time.time.hour, ff_time.time.min, ff_time.time.x2sec * 2);
	// Get the Modify time/date and save it to the location *F_MODIFY
	ff_time.timeHILO.HI = dep->mod_time.timeHILO.LO;
	ff_time.timeHILO.LO = dep->mod_time.timeHILO.HI;
	ff_date.dateHILO.HI = dep->mod_date.dateHILO.LO;
	ff_date.dateHILO.LO = dep->mod_date.dateHILO.HI;
	sprintf(F_MODIFY, "%02d/%02d/%04d %02d:%02d:%02d",
		ff_date.date.month, ff_date.date.day, ff_date.date.year+1980,
		ff_time.time.hour, ff_time.time.min, ff_time.time.x2sec * 2);
  #endif
	return (0);
}

// Get File data and increment file pointer
int fgetc_(FILE *rp)
{
	unsigned char get_data;
//	unsigned int n;
	unsigned long addr_temp;

	if (rp==NULL)
		return ((int)EOF);

	if (rp->position == rp->length)
	{
		rp->error = POS_ERR;
		return ((int)EOF);
	}

	get_data = *rp->pntr;

  #ifndef _NO_FILE_DATA_BUFFER_
   #ifdef _BytsPerSec_512_
	if (rp->pntr == &rp->buff[511])
   #else
	if (rp->pntr == &rp->buff[BPB_BytsPerSec.ival-1])
   #endif
  #else
   #ifdef _BytsPerSec_512_
	if (rp->pntr == &_FF_buff[511])
   #else
	if (rp->pntr == &_FF_buff[BPB_BytsPerSec.ival-1])
   #endif
  #endif
	{	// Check to see if pointer is at the end of a sector
		#ifndef _READ_ONLY_
		if ((rp->mode==WRITE) || (rp->mode==APPEND))
		{	// if in write or append mode, update the current sector before loading next
			addr_temp = clust_to_addr(rp->clus_current) + (rp->sec_offset);
		  #ifndef _NO_FILE_DATA_BUFFER_
			if (_FF_write(addr_temp, rp->buff))
		  #else
			if (_FF_write(addr_temp, _FF_buff))
		  #endif
				return ((int)EOF);
		}
		#endif
		if ((++(rp->sec_offset)) < BPB_SecPerClus)
		{	// Goto next sector if not at the end of a cluster
			addr_temp = clust_to_addr(rp->clus_current) + (rp->sec_offset);
		}
		else
		{	// End of Cluster, find next
			if (rp->clus_next>=0xFFF8)	// No next cluster, EOF marker
			{
				rp->EOF_flag = 1;	// Set flag so Putchar knows to get new cluster
				rp->position++;		// Only time doing this, position + 1 should equal length
				return(get_data);
			}
			addr_temp = clust_to_addr(rp->clus_next);
			if (addr_temp == 0)
				return((int)EOF);
			rp->sec_offset = 0;
			rp->clus_prev = rp->clus_current;
			rp->clus_current = rp->clus_next;
			rp->clus_next = next_cluster(rp->clus_current, SINGLE);
			if (NextClusterValidFlag == 0)
				return ((int)EOF);
		}
	  #ifndef _NO_FILE_DATA_BUFFER_
		if (_FF_read(addr_temp, rp->buff))
			return ((int)EOF);
		rp->pntr = &rp->buff[0];
	  #else
		if (_FF_read(addr_temp, _FF_buff))
			return ((int)EOF);
		rp->pntr = _FF_buff[0];
	  #endif
	}
	else
		rp->pntr++;

	rp->position++;
	return(get_data);
}

int fread(void *rd_buff, unsigned int dat_size, unsigned int num_items, FILE *rp)
{
	unsigned char *temp_char_pntr;
	unsigned int temp_int, temp_int2;
	unsigned int no_bytes_cntr, i;

	no_bytes_cntr = 0;
	temp_char_pntr = rd_buff;
	temp_int = dat_size * num_items;

	for (i=0; i<temp_int; i++)
	{
		temp_int2 = fgetc_(rp);
		if (temp_int2 == (unsigned int)EOF)
			return (no_bytes_cntr);
		else
		{
		  #ifdef _IAR_EWAVR_
			*temp_char_pntr = (unsigned char) temp_int2;
			temp_char_pntr++;
		  #else
			*temp_char_pntr++ = (unsigned char) temp_int2;
		  #endif
			no_bytes_cntr++;
		}
	}
	return (no_bytes_cntr/dat_size);
}


unsigned char *fgets(unsigned char *buffer, int n, FILE *rp)
{
	int c, temp_data;
	unsigned char *temp_pntr;

	temp_pntr = buffer;
	temp_data = fgetc_(rp);
	for (c=0; ((c<(n-1))&&(temp_data!='\n')&&(temp_data!=(int)EOF)); c++)
	{
	  #ifdef _IAR_EWAVR_
		*temp_pntr = (unsigned char) temp_data & 0xFF;
		temp_pntr++;
	  #else
		*temp_pntr++ = (unsigned char) temp_data & 0xFF;
	  #endif
		temp_data = fgetc_(rp);
	}
	*temp_pntr = '\0';
	if (temp_data == (int)EOF)
		return (NULL);
	return (buffer);
}

#ifndef _READ_ONLY_
// Decrement file pointer, then get file data
int ungetc(unsigned char file_data, FILE *rp)
{
//	unsigned int n;
	unsigned long addr_temp;

	if ((rp==NULL) || (rp->position==0))
		return ((int)EOF);
	if ((rp->mode!=APPEND) && (rp->mode!=WRITE))
		return ((int)EOF);	// needs to be in WRITE or APPEND mode

	if (((rp->position)==(rp->length)) && (rp->EOF_flag))
	{	// if the file posisition is equal to the length, return data, turn flag off
		rp->EOF_flag = 0;
		*rp->pntr = file_data;
		return (*rp->pntr);
	}
  #ifndef _NO_FILE_DATA_BUFFER_
	if ((rp->pntr)==(&rp->buff[0]))
	{	// Check to see if pointer is at the beginning of a Sector
		// Update the current sector before loading next
  #else
	if ((rp->pntr)==(&_FF_buff[0]))
	{	// Check to see if pointer is at the beginning of a Sector
  #endif
		if (addr_temp == 0)
			return((int)EOF);
		addr_temp += (rp->sec_offset);
	  #ifndef _NO_FILE_DATA_BUFFER_
		if (_FF_write(addr_temp, rp->buff))
	  #else
		if (_FF_write(addr_temp, _FF_buff))
	  #endif
			return ((int)EOF);

		if ((rp->sec_offset) > 0)
		{	// Goto previous sector if not at the beginning of a cluster
			rp->sec_offset--;
			addr_temp = clust_to_addr(rp->clus_current);
			if (addr_temp == 0)
				return((int)EOF);
			addr_temp += (rp->sec_offset);
		}
		else
		{	// Beginning of Cluster, find previous
			if (rp->clus_start==rp->clus_current)
			{	// Positioned @ Beginning of File
				_FF_error = SOF_ERR;
				return((int)EOF);
			}
			rp->sec_offset = BPB_SecPerClus-1;	// Set sector offset to last sector
			rp->clus_next = rp->clus_current;
			rp->clus_current = rp->clus_prev;
			if (rp->clus_current != rp->clus_start)
				rp->clus_prev = prev_cluster(rp->clus_current);
			else
				rp->clus_prev = 0;
			addr_temp = clust_to_addr(rp->clus_current);
			if (addr_temp == 0)
				return ((int)EOF);
			addr_temp += (BPB_SecPerClus-1);
		}

	  #ifndef _NO_FILE_DATA_BUFFER_
		if (_FF_read(addr_temp, rp->buff))
			return ((int)EOF);
		rp->pntr = &rp->buff[511];
	  #else
		if (_FF_read(addr_temp, _FF_buff))
			return ((int)EOF);
		rp->pntr = &_FF_buff[511];
	  #endif
	}
	else
		rp->pntr--;

	rp->position--;
	*rp->pntr = file_data;
	return(*rp->pntr);	// Get data
}
#endif

#ifndef _READ_ONLY_
#ifdef _DEBUG_FUNCTIONS_
 #define _FPUTC_DEBUG_
 #ifdef _FPUTC_DEBUG_
  char flash _fputc_debug_str_[] = "\tFPutcERR-%02d";
  char flash _fputc_info_str_[] = "\r\nFPUTC(%X)";
 #endif
#endif
/**************************************************************************
This function writes one character to the to the open file buffer.  It
verifies that a valid file pointer was input to the function, and verifies
the file was not in "READ" mode.  The length of the file is then checked
to see if the file is blank (length = 0).  If it is blank, a starting
cluster will be calculated and written to the FAT table.
**************************************************************************/
int fputc_(unsigned char file_data, FILE *rp)
{
	unsigned long addr_temp;

  #ifdef _FPUTC_DEBUG_
//	_FF_printf(_fputc_info_str_, file_data);
  #endif

	if (rp==NULL)
	{
	  #ifdef _FPUTC_DEBUG_
		_FF_printf(_fputc_debug_str_, 1);
	  #endif
		return ((int)EOF);
	}

	if (rp->mode == READ)
	{
		_FF_error = READONLY_ERR;
	  #ifdef _FPUTC_DEBUG_
		_FF_printf(_fputc_debug_str_, 2);
	  #endif
		return((int)EOF);
	}
	if (rp->length == 0)
	{	// Blank file start writing cluster table
		rp->clus_start = prev_cluster(0);
		rp->clus_next = 0xFFFF;
		rp->clus_current = rp->clus_start;
		if (write_clus_table(rp->clus_current, rp->clus_next, SINGLE))
		{
		  #ifdef _FPUTC_DEBUG_
			_FF_printf(_fputc_debug_str_, 3);
		  #endif
			return ((int)EOF);
		}
		rp->EOF_flag = 0;
	}

	if ((rp->position==rp->length) && (rp->EOF_flag))
	{	// At end of file, and end of cluster, flagged
		rp->clus_prev = rp->clus_current;
		rp->clus_current = prev_cluster(0);	// Find first cluster pointing to '0'
		if (rp->clus_current == 0)
		{
		  #ifdef _FPUTC_DEBUG_
			_FF_printf(_fputc_debug_str_, 4);
		  #endif
			return ((int)EOF);
		}
		rp->clus_next = 0xFFFF;
		rp->sec_offset = 0;
		if (write_clus_table(rp->clus_prev, rp->clus_current, START_CHAIN))
		{
		  #ifdef _FPUTC_DEBUG_
			_FF_printf(_fputc_debug_str_, 5);
		  #endif
			return ((int)EOF);
		}
		if (write_clus_table(rp->clus_current, rp->clus_next, END_CHAIN))
		{
 		  #ifdef _FPUTC_DEBUG_
			_FF_printf(_fputc_debug_str_, 6);
		  #endif
			return ((int)EOF);
		}
		if (append_toc(rp))
		{
		  #ifdef _FPUTC_DEBUG_
			_FF_printf(_fputc_debug_str_, 7);
		  #endif
			return ((int)EOF);
		}
		rp->EOF_flag = 0;
	  #ifndef _NO_FILE_DATA_BUFFER_
		rp->pntr = &rp->buff[0];
	  #else
		rp->pntr = &_FF_buff[0];
	  #endif
	}

	*rp->pntr = file_data;

  #ifndef _NO_FILE_DATA_BUFFER_
   #ifdef _BytsPerSec_512_
	if (rp->pntr == &rp->buff[511])
   #else
	if (rp->pntr == &rp->buff[BPB_BytsPerSec.ival-1])
   #endif
  #else
   #ifdef _BytsPerSec_512_
	if (rp->pntr == &_FF_buff[511])
   #else
	if (rp->pntr == &_FF_buff[BPB_BytsPerSec.ival-1])
   #endif
  #endif
	{	// This is on the Sector Limit
		if (rp->position > rp->length)
		{	// ERROR, position should never be greater than length
			_FF_error = 0x10;		// file position ERROR
		  #ifdef _FPUTC_DEBUG_
			_FF_printf(_fputc_debug_str_, 8);
		  #endif
			return ((int)EOF);
		}
		// Position is at end of a sector?
		addr_temp = clust_to_addr(rp->clus_current);
		if (addr_temp == 0)
		{
		  #ifdef _FPUTC_DEBUG_
			_FF_printf(_fputc_debug_str_, 10);
		  #endif
			return((int)EOF);
		}
		addr_temp += (rp->sec_offset);
	  #ifndef _NO_FILE_DATA_BUFFER_
		if (_FF_write(addr_temp, rp->buff))
	  #else
		if (_FF_write(addr_temp, _FF_buff))
	  #endif
		{
		  #ifdef _FPUTC_DEBUG_
			_FF_printf(_fputc_debug_str_, 9);
		  #endif
			return((int)EOF);
		}

		// Save MMC buffer to card, set pointer to begining of new buffer
		if ((++(rp->sec_offset)) < BPB_SecPerClus)
		{	// Are there more sectors in this cluster?
			addr_temp = clust_to_addr(rp->clus_current);
			if (addr_temp == 0)
			{
			  #ifdef _FPUTC_DEBUG_
				_FF_printf(_fputc_debug_str_, 10);
			  #endif
				return((int)EOF);
			}
			addr_temp += (rp->sec_offset);
		}
		else
		{	// Find next cluster, load first sector into file.buff
			if ((((rp->clus_next)>=0xFFF8)&&(BPB_FATType==0x36)) ||
				(((rp->clus_next)>=0xFF8)&&(BPB_FATType==0x32)))
			{	// EOF, need to find new empty cluster
				if (rp->position != rp->length)
				{	// if not equal there's an error
					_FF_error = 0x20;		// EOF position error
				  #ifdef _FPUTC_DEBUG_
					_FF_printf(_fputc_debug_str_, 10);
				  #endif
					return ((int)EOF);
				}
				rp->EOF_flag = 1;
			}
			else
			{	// Not EOF, find next cluster
				rp->clus_prev = rp->clus_current;
				rp->clus_current = rp->clus_next;
				rp->clus_next = next_cluster(rp->clus_current, SINGLE);
				if (NextClusterValidFlag == 0)
				{	// if not equal there's an error
				  #ifdef _FPUTC_DEBUG_
					_FF_printf(_fputc_debug_str_, 11);
				  #endif
					return ((int)EOF);
				}
			}
			rp->sec_offset = 0;
			addr_temp = clust_to_addr(rp->clus_current);
			if (addr_temp == 0)
			{
			  #ifdef _FPUTC_DEBUG_
				_FF_printf(_fputc_debug_str_, 13);
			  #endif
				return((int)EOF);
			}
		}

		if (append_toc(rp))
		{
		  #ifdef _FPUTC_DEBUG_
			_FF_printf(_fputc_debug_str_, 12);
		  #endif
			return((int)EOF);
		}
		if (rp->EOF_flag == 0)
		{
		  #ifndef _NO_FILE_DATA_BUFFER_
			if (_FF_read(addr_temp, rp->buff))
			{
			  #ifdef _FPUTC_DEBUG_
				_FF_printf(_fputc_debug_str_, 13);
			  #endif
				return((int)EOF);
			}
			rp->pntr = &rp->buff[0];	// Set pointer to next location
		  #else
			if (_FF_read(addr_temp, _FF_buff))
			{
			  #ifdef _FPUTC_DEBUG_
				_FF_printf(_fputc_debug_str_, 14);
			  #endif
				return((int)EOF);
			}
			rp->pntr = _FF_buff;
		  #endif
		}
		if (rp->length==rp->position)
			rp->length++;
	}
	else
	{
		rp->pntr++;
		if (rp->length==rp->position)
			rp->length++;
	}
	rp->position++;
	return(file_data);
}


//**********************************************************************//
// Writes the buffer *wr_buff to disk.  dat_size indicates the size of	//
// each item (in bytes ie. 1 for 8-bit, 2 for 16-bit, etc.).  num_items	//
// indicates the number of items to be written.  returns the number of 	//
// items written - number of errors.									//
//**********************************************************************//
int fwrite(void *wr_buff, unsigned int dat_size, unsigned int num_items, FILE *rp)
{
	unsigned char *temp_char_pntr;
	unsigned int temp_int;
	unsigned int no_bytes_cntr, i;

	no_bytes_cntr = 0;
	temp_char_pntr = wr_buff;
	temp_int = dat_size * num_items;

	for (i=0; i<temp_int; i++)
	{
		if (fputc_(temp_char_pntr[i], rp) == temp_char_pntr[i])
			no_bytes_cntr++;
		else
			no_bytes_cntr--;
	}
	return (no_bytes_cntr/dat_size);
}


int fputs(unsigned char *file_data, FILE *rp)
{
	while(*file_data)
	{
	  #ifdef _IAR_EWAVR_
		if (fputc_(*file_data,rp) == (int)EOF)
			return ((int)EOF);
		file_data++;
	  #else
		if (fputc_(*file_data++,rp) == (int)EOF)
			return ((int)EOF);
	  #endif
	}
	if (fputc_('\r',rp) == (int)EOF)
		return ((int)EOF);
	if (fputc_('\n',rp) == (int)EOF)
		return ((int)EOF);
	return (0);
}

  #if defined(_CVAVR_) || defined(_ICCAVR_)
int fputsc(unsigned char flash *file_data, FILE *rp)
  #else
int fputsc(PGM_P file_data, FILE *rp)
  #endif
{
	while(*file_data)
	{
	  #ifdef _IAR_EWAVR_
		if (fputc_(*file_data,rp) == (int)EOF)
			return ((int)EOF);
		file_data++;
	  #else
		if (fputc_(*file_data++,rp) == (int)EOF)
			return ((int)EOF);
	  #endif
	}
	if (fputc_('\r',rp) == (int)EOF)
		return ((int)EOF);
	if (fputc_('\n',rp) == (int)EOF)
		return ((int)EOF);
	return (0);
}
#endif

#ifndef _READ_ONLY_
  #if defined(_CVAVR_) || defined(_ICCAVR_)
void fprintf(FILE *rp, unsigned char flash *pstr, ...)
  #else
void fprintf(FILE *rp, PGM_P pstr, ...)
  #endif
{
	va_list arglist;
  #ifdef _CVAVR_
	unsigned char temp_buff[_FF_MAX_FPRINTF], *fp;

	va_start(arglist, pstr);
	vsprintf(temp_buff, pstr, arglist);
  #else
	char temp_buff[_FF_MAX_FPRINTF], *fp;
	char pstr_sram[_FF_MAX_FPRINTF];
	int cntr;

	cntr = 0;
	while ((pstr[cntr]) && (cntr <= _FF_MAX_FPRINTF))
	{
		pstr_sram[cntr] = pstr[cntr];
		cntr++;
	}
	pstr_sram[cntr] = 0;

	va_start(arglist, pstr);
	vsprintf(temp_buff, pstr_sram, arglist);
  #endif
	va_end(arglist);

	fp = temp_buff;
	while (*fp)
	{
	  #ifdef _IAR_EWAVR_
		if (fputc_(*fp, rp)==(int)EOF)
			return;
		fp++;
	  #else
		if (fputc_(*fp++, rp)==(int)EOF)
			return;
	  #endif
	}
}
#endif

// Set file pointer to the end of the file
int fend(FILE *rp)
{
	return (fseek(rp, 0, SEEK_END));
}

// Goto position "off_set" of a file
int fseek(FILE *rp, unsigned long off_set, unsigned char mode)
{
	unsigned int clus_temp;
	unsigned long length_check, addr_calc;

	if (rp==NULL)
	{	// ERROR if FILE pointer is NULL
		_FF_error = FILE_ERR;
		return ((int)EOF);
	}
	if (mode==SEEK_CUR)
	{	// Trying to position pointer to offset from current position
		off_set += rp->position;
	}
	if (off_set > rp->length)
	{	// trying to position beyond or before file
		rp->error = POS_ERR;
		_FF_error = POS_ERR;
		return ((int)EOF);
	}
	if (mode==SEEK_END)
	{	// Trying to position pointer to offset from EOF
		off_set = rp->length - off_set;
	}
  #ifndef _READ_ONLY_
	if (rp->mode != READ)
		if (fflush(rp))
			return ((int)EOF);
  #endif
	clus_temp = rp->clus_start;
	rp->clus_current = clus_temp;
	rp->clus_next = next_cluster(clus_temp, SINGLE);
	rp->clus_prev = 0;
	if (NextClusterValidFlag == 0)
		return ((int)EOF);

  #ifdef _BytsPerSec_512_
	addr_calc = off_set / ((unsigned long) BPB_SecPerClus << 9);
	length_check = off_set % ((unsigned long) BPB_SecPerClus << 9);
  #else
	addr_calc = off_set / ((unsigned long) BPB_BytsPerSec.ival * (unsigned long) BPB_SecPerClus);
	length_check = off_set % ((unsigned long) BPB_BytsPerSec.ival * (unsigned long) BPB_SecPerClus);
  #endif
	rp->EOF_flag = 0;

	while (addr_calc)
	{
		if (rp->clus_next >= 0xFFF8)
		{	// trying to position beyond or before file
			if ((addr_calc==1) && (length_check==0))
			{
				rp->EOF_flag = 1;
				break;
			}
			rp->error = POS_ERR;
			_FF_error = POS_ERR;
			return ((int)EOF);
		}
		clus_temp = rp->clus_next;
		rp->clus_prev = rp->clus_current;
		rp->clus_current = clus_temp;
		rp->clus_next = next_cluster(clus_temp, CHAIN_R);
		if (NextClusterValidFlag == 0)
			return ((int)EOF);
		addr_calc--;
	}

	addr_calc = clust_to_addr(rp->clus_current);
	rp->sec_offset = 0;			// Reset Reading Sector

  #ifdef _BytsPerSec_512_
	while (length_check >= 512)
	{
		length_check -= 512;
  #else
	while (length_check >= BPB_BytsPerSec.ival)
	{
		length_check -= BPB_BytsPerSec.ival;
  #endif
		addr_calc++;
		rp->sec_offset++;
	}

  #ifndef _NO_FILE_DATA_BUFFER_
	if (_FF_read(addr_calc, rp->buff))		// Read Current Data Sector
		return((int)EOF);		// Read Error

	if ((rp->EOF_flag == 1) && (length_check == 0))
	  #ifdef _BytsPerSec_512_
		rp->pntr = &rp->buff[511];
	  #else
		rp->pntr = &rp->buff[BPB_BytsPerSec.ival-1];
	  #endif
	else
		rp->pntr = &rp->buff[length_check];
  #else
	if (_FF_read(addr_calc, _FF_buff))		// Read Current Data Sector
		return((int)EOF);		// Read Error

	if ((rp->EOF_flag == 1) && (length_check == 0))
	  #ifdef _BytsPerSec_512_
		rp->pntr = &_FF_buff[511];
	  #else
		rp->pntr = &_FF_buff[BPB_BytsPerSec.ival-1];
	  #endif
	else
		rp->pntr = &_FF_buff[length_check];
  #endif
	rp->position = off_set;

	return (0);
}

// Return the current position of the file rp with respect to the begining of the file
unsigned long ftell(FILE *rp)
{
	if (rp==NULL)
		return ((unsigned long)EOF);
	else
		return (rp->position);
}

// Funtion that returns a '1' for @EOF, '0' otherwise
int feof(FILE *rp)
{
	if (rp==NULL)
		return ((int)EOF);

	if (rp->length==rp->position)
		return (1);
	else
		return (0);
}

flash char __CRLF[] = "\r\n"; 	//  = "\r\n" added from sd_cmd.c
#ifdef _DEBUG_ON_
#ifndef _NO_FILE_DATA_BUFFER_
flash char __Xstr[] = "%02X ";	//  = "%02X " added from sd_cmd.c
flash char _buff_addr_str[] = "\r\n_FF_buff @ 0x%08lX";

void dump_buff_data_hex(void)
{
	unsigned int n, c;

	_FF_printf(_buff_addr_str, _FF_buff_addr<<9);
	for (n=0; n<0x20; n++)
	{
		_FF_printf(__CRLF);
		for (c=0; c<0x10; c++)
			_FF_printf(__Xstr, _FF_buff[(n<<5)+c]);
	}
}



void dump_file_entry_hex(FILE_DIR_ENTRY *dep)
{
	unsigned char *p;
	unsigned char n, c;

	p = (unsigned char *) dep;
	for (n=0; n<2; n++)
	{
		_FF_printf(__CRLF);
		for (c=0; c<0x10; c++)
			_FF_printf(__Xstr, p[(n<<5)+c]);
	}
}

void dump_file_data_hex(FILE *rp)
{
	unsigned int n, c;

	if (rp == NULL)
		return;

	for (n=0; n<0x20; n++)
	{
		_FF_printf(__CRLF);
		for (c=0; c<0x10; c++)
			_FF_printf(__Xstr, rp->buff[(n<<5)+c]);
	}
}

void dump_file_data_view(FILE *rp)
{
	unsigned int n;

	if (rp == NULL)
		return;

	_FF_printf(__CRLF);
  #ifdef _BytsPerSec_512_
	for (n=0; n<512; n++)
  #else
	for (n=0; n<BPB_BytsPerSec.ival; n++)
  #endif
		putchar(rp->buff[n]);
}
#endif
#endif
