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
#include "flash\sd_cmd.h"
#include "flash\file_sys.h"
#ifdef _RTC_ON_
  #include "flash\twi.h"
#endif
#include <mega128.h>	//SJL - CAVR2 - required to access avr ports
#include "options.h"	//SJL - CAVR2 - required to access _FF_PATH_LENGTH

eeprom int sector_read_count = 0;
eeprom int sector_write_count = 0;

#ifdef _DEBUG_ON_
  char flash _FF_VERSION_[] = "v2.11";	//SJL - CAVR2 - definition commented out in file_sys.c
#endif
//unsigned char _FF_buff[512];
#ifndef _BytsPerSec_512_
uHILO16 BPB_BytsPerSec;
#endif
//unsigned char BPB_SecPerClus;
//uHILO16 BPB_RsvdSecCnt;
//uHILO16 BPB_RootEntCnt;
//uHILO16 BPB_FATSz16;
//unsigned char BPB_FATType;
//#ifndef _BIG_ENDIAN_
//unsigned long BS_VolSerial;
//#else
//uHILO32 BS_VolSerial;
//#endif
//unsigned char BS_VolLab[12];
//unsigned int _FF_PART_ADDR;
//unsigned long _FF_ROOT_ADDR;
//#ifdef _DIRECTORIES_SUPPORTED_
//unsigned long _FF_DIR_ADDR;
//#endif
//unsigned int _FF_FAT1_ADDR;
//unsigned long _FF_FAT2_ADDR;
//unsigned int FirstDataSector;
//unsigned char _FF_error;
//unsigned long _FF_buff_addr;
//unsigned int clus_0_counter;
//unsigned char _FF_FULL_PATH[_FF_PATH_LENGTH];
//#ifdef _NO_MALLOC_
//	FILE _FF_File_Space_Allocation[_FF_MAX_FILES_OPEN];
//#endif
//unsigned int DataClusTot;

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

#ifndef _PICC_CCS_
unsigned char _FF_spi(unsigned char mydata)
{
    SPDR = mydata;          //byte 1
    while ((SPSR&0x80) == 0);
    return SPDR;
}
#else
  #define _FF_spi	spi_read
#endif

void _SD_send_cmd(unsigned char command, unsigned long argument)
{
	uHILO32 arg_temp;

	SD_CS_ON();			// select chip

	// send the command
	_FF_spi(command);

	arg_temp.lval = argument;
	_FF_spi(arg_temp.cval.HI);
	_FF_spi(arg_temp.cval.MH);
	_FF_spi(arg_temp.cval.ML);
	_FF_spi(arg_temp.cval.LO);

	// Send the CRC byte
	if (command == CMD0)
		_FF_spi(0x95);		// CRC byte, don't care except for CMD0=0x95
	else
		_FF_spi(0xFF);

	return;
}

void clear_sd_buff(void)
{
	SD_CS_OFF();
	_FF_spi(0xFF);
	_FF_spi(0xFF);
}

#ifdef _DEBUG_ON_
 flash char _FF_VersionInfo_str[] = "\r\nFlashFile ";
 flash char _FF_BSec_str[] = "\r\nBoot_Sec:\t[%X %X %X] [%X] [%X]";
 flash char _FF_BS_PA_str[] = "\r\nPart Address:\t%lX";
 flash char _FF_BPB_BPS_str[] = "\r\nBPB_BytsPerSec:\t%X";
 flash char _FF_BPB_SPC_str[] = "\r\nBPB_SecPerClus:\t%X";
 flash char _FF_BPB_RSC_str[] = "\r\nBPB_RsvdSecCnt:\t%X";
 flash char _FF_BPB_NFAT_str[] = "\r\nBPB_NumFATs:\t%X";
 flash char _FF_BPB_REC_str[] = "\r\nBPB_RootEntCnt:\t%X";
 flash char _FF_BPB_Fz16_str[] = "\r\nBPB_FATSz16:\t%X";
 flash char _FF_BPB_TS16_str[] = "\r\nBPB_TotSec16:\t%lX";
 flash char _FF_BPB_FT_str[] = "\r\nBPB_FATType:\tFAT";
 flash char _FF_1C_str[] = "1%c";
 flash char _FF_ERR_str[] = " ERROR!!";
 flash char _FF_BPB_FTE_str[] = "\r\nBPB_FATType:\tFAT ERROR!!!";
 flash char _FF_CCnt_str[] = "\r\nClusterCnt:\t%lX";
 flash char _FF_RAddr_str[] = "\r\nROOT_ADDR:\t%lX";
 flash char _FF_F2Addr_str[] = "\r\nFAT2_ADDR:\t%lX";
 flash char _FF_RDSec_str[] = "\r\nRootDirSectors:\t%X";
 flash char _FF_FDSec_str[] = "\r\nFirstDataSector:\t%X";
 flash char _FF_RErr_str[] = "\r\nRead ERROR!!!";
 flash char _FF_InvParTbl_str[] = "\r\nInvalid Partition Table Error";
