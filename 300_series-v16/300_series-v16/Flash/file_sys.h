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

//#define _DIRECTORIES_SUPPORTED_

#ifndef _file_sys_h_
	#define _file_sys_h_

  #ifndef NULL
	#define NULL	0
  #endif

  #ifndef EOF
	#define EOF		-1
  #endif

	typedef struct
	{
		unsigned char name[13];		//uchar name[13];
		unsigned char attr;			//uchar attr;
		unsigned int ftime;			//uint ftime;
		unsigned int fdate;			//uint fdate;
		unsigned long fsize;		//ulong fsize;
		unsigned long folder_place;	//ulong folder_place;
	} ffblk;

	typedef struct
	{
		unsigned int day	: 5;	//uint day		: 5;
		unsigned int month	: 4;	//uint month	: 4;
		unsigned int year	: 7; 	//uint year		: 7;
	} DATE_BITFIELD;

    typedef union
	{
		unsigned int date_word;		//uint date_word;
		DATE_BITFIELD date;
	  #ifdef _BIG_ENDIAN_
		sHILO16c dateHILO;
	  #endif
	} DIR_ENT_DATE;

	typedef struct
	{
		unsigned int x2sec	: 5;	//uint x2sec	: 5;
		unsigned int min	: 6;  	//uint min		: 6;
		unsigned int hour	: 5;   	//uint hour		: 5;
	} TIME_BITFIELD;

    typedef union
	{
		unsigned int time_word;		//uint time_word;
		TIME_BITFIELD time;
	  #ifdef _BIG_ENDIAN_
		sHILO16c timeHILO;
	  #endif
	} DIR_ENT_TIME;

	typedef struct
	{	// 32 byte structure
		unsigned char name_entry[11];	//uchar name_entry[11];
		unsigned char attr;				//uchar attr;
		unsigned char reserved;			//uchar reserved;
		unsigned char crt_tenth_sec;	//uchar crt_tenth_sec;
		DIR_ENT_TIME crt_time;
		DIR_ENT_DATE crt_date;
		DIR_ENT_DATE acc_date;
		unsigned int start_clus_hi; 	//uint start_clus_hi;
		DIR_ENT_TIME mod_time;
		DIR_ENT_DATE mod_date;
	  #ifndef _BIG_ENDIAN_
		unsigned int start_clus_lo;  	//uint start_clus_lo;
		unsigned long file_size;  		//ulong file_size;
	  #else
		uHILO16 start_clus_lo;
		uHILO32 file_size;
	  #endif
	} FILE_DIR_ENTRY;

	typedef struct
	{	// 552 bytes (+4 overhead using malloc) ==>  556 bytes required per file opened
//		uchar name[12];						// Filename as 8.3 0x20-padded string
		unsigned int clus_start;					// Starting cluster of open file
		unsigned int clus_current;					// Current cluster of open file
		unsigned int clus_next;						// Next (reading) cluster of open file
		unsigned int clus_prev;						// Previous cluster of open file
		unsigned char sec_offset;					// Current sector withing current cluster
		unsigned long entry_sec_addr;				// Sector address of File entry of open file
		FILE_DIR_ENTRY *file_dep;			// Entry Pointer to the offset of the File Info
	  #ifndef _NO_FILE_DATA_BUFFER_
		unsigned char buff[512];					// Open file read/write buffer
	  #endif
		unsigned long length;						// Length of file
		unsigned long position;						// Current position in file relative to begining
		unsigned char mode;							// 0=>closed, 1=>open for read, 2=>open for write, 3=> open for append
		unsigned char error;						// Error indicator
		unsigned char EOF_flag;						// End of File Flag
		unsigned char *pntr;						// Pointer for file data use
	  //#ifdef _NO_MALLOC_
		unsigned char used_flag;					// Flag to see if the file is used or not
	  //#endif
	} FILE;

	enum {NO_ERR=0, INIT_ERR, FILE_ERR, WRITE_ERR, READ_ERR, NAME_ERR, READONLY_ERR,
		SOF_ERR, EOF_ERR, ALLOC_ERR, POS_ERR, MODE_ERR, FAT_ERR, DISK_FULL,
		PATH_ERR, NO_ENTRY_AVAL, EXIST_ERR, INV_BOOTSEC_ERR, INV_PARTTABLE_ERR, INV_PARTENTRY_ERR};
	enum {CLOSED=0,READ, WRITE, APPEND};
	enum {SCAN_DIR_R, SCAN_DIR_W, SCAN_FILE_R, SCAN_FILE_W};
	enum {CHAIN_R, CHAIN_W, SINGLE, START_CHAIN, END_CHAIN};

	#define ATTR_READ_ONLY	0x01
	#define ATTR_HIDDEN		0x02
	#define ATTR_SYSTEM		0x04
	#define ATTR_VOL_ID		0x08
	#define ATTR_DIRECTORY	0x10
	#define ATTR_ARCHIVE	0x20
	#define ATTR_LONG_NAME	0x0F

	#define fgetc	fgetc_
	#define fputc	fputc_

	enum {SEEK_CUR, SEEK_END, SEEK_SET};

	unsigned char ascii_to_char(unsigned char ascii_char);
	unsigned char valid_file_char(unsigned char file_char);

  //void dump_file_entry_hex(FILE_DIR_ENTRY *dep); // moved from line 180
  //FILE *fcreatec(unsigned char flash *NAMEC, unsigned char MODE);

  #ifdef _DEBUG_ON_
	void read_directory(void);
   #ifndef _NO_FILE_DATA_BUFFER_
	void dump_file_data_hex(FILE *rp);
	void dump_file_data_view(FILE *rp);
   #endif
	void dump_buff_data_hex(void);
	void dump_file_entry_hex(FILE_DIR_ENTRY *dep); // moved out of if satement - linker error - see line 171
  #endif
	void GetVolID(void);
	unsigned long clust_to_addr (unsigned int clust_no);
	unsigned int addr_to_clust(unsigned long clus_addr);
	unsigned int next_cluster(unsigned int current_cluster, unsigned char mode);
  #ifndef _READ_ONLY_
	unsigned int prev_cluster(unsigned int clus_no);
	char write_clus_table(unsigned int current_cluster, unsigned int next_value, unsigned char mode);
	char append_toc(FILE *rp);
	char erase_clus_chain(unsigned int start_clus);
	int fquickformat(void);
	FILE *fcreate(unsigned char *NAME, unsigned char MODE);
   #if defined(_CVAVR_) || defined(_ICCAVR_)
	FILE *fcreatec(unsigned char flash *NAMEC, unsigned char MODE);
	int removec(unsigned char flash *NAMEC);
   #endif
   #ifdef _IAR_EWAVR_
	FILE *fcreatec(PGM_P NAMEC, unsigned char MODE);
	int removec(PGM_P NAMEC);
   #endif
	int remove(unsigned char *NAME);
	int rename(unsigned char *NAME_OLD, unsigned char *NAME_NEW);
   //#ifdef _DIRECTORIES_SUPPORTED_
	int mkdir(unsigned char *F_PATH);
	int rmdir(unsigned char *F_PATH);
   //#endif
	int ungetc(unsigned char file_data, FILE *rp);
	int fwrite(void *wr_buff, unsigned int dat_size, unsigned int num_items, FILE *rp);
	int fputs(unsigned char *file_data, FILE *rp);
   #if defined(_CVAVR_) || defined(_ICCAVR_)
	int fputsc(unsigned char flash *file_data, FILE *rp);
   #endif
   #ifdef _IAR_EWAVR_
	int fputsc(PGM_P file_data, FILE *rp);
   #endif
	int fputc_(unsigned char file_data, FILE *rp);
   #if defined(_CVAVR_) || defined(_ICCAVR_)
	void fprintf(FILE *rp, unsigned char flash *pstr, ...);
   #endif
   #ifdef _IAR_EWAVR_
	void fprintf(FILE *rp, PGM_P pstr, ...);
   #endif
	int fflush(FILE *rp);
  #endif
	char file_name_conversion(unsigned char *input_name, unsigned char *save_name);
  #if defined(_CVAVR_) || defined(_ICCAVR_)
	FILE *fopenc(unsigned char flash *NAMEC, unsigned char MODEC);
  #endif
  #ifdef _IAR_EWAVR_
	FILE *fopenc(PGM_P NAMEC, unsigned char MODEC);
  #endif
	FILE *fopen(unsigned char *NAME, unsigned char MODE);
	int fclose(FILE *rp);
	int ffreemem(FILE *rp);
  //#ifdef _DIRECTORIES_SUPPORTED_
   #if defined(_CVAVR_) || defined(_ICCAVR_)
	int chdirc(unsigned char flash *F_PATH);
   #endif
   #ifdef _IAR_EWAVR_
	int chdirc(PGM_P F_PATH);
   #endif
	int chdir(unsigned char *F_PATH);
	char _FF_chdir(unsigned char *F_PATH);
  //#endif
	int fget_file_info(unsigned char *NAME, unsigned long *F_SIZE, unsigned char *F_CREATE,
		unsigned char *F_MODIFY, unsigned char *F_ATTRIBUTE, unsigned int *F_CLUS_START);
  #if defined(_CVAVR_) || defined(_ICCAVR_)
	int fget_file_infoc(unsigned char flash *NAMEC, unsigned long *F_SIZE, unsigned char *F_CREATE,
		unsigned char *F_MODIFY, unsigned char *F_ATTRIBUTE, unsigned int *F_CLUS_START);
  #endif
  #ifdef _IAR_EWAVR_
	int fget_file_infoc(PGM_P NAMEC, unsigned long *F_SIZE, unsigned char *F_CREATE,
		unsigned char *F_MODIFY, unsigned char *F_ATTRIBUTE, unsigned int *F_CLUS_START);
  #endif
	int fgetc_(FILE *rp);
	int fread(void *rd_buff, unsigned int dat_size, unsigned int num_items, FILE *rp);
	unsigned char *fgets(unsigned char *buffer, int n, FILE *rp);
	int fend(FILE *rp);
	int fseek(FILE *rp, unsigned long off_set, unsigned char mode);
	unsigned long ftell(FILE *rp);
	int feof(FILE *rp);

	//sint fstream_filec(uchar flash *FNAME, ulong fsize);
	//sint fstream_file(uchar *FNAME, ulong fsize);

    /* 	Global variable declarations moved from file_sys.c
    	SJL - CAVR2											*/
    extern unsigned char _FF_buff[512];	//in config.h
	//#ifndef _BytsPerSec_512_
	//extern uHILO16 BPB_BytsPerSec;	//sd_cmd.h
	//#endif
	extern unsigned char BPB_SecPerClus;
	//extern uHILO16 BPB_RsvdSecCnt;	//sd_cmd.h
	//extern uHILO16 BPB_RootEntCnt;	//sd_cmd.h
	//extern uHILO16 BPB_FATSz16;  		//sd_cmd.h
	extern unsigned char BPB_FATType;
	extern unsigned char BS_VolLab[12];
	extern unsigned int _FF_PART_ADDR;
	extern unsigned long _FF_ROOT_ADDR;
	//#ifndef _BIG_ENDIAN_
	//extern unsigned long BS_VolSerial;
	//#else
	//extern uHILO32 BS_VolSerial; 		//sd_cmd.h
	//#endif
	//#ifdef _DIRECTORIES_SUPPORTED_
	extern unsigned long _FF_DIR_ADDR;
	//#endif
	extern unsigned int _FF_FAT1_ADDR;
	extern unsigned long _FF_FAT2_ADDR;
	extern unsigned int FirstDataSector;
	extern unsigned char _FF_error;
	extern unsigned long _FF_buff_addr;
	extern unsigned int DataClusTot;

    extern unsigned char _FF_FULL_PATH[];	//extern unsigned char _FF_FULL_PATH[_FF_PATH_LENGTH];
	extern unsigned int clus_0_counter;

    extern flash char __CRLF[];
	//#ifdef _DEBUG_ON_
	extern flash char __Xstr[];
	//#endif
	//#ifdef _NO_MALLOC_
		extern FILE _FF_File_Space_Allocation[];
    //#endif

#endif
