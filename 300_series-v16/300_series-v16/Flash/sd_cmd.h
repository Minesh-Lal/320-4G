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

#ifndef _sd_cmd_h_
	#define _sd_cmd_h_

	// SD/MMC COMMANDS			ARGUMENT		RESPONSE TYPE		DESCRIPTION
	#define CMD0	0x40		// NO_ARG			RESP_1			GO_IDLE_STATE
	#define CMD1	0x41		// NO_ARG			RESP_1			SEND_OP_COND (ACMD41 = 0x69)
	#define CMD9	0x49		// NO_ARG			RESP_1			SEND_CSD
	#define CMD10	0x4A		// NO_ARG			RESP_1			SEND_CID
	#define CMD12	0x4C		// NO_ARG			RESP_1			STOP_TRANSMISSION
	#define CMD13	0x4D		// NO_ARG			RESP_2			SEND_STATUS
	#define CMD16	0x50		// BLOCK_LEN		RESP_1			SET_BLOCKLEN
	#define CMD17	0x51		// DATA_ADDR		RESP_1			READ_SINGLE_BLOCK
	#define CMD18	0x52		// DATA_ADDR		RESP_1			READ_MULTIPLE_BLOCK
	#define CMD24	0x58		// DATA_ADDR		RESP_1			WRITE_BLOCK
	#define CMD25	0x59		// DATA_ADDR		RESP_1			WRITE_MULTIPLE_BLOCK
	#define CMD27	0x5B		// NO_ARG			RESP_1			PROGRAM_CSD
	#define CMD28	0x5C		// DATA_ADDR		RESP_1b			SET_WRITE_PROT
	#define CMD29	0x5D		// DATA_ADDR		RESP_1b			CLR_WRITE_PROT
	#define CMD30	0x5E		// DATA_ADDR		RESP_1			SEND_WRITE_PROT
	#define CMD32	0x60		// DATA_ADDR		RESP_1			TAG_SECTOR_START
	#define CMD33	0x61		// DATA_ADDR		RESP_1			TAG_SECTOR_END
	#define CMD34	0x62		// DATA_ADDR		RESP_1			UNTAG_SECTOR
	#define CMD35	0x63		// DATA_ADDR		RESP_1			TAG_ERASE_GROUP_START
	#define CMD36	0x64		// DATA_ADDR		RESP_1			TAG_ERASE_GROUP_END
	#define CMD37	0x65		// DATA_ADDR		RESP_1			TAG_ERASE_GROUP
	#define CMD38	0x66		// STUFF_BITS		RESP_1b			ERASE
	#define CMD42	0x6A		// STUFF_BITS		RESP_1b			LOCK_UNLOCK
	#define CMD58	0x7A		// NO_ARG			RESP_3			READ_OCR
	#define CMD59	0x7B		// STUFF_BITS		RESP_1			CRC_ON_OFF
	#define ACMD41	0x69		// NO_ARG			RESP_1

	#define SD_START_TOKEN	0xFE
	#define SD_MULTI_START	0xFC
	#define SD_STOP_TRANS	0xFD

    #define _FF_PATH_LENGTH		50	//SJL - CAVR2 - moved from options.h

	char reset_sd(void);
	char init_sd(void);
  #ifdef _DEBUG_ON_
	void _FF_read_disp(unsigned long sd_addr);
  #endif
	char _FF_read(unsigned long sd_addr, unsigned char *sd_read_buff);
	unsigned char initialize_media(void);
  #ifndef _READ_ONLY_
	char _FF_write(unsigned long sd_addr, unsigned char *sd_write_buff);
  #endif

	typedef struct
	{
	  #ifndef _BIG_ENDIAN_
		unsigned int NIB_0	: 4;
		unsigned int NIB_1	: 4;
		unsigned int NIB_2	: 4;
		unsigned int NIB_3	: 4;
	  #else
		unsigned int NIB_3	: 4;
		unsigned int NIB_2	: 4;
		unsigned int NIB_1	: 4;
		unsigned int NIB_0	: 4;
	  #endif
	} sNIBBLES16;

	typedef struct
	{
	  #ifndef _BIG_ENDIAN_
		unsigned char LO;
		unsigned char ML;
		unsigned char MH;
		unsigned char HI;
	  #else
		unsigned char HI;
		unsigned char MH;
		unsigned char ML;
		unsigned char LO;
	  #endif
	} sHILO32c;

	typedef struct
	{
	  #ifndef _BIG_ENDIAN_
		unsigned char LO;
		unsigned char HI;
	  #else
		unsigned char HI;
		unsigned char LO;
	  #endif
	} sHILO16c;

	typedef struct
	{
	  #ifndef _BIG_ENDIAN_
		unsigned int LO;
		unsigned int HI;
	  #else
		unsigned int HI;
		unsigned int LO;
	  #endif
	} sHILO32i;

	typedef union
	{
		unsigned long lval;
		sHILO32c cval;
		sHILO32i ival;
	} uHILO32;

    #ifndef _BIG_ENDIAN_
		extern unsigned long BS_VolSerial;
	#else
		extern uHILO32 BS_VolSerial; 	//SJL - CAVR2 - declared - sd_cmd.h
	#endif
    //extern uHILO32 BS_VolSerial;

	typedef union
	{
		unsigned int ival;
		sHILO16c cval;
		sNIBBLES16 nib;
	} uHILO16;

    #ifndef _BytsPerSec_512_
		extern uHILO16 BPB_BytsPerSec;
	#endif
    extern uHILO16 BPB_RsvdSecCnt;
	extern uHILO16 BPB_RootEntCnt;
	extern uHILO16 BPB_FATSz16;

	typedef struct
	{
		unsigned char Active_Flagg;
		unsigned char Starting_Head;
		unsigned int Starting_Cylinder;
		unsigned char Partition_Type;
		unsigned char Ending_Head;
		unsigned int Ending_Cylinder;
		uHILO32 Starting_Sector;
	  #ifndef _BIG_ENDIAN_
		unsigned long Partition_Length;
	  #else
		uHILO32 Partition_Length;
	  #endif
	} PARTITION_ENTRY;

	typedef struct
	{
		unsigned char jmpBoot[3];
		unsigned char OEMName[8];
	  #ifndef _BIG_ENDIAN_
		unsigned int BytsPerSec;
		unsigned char SecPerClus;
		unsigned int RsvdSecCnt;
		unsigned char NumFATs;
		unsigned int RootEntCnt;
		unsigned int TotSec16;
		unsigned char Media;
		unsigned int FATSz16;
		unsigned int SecPerTrk;
		unsigned int NumHeads;
		unsigned long HiddSec;
		unsigned long TotSec32;
		unsigned char DrvNum;
		unsigned char Reserved1;
		unsigned char BootSig;
		unsigned long VolID;
	  #else
		uHILO16 BytsPerSec;
		unsigned char SecPerClus;
		uHILO16 RsvdSecCnt;
		unsigned char NumFATs;
		uHILO16 RootEntCnt;
		uHILO16 TotSec16;
		unsigned char Media;
		uHILO16 FATSz16;
		uHILO16 SecPerTrk;
		uHILO16 NumHeads;
		uHILO32 HiddSec;
		uHILO32 TotSec32;
		unsigned char DrvNum;
		unsigned char Reserved1;
		unsigned char BootSig;
		uHILO32 VolID;
	  #endif
		unsigned char VolLab[11];
		unsigned char FilSysType[8];
		unsigned char junk[384];
		PARTITION_ENTRY PartEnt[4];
		unsigned int SigWord;
	} FAT16_BOOT_SECT;

  #ifndef _BIG_ENDIAN_
	#define SIGNATURE_WORD	0xAA55
  #else
	#define SIGNATURE_WORD	0x55AA
  #endif

  	/* 	Global variable declarations moved from file_sys.c
    	SJL - CAVR2											*/

	#ifdef _DEBUG_ON_
  		extern char flash _FF_VERSION_[];
	#endif

    //#ifdef _NO_MALLOC_
	//	extern FILE _FF_File_Space_Allocation[];
	//#endif

#endif