#endif

unsigned char initialize_media(void)
{
  #ifndef _BIG_ENDIAN_
	unsigned int PT_SecStart;
  #else
	uHILO16 PT_SecStart;
  #endif
	unsigned char data_temp;
	unsigned char n;
	unsigned long _FF_RootDirSectors;
	uHILO32 BPB_TotSec;
	FAT16_BOOT_SECT *bpb;

	// SPI BUS SETUP
	// SPI initialization
	// SPI Type: Master
	// SPI Clock Rate: 921.600 kHz
	// SPI Clock Phase: Cycle Half
	// SPI Clock Polarity: Low
	// SPI Data Order: MSB First


	DDRB |= 0x07;		// Set SS, SCK, and MOSI to Output (If not output, processor will be a slave)
	DDRB &= 0xF7;		// Set MISO to Input
	CS_DDR_SET();		// Set CS to Output
	SPSR=0x00;
	SPCR=0x50; 			// Initialize the SPI bus as a master


  #ifndef _BytsPerSec_512_
	BPB_BytsPerSec.ival = 512;	// Initialize sector size to 512 (all SD cards have a 512 sector size)
  #endif

	if (reset_sd())
		return (0);

	data_temp = 0xFF;
	for (n=0; ((n<100)||(data_temp==0)) ; n++)
	{
		SD_CS_ON();
		data_temp = _FF_spi(0xFF);
		SD_CS_OFF();
	}
	// delay_ms(50);
	for (n=0; n<100; n++)
	{
		if (init_sd()==0)		// Initialization Succeeded
			break;
		if (n==99)
			return (0);
	}

	if (_FF_read(0x0, _FF_buff))
	{
	  #ifdef _DEBUG_ON_
		_FF_printf(_FF_RErr_str);
	  #endif
		_FF_error = INIT_ERR;
		return (0);
	}

	bpb = (FAT16_BOOT_SECT *) _FF_buff;
	if (bpb->SigWord != SIGNATURE_WORD)
	{	// Location Has to have respective values to be a valid Boot or Partition Sector
	  #ifdef _DEBUG_ON_
		_FF_printf(_FF_InvParTbl_str);
	  #endif
		_FF_error = INV_PARTTABLE_ERR;
		return (0);
	}

	if (((_FF_buff[0]==0xEB)&&(_FF_buff[2]==0x90))||(_FF_buff[0]==0xE9))
	  #ifndef _BIG_ENDIAN_
		PT_SecStart = 0;	// Valid Jump, must be Drive
	  #else
		PT_SecStart.ival = 0;	// Valid Jump, must be Drive
	  #endif
	else
	{	// Find valid Partition Entry
		for (n=0; n<4; n++)
		{
			if (bpb->PartEnt[n].Starting_Head)
			{
			  #ifndef _BIG_ENDIAN_
				PT_SecStart = bpb->PartEnt[n].Starting_Sector.ival.LO;
			  #else
				PT_SecStart.cval.HI = bpb->PartEnt[n].Starting_Sector.cval.MH;
				PT_SecStart.cval.LO = bpb->PartEnt[n].Starting_Sector.cval.HI;
			  #endif
				n = 0x10;
			}
		}
		if (n != 0x10)
		{
		  #ifndef _BIG_ENDIAN_
			PT_SecStart = bpb->PartEnt[0].Starting_Sector.ival.LO;
		  #else
			PT_SecStart.cval.HI = bpb->PartEnt[0].Starting_Sector.cval.MH;
			PT_SecStart.cval.LO = bpb->PartEnt[0].Starting_Sector.cval.HI;
		  #endif
		}
	}

	// Store the Sector address of the Start of the Partition
  #ifndef _BIG_ENDIAN_
 	_FF_PART_ADDR = (unsigned long) PT_SecStart;

	if (PT_SecStart)
  #else
 	_FF_PART_ADDR = (unsigned long) PT_SecStart.ival;

	if (PT_SecStart.ival)
  #endif
	{	// If it is not 0 a new sector has to be read
		if (_FF_read(_FF_PART_ADDR, _FF_buff))
		{
		  #ifdef _DEBUG_ON_
			_FF_printf(_FF_RErr_str);
		  #endif
			_FF_error = INIT_ERR;
			return (0);
		}
		if ( !( (bpb->SigWord == SIGNATURE_WORD) &&
		   ( ((_FF_buff[0]==0xEB)&&(_FF_buff[2]==0x90)) || (_FF_buff[0]==0xE9) ) ) )
		{	// Location Has to have respective values to be a valid Boot or Partition Sector
		  #ifdef _DEBUG_ON_
			_FF_printf(_FF_InvParTbl_str);
		  #endif
			_FF_error = INV_PARTTABLE_ERR;
			return (0);
		}
	}

  #ifdef _DEBUG_ON_
	_FF_printf(_FF_VersionInfo_str);
	_FF_printf(_FF_VERSION_);
	_FF_printf(_FF_BSec_str, _FF_buff[0],_FF_buff[1],_FF_buff[2],_FF_buff[510],_FF_buff[511]);
  #endif

    // Get the number of Bytes per Sector (should be 512)
  #ifdef _BytsPerSec_512_
   #ifndef _BIG_ENDIAN_
	if (bpb->BytsPerSec != 0x0200)
   #else
	if (bpb->BytsPerSec.ival != 0x0002)
   #endif
	{
	  #ifdef _DEBUG_ON_
		_FF_printf(_FF_ERR_str,1);
	  #endif
		return (0);
	}
  #else
   #ifndef _BIG_ENDIAN_
	BPB_BytsPerSec.ival = bpb->BytsPerSec;
   #else
	BPB_BytsPerSec.cval.LO = bpb->BytsPerSec.cval.HI;
	BPB_BytsPerSec.cval.HI = bpb->BytsPerSec.cval.LO;
   #endif
  #endif
    // Get the number of Sectors per Cluster
    BPB_SecPerClus = bpb->SecPerClus;
  #ifndef _BIG_ENDIAN_
    // Get the number of Reserved Sectors
	BPB_RsvdSecCnt.ival = bpb->RsvdSecCnt;
	// Get the number of Root Directory entries (should be 512)
	BPB_RootEntCnt.ival = bpb->RootEntCnt;
	// Get the FATSz16 value
	BPB_FATSz16.ival = bpb->FATSz16;
	// Get the number of Total Sectors available
	if (bpb->TotSec16)
	{
		BPB_TotSec.ival.HI = 0;
		BPB_TotSec.ival.LO = bpb->TotSec16;
	}
	else
	{	// If the read value is 0, stored in a different location, read from other location
		BPB_TotSec.lval = bpb->TotSec32;
	}
	// Get the volume's Serial number
	BS_VolSerial = bpb->VolID;
  #else
    // Get the number of Reserved Sectors
	BPB_RsvdSecCnt.cval.LO = bpb->RsvdSecCnt.cval.HI;
    BPB_RsvdSecCnt.cval.HI = bpb->RsvdSecCnt.cval.LO;
	// Get the number of Root Directory entries (should be 512)
	BPB_RootEntCnt.cval.LO = bpb->RootEntCnt.cval.HI;
	BPB_RootEntCnt.cval.HI = bpb->RootEntCnt.cval.LO;
	// Get the FATSz16 value
	BPB_FATSz16.cval.LO = bpb->FATSz16.cval.HI;
	BPB_FATSz16.cval.HI = bpb->FATSz16.cval.LO;
	// Get the number of Total Sectors available
	if (bpb->TotSec16.ival)
	{
		BPB_TotSec.ival.HI = 0;
		BPB_TotSec.cval.LO = bpb->TotSec16.cval.HI;
		BPB_TotSec.cval.ML = bpb->TotSec16.cval.LO;
	}
	else
	{	// If the read value is 0, stored in a different location, read from other location
		BPB_TotSec.cval.LO = bpb->TotSec32.cval.HI;
		BPB_TotSec.cval.ML = bpb->TotSec32.cval.MH;
		BPB_TotSec.cval.MH = bpb->TotSec32.cval.ML;
		BPB_TotSec.cval.HI = bpb->TotSec32.cval.LO;
	}
	// Get the volume's Serial number
	BS_VolSerial.cval.LO = bpb->VolID.cval.HI;
	BS_VolSerial.cval.ML = bpb->VolID.cval.MH;
	BS_VolSerial.cval.MH = bpb->VolID.cval.ML;
	BS_VolSerial.cval.HI = bpb->VolID.cval.LO;
  #endif
	// Store the Volume's label
	memcpy(BS_VolLab, bpb->VolLab, 11);
	BS_VolLab[11] = 0;		// Terminate the string
	// Calculate the Primary FAT table's Address
	_FF_FAT1_ADDR = _FF_PART_ADDR + BPB_RsvdSecCnt.ival;
	// Calculate the Secondary FAT table's Address
	_FF_FAT2_ADDR = _FF_FAT1_ADDR + BPB_FATSz16.ival;
	// Calculate the Address of the Root Directory
	_FF_ROOT_ADDR = ((unsigned long) bpb->NumFATs * (unsigned long) BPB_FATSz16.ival) + (unsigned long) BPB_RsvdSecCnt.ival;
	_FF_ROOT_ADDR += _FF_PART_ADDR;

	// Calculate the number of Total data Clusters
  #ifdef _BytsPerSec_512_
	_FF_RootDirSectors = (((unsigned long) BPB_RootEntCnt.ival << 5) + 511) >> 9;
  #else
	_FF_RootDirSectors = (((unsigned long) BPB_RootEntCnt.ival << 5) + BPB_BytsPerSec.ival - 1) / BPB_BytsPerSec.ival;
  #endif

	FirstDataSector = (bpb->NumFATs * BPB_FATSz16.ival) + BPB_RsvdSecCnt.ival + _FF_RootDirSectors;
	// Find the total number of data clusters (_FF_RootDirSectors is just a temp variable since DataClusTot is a int)
	_FF_RootDirSectors = (BPB_TotSec.lval - FirstDataSector) / BPB_SecPerClus;
	clus_0_counter = 2;
	_FF_buff_addr = 0;

	if (_FF_RootDirSectors < 4085)				// FAT12
		BPB_FATType = 0x32;
	else if (_FF_RootDirSectors < 65525)		// FAT16
		BPB_FATType = 0x36;
	else
	{
		BPB_FATType = 0;
		_FF_error = FAT_ERR;
	  #ifdef _DEBUG_ON_
		_FF_printf(_FF_ERR_str,2);
	  #endif
		return (0);
	}
	DataClusTot = (unsigned int) _FF_RootDirSectors;
  #ifdef _DIRECTORIES_SUPPORTED_
	_FF_DIR_ADDR = _FF_ROOT_ADDR;		// Set current directory to root address
  #endif
	_FF_FULL_PATH[0] = 0x5C;	// a '\'
	_FF_FULL_PATH[1] = 0;

  #ifdef _DEBUG_ON_
   #ifdef _BytsPerSec_512_
	_FF_printf(_FF_BS_PA_str, (_FF_PART_ADDR<<9));
	_FF_printf(_FF_BPB_BPS_str, 0x200);
   #else
	_FF_printf(_FF_BS_PA_str, (_FF_PART_ADDR*BPB_BytsPerSec.ival));
	_FF_printf(_FF_BPB_BPS_str, BPB_BytsPerSec.ival);
   #endif
	_FF_printf(_FF_BPB_SPC_str, BPB_SecPerClus);
	_FF_printf(_FF_BPB_RSC_str, BPB_RsvdSecCnt.ival);
	_FF_printf(_FF_BPB_NFAT_str, bpb->NumFATs);
	_FF_printf(_FF_BPB_REC_str, BPB_RootEntCnt.ival);
	_FF_printf(_FF_BPB_Fz16_str, BPB_FATSz16.ival);
	_FF_printf(_FF_BPB_TS16_str, BPB_TotSec.lval);
	_FF_printf(_FF_BPB_FT_str);
  #endif
	if ((BPB_FATType != 0x32) && (BPB_FATType != 0x36))
	{
	  #ifdef _DEBUG_ON_
		_FF_printf(_FF_ERR_str,3);
	  #endif
		return(0);
	}
  #ifdef _DEBUG_ON_
	else
		_FF_printf(_FF_1C_str, BPB_FATType);
	_FF_printf(_FF_CCnt_str, DataClusTot);
   #ifdef _BytsPerSec_512_
	_FF_printf(_FF_RAddr_str, _FF_ROOT_ADDR<<9);
	_FF_printf(_FF_F2Addr_str, _FF_FAT2_ADDR<<9);
   #else
	_FF_printf(_FF_RAddr_str, _FF_ROOT_ADDR*BPB_BytsPerSec.ival);
	_FF_printf(_FF_F2Addr_str, _FF_FAT2_ADDR*BPB_BytsPerSec.ival);
   #endif
	_FF_printf(_FF_RDSec_str, _FF_RootDirSectors);
	_FF_printf(_FF_FDSec_str, FirstDataSector);
  #endif

  #ifdef _NO_MALLOC_
	// Verify that all of the file Structure information is cleared
	for (n=0; n<_FF_MAX_FILES_OPEN; n++)
		memset(&_FF_File_Space_Allocation[n], 0x00, sizeof(FILE));
  #endif

	return (1);
}


#ifdef _DEBUG_ON_
  flash char reset_str[] = "\r\nReset CMD:  ";
  flash char ok_str[] = "OK!!!";
  flash char err_str[] = "ERROR-%02X";
  flash char init_str[] = "\r\nInitialization:  ";
#endif

char reset_sd(void)
{
	unsigned char resp, n, c;

  #ifdef _DEBUG_ON_
	_FF_printf(reset_str);
  #endif
	for (c=0; c<4; c++)		// try reset command 3 times if needed
	{
		SD_CS_OFF();
		for (n=0; n<10; n++)	// initialize clk signal to sync card
			_FF_spi(0xFF);
		_SD_send_cmd(CMD0, 0);
		for (n=0; n<200; n++)
		{
			resp = _FF_spi(0xFF);
			if (resp == 0x1)
			{
				SD_CS_OFF();
			  #ifdef _DEBUG_ON_
				_FF_printf(ok_str);
			  #endif

				return(0);
			}
		}
	  #ifdef _DEBUG_ON_
		_FF_printf(err_str, 1);
	  #endif
	}
	return ((char)EOF);
}

char init_sd(void)
{
	unsigned char resp, c;
	unsigned int i;

	clear_sd_buff();

  #ifdef _DEBUG_ON_
	_FF_printf(init_str);
  #endif
	resp = 0xFF;
    for (i=0; ((i<1000)&&(resp!=0)); i++)
    {
    	_SD_send_cmd(CMD1, 0);
    	for (c=0; ((c<100)&&(resp!=0)); c++)
	   		resp = _FF_spi(0xFF);
	}
   	if (resp == 0)
	{
	  #ifdef _DEBUG_ON_
   		_FF_printf(ok_str);
	  #endif
		return (0);
	}
	else
	{
	  #ifdef _DEBUG_ON_
   		_FF_printf(err_str, resp);
	  #endif
		return ((char)EOF);
 	}
}

#ifdef _DEBUG_ON_
//  flash char __CRLF[] = "\r\n";
//  flash char __Xstr[] = "%02X "; //SJL - CAVR2 - moved to file_sys.c

void _FF_read_disp(unsigned long sd_addr)
{
	unsigned char resp;
	unsigned long n, remainder;

	clear_sd_buff();
	_SD_send_cmd(CMD17, sd_addr);		// Send read request

	resp = _FF_spi(0xFF);
	while (resp != SD_START_TOKEN)
		resp = _FF_spi(0xFF);

	for (n=0; n<512; n++)
	{
		remainder = n & 0xF;
		if (remainder == 0)
			_FF_printf(__CRLF);
		_FF_buff[n] = _FF_spi(0xFF);
		_FF_printf(__Xstr, _FF_buff[n]);
	}
	_FF_spi(0xFF);
	_FF_spi(0xFF);
	_FF_spi(0xFF);
	SD_CS_OFF();
	return;
}
#endif



#ifdef _DEBUG_FUNCTIONS_
 #define _FF_READ_DEBUG_
 #ifdef _FF_READ_DEBUG_
  char flash _read_addr_str_[] = "\r\nReadADDR[0x%lX]";
 #endif
#endif

// Read data from a SD card @ address
// This function reads 1 sector of bytes @ sd_addr to
// the buffere designated by *sd_read_buff
char _FF_read(unsigned long sd_addr, unsigned char *sd_read_buff)
{
	unsigned char resp, c;
	unsigned int n;
   sector_read_count++;
  #ifdef _FF_READ_DEBUG_
	_FF_printf(_read_addr_str_, sd_addr<<9);
  #endif

	for (c=0;c<4;c++)
	{
		clear_sd_buff();
	  #ifdef _BytsPerSec_512_
		_SD_send_cmd(CMD17, (sd_addr<<9));	// read block command
	  #else
		_SD_send_cmd(CMD17, (sd_addr*BPB_BytsPerSec.ival));	// read block command
	  #endif
		resp = _FF_spi(0xFF);
		for (n=0; ((n<1000)&&(resp!=SD_START_TOKEN)); n++)
			resp = _FF_spi(0xFF);
		if (resp==SD_START_TOKEN)
		{	// if it is a valid start byte => start reading SD Card
		  #ifdef _BytsPerSec_512_
			for (n=0; n<512; n++)
			{
		  #else
			for (n=0; n<BPB_BytsPerSec.ival; n++)
			{
		  #endif
				sd_read_buff[n] = _FF_spi(0xFF);
			}
			_FF_spi(0xFF);
			_FF_spi(0xFF);
			_FF_spi(0xFF);
			SD_CS_OFF();
			_FF_error = NO_ERR;
			if (sd_read_buff == _FF_buff)
				_FF_buff_addr = sd_addr;

			return (0);
		}
		SD_CS_OFF();
	}
	_FF_error = READ_ERR;
	return((char)EOF);
}


#ifndef _READ_ONLY_
#ifdef _DEBUG_FUNCTIONS_
 #define _FF_WRITE_DEBUG_
 #ifdef _FF_WRITE_DEBUG_
  char flash _write_addr_str_[] = "\r\nWriteADDR[0x%lX]";
 #endif
#endif

// This function writes 1 sector of bytes designated by
// *sd_write_buff to the address sd_addr
char _FF_write(unsigned long sd_addr, unsigned char *sd_write_buff)
{
	unsigned char resp, calc, c;
	unsigned int n;

   // #ifdef _BLOODY_FRUSTRATED_
   // sprintf(buffer,"[%lu]",sd_addr);print(buffer);
   // #endif

  #ifdef _FF_WRITE_DEBUG_
	_FF_printf(_write_addr_str_, sd_addr<<9);
  #endif

	if (sd_addr <= _FF_PART_ADDR)
	{	// if the Address is l
		_FF_error = WRITE_ERR;
		return ((char)EOF);
	}

	for (c=0; c<4; c++)
	{
		clear_sd_buff();
	  #ifdef _BytsPerSec_512_
		_SD_send_cmd(CMD24, (sd_addr<<9));
	  #else
		_SD_send_cmd(CMD24, (sd_addr*(unsigned long)BPB_BytsPerSec.ival));
	  #endif
		resp = _FF_spi(0xFF);
		for (n=0; ((n<10000)&&(resp!=0x00)); n++)
			resp = _FF_spi(0xFF);

		if (resp==0)
		{
			_FF_spi(0xFF);
			_FF_spi(SD_START_TOKEN);			// Start Block Token
		  #ifdef _BytsPerSec_512_
			for (n=0; n<512; n++)
			{
		  #else
			for (n=0; n<BPB_BytsPerSec.ival; n++)
			{
		  #endif
				_FF_spi(sd_write_buff[n]);		// Write Data in buffer to card
			}
			_FF_spi(0xFF);					// Send 2 blank CRC bytes
			_FF_spi(0xFF);
			resp = _FF_spi(0xFF);			// Response should be 0bXXX00101
			calc = resp | 0xE0;
			if (calc==0xE5)
			{
				while(_FF_spi(0xFF)==0)
					;	// Clear Buffer before returning 'OK'
				SD_CS_OFF();
				_FF_error = NO_ERR;

				return(0);
			}
		}
		SD_CS_OFF();
	}
	_FF_error = WRITE_ERR;
	return((char)EOF);
}
#endif
