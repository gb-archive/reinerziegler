/*********************************************/
/* GameBoy cartridge reader and writer       */
/*									       	 */
/* YOU CAN USE AND CHANGE THIS SOFTWARE ONLY */
/* IF YOU ARE WILLING TO SHARE               */
/* IMPROVEMENTS WITH THE GAMEBOY COMMUNITY   */
/* PLEASE REPORT ANY CHANGE AND              */
/* IMPROVEMENT TO REINERZIEGLER@GMX.DE       */
/*                                           */
/* original Read program    				 */
/* last edit 06-Nov-95 by Pascal Felber		 */
/*											 */
/* original EEPROM / Flash EPROM Programmer  */
/* last edit 15-Mar-97 by Jeff Frohwein 	 */
/*											 */
/* new  R e a d P l u s  version for   		 */
/* (X)CARTIO, IO-56, IO-48, Carbon Copy Card,*/
/* PIO 24/48 II, GB-Xchanger from Bung and   */
/* Altera EPLD hardware                      */
/* last edit 31-Mar-00 by Reiner Ziegler	 */
/*											 */
/*											 */
/* last edit 05-Jan-01 by F.D.          	 */
/*											 */
/*											 */
/* V3.31 programming MBC5 cart with 1Mbit    */
/*       RAM and Intel 28F320/28F640 Flash   */
/*       with Bung Xchanger,                 */
/*       recognise RAMsize 05=512kbit=       */
/*       64kB=8banks/8kB                     */
/* V3.30 programming of new Nintendo cart	 */
/*       with Intel DA28F320 chip            */
/* V3.29 identification of Bung carts	     */
/* V3.28 add erase command for flash    	 */
/*       minor bug fix for RAM R/W           */
/* V3.27 add support for Nintendo flash      */
/* V3.26 fix MBC5 programming bug and 		 */
/*       rewrote programming routine         */
/* V3.25 add support for ATMEL and progr. of */
/*       MBC3 and MBC5(32MB) cartridges      */
/* V3.24 add MBC5 and MBC4 types             */
/*       fix MBC2 read bug                   */
/* V3.23 add SGS flash chip, set wait_delay  */
/*       to 1/20 for programming mode        */
/*       add type for MBC5 and MBC3+crystal  */
/* V3.22 fix bug with GB Camera              */
/*       changed type to 0x13                */
/* V3.21 add verify command for RAM          */
/* V3.2  supports GB Camera and programming  */
/*       GB via GameLink cable				 */
/*       rewrite verify cmd incl. MBC3  	 */
/* V3.1  now supports MBC3 chips             */
/*       including time calculation			 */
/* V3.0  supports multimodul programming     */
/*       new mode for 29F016 programming	 */
/*		 skip data valid programming         */
/*       add -m command                      */
/*       fix minor bugs                      */
/* V2.7  add support for Fujitsu flash       */
/*       memories							 */
/* V2.6  reset MBC1 registers to default	 */
/*       add support for new modules         */
/*       identify different flash memories   */
/*       add licencee codes				     */
/*       use -a also for analysing files	 */
/* V2.51 change LED_OFF calls				 */
/* V2.5  change internal structure for       */
/*       new hardware support				 */
/*       common.h, io56.h, io48.h  			 */
/*       cartio.h, epld.h, newhw.h           */
/* V2.41 fix - W bug						 */
/* V2.4  add overwrite command for RAM		 */
/* V2.3  make RAM visible for Dump command 	 */
/*       -d 160 = base 0xA000                */
/*       some minor changes for IO-56 hw     */
/* V2.2  add support for IO-56 Hardware      */
/*       skip 10ms routine for programming   */
/*       add readout for 64in1 module        */
/*       speed up programming routine        */
/*       (input from Jeff Frohwein)			 */
/* V2.1  fix bug with filesize calculation,  */
/*       add default file extensions.		 */
/* V2.0  first common version, works...      */
/*       change erase routine, commands, ... */
/*											 */
/* Compiled with Microsoft C++ 1.51			 */
/*********************************************/

#include <stdio.h>
#include <conio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <io.h>
#include <math.h>
#include <time.h>
#include <ctype.h>

#include "common.h"					/* defines for ReadPlus 				*/
// #include "cartio.h" 				/* defines for CARTIO and C3 hardware 	*/
// #include "io56.h"          		/* defines for IO-56 hardware  			*/
// #include "io48.h"   				/* defines for IO-48 hardware  			*/
// #include "epld.h"   				/* defines for Altera EPLD     			*/
// #include "multi-io.h"			/* defines for multi 8255/8253 hardware */
#include "bung.h"               	/* defines for Bung programmer          */
// #include "newhw.h"   			/* defines for new hardware    			*/

// #include "b2201.h"               /* defines for Uli Schiefer             */
// #include "cartio0.h" 			/* defines for Pascals CARTIO 	        */
// #include "martio.h"	 			/* defines for Martin Eyre			    */
// #include "io56-2.h"              /* defines for Henry Smith 				*/

void usage(void)
{
	printf("\nReadPlus Version 3.30 from Reiner Ziegler\n");
	usage_hardware();
	printf("\t-w\tSpecify the time to wait between accesses to the cartridge\n");
	printf("\t-t\tTest the cartridge reader\n");
	printf("\t-d\tDump 256 bytes to screen (default = 1)\n");
	printf("\t-a\tAnalyze cartridge or file header\n");
	printf("\t-e\t0: Erase flash cartridge 4MBit\n");
	printf("\t\t1: Erase flash cartridge 16/32MBit\n");
	printf("\t\t2: Erase flash cartridge Nintendo 32Mbit\n");
	printf("\t\t3: Erase flash cartridge MBC5/IntelFlash 32/64mbit\n");
	printf("\t-p\tProgram cartridge from file\n");
	printf("\t-n\tNew cartridge (MBC3/5) programming\n");
	printf("\t-m\tMulticartridge programming (file1 file2 file3)\n");
	printf("\t-s\tSave the cartridge into file\n");
  	printf("\t-c\tCombined module 64in1 save into file\n");
	printf("\t-v\tVerify cartridge matches file\n");
	printf("\t-o\tOverwrite SRAM with byte (default = 0)\n");
	printf("\t-b\tBackup the SRAM into file\n");
	printf("\t-r\tRestore the SRAM from file\n");
	printf("\t-f\tFile compare with SRAM\n");
}

void select_mbc_defaults  (void)
{
    WRITE_BKSW(ROM16_RAM8,0x6000);			/* Set MBC to 16Mbit ROM & 8KByte RAM mode */
    WRITE_BKSW(0x00,0x4000);                /* Set high memory bank select to 0x00 */
    WRITE_BKSW(ROM4_RAM32,0x6000);
//    WRITE_BKSW(0x00,0x3000);
    WRITE_BKSW(0x00,0x2100);
    WRITE_BKSW(0x00,0x0000);
}

void select_mbc_bank (BYTE type, int nbbank)
{
	WRITE_BKSW(type,0x6000);
	WRITE_BKSW((BYTE)nbbank,0x4000);
}

/* exit program with errorcode 1 */

void ErrorExit(void)
{
   LED_OFF;
   exit(1);
}

void read_data(FILE *fp, int type, int sizekB)
{
	int bank, page, byte, nbbank, nloops, newreg, reg3000 = 0;
	unsigned char val;
    unsigned chk;

	/* One bank is 16k bytes */
	nbbank = sizekB / 16;
    chk=0;

 	switch (type)
  	{
  		case NO_MBC:
      	    nloops = 1;
   			break;
  		case MBC1:
			if(nbbank > 32) { nloops = (nbbank/32); nbbank = 32; }  else nloops = 1;
       		break;
       	case MBC2:
       	    nloops = 1;
       		break;
  		case MBC3:
			nloops = 1;
       		break;
  		case MBC4:
			nloops = 1;
       		break;
  		case MBC5:
			nloops = 1;
			if(nbbank > 256) { reg3000 = 1; }
       		break;
  	}

	for (newreg = 0; newreg < nloops; newreg++) {
 	  if(nloops > 1) { select_mbc_bank(ROM16_RAM8,newreg);
         printf("Reading MBC bank %d\n", newreg);
  	     WRITE_BKSW(ROM4_RAM32,0x6000);	/* protect register */ }

	  for(bank = 0; bank < nbbank; bank++) {
        printf("Reading bank %d\n", bank);
		if(bank) { WRITE_BKSW(bank,0x2100); }

		for(page = (bank ? 0x40 : 0); page < (bank ? 0x80 : 0x40); page++) {
			printf(".");
			ADDRESS_HIGH(page);

			for(byte = 0; byte <= 0xFF; byte++) {
				ADDRESS_LOW(byte);
				val = READ(ROM);
                fputc(val, fp);
                chk += val;
			}
		}
		printf("\n");
	  }
	}
	chk -= checksum & 0xFF;
	chk -= (checksum >> 8);
	if(chk == checksum)
		printf("Checksum OK\n");
	else
		printf("Warning: checksum is wrong (waiting for %uX, got %uX)\n", checksum, chk);
}

void verify_data(FILE *fp, int type, int sizekB)
{
	int bank, page, byte, nbbank, nloops, newreg, reg3000 = 0;

	/* One bank is 16k bytes */
	nbbank = sizekB / 16;

    PrintChipSize();

	switch (type)
  	{
  		case NO_MBC:
      	    nloops = 1;
   			break;
  		case MBC1:
			if(nbbank > 32) { nloops = (nbbank/32); nbbank = 32; }  else nloops = 1;
       		break;
       	case MBC2:
       	    nloops = 1;
       		break;
  		case MBC3:
			nloops = 1;
       		break;
  		case MBC4:
			nloops = 1;
       		break;
  		case MBC5:
			nloops = 1;
			if(nbbank > 256) { reg3000 = 1; }
       		break;
  	}

	for (newreg = 0; newreg < nloops; newreg++) {
 	  if(nloops > 1) { select_mbc_bank(ROM16_RAM8,newreg);
         printf("Verify MBC bank %d\n", newreg);
  	     WRITE_BKSW(ROM4_RAM32,0x6000);	/* protect register */ }

	  for(bank = 0; bank < nbbank; bank++) {
        printf("Verify bank %d\n", bank);
		if(bank) { WRITE_BKSW(bank,0x2100); }

		for(page = (bank ? 0x40 : 0); page < (bank ? 0x80 : 0x40); page++) {
			printf(".");
			ADDRESS_HIGH(page);

			for(byte = 0; byte <= 0xFF; byte++) {
				if(feof(fp)) {
					printf("Unexpected EOF\n");
					ErrorExit();
				}
				ADDRESS_LOW(byte);
				if (fgetc(fp) != READ(ROM)) {
				  printf("\n\nverify failed !\n");
				  ErrorExit();
				}
			}
		}
		printf("\n");
	  }
	}
	printf("\n\nverify ok !");
}

void read_sram(FILE *fp, int type, int sizekB)
{
	int bank, page, byte, nbbank, banksize;

   printf("Type= %d, SizekB= %dkB", type, sizekB);

	if((type == MBC1) || (type == MBC3)) {
		/* One bank is 8k bytes */
		nbbank = sizekB / 8;
		if(nbbank == 0) {
			nbbank = 1;
			banksize = sizekB << 2; }
		else
			banksize = 0x20; }
	else if(type == MBC2) {
			/* SRAM is 512 * 4 bits */
			nbbank = 1;
			banksize = 0x02; }
		 else {
			printf("No RAM with battery\n");
			return; }

	/* Initialize the bank-switch IC */
 	WRITE_BKSW(0x00,0x6000);
	WRITE_BKSW(0x0A,0x0000);

    for(bank = 0; bank < nbbank; bank++) {
		printf("Reading bank %d\n", bank);

		if(type == MBC1) { select_mbc_bank(ROM4_RAM32,bank); }
		if(type == MBC3) { WRITE_BKSW(bank,0x4000); }

		for(page = 0xA0; page < 0xA0 + banksize; page++) {
			printf(".");
			ADDRESS_HIGH(page);

			for(byte = 0; byte <= 0xFF; byte++) {
				ADDRESS_LOW(byte);
				fputc(READ(RAM), fp);
			}
		}
        printf("\n");
	}

    /* disable bank-switch IC */
	WRITE_BKSW(0x00,0x0000);
}

void write_sram(FILE *fp, int type, int sizekB)
{
	int bank, page, byte, nbbank, banksize;

	if((type == MBC1) || (type == MBC3)) {
		/* One bank is 8k bytes */
		nbbank = sizekB / 8;
		if(nbbank == 0) {
			nbbank = 1;
			banksize = sizekB << 2; }
		else
			banksize = 0x20; }
	else if(type == MBC2) {
			/* SRAM is 512 * 4 bits */
			nbbank = 1;
			banksize = 0x02; }
		 else {
			printf("No RAM with battery\n");
			return; }

	/* Initialize the bank-switch IC */
	WRITE_BKSW(0x00,0x6000);
	WRITE_BKSW(0x0A,0x0000);

	for(bank = 0; bank < nbbank; bank++) {
		printf("Writing bank %d\n", bank);

		if(type == MBC1) { select_mbc_bank(ROM4_RAM32,bank); }
		if(type == MBC3) { WRITE_BKSW(bank,0x4000); }

		for(page = 0xA0; page < 0xA0 + banksize; page++) {
			printf(".");
			ADDRESS_HIGH(page);

			for(byte = 0; byte <= 0xFF; byte++) {
				if(feof(fp)) {
					printf("Unexpected EOF\n");
					ErrorExit();
				}
				ADDRESS_LOW(byte);
				WRITE_MEM(fgetc(fp));
			}
		}
		printf("\n");
	}

    /* disable bank-switch IC */
	WRITE_BKSW(0x00,0x0000);
}

void verify_sram(FILE *fp, int type, int sizekB)
{
	int bank, page, byte, nbbank, banksize;

	if((type == MBC1) || (type == MBC3)) {
		/* One bank is 8k bytes */
		nbbank = sizekB / 8;
		if(nbbank == 0) {
			nbbank = 1;
			banksize = sizekB << 2; }
		else
			banksize = 0x20; }
	else if(type == MBC2) {
			/* SRAM is 512 * 4 bits */
			nbbank = 1;
			banksize = 0x02; }
		 else {
			printf("No RAM with battery\n");
			return; }

	/* Initialize the bank-switch IC */
	WRITE_BKSW(0x00,0x6000);
	WRITE_BKSW(0x0A,0x0000);

	for(bank = 0; bank < nbbank; bank++) {
		printf("Verify RAM bank %d\n", bank);

		if(type == MBC1) { select_mbc_bank(ROM4_RAM32,bank); }
		if(type == MBC3) { WRITE_BKSW(bank,0x4000); }

		for(page = 0xA0; page < 0xA0 + banksize; page++) {
			printf(".");
			ADDRESS_HIGH(page);

			for(byte = 0; byte <= 0xFF; byte++) {
				if(feof(fp)) {
					printf("Unexpected EOF\n");
					ErrorExit();
				}
				ADDRESS_LOW(byte);
				if (fgetc(fp) != READ(RAM)) {
				  printf("\n\nverify failed !\n");
				  WRITE_BKSW(0x00,0x0000);
				  ErrorExit();
				}
			}
		}
		printf("\n");
	}
	printf("\n\nverify ok !");

    /* disable bank-switch IC */
	WRITE_BKSW(0x00,0x0000);
}

void clear_sram(int type, int sizekB, BYTE val)
{
	int bank, page, byte, nbbank, banksize;

	if((type == MBC1) || (type == MBC3)) {
		/* One bank is 8k bytes */
		nbbank = sizekB / 8;
		if(nbbank == 0) {
			nbbank = 1;
			banksize = sizekB << 2; }
		else
			banksize = 0x20; }
	else if(type == MBC2) {
			/* SRAM is 512 * 4 bits */
			nbbank = 1;
			banksize = 0x02; }
		 else {
			printf("No RAM with battery\n");
			return; }

	/* Initialize the bank-switch IC */
   	WRITE_BKSW(0x00,0x6000);
	WRITE_BKSW(0x0A,0x0000);

    for(bank = 0; bank < nbbank; bank++) {
		printf("Writing bank %d\n", bank);

		if(type == MBC1) { select_mbc_bank(ROM4_RAM32,bank); }
		if(type == MBC3) { WRITE_BKSW(bank,0x4000); }

		for(page = 0xA0; page < 0xA0 + banksize; page++) {
			printf(".");
			ADDRESS_HIGH(page);

			for(byte = 0; byte <= 0xFF; byte++) {
				ADDRESS_LOW(byte);
				WRITE_MEM(val);
			}
		}
		printf("\n");
	}

    /* disable bank-switch IC */
	WRITE_BKSW(0x00,0x0000);
}


void display_licencee (int code)
{
	switch (code)
		{
        case 0x3031:
             printf("Nintendo\n");
             break;
        case 0x3038:
             printf("Capcom\n");
             break;
        case 0x3039:
             printf("HOT.B.Co.,Ltd\n");
             break;
        case 0x3041:
             printf("Jaleco\n");
             break;
        case 0x3042:
             printf("Coconuts\n");
             break;
        case 0x3043:
             printf("Coconuts\n");
             break;
        case 0x3133:
             printf("Electronic Arts\n");
             break;
        case 0x3138:
             printf("Hudson Soft\n");
             break;
        case 0x3141:
             printf("Yanoman\n");
             break;
        case 0x3146:
             printf("Virgin Games or 7-UP\n");
             break;
        case 0x3234:
             printf("PCM Complete\n");
             break;
        case 0x3235:
             printf("San-X\n");
             break;
        case 0x3238:
             printf("Kotobuki or Walt Disney\n");
             break;
        case 0x3239:
             printf("Seta\n");
             break;
        case 0x3330:
             printf("Infogrames\n");
             break;
        case 0x3331:
             printf("Nintendo\n");
             break;
        case 0x3332:
             printf("Bandai\n");
             break;
        case 0x3333:
             printf("Acclaim\n");
             break;
        case 0x3334:
             printf("Konami\n");
             break;
        case 0x3335:
             printf("Hector\n");
             break;
        case 0x3337:
             printf("Takara\n");
             break;
        case 0x3338:
             printf("Kotobuki Systems or Interactive TV Ent.\n");
             break;
        case 0x3339:
             printf("Telstar or Accolade\n");
             break;
        case 0x3343:
             printf("Entertainment Int. or Empire/Twilight\n");
             break;
        case 0x3345:
             printf("Gremlin Graphics or Chup Chup\n");
             break;
        case 0x3431:
             printf("UBI Soft\n");
             break;
        case 0x3432:
             printf("Altus\n");
             break;
        case 0x3434:
             printf("Malibu\n");
             break;
        case 0x3436:
             printf("Angel Co.\n");
             break;
        case 0x3437:
             printf("Spectrum Holobyte\n");
             break;
        case 0x3439:
             printf("Irem\n");
             break;
        case 0x3341:
             printf("Virgin\n");
             break;
        case 0x3444:
             printf("Malibu (T.HQ)\n");
             break;
        case 0x3446:
             printf("U.S.Gold\n");
             break;
        case 0x344A:
             printf("Fox Interactive or Probe\n");
             break;
        case 0x344B:
             printf("Time Warner\n");
             break;
        case 0x3453:
             printf("Black Pearl (T.HQ)\n");
             break;
        case 0x3530:
             printf("Absolute Entertainment\n");
             break;
        case 0x3531:
             printf("Acclaim\n");
             break;
        case 0x3532:
             printf("Activision\n");
             break;
        case 0x3533:
             printf("American Sammy\n");
             break;
        case 0x3534:
             printf("Gametek/Infogenius Systems\n");
             break;
        case 0x3535:
             printf("HiTech Expressions\n");
             break;
        case 0x3536:
             printf("LJN.Toys Ltd\n");
             break;
        case 0x3537:
             printf("Matchbox International\n");
             break;
        case 0x3541:
             printf("Mindscape\n");
             break;
        case 0x3542:
             printf("ROMStar\n");
             break;
        case 0x3543:
             printf("Taxan\n");
             break;
        case 0x3544:
             printf("Midway/Williams/Tradewest or Rare\n");
             break;
        case 0x3630:
             printf("Titus\n");
             break;
        case 0x3631:
             printf("Virgin\n");
             break;
        case 0x3637:
             printf("Ocean\n");
             break;
        case 0x3639:
             printf("Electronic Arts\n");
             break;
        case 0x3645:
             printf("Elite Systems\n");
             break;
        case 0x3646:
             printf("Electro Brain\n");
             break;
        case 0x3730:
             printf("Infogrames\n");
             break;
        case 0x3731:
             printf("Interplay Productions\n");
             break;
        case 0x3732:
             printf("First Star Software or Broderbund\n");
             break;
        case 0x3733:
             printf("Sculptured Software\n");
             break;
        case 0x3735:
             printf("The sales curve Ltd\n");
             break;
        case 0x3738:
             printf("T.HQ Inc.\n");
             break;
        case 0x3739:
             printf("Accolade\n");
             break;
        case 0x3741:
             printf("Triffix Ent.Inc.\n");
             break;
        case 0x3743:
             printf("MicroProse or NMS\n");
             break;
        case 0x3746:
             printf("Kotobuki Systems or Kemco\n");
             break;
        case 0x3830:
             printf("Misawa or NMS\n");
             break;
        case 0x3833:
             printf("LOZC or G.Amusements\n");
             break;
        case 0x3836:
             printf("Zener Works or Tokuna Shoten\n");
             break;
        case 0x3842:
             printf("Bullet-Proof Software\n");
             break;
        case 0x3843:
             printf("Vic Tokai\n");
             break;
        case 0x3845:
             printf("Character Soft or Sanrio\n");
             break;
        case 0x3846:
             printf("I'Max\n");
             break;
        case 0x3932:
             printf("Video System\n");
             break;
        case 0x3933:
             printf("Bec\n");
             break;
        case 0x3935:
             printf("Varie\n");
             break;
        case 0x3936:
             printf("Yonesawa or S'Pal\n");
             break;
        case 0x3937:
             printf("Kaneko\n");
             break;
        case 0x3939:
             printf("Pack-in-video\n");
             break;
        case 0x3941:
             printf("Nihon Bussan\n");
             break;
        case 0x3942:
             printf("Temco\n");
             break;
        case 0x3943:
             printf("Imagineer Co.,Ltd\n");
             break;
        case 0x3946:
             printf("Namco or Nova\n");
             break;
        case 0x4131:
             printf("Hori Electric\n");
             break;
        case 0x4134:
             printf("Konami\n");
             break;
        case 0x4136:
             printf("Kawada\n");
             break;
        case 0x4137:
             printf("Takara\n");
             break;
        case 0x4139:
             printf("Technos Japan Corp.\n");
             break;
        case 0x4141:
             printf("First Star Software or Broderbund/dB Soft\n");
             break;
        case 0x4143:
             printf("Toei Animation\n");
             break;
        case 0x4144:
             printf("TOHO Co.,Ltd\n");
             break;
        case 0x4146:
             printf("Namco Hometek\n");
             break;
        case 0x4147:
             printf("Playmates/Shiny\n");
             break;
        case 0x4230:
             printf("Acclaim or LJN\n");
             break;
        case 0x4231:
             printf("Nexoft\n");
             break;
        case 0x4232:
             printf("Bandai\n");
             break;
        case 0x4234:
             printf("Enix\n");
             break;
        case 0x4236:
             printf("HAL\n");
             break;
        case 0x4237:
             printf("SNK (america)\n");
             break;
        case 0x4239:
             printf("Pony Canyon\n");
             break;
        case 0x4241:
             printf("Culture Brain\n");
             break;
        case 0x4242:
             printf("SunSoft\n");
             break;
        case 0x4244:
             printf("Sony Imagesoft\n");
             break;
        case 0x4246:
             printf("Sammy\n");
             break;
        case 0x4330:
             printf("Taito\n");
             break;
        case 0x4332:
             printf("Kemco\n");
             break;
        case 0x4333:
             printf("SquareSoft\n");
             break;
        case 0x4334:
             printf("Tokuma Shoten\n");
             break;
        case 0x4335:
             printf("Data East\n");
             break;
        case 0x4336:
             printf("Tonkin House\n");
             break;
        case 0x4338:
             printf("Koei\n");
             break;
        case 0x4339:
             printf("UPL Comp.Ltd\n");
             break;
        case 0x4341:
             printf("Ultra games/konami\n");
             break;
        case 0x4342:
             printf("Vap Inc.\n");
             break;
        case 0x4343:
             printf("USE Co.,Ltd\n");
             break;
        case 0x4344:
             printf("Meldac\n");
             break;
        case 0x4345:
             printf("FCI\n");
             break;
        case 0x4346:
             printf("Angel Co.\n");
             break;
        case 0x4431:
             printf("Sofel\n");
             break;
        case 0x4432:
             printf("Quest\n");
             break;
        case 0x4433:
             printf("Sigma Enterprises\n");
             break;
        case 0x4434:
             printf("Lenar or Ask kodansha\n");
             break;
        case 0x4436:
             printf("Naxat Soft\n");
             break;
        case 0x4437:
             printf("Copya Systems\n");
             break;
        case 0x4439:
             printf("Banpresto\n");
             break;
        case 0x4441:
             printf("Tomy\n");
             break;
        case 0x4442:
             printf("Hiro\n");
             break;
        case 0x4444:
             printf("NCS or Masiya\n");
             break;
        case 0x4445:
             printf("Human\n");
             break;
        case 0x4446:
             printf("Altron\n");
             break;
        case 0x4530:
             printf("KK DCE or Yaleco\n");
             break;
        case 0x4531:
             printf("Towachiki\n");
             break;
        case 0x4532:
             printf("Yutaka\n");
             break;
        case 0x4533:
             printf("Varie\n");
             break;
        case 0x4535:
             printf("Epoch\n");
             break;
        case 0x4537:
             printf("Athena\n");
             break;
        case 0x4538:
             printf("Asmik\n");
             break;
        case 0x4539:
             printf("Natsume\n");
             break;
        case 0x4541:
             printf("King Records or A Wave\n");
             break;
        case 0x4542:
             printf("Altus\n");
             break;
        case 0x4543:
             printf("Epic/Sony\n");
             break;
        case 0x4545:
             printf("IGS Corp\n");
             break;
        case 0x454A:
             printf("Virgin\n");
             break;
        case 0x4630:
             printf("A Wave or Accolade\n");
             break;
        case 0x4633:
             printf("Extreme or Malibu Int.\n");
             break;
        case 0x4642:
             printf("Psycnosis\n");
             break;
        default:
             printf("%hX unknown\n",code);
             break;
 		}
}

void display_clock()
{
 	unsigned char second, minute, hour, day, carry;

 	/* Initialize the bank-switch IC */
	WRITE_BKSW(0x0A,0x0000);

	/* latches all clock counter data */
	WRITE_BKSW(0x0,0x6000);
	WRITE_BKSW(0x1,0x6000);

	WRITE_BKSW(0x08,0x4000);
	ADDRESS_HIGH(0xA0);
    ADDRESS_LOW(0x00);
	second = READ(RAM);

	WRITE_BKSW(0x09,0x4000);
	ADDRESS_HIGH(0xA0);
    ADDRESS_LOW(0x00);
	minute = READ(RAM);

	WRITE_BKSW(0x0A,0x4000);
	ADDRESS_HIGH(0xA0);
    ADDRESS_LOW(0x00);
	hour = READ(RAM);

	WRITE_BKSW(0x0B,0x4000);
	ADDRESS_HIGH(0xA0);
    ADDRESS_LOW(0x00);
	day = READ(RAM);

	WRITE_BKSW(0x0C,0x4000);
	ADDRESS_HIGH(0xA0);
    ADDRESS_LOW(0x00);
	carry = READ(RAM);

    printf("Cartridge time: %02d:%02d:%02d HH:MM:SS   day %d of the year\n",
     	hour,minute,second,(carry & 0x01)*256+day);

	WRITE_BKSW(0x0,0x6000);
	WRITE_BKSW(0x0,0x4000);
	WRITE_BKSW(0x0,0x0000);
}

void read_header(unsigned char *h, BYTE mode, FILE * fp)
{
	int byte, index = 0;
	int left, right;
 	fpos_t pos;
    char Gname[30] = {0};

	if(mode == File) {
		PrintChipSize();
		/* Set file position to header and read data */
    	pos = 0x100;
   		fsetpos( fp, &pos );
		for(byte = 0; byte < 0x50; byte++) {
			h[index++] = fgetc(fp);
		}
		pos = 0x00;
		fsetpos( fp, &pos );
	}
	else {
		ADDRESS_HIGH(0x01);
		for(byte = 0; byte < 0x50; byte++) {
			ADDRESS_LOW(byte);
			h[index++] = READ(ROM);
		}
	}
	strncat(Gname,&h[0x34],16);
	printf("Game name     : %s\n", Gname);

	printf("Licencee      : ");

 	if(h[0x4B] == 0x33) { display_licencee(h[0x44]*256+h[0x45]); }
	else {
		left  = ((h[0x4B] & 0xf0) >> 4);
		if(left > 9) left = left + 55; else left = left + 0x30;
		right = (h[0x4B] & 0x0f);
		if(right > 9) right = right + 55; else right = right + 0x30;
		display_licencee(left*256 + right); }

	if(h[0x43] == 0x80)
		printf("              : Color GameBoy Game, but will run on old GameBoy also\n");

	if(h[0x43] == 0xC0)
		printf("              : Color GameBoy Game, will not run on old GameBoy\n");

	if(h[0x46] == 0x03)
		printf("              : Super GameBoy functions included\n");

	if(h[0x4A] == 0x01)
		printf("              : Non-Japanese Game\n");

	if(h[0x4A] == 0x00)
		printf("              : Japanese Game\n");

	if(h[0x47] <= 0xFB) {
		printf("Cartridge type: %d(%s)\n", h[0x47], type[h[0x47]]);
		if (mbc[h[0x47]] == MBC3) display_clock(); }

	if(h[0x47] == 0xFC) {
		printf("Cartridge type: %d(%s)\n", h[0x47], "Pocket Camera");
	    h[0x47] = 0x13; }

	if(h[0x47] == 0xFD) {
		printf("Cartridge type: %d(%s)\n", h[0x47], "Bandai TAMA5");
	    h[0x47] = 0x13; }

	if(h[0x47] == 0xFE) {
		printf("Cartridge type: %d(%s)\n", h[0x47], "ROM+HuC3");
	    h[0x47] = 0x03; }

	if(h[0x47] == 0xFF) {
		printf("Cartridge type: %d(%s)\n", h[0x47], "ROM+HuC1+SRAM(Battery)");
	    h[0x47] = 0x03; }

	printf("ROM size      : %d(%d kB)\n", h[0x48], rom[h[0x48]]);

	if(sram[h[0x47]] == MBC2)
		printf("RAM size      : 512*4 Bit\n");
	else
		printf("RAM size      : %d(%d kB)\n", h[0x49], ram[h[0x49]]);
	checksum = ((unsigned)h[0x4E] << 8) + h[0x4F];
}

/* Return actual chip size in bits */

LONG ChipSize (BYTE size)
   {
   BYTE i;
   LONG Length = 16384;

   for(i=0; i<size; i++)
      Length *= 2;

   return(Length);
   }

/* Print chip size */

void PrintChipSize (void)
   {
   LONG i = ChipSize(Size);
   printf("Chip size     : %lu bits (%lu bytes)\n", i, i/8);
   }

/* Set chip address */

void SetAddr (LONG Addr)
{
	int bank, AdrReg, newreg;

  	Address = Addr;

	if (newcart == 0) {	   /* MBC1 carts */
	  if (Cart_Bank == 0) newreg = (int)((Address & 0x180000) >>19);
	  else newreg = Cart_Bank;

      select_mbc_bank(ROM16_RAM8,newreg);
      WRITE_BKSW(ROM4_RAM32,0x6000);			/* protect register */

      bank = (int)((Address & 0x7ffff) >> 14);
      WRITE_BKSW(bank,0x2100);

      AdrReg = (int)(Address & 0x3fff);
      if ((Address & 0x7ffff) > 0x3fff) AdrReg += 16384;
    }
    else {				   /* MBC3 or MBC5(32MB) carts */
      bank = (int)((Address & 0x3fffff) >> 14);
      WRITE_BKSW(bank,0x2100);

      AdrReg = (int)(Address & 0x3fff);
	  if ((Address & 0x3fffff) > 0x3fff) AdrReg += 16384;
    }

	ADDRESS_LOW(AdrReg & 0x00ff);
	ADDRESS_HIGH(AdrReg >> 8);
//	printf("Bank: %hx  Adresse: %hx\n",bank,AdrReg);
}

/* Increment chip address */

void IncAddr(void)
{
   	Address++;
   	SetAddr(Address);
}

/* Write a byte to chip */

void WriteByte(BYTE val)
{
    WRITE_FLASH(val);
}

/* Read a byte from chip */

BYTE ReadByte(void)
{
   	BYTE i;

	i = READ(ROM);
  	return(i);
}

int Identify_Flashrom (void)
{
	  int DeviceType, Manufacturer, Flashsize, Mode = 0;

	  WRITE_BKSW(0x01,0x2100);
      ADDRESS_LOW(0x55); ADDRESS_HIGH(0x55); WRITE_FLASH(0xAA);
      ADDRESS_LOW(0xAA); ADDRESS_HIGH(0x2A); WRITE_FLASH(0x55);
      ADDRESS_LOW(0x55); ADDRESS_HIGH(0x55); WRITE_FLASH(0x90);

   	  SetAddr(0x00);
      Manufacturer = ReadByte();
   	  SetAddr(0x01);
      DeviceType   = ReadByte();

//	   printf ("\nManufacturer: %hx, DeviceID: %hx\n",Manufacturer,DeviceType);

      if ((Manufacturer != 0x01) && (Manufacturer != 0x04) && (Manufacturer != 0xDA)
       && (Manufacturer != 0x1F) && (Manufacturer != 0x20)) {

	   	 select_mbc_defaults();
	     WRITE_BKSW(0xAA,0x555);
	     WRITE_BKSW(0x55,0x2AA);
	     WRITE_BKSW(0x90,0x555);

		 ADDRESS_LOW(0x00);
		 ADDRESS_HIGH(0x00);
		 Manufacturer = ReadByte();
    	 ADDRESS_LOW(0x01);
         DeviceType  = ReadByte();
    	 Mode = 1;

         if ((Manufacturer != 0x01) && (Manufacturer != 0x04)
          && (Manufacturer != 0x1F) && (Manufacturer != 0x20)) {

            WRITE_BKSW(0xff,0x4000);            /* Set flash read mode */
            WRITE_BKSW(0x00,0x3000);            /* Set bank ms byte */
            WRITE_BKSW(0x00,0x2000);            /* Set bank ls byte */
            WRITE_BKSW(0x90,0x4000);            /* Set read manufacturer mode */

            SetAddr(0x00);
            Manufacturer = ReadByte();
  			if (Manufacturer == 0x89) /* Intel DA28f320J5 */ SetAddr(0x02);
   			 else SetAddr(0x01);
            DeviceType   = ReadByte();
            Mode = 2;

 //      	printf ("\n89 Manufacturer: %hx, DeviceID: %hx\n",Manufacturer,DeviceType);

	        WRITE_BKSW(0xff,0x4000);      		/* Set read memory mode */

            if ((Manufacturer != 0xB0) && (Manufacturer != 0x89)) {			/* no Nintendo cart */

	   	 		select_mbc_defaults();
	   	 		WRITE_BKSW(0x00,0x3000);

                WRITE_BKSW(0x02,0x2000);
      			ADDRESS_LOW(0xAA); ADDRESS_HIGH(0x6A);
      			WRITE_FLASH(0xAA);

	   	 		WRITE_BKSW(0x00,0x3000);
	  			WRITE_BKSW(0x01,0x2000);
      			ADDRESS_LOW(0x54); ADDRESS_HIGH(0x55);
      			WRITE_FLASH(0x55);

	   	 		WRITE_BKSW(0x00,0x3000);
	  			WRITE_BKSW(0x02,0x2000);
      			ADDRESS_LOW(0xAA); ADDRESS_HIGH(0x6A);WRITE_FLASH(0x90);

            	SetAddr(0x00);
      			Manufacturer = ReadByte();
 	  			ADDRESS_LOW(0x02);
      			DeviceType   = ReadByte();

//	printf ("\nManufacturer: %hx, DeviceID: %hx\n\n",Manufacturer,DeviceType);


	  			WRITE_BKSW(0x02,0x2000);
      			ADDRESS_LOW(0xAA); ADDRESS_HIGH(0x6A); WRITE_FLASH(0xAA);
	  			WRITE_BKSW(0x01,0x2000);
      			ADDRESS_LOW(0x54); ADDRESS_HIGH(0x55); WRITE_FLASH(0x55);
	  			WRITE_BKSW(0x02,0x2000);
      			ADDRESS_LOW(0xAA); ADDRESS_HIGH(0x6A); WRITE_FLASH(0xF0);

                if (Manufacturer != 0xC2) {    /* no Bung 16Mbit cart */
               		printf("\nManufacturer (%hX) not supported !\n",Manufacturer);
   		       		ErrorExit();
                }
   		    }
   		 }
      }

//	    printf ("\nManufacturer: %hx, DeviceID: %hx\n\n",Manufacturer,DeviceType);

      printf("Manufacturer  : ");
      switch (Manufacturer)
            {
            case 0x01:
               printf("AMD or TI\n");
               break;
            case 0x04:
               printf("Fujitsu\n");
               break;
            case 0x1F:
               printf("Atmel\n");
               break;
            case 0x20:
               printf("SGS-Thomson\n");
               break;
            case 0x89:
               printf("Intel\n");
               break;
            case 0xB0:
               printf("Sharp\n");
               break;
            case 0xC2:
               printf("Macronix\n");
               break;
            case 0xDA:
               printf("Winbond\n");
               break;
             }

      printf("DeviceType    : ");
      switch (DeviceType)
            {
            case 0x00:
               printf("29F100B/T\n");
               Flashsize = 5;
               break;
            case 0x57:
               printf("29F200B\n");
               Flashsize = 6;
               break;
            case 0x51:
               printf("29F200T\n");
               Flashsize = 6;
               break;
            case 0x17:
               Flashsize = 6;
			   if (Mode == 1) {
			     Algorithm = NEW_ALGOR;
			     printf("49F010 with new programming mode\n"); }
			   else printf("49F010\n");
               break;
            case 0x20:
               printf("29F010\n");
               Flashsize = 6;
               break;
            case 0xAB:
               printf("29F400B\n");
               Flashsize = 7;
               break;
            case 0x23:
               printf("29F400T\n");
                Flashsize = 7;
               break;
            case 0xA4:
               Flashsize = 8;
			   if (Mode == 1) {
			     Algorithm = NEW_ALGOR;
			     printf("29F040 with new programming mode\n"); }
			   else printf("29F040\n");
               break;
            case 0xE2:
               Flashsize = 8;
			   if (Mode == 1) {
			     Algorithm = NEW_ALGOR;
			     printf("29F040 with new programming mode\n"); }
			   else printf("29F040\n");
               break;
            case 0x46:
               Flashsize = 0x5008; /* = 8 */
			   printf("W29C040P\n");
			   printf("Not supported by ReadPlus !\n");
			   ErrorExit();
               break;
            case 0x12:
               Flashsize = 8;
			   if (Mode == 1) {
			     Algorithm = NEW_ALGOR;
			     printf("AT49F040T with new programming mode\n"); }
			   else printf("AT49F040T\n");
               break;
            case 0x13:
               Flashsize = 8;
			   if (Mode == 1) {
			     Algorithm = NEW_ALGOR;
			     printf("AT49F040 with new programming mode\n"); }
			   else printf("AT49F040\n");
               break;
            case 0xD5:
               printf("29F080\n");
               Flashsize = 9;
               break;
            case 0xAD:
               Flashsize = 10;
			   if (Mode == 1) {
			     Algorithm = NEW_ALGOR;
			     printf("29F016B with new programming mode\n"); }
			   else printf("29F016B\n");
               break;
            case 0xF1:
               Flashsize = 0x500A; /* = 10 (16Mbit)*/
			   printf("MX29F1610\n");
			   printf("Not supported by ReadPlus !\n");
			   ErrorExit();
               break;
            case 0x41:
               Flashsize = 11;     /* 32Mbit*/
			   if (Mode == 1) {
			     Algorithm = NEW_ALGOR;
			     printf("29F032B with new programming mode\n"); }
			   else printf("29F032B\n");
               break;
            case 0x88:
               Flashsize = 0x400B; /* = 11 32Mbit*/
               printf("LH28F032SU based Nintendo flash cartridge\n");
               break;
            case 0x14:
               // Nintendo cartridge is not supported by Bung Xchanger
			   if (BungHW == 0) {
				   Flashsize = 0x410B; /* = 11 (32Mbit)*/
				   printf("DA28F320J5 based Nintendo flash cartridge\n");}
               else {
				   Flashsize = 0x510B; /* = 11 (32Mbit)*/
				   printf("28F320J5\n");}
			   break;
            case 0x15:
               Flashsize = 0x510C; /* = 12 (64Mbit)*/
               printf("28F640J5\n");
               // printf("FlashSize :%hx\n",Flashsize);
			   // printf ("Bung Hardware =%d",BungHW);
               break;
            default:
               printf("unknown\n\n");
               Flashsize = 0;
               break;
            }

	  if (Mode == 2) return (Flashsize);

      if (Algorithm == NEW_ALGOR) WRITE_BKSW(0xF0,0x0000);  /* Reset Flashchip */
      else {SetAddr(0x00); WRITE_FLASH(0xF0);}
      return (Flashsize);
}

/* Wait for program byte completion */

BYTE ProgramDone (BYTE val)
{
   BYTE Check;
   BYTE Pass = 0;

   Check = ReadByte();

   /* Loop until chip done programming */
   while ((Check != val) && ((Check & 0x20)==0)) Check = ReadByte();

   if (Check == val) Pass = 1;
   else {
      	Check = ReadByte();
       	if (Check == val) Pass = 1;
   }
   return (Pass);
}


/* Program a byte on chip */

BYTE ProgramByte(LONG Addr, BYTE val)
   {
   BYTE Pass = 0;
   int AdrReg;

   if (Algorithm == AMD29FXXX)
      {
	  WRITE_BKSW(0x01,0x2100);
      ADDRESS_LOW(0x55); ADDRESS_HIGH(0x55); WRITE_FLASH(0xAA);
      ADDRESS_LOW(0xAA); ADDRESS_HIGH(0x2A); WRITE_FLASH(0x55);
      ADDRESS_LOW(0x55); ADDRESS_HIGH(0x55); WRITE_FLASH(0xA0);
   	  SetAddr(Addr);
   	  WriteByte(val);
      }

   if (Algorithm == NEW_ALGOR)
      {
      SetAddr(Addr);
      WRITE_BKSW(0xAA,0x555);
      WRITE_BKSW(0x55,0x2AA);
      WRITE_BKSW(0xA0,0x555);

      AdrReg = (int)(Addr & 0x3fff);
   	  if ((Addr & 0x3fffff) > 0x3fff) AdrReg += 0x4000;

      WRITE_BKSW(val,AdrReg);
      SetAddr(Addr);
      }

   /* Wait until chip done programming */
   Pass = ProgramDone(val);

   if (Pass == 0)
      printf("\nWrite failed at Address %lx of cartridge.\n", Address);

   return (Pass);
   }

void dump(BYTE BaseAdr)
   {
   int i;
   BYTE First = 1;
   BYTE val,ramrom;
   BYTE Display[17] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};

   if((BaseAdr >= 0xA0) & (BaseAdr < 0xC0))
   		{ramrom=1; WRITE_BKSW(0x0A,0x0000); WRITE_BKSW(0x00,0x4000); }
   else {ramrom=0; WRITE_BKSW(0x01,0x2100);	}

   ADDRESS_HIGH(BaseAdr);

   for(i=0; i<256; i++)
      {
      if (First == 1)
         {
         if (i < 16) printf("0");
         printf("%hx - ",i);
         First = 0;
         }
	  ADDRESS_LOW(i);
      val =  READ(ramrom);

      if ((val > 31) & (val < 127))
       Display[i & 0xf] = val;
      else
       Display[i & 0xf] = 46;

      if (val < 16)
         printf("0");
      printf("%hx ",val);
      if ((i & 0xf)==0xf)
         {
         First = 1;
         printf("   %3s",(&Display[0]));
         printf("\n");
         }
      }
   if(ramrom == 1) { WRITE_BKSW(0x00,0x0000); }
   }

/* Erase AMD chips */

BYTE EraseAm29Fxxx(void)
   {
   BYTE Pass = 0;
   BYTE Val  = 0;

   if (Algorithm == AMD29FXXX) {
     WRITE_BKSW(0x01,0x2100);
     ADDRESS_LOW(0x55); ADDRESS_HIGH(0x55); WRITE_FLASH(0xAA);
     ADDRESS_LOW(0xAA); ADDRESS_HIGH(0x2A); WRITE_FLASH(0x55);
     ADDRESS_LOW(0x55); ADDRESS_HIGH(0x55); WRITE_FLASH(0x80);
     ADDRESS_LOW(0x55); ADDRESS_HIGH(0x55); WRITE_FLASH(0xAA);
     ADDRESS_LOW(0xAA); ADDRESS_HIGH(0x2A); WRITE_FLASH(0x55);
     ADDRESS_LOW(0x55); ADDRESS_HIGH(0x55); WRITE_FLASH(0x10);
     ADDRESS_LOW(0x00); ADDRESS_HIGH(0x00);
   }

   if (Algorithm == NEW_ALGOR) {

     WRITE_BKSW(0xAA,0x555);
     WRITE_BKSW(0x55,0x2AA);
     WRITE_BKSW(0x80,0x555);
     WRITE_BKSW(0xAA,0x555);
     WRITE_BKSW(0x55,0x2AA);
     WRITE_BKSW(0x10,0x555);
     ADDRESS_LOW(0x00); ADDRESS_HIGH(0x00);
   }

   printf("Erasing GameBoy cartridge...");

   Val = ReadByte();
   /* Wait for erase to complete */
   while (((Val & 0x80)==0) && ((Val & 0x20)==0))
      {
      Val = ReadByte();
   	  }
   if ((Val & 0x80)==0x80) Pass=1;

   if (Pass == 0)
      printf("Cartridge erase failed!\n");
   else
      printf("ok\n");

   return (Pass);
   }


/* Erase Nintendo Type 1 (LH28F032) flash cart */

BYTE EraseNFlash (int FlashBlocks)
{
   BYTE Pass = 0;
   BYTE Val  = 0;
   int i;

   printf("Erasing Nintendo Type 1 (LH28F032) flash cartridge...");

   for (i=0; i<FlashBlocks; i++)
      {

//    printf("Bank %hx \n",i);
      /* Set read memory mode */
      ADDRESS_LOW(0x00); ADDRESS_HIGH(0x40); WRITE_NFLASH(0xff);

      /* Set read extended status register */
      ADDRESS_LOW(0x00); ADDRESS_HIGH(0x40); WRITE_NFLASH(0x71);

      /* Set read block status register */
      ADDRESS_LOW(0x02); ADDRESS_HIGH(0x40);

      /* wait until queue is available */
      while (READ(RAM) & 0x8)
         {
         };

      /* Set memory bank ls byte */
      ADDRESS_LOW(0x00); ADDRESS_HIGH(0x20); WRITE_NFLASH(i*4);

      /* Set block erase/confirm mode */
      ADDRESS_LOW(0x00); ADDRESS_HIGH(0x40); WRITE_NFLASH(0x20);
      ADDRESS_LOW(0x00); ADDRESS_HIGH(0x40); WRITE_NFLASH(0xd0);
      ADDRESS_LOW(0x00); ADDRESS_HIGH(0x40); WRITE_NFLASH(0xd0);

      /* Set read extended status register */
      ADDRESS_LOW(0x00); ADDRESS_HIGH(0x40); WRITE_NFLASH(0x71);

      /* Set read global status register */
      ADDRESS_LOW(0x02); ADDRESS_HIGH(0x40);

      /* wait until block is ready */
      while (!(READ(RAM) & 0x80))
         {
         };
      /* Set clear status registers */
      ADDRESS_LOW(0x00); ADDRESS_HIGH(0x40); WRITE_NFLASH(0x50);
      }
   Pass = 1;

   /* Set read memory mode */
   ADDRESS_LOW(0x00); ADDRESS_HIGH(0x40); WRITE_NFLASH(0xff);

   if (Pass == 0)
      printf("Cartridge erase failed!\n");
   else
      printf("ok\n");

   return (Pass);
}

/* Erase Nintendo Type 2 (DA28F320) flash cart */

BYTE EraseNFlash2 (int FlashBlocks)
{
   BYTE Pass = 0;
   BYTE Val  = 0;
   int i;

   printf("Erasing Nintendo Type 2 (DA28F320) flash cartridge...");

   for (i=0; i<FlashBlocks; i++)
      {

      /* Set memory bank ls byte */
      ADDRESS_LOW(0x00); ADDRESS_HIGH(0x20); WRITE_NFLASH(i*8);

      /* Set block erase/confirm mode */
      ADDRESS_LOW(0x00); ADDRESS_HIGH(0x40); WRITE_NFLASH(0x20);
      ADDRESS_LOW(0x00); ADDRESS_HIGH(0x40); WRITE_NFLASH(0xd0);

      /* Set read global status register */
      ADDRESS_LOW(0x02); ADDRESS_HIGH(0x40);

      /* wait until block is ready */
      while (!(READ(RAM) & 0x80))
         {
         };
      /* Set clear status registers */
      ADDRESS_LOW(0x00); ADDRESS_HIGH(0x40); WRITE_NFLASH(0x50);
      }
   Pass = 1;

   /* Set read memory mode */
   ADDRESS_LOW(0x00); ADDRESS_HIGH(0x40); WRITE_NFLASH(0xff);

   if (Pass == 0)
      printf("Cartridge erase failed!\n");
   else
      printf("ok\n");

   return (Pass);
}

/* Erase MBC5 homemade (28F640J5/28F320J5) flash cart */

BYTE EraseNFlash3 (int FlashBlocks)
{
   BYTE Pass = 0;
   BYTE Val  = 0;
   int MBCbank;
   int i =0;
   // int ResultSR;

   printf("Erasing MBC5 homemade 28F640J5/28F320J5 flash card ...\n");

	  // set MBC to any address
	//  WRITE_BKSW(0x01,0x2100);

      /* clear lock bits */
      ADDRESS_LOW(0x00); ADDRESS_HIGH(0x40); WRITE_FLASH(0x60);
      ADDRESS_LOW(0x00); ADDRESS_HIGH(0x40); WRITE_FLASH(0xd0);
      // back to read mode
      //ADDRESS_LOW(0x00); ADDRESS_HIGH(0x40); WRITE_FLASH(0xff);

    for (i=0; i<(FlashBlocks); i++)
  // for (i=0; i<1; i++)
      {
      if (FlashBlocks == 32) MBCbank = i*4;
	  else  MBCbank = i*8;
      printf("\rMBC5 bank: %d, FlashBlock: %d of %d",(MBCbank),(i+1),FlashBlocks);
      //printf(".");

	  /* first set the MBC5 to specific block in ROMB0*/
      WRITE_BKSW((MBCbank),0x2100);
	  //set MBC5 ROMB1 bit for blocks more as 255
	  if ((MBCbank) > 255)
		WRITE_BKSW(0x01,0x3100);
      else
        WRITE_BKSW(0x00,0x3100);

	  // device back to read mode
	  //ADDRESS_LOW(0x00);ADDRESS_HIGH(0x40); WRITE_FLASH(0xff);
      /* Write 0x20 to the block start address to init erase*/
      ADDRESS_LOW(0x00); ADDRESS_HIGH(0x41); WRITE_FLASH(0x20);
      /* Set block erase/confirm mode */
      ADDRESS_LOW(0x00); ADDRESS_HIGH(0x41); WRITE_FLASH(0xd0);

      /* wait until block is ready */
      while (!(READ(RAM) & 0x80))
         {
 		 Pass = (int) (!(READ(RAM) & 0x20));
		 //printf ("StatusReg=%hx\n",READ(RAM));
         };

	  // device back to read mode
	  //ADDRESS_LOW(0x00);ADDRESS_HIGH(0x40); WRITE_FLASH(0xff);

	  //why do this twice ????
      //Write 0x20 to the block start address to init erase
      ADDRESS_LOW(0x00); ADDRESS_HIGH(0x41); WRITE_FLASH(0x20);
      // Set block erase/confirm mode
      ADDRESS_LOW(0x00); ADDRESS_HIGH(0x41); WRITE_FLASH(0xd0);

      //wait until block is ready
      while (!(READ(RAM) & 0x80))
         {
 		 Pass = (int) (!(READ(RAM) & 0x20));
		 //printf ("StatusReg=%hx\n",READ(RAM));
         };

	  // device back to read mode
	  ADDRESS_LOW(0x00);ADDRESS_HIGH(0x40); WRITE_FLASH(0xff);

      }


      ADDRESS_LOW(0x00);ADDRESS_HIGH(0x40); WRITE_FLASH(0xff);

   if (Pass == 0)
      printf("\nCartridge erase failed!\n");
   else
      printf("....ok\n");

   return (Pass);
}


/* Read from file & write to Type 1 (LH28F032) flash cart  */

void ReadWriteBlocks (FILE *fp)
{
   int j = 256;
   int AdrReg,bank,c,i;
   LONG Addr = 0;

   /* Set Nintendo flash cart to memory read */
   ADDRESS_LOW(0x00); ADDRESS_HIGH(0x40); WRITE_NFLASH(0xff);
   /*  Set bank ms byte */
   ADDRESS_LOW(0x00); ADDRESS_HIGH(0x30); WRITE_NFLASH(0x0);
   /*  Set bank ms byte */
   ADDRESS_LOW(0x00); ADDRESS_HIGH(0x20); WRITE_NFLASH(0x0);

   printf("Programming Nintendo flash cartridge...");

   while ( (j == 256) &&
           (Addr < (ChipSize(Size)/8)) )
      {

      if ((Addr & 0x7fff) == 0)
         printf(".",Addr);

      bank = (int)(( (Addr & 0x3fffff) >> 16)*4);
      ADDRESS_LOW(0x00); ADDRESS_HIGH(0x20); WRITE_NFLASH(bank);

      /* Set read extended status register */
      ADDRESS_LOW(0x00); ADDRESS_HIGH(0x40); WRITE_NFLASH(0x71);

      /* Set read block status register */
      ADDRESS_LOW(0x04); ADDRESS_HIGH(0x40);

      /* wait until page buffer is available */
      while (READ(RAM) & 0x4)
         {
         };

      bank = (int)((Addr & 0x3fffff) >> 14);
      ADDRESS_LOW(0x00); ADDRESS_HIGH(0x20); WRITE_NFLASH(bank);

      /* Sequential load to page buffer */
      ADDRESS_LOW(0x00); ADDRESS_HIGH(0x40); WRITE_NFLASH(0xe0);
      ADDRESS_LOW(0x00); ADDRESS_HIGH(0x40); WRITE_NFLASH(0xff);
      ADDRESS_LOW(0x00); ADDRESS_HIGH(0x40); WRITE_NFLASH(0x00);

      i = 0;
      j = 0;
      while (i<256)
         {
         AdrReg = (int)(Addr & 0x3fff) + 0x4000;

         ADDRESS_LOW(AdrReg & 0x00ff);
         ADDRESS_HIGH(AdrReg >> 8);

         if (!feof(fp))
            {
            c= fgetc(fp);
            j++;
            }
         else
            c = 0xff;
         WRITE_NFLASH(c);
         Addr++;
         i++;
         }

      ADDRESS_LOW(0x00); ADDRESS_HIGH(0x40); WRITE_NFLASH(0xff);

      if (i>0)
         {
         bank = (int)(( (Addr & 0x3fffff) >> 16)*4);
         ADDRESS_LOW(0x00); ADDRESS_HIGH(0x20); WRITE_NFLASH(bank);

         /* Set read extended status register */
         ADDRESS_LOW(0x00); ADDRESS_HIGH(0x40); WRITE_NFLASH(0x71);

         /* Set read block status register */
         ADDRESS_LOW(0x02); ADDRESS_HIGH(0x40);

         /* wait until queue is available */
         while (READ(RAM) & 0x8)
            {
            };

         bank = (int)(((Addr-256) & 0x3fffff) >> 14);
         ADDRESS_LOW(0x00); ADDRESS_HIGH(0x20); WRITE_NFLASH(bank);

         /* Write page buffer to flash */
         ADDRESS_LOW(0x00); ADDRESS_HIGH(0x40); WRITE_NFLASH(0x0c);
         ADDRESS_LOW(0x00); ADDRESS_HIGH(0x40); WRITE_NFLASH(0xff);

         AdrReg = (int)((Addr-256) & 0x3fff) + 0x4000;
         ADDRESS_LOW(AdrReg & 0x00ff);
         ADDRESS_HIGH(AdrReg >> 8);
         WRITE_NFLASH(0x00);


         bank = (int)(( ((Addr-256) & 0x3fffff) >> 16)*4);
         ADDRESS_LOW(0x00); ADDRESS_HIGH(0x20); WRITE_NFLASH(bank);

         /* Set read extended status register */
         ADDRESS_LOW(0x00); ADDRESS_HIGH(0x40); WRITE_NFLASH(0x71);

         /* Set read block status register */
         ADDRESS_LOW(0x02); ADDRESS_HIGH(0x40);

         /* wait until block is ready */
         while (!(READ(RAM) & 0x80))
            {
            };

         /* Set clear status registers */
         ADDRESS_LOW(0x00); ADDRESS_HIGH(0x40); WRITE_NFLASH(0x50);

         /* Set read memory mode */
         ADDRESS_LOW(0x00); ADDRESS_HIGH(0x40); WRITE_NFLASH(0xff);

        }
      }
}


/* Read from file & write to Type 2 (DA28F320) flash cart */

void ReadWriteBlocks2 (FILE *fp)
{
   int j = 32;
   int AdrReg,bank,c,i;
   LONG Addr = 0;
   int waiting;

   /* Set Nintendo flash cart to memory read */
   ADDRESS_LOW(0x00); ADDRESS_HIGH(0x40); WRITE_NFLASH(0xff);
   /*  Set bank ms byte */
   ADDRESS_LOW(0x00); ADDRESS_HIGH(0x30); WRITE_NFLASH(0x0);
   /*  Set bank ms byte */
   ADDRESS_LOW(0x00); ADDRESS_HIGH(0x20); WRITE_NFLASH(0x0);

   printf("Programming Nintendo flash cartridge...");

   while ( (j == 32) &&
           (Addr < (ChipSize(Size)/8)) )
      {

      if ((Addr & 0x7fff) == 0)
         printf(".");

      bank = (int)(( (Addr & 0x3fffff) >> 17)*8);
      ADDRESS_LOW(0x00); ADDRESS_HIGH(0x20); WRITE_NFLASH(bank);

	  waiting = 1;

      while (waiting)
         {
         /* block write */
         ADDRESS_LOW(0x00); ADDRESS_HIGH(0x40); WRITE_NFLASH(0xe8);

         /* Set read block status register */
         ADDRESS_LOW(0x02); ADDRESS_HIGH(0x40);

         /* wait until queue is available */
         if (READ(RAM) & 0x80)
            waiting = 0;

         };

      bank = (int)((Addr & 0x3fffff) >> 14);
      ADDRESS_LOW(0x00); ADDRESS_HIGH(0x20); WRITE_NFLASH(bank);

      /* write 256 bytes */
      ADDRESS_LOW(0x00); ADDRESS_HIGH(0x40); WRITE_NFLASH(0xff);

      i = 0;
      j = 0;
      while (i<32)
         {
         AdrReg = (int)(Addr & 0x3fff) + 0x4000;

         ADDRESS_LOW(AdrReg & 0x00ff);
         ADDRESS_HIGH(AdrReg >> 8);

         if (!feof(fp))
            {
            c= fgetc(fp);
            j++;
            }
         else
            c = 0xff;
         WRITE_NFLASH(c);
         Addr++;
         i++;
         }

      if (i>0)
         {
         bank = (int)(( (Addr & 0x3fffff) >> 17)*8);
         ADDRESS_LOW(0x00); ADDRESS_HIGH(0x20); WRITE_NFLASH(bank);

         /* Write page buffer to flash */
         ADDRESS_LOW(0x00); ADDRESS_HIGH(0x40); WRITE_NFLASH(0xd0);

         /* Set read block status register */
         ADDRESS_LOW(0x02); ADDRESS_HIGH(0x40);

         /* wait until block is ready */
         while (READ(RAM) & 0x80)
            {
            };

         /* Set clear status registers */
         ADDRESS_LOW(0x00); ADDRESS_HIGH(0x40); WRITE_NFLASH(0x50);

         /* Set read memory mode */
         ADDRESS_LOW(0x00); ADDRESS_HIGH(0x40); WRITE_NFLASH(0xff);

        }
      }
}


// read write homemade 28Fxxx card with MBC5
void ReadWriteBlocks3 (FILE *fp, int Size)
{
   int j = 32;
   int AdrReg;
   int c,i;
   int bank =0;

   LONG Addr = 0; //address counter
   int waiting;
   int EstimatedBanks;

   EstimatedBanks = (ChipSize(Size)/131072);

   printf("Programming homemade 28F640J5/28F320J5 flash cartridge...\n");

   /* clear lock bits */
   ADDRESS_LOW(0x00); ADDRESS_HIGH(0x40); WRITE_FLASH(0x60);
   ADDRESS_LOW(0x00); ADDRESS_HIGH(0x40); WRITE_FLASH(0xd0);

   while ( (j == 32) &&
           (Addr < (ChipSize(Size)/8)) )
       {

      //calculate the bank (128kbit) of the MBC5
      // the MBC5 moves every bank to addr 0x4000-0x7fff = 16kByte
      bank = (int)((Addr & 0x3fffff) >> 14);

      //set MBC5 ROMB0 bit for banks 0-255
      WRITE_BKSW(bank,0x2100);
	  //set MBC5 ROMB1 bit for banks more as 255
	  if (bank > 255)
	    WRITE_BKSW(0x01,0x3100);
	  else
        WRITE_BKSW(0x00,0x3100);

	 waiting = 1;

     while (waiting)
         {

         // give the write buffer command
	     ADDRESS_LOW(0x00); ADDRESS_HIGH(0x40); WRITE_FLASH(0xe8);

		/* wait until buffer is available */
        if (READ(RAM) & 0x80) {
		    //printf ("StatusReg=%hx\n",READ(RAM));
            waiting = 0; }
         };

      /* write the max. count of 32 bytes to the flash */
	  /* use switched block area 0x4000-0x7fff*/
      ADDRESS_LOW(0x00); ADDRESS_HIGH(0x40); WRITE_FLASH(0x1f);

      i = 0; //buffer byte counter
      j = 0; // file end indicator
      while (i<32)
         {
         AdrReg = (int)(Addr & 0x3fff) + 0x4000;

         ADDRESS_LOW(AdrReg & 0x00ff);
         ADDRESS_HIGH(AdrReg >> 8);

      if ((Addr & 0x3fff) == 0 || Addr == 0) {
         //   printf(".");
         // do some arithmetics to calm the user
         printf("\rMBC bank = %d of %d",bank,(EstimatedBanks-1));
		}

         if (!feof(fp))
            {
            // as long as there are data from the file, take these
			c= fgetc(fp);
            j++;
            }
         else
            c = 0xff;

		 WRITE_FLASH(c);
         Addr++;
         i++;
         }

      if (i>0)
         {

     /* Write page buffer to flash */
     ADDRESS_LOW(0x00); ADDRESS_HIGH(0x40); WRITE_FLASH(0xd0);

	 waiting = 1;

     while (waiting)
         {

         ADDRESS_LOW(0x00); ADDRESS_HIGH(0x40);
         /* wait until buffer is available */
         if (READ(RAM) & 0x80) {
			//printf ("StatusReg=%hx\n",READ(RAM));
            waiting = 0; }

         };

         /* Set clear status registers */
         ADDRESS_LOW(0x00); ADDRESS_HIGH(0x40); WRITE_FLASH(0x50);

         /* Set read memory mode */
         ADDRESS_LOW(0x00); ADDRESS_HIGH(0x40); WRITE_FLASH(0xff);

        }
      }
}


int data_polling(void)
{
   unsigned char loop,predata,currdata;
   unsigned long timeout=0;

   loop = 1;
   predata = READ(ROM) & 0x40;
   while ((timeout<0x07ffffff) && (loop))
      {
      currdata = READ(ROM) & 0x40;
      if (predata == currdata)
	    loop = 0;			// ready to exit the while loop
        predata = currdata;
        timeout++;
      }
   return(loop);
}

int data_polling_data(unsigned char last_data)
{
   unsigned char loop;
   unsigned long timeout=0;
   loop = 1;
   while ((timeout<0x07ffffff) && (loop))
      {
      if (((READ(ROM) ^ last_data) & 0x80)==0)        // end wait
         loop = 0;                      // ready to exit the while loop
      timeout++;
      }
   return(loop);
}

/* Pauses for a specified number of microseconds. */
void sleep_pause( clock_t wait )
{
   clock_t goal;
   goal = wait + clock();
   while( goal > clock() );
}

void wait_CIR(void)
{
	 WRITE_BKSW(0x02,0x2000);
     ADDRESS_LOW(0xAA); ADDRESS_HIGH(0x6A); WRITE_FLASH(0xAA);
	 WRITE_BKSW(0x01,0x2000);
     ADDRESS_LOW(0x54); ADDRESS_HIGH(0x55); WRITE_FLASH(0x55);
	 WRITE_BKSW(0x02,0x2000);
     ADDRESS_LOW(0xAA); ADDRESS_HIGH(0x6A); WRITE_FLASH(0x70);

	 while ((READ(ROM) & 0x80) == 0x00) { sleep_pause( (clock_t)CLOCKS_PER_SEC/32); /* wait */ };

	 switch(READ(ROM) & 0xF0) {
			case 0x80:
					// printf("done\n");
					break;
			case 0x90:
					printf("program fail\n");
					ErrorExit();
					break;
			case 0xA0:
					printf(", but failed !\n");
					ErrorExit();
					break;
			default:
				    printf("unknown error code \n");
				    ErrorExit();
				    break;
	 }
	 WRITE_BKSW(0x02,0x2000);
     ADDRESS_LOW(0xAA); ADDRESS_HIGH(0x6A); WRITE_FLASH(0xAA);
	 WRITE_BKSW(0x01,0x2000);
     ADDRESS_LOW(0x54); ADDRESS_HIGH(0x55); WRITE_FLASH(0x55);
	 WRITE_BKSW(0x02,0x2000);
     ADDRESS_LOW(0xAA); ADDRESS_HIGH(0x6A); WRITE_FLASH(0xF0);
}

void clear_CIR(void)
{
	 WRITE_BKSW(0x02,0x2000);
     ADDRESS_LOW(0xAA); ADDRESS_HIGH(0x6A); WRITE_FLASH(0xAA);
	 WRITE_BKSW(0x01,0x2000);
     ADDRESS_LOW(0x54); ADDRESS_HIGH(0x55); WRITE_FLASH(0x55);
	 WRITE_BKSW(0x02,0x2000);
     ADDRESS_LOW(0xAA); ADDRESS_HIGH(0x6A); WRITE_FLASH(0x50);
}

/* Begin chip programming */

void ProgramChip(FILE * fp)
{
   int bank, page, byte, nbbank, nloops, newreg;
   BYTE Check, c, Buffer[256];
   int FlashSize, error, floop;

   PrintChipSize();
   FlashSize = Identify_Flashrom();

   if (FlashSize == 0x400B) {  /* routine for Nintendo Type 1 (LH28F032) flash cartridges */
     FlashSize = FlashSize - 0x4000;
     if (Size > FlashSize) { printf("Flash ROM size too small !\n\n"); ErrorExit(); }
     nbbank = (int)(ChipSize(Size)/8/65536);
     if ((LONG)(nbbank*8*65536) < ChipSize(Size)) nbbank++;
     printf("Erasing of %d banks with 64K each...\n",nbbank);
     if (!EraseNFlash(nbbank)) ErrorExit();
     ReadWriteBlocks (fp);
     printf("\nfinished !\n%lu bytes programmed !\n", (ChipSize(Size)/8));
     return;
   }

   if (FlashSize == 0x410B) {  /* routine Nintendo cardridges*/
     FlashSize = FlashSize - 0x4100;
     if (Size > FlashSize) { printf("Flash ROM size too small !\n\n"); ErrorExit(); }
     nbbank = (int)(ChipSize(Size)/8/131072);
     if ((LONG)(nbbank*8*131072) < ChipSize(Size)) nbbank++;
     printf("Erasing of %d banks with 128K each...\n",nbbank);
     if (!EraseNFlash2(nbbank)) ErrorExit();
     ReadWriteBlocks2 (fp);
     printf("\nfinished !\n%lu bytes programmed !\n", (ChipSize(Size)/8));
     return;
   }

//------------------------------------------------------------------------
   if (FlashSize == 0x510B) {  /* routine MBC5 (DA28F320J5) Intel Flash*/
     FlashSize = FlashSize - 0x5100;
        //printf("FlashSize: %d\n",FlashSize);
	   //Size is the Game size from the Header !
     if (Size > FlashSize) { printf("Flash ROM size too small !\n\n"); ErrorExit(); }
	   //printf("Chipsize: %d\n",(ChipSize(Size)/8/131072));
	   //PrintChipSize;
     nbbank = (int)(ChipSize(Size)/8/131072);
     if ((LONG)(nbbank*8*131072) < ChipSize(Size)) nbbank++;
     printf("Erasing of %d banks with 128K each...\n",nbbank);
       //printf("Nbbanks: %d\n",nbbank);
     if (!EraseNFlash3 (nbbank)) ErrorExit();
       //printf ("now writing data to flash\n");
     ReadWriteBlocks3 (fp, Size);
     printf("\nfinished !\n%lu bytes programmed !\n", (ChipSize(Size)/8));
     return;
   }

   if (FlashSize == 0x510C) {  /* routine MBC5 (DA28F640J5) Intel Flash*/
     FlashSize = FlashSize - 0x5100;
	   //printf("FlashSize: %d\n",FlashSize);
	   //Size is the Game size from the Header !
     if (Size > FlashSize) { printf("Flash ROM size too small !\n\n"); ErrorExit(); }
	   //printf("Chipsize: %d\n",(ChipSize(Size)/8/131072));
	   //PrintChipSize;
     nbbank = (int)(ChipSize(Size)/8/131072);
     if ((LONG)(nbbank*8*131072) < ChipSize(Size)) nbbank++;
     printf("Erasing of %d banks with 128K each...\n",nbbank);
       //printf("Nbbanks: %d\n",nbbank);
     if (!EraseNFlash3 (nbbank)) ErrorExit();
       //printf ("now writing data to flash\n");
     ReadWriteBlocks3 (fp, Size);
     printf("\nfinished !\n%lu bytes programmed !\n", (ChipSize(Size)/8));
     return;
   }

//--------------------------------------------------------------------------

   if (FlashSize == 0x5008) {   /* routine for 4MBit Bung cart, bankswitching doesn't work !!!! */
     FlashSize = FlashSize - 0x5000;
     if (Size > FlashSize) { printf("Flash ROM size too small !\n\n"); ErrorExit(); }

 	 newcart = 1;

     WRITE_BKSW(0x01,0x2100);
     ADDRESS_LOW(0x55); ADDRESS_HIGH(0x55); WRITE_FLASH(0xAA);
     ADDRESS_LOW(0xAA); ADDRESS_HIGH(0x2A); WRITE_FLASH(0x55);
     ADDRESS_LOW(0x55); ADDRESS_HIGH(0x55); WRITE_FLASH(0x80);
     ADDRESS_LOW(0x55); ADDRESS_HIGH(0x55); WRITE_FLASH(0xAA);
     ADDRESS_LOW(0xAA); ADDRESS_HIGH(0x2A); WRITE_FLASH(0x55);
     ADDRESS_LOW(0x55); ADDRESS_HIGH(0x55); WRITE_FLASH(0x10);
 	 sleep_pause( (clock_t)CLOCKS_PER_SEC);

   	 if (data_polling()) { printf("\n4MBit Bung cart erase error !\n");  ErrorExit(); }
     else printf("\n4MBit Bung cart erased !\n");

     WRITE_BKSW(0x01,0x6000);

     /* One bank is 16k bytes */
     nbbank = (int)(ChipSize(Size)/8/16384);

     for(bank = 0; bank < nbbank; bank++) {

       printf("\nWrite bank    \b\b\b%3d ", bank);
//       WRITE_BKSW(bank,0x2100);
	   for(page = (bank ? 0x40 : 0); page < (bank ? 0x80 : 0x40); page++) {

	     for (byte=0;byte<256;byte++){ Buffer[byte] = fgetc(fp); }
         do {
         	error = 0;
		 	printf(".");

         	WRITE_BKSW(0x01,0x2100);
   	 	 	ADDRESS_LOW(0x55); ADDRESS_HIGH(0x55); WRITE_FLASH(0xAA);
  	 	 	ADDRESS_LOW(0xAA); ADDRESS_HIGH(0x2A); WRITE_FLASH(0x55);
   	 	 	ADDRESS_LOW(0x55); ADDRESS_HIGH(0x55); WRITE_FLASH(0xA0);

       		WRITE_BKSW(bank,0x2100);
		 	ADDRESS_HIGH(page);

         	for (byte = 0x00; byte <= 0xFF; byte++) {
		   		ADDRESS_LOW(byte);
		   		WRITE_FLASH(Buffer[byte]);
 	  	 	}

// 	        if (data_polling_data(Buffer[0xFF])) { printf("\nWrite error !");  ErrorExit(); }
// 	        sleep_pause( (clock_t)CLOCKS_PER_SEC/16);

			byte = 0;
			do {
			  byte++;
			}
			while ((byte < 0x200) && (READ(ROM) != Buffer[0xFF]));

            printf("\nbank %hx  page %hx  write %hx  read %hx count %hx\n",bank,page,Buffer[0xFF],READ(ROM),byte);
// getch();
    	    byte = 0;
			do {
		   		ADDRESS_LOW(byte);
           		if (READ(ROM) != Buffer[byte]) {
           		  error = 1;
           		  printf("\b\bx");
//         	      WRITE_BKSW(0x01,0x2100);
// 	 	 	      ADDRESS_LOW(0x55); ADDRESS_HIGH(0x55); WRITE_FLASH(0xAA);
//	 	 		  ADDRESS_LOW(0xAA); ADDRESS_HIGH(0x2A); WRITE_FLASH(0x55);
// 	 	 		  ADDRESS_LOW(0x55); ADDRESS_HIGH(0x55); WRITE_FLASH(0xF0);
//   		 	      sleep_pause( (clock_t)CLOCKS_PER_SEC/16);
           		}
           		byte++;
           	}
			while ((byte < 0x100) && (error == 0));
         }
         while (error == 1);
       }
     }
     printf("\nwriting banks finished !\n%lu bytes programmed !\n", (ChipSize(Size)/8));
	 return;
   }

   if (FlashSize == 0x500A) {   /* routine for 16MBit Bung cart, bankswitching doesn't work !!!! */
     FlashSize = FlashSize - 0x5000;
     if (Size > FlashSize) { printf("Flash ROM size too small !\n\n"); ErrorExit(); }

     newcart = 1;
	 select_mbc_defaults();

     clear_CIR();
	 WRITE_BKSW(0x02,0x2000);
     ADDRESS_LOW(0xAA); ADDRESS_HIGH(0x6A); WRITE_FLASH(0xAA);
	 WRITE_BKSW(0x01,0x2000);
     ADDRESS_LOW(0x54); ADDRESS_HIGH(0x55); WRITE_FLASH(0x55);
	 WRITE_BKSW(0x02,0x2000);
     ADDRESS_LOW(0xAA); ADDRESS_HIGH(0x6A); WRITE_FLASH(0x80);
	 WRITE_BKSW(0x02,0x2000);
     ADDRESS_LOW(0xAA); ADDRESS_HIGH(0x6A); WRITE_FLASH(0xAA);
	 WRITE_BKSW(0x01,0x2000);
     ADDRESS_LOW(0x54); ADDRESS_HIGH(0x55); WRITE_FLASH(0x55);
	 WRITE_BKSW(0x02,0x2000);
     ADDRESS_LOW(0xAA); ADDRESS_HIGH(0x6A); WRITE_FLASH(0x10);

	 printf("\nErase 16MBit Bung cart: done");
 	 wait_CIR();

     /* One bank is 16k bytes */
     nbbank = (int)(ChipSize(Size)/8/16384);

     for(bank = 0; bank < nbbank; bank++) {

       printf("\nWrite bank    \b\b\b%3d", bank);
	   for(page = (bank ? 0x40 : 0); page < (bank ? 0x80 : 0x40); page++) {

	    for (byte=0;byte<256;byte++){ Buffer[byte] = fgetc(fp); }

	 	printf(".");
 		for (floop=0;floop<2;floop++) {
		  clear_CIR();

	 	  WRITE_BKSW(0x02,0x2000);
     	  ADDRESS_LOW(0xAA); ADDRESS_HIGH(0x6A); WRITE_FLASH(0xAA);
	 	  WRITE_BKSW(0x01,0x2000);
     	  ADDRESS_LOW(0x54); ADDRESS_HIGH(0x55); WRITE_FLASH(0x55);
	 	  WRITE_BKSW(0x02,0x2000);
     	  ADDRESS_LOW(0xAA); ADDRESS_HIGH(0x6A); WRITE_FLASH(0xA0);

          WRITE_BKSW(bank,0x2000);
		  ADDRESS_HIGH(page);

          for (byte = 0x00; byte < 0x80; byte++) {
		    ADDRESS_LOW(byte+(128*floop));
		   	WRITE_FLASH(Buffer[byte+(128*floop)]);
 	  	  }
 	      wait_CIR();
        }
       }
     }
     printf("\nwriting banks finished !\n%lu bytes programmed !\n", (ChipSize(Size)/8));
	 return;
   }

   /* routine for all other carts */

   if (Size > FlashSize) { printf("Flash ROM size too small !\n\n"); ErrorExit(); }
   if (Cart_Bank == 0) {
     if (!EraseAm29Fxxx())   /* Chip erase */
          ErrorExit(); }
   else if (FlashSize < 10) { printf("Flash ROM size too small for \nmulticartridge programming !\n\n");
   			ErrorExit(); }

   /* One bank is 16k bytes */
   nbbank = (int)(ChipSize(Size)/8/16384);
   if (newcart == 0) {	   			/* MBC1 carts */
	 select_mbc_bank(ROM16_RAM8,Cart_Bank);
     WRITE_BKSW(ROM4_RAM32,0x6000);	/* protect register */
	 if(nbbank > 32) { nloops = (nbbank/32); nbbank = 32; } else nloops = 1; }
   else nloops = 1;        			/* MBC3/MBC5 carts */

   for (newreg = 0; newreg < nloops; newreg++) {
	  if(nloops > 1) { select_mbc_bank(ROM16_RAM8,newreg);
         printf("\nWriting MBC bank %d\n", newreg);
  	     WRITE_BKSW(ROM4_RAM32,0x6000);	/* protect register */ }

      printf("\nWriting bank    ");
	  for(bank = 0; bank < nbbank; bank++) {
        printf("\b\b\b%3d", bank);
		if(bank) { WRITE_BKSW(bank,0x2100); }

		for(page = (bank ? 0x40 : 0); page < (bank ? 0x80 : 0x40); page++) {
         for(byte = 0x00; byte <= 0xFF; byte++) {

		   c = fgetc(fp);
   		   if (Algorithm == NEW_ALGOR)
      		{
      		WRITE_BKSW(0xAA,0x555);
      		WRITE_BKSW(0x55,0x2AA);
      		WRITE_BKSW(0xA0,0x555);
     		WRITE_BKSW(c,page*256+byte);
      		WRITE_BKSW(bank,0x2100);
			if (newcart == 0) { /* MBC1 ? */
     		  if (Cart_Bank == 0) select_mbc_bank(ROM16_RAM8,newreg);
     		  else select_mbc_bank(ROM16_RAM8,Cart_Bank);
     		  WRITE_BKSW(ROM4_RAM32,0x6000);	/* protect register */
 			}
   			ADDRESS_HIGH(page);
		    ADDRESS_LOW(byte);
      		}
           else			/* old modules up to 4MBit with AudioIN line in use */
           	{
    		WRITE_BKSW(0x01,0x2100);
       		ADDRESS_LOW(0x55); ADDRESS_HIGH(0x55); WRITE_FLASH(0xAA);
      		ADDRESS_LOW(0xAA); ADDRESS_HIGH(0x2A); WRITE_FLASH(0x55);
      		ADDRESS_LOW(0x55); ADDRESS_HIGH(0x55); WRITE_FLASH(0xA0);
    		WRITE_BKSW(bank,0x2100);
  			ADDRESS_HIGH(page);
		    ADDRESS_LOW(byte);
   	  		WRITE_FLASH(c);
       		}

   		   /* Wait until chip done programming */
   		   Check = READ(ROM);

           /* Loop until chip done programming */
           while ((Check != c) && ((Check & 0x20)==0)) Check = READ(ROM);

           if (Check != c) {
      	     Check = READ(ROM);
             if (Check != c) {
      	       printf("\b\b\bfailed !!!!\n");
               if (newcart == 0) {  /* MBC1 ? */
                if (Cart_Bank == 0) printf("MBC bank   : %d\n",newreg);
                else printf("MBC bank   : %d\n",Cart_Bank);
               }
               printf("bank       : %hx\n",bank);
               printf("address    : %hx\n",page*256+byte);
               printf("write byte : %hx\n",c);
               printf("read byte  : %hx\n",Check);
      	       ErrorExit();
             }
           }
         }
        }
      }
   }
   printf("\b\b\b\bs finished !\n%lu bytes programmed !\n", (ChipSize(Size)/8));
}

void multimodul(FILE * fp)
{
	int bank, page, byte, nbbank;
	unsigned char val;
    BYTE i, Block;

    for (i=0;i<8;i++) {
		if (i>3) { Block = ( i & 0x03 ) + 0x08; } /* select 2.ROM */
		else     { Block = i; }

		WRITE_BKSW(Block,0x2081);
		printf("Block %d\n",i);

		/* One bank is 16k bytes */
		nbbank = 16;

		for(bank = 0; bank < nbbank; bank++) {
			printf("Reading bank %d\n", bank);
			WRITE_BKSW(bank,0x2080);

			for(page = (bank ? 0x40 : 0); page < (bank ? 0x80 : 0x40); page++) {
				printf(".");
				ADDRESS_HIGH(page);

				for(byte = 0; byte <= 0xFF; byte++) {
					ADDRESS_LOW(byte);
    				val = READ(ROM);
                	fputc(val, fp);
				}
			}
  			printf("\n");
    	}
    }
}

void main(int argc, char **argv)
{
	int arg, fh;
	BYTE Base,Overwrite;
	FILE *fp;
 	char fname[20] = {"startup.gb"};
	double Fsize = 0;
    enum BOOLEAN lock = false;

	printf("\n");
	if(argc < 2) {
		usage();
		ErrorExit();
	}

    Algorithm = AMD29FXXX;  /* default to Am29Fxxx programming algorithm */
	Size = 8;
    Cart_Bank = 0;
    newcart = 0;			/* no MBC3/5 cart programming */

    wait_delay = 2000;
	init_port(1);

    /* Select default MBC settings */
    select_mbc_defaults();
    sleep_pause( (clock_t)CLOCKS_PER_SEC);

	for(arg = 1; arg < argc; arg++) {
		if(argv[arg][0] != '-') {
			usage();
			ErrorExit();
		}
		switch(toupper(argv[arg][1])) {
			case 'C':
                    /* add extension .GB if none present */
         			strcpy(fname, argv[++arg]);
         			if (strchr(fname, '.') == NULL)
            		strcat(fname, ".gb");

					if((fp = fopen(fname, "wb")) == NULL) {
						printf("Error opening file %s\n", fname);
						ErrorExit();
					}
					multimodul(fp);
					fclose(fp);
					break;
            case 'D':
					if (argv[++arg] == NULL) Base = 1;
					else Base = (BYTE)(atoi(argv[arg]));
					printf("Base address : %hx\n",Base*256);
            		dump(Base);
                    break;
    		case 'N':
    				newcart = 1;
            case 'P':
    				wait_delay = wait_delay/20;
                    /* add extension .GB if none present */
         			strcpy(fname, argv[++arg]);
         			if (strchr(fname, '.') == NULL)
            		strcat(fname, ".gb");
              		if ((fh = _open( fname, _O_RDONLY))  == -1 ) {
                       	printf("Error opening file %s\n", fname);
                       	ErrorExit();
                    }
   				    if (_filelength( fh ) <= 2048L) {
   				       Size = 0; }
   				    else {
   					   Fsize = (log(_filelength( fh )/2048)/log(2));
   					   printf("File size     : %.0f\n",Fsize);
   					   Size = (unsigned char)Fsize;
   					   _close( fh ); }
				  	if ((fp = fopen(fname, "rb")) == NULL) {
                       	printf("Error opening file %s\n", fname);
                       	ErrorExit();
                    }
					read_header(header,File,fp);
					printf("\n");
                    ProgramChip(fp);
                    fclose(fp);
                    break;
            case 'M':
    				wait_delay = wait_delay/20;
                    do {
                   		if ((fh = _open( fname, _O_RDONLY))  == -1 ) {
                       		printf("Error opening file %s\n", fname);
                       		ErrorExit();
                    	}
   				    	if (_filelength( fh ) <= 2048L) { Size = 0; }
   				    	else {
   					   		Fsize = (log(_filelength( fh )/2048)/log(2));
   					   		printf("File size     : %.0f\n",Fsize);
   					   		Size = (unsigned char)Fsize;
   					   		_close( fh ); }
				  		if ((fp = fopen(fname, "rb")) == NULL) {
                       		printf("Error opening file %s\n", fname);
                       		ErrorExit();
                    	}
   						read_header(header,File,fp);
						printf("\n");
                    	ProgramChip(fp);
                    	fclose(fp);
						printf("\n\n");
   						/* add extension .GB if none present */
         				strcpy(fname, argv[++arg]);
         				if (strchr(fname, '.') == NULL)
            			strcat(fname, ".gb");
            			Cart_Bank++; }
            		while ((strlen(fname) > 3) & (Cart_Bank < 4));
                    break;
			case 'V':
	           		/* add extension .GB if none present */
         			strcpy(fname, argv[++arg]);
         			if (strchr(fname, '.') == NULL)
            		strcat(fname, ".gb");
             		if ((fh = _open( fname, _O_RDONLY))  == -1 ) {
                       	printf("Error opening file %s\n", fname);
                       	ErrorExit();
                    }
   				    if (_filelength( fh ) <= 2048L) {
   				       Size = 0; }
   				    else {
   				       Fsize = (log(_filelength( fh )/2048)/log(2));
   					   printf("File size     : %.0f\n",Fsize);
   					   Size = (unsigned char)Fsize;
   					   _close( fh ); }
                    if ((fp = fopen(fname, "rb")) == NULL) {
                   		printf("Error opening file %s\n", fname);
                       	ErrorExit();
                    }
					read_header(header,File,fp);
                    printf("\n");
					verify_data(fp, mbc[header[0x47]], rom[header[0x48]]);
                    fclose(fp);
                    break;
			case 'L':
					init_port(atoi(argv[++arg]));
			    	break;
			case 'W':
					wait_delay = atoi(argv[++arg]);
					break;
			case 'E':
					if (argv[++arg] == NULL) Base = 1;
                    else Base = (BYTE)(atoi(argv[arg]));
					switch (Base) {
					  case 0:  Algorithm = AMD29FXXX;
 		   					   printf("4MBit Module\n");
 		   					   if (!EraseAm29Fxxx()) ErrorExit();
	                    	   break;
					  case 1:  Algorithm = NEW_ALGOR;
 							   printf("16/32MBit Module\n");
 					  		   if (!EraseAm29Fxxx()) ErrorExit();
	                    	   break;
	              	  case 2:  printf("32MBit Module\n");
     						   newcart = 1;
            				   WRITE_BKSW(0xff,0x4000);            /* Set flash read mode */
            				   WRITE_BKSW(0x00,0x3000);            /* Set bank ms byte */
            				   WRITE_BKSW(0x00,0x2000);            /* Set bank ls byte */
            				   WRITE_BKSW(0x90,0x4000);            /* Set read manufacturer mode */
                               SetAddr(0x00);
            			       if (ReadByte() == 0x89)
            			       {
            			         if (!EraseNFlash2(256)) ErrorExit();
            			       }
            			       else
            			       {
            			         if (!EraseNFlash(512)) ErrorExit();
            			       }
					  case 3:  printf("32/64MBit MBC5/Intel Falsh Module\n");
     	 		    		   //wait_delay = wait_delay/200;
							   WRITE_BKSW(0x00,0x3000);
	  			               WRITE_BKSW(0x02,0x2000);
      			               ADDRESS_LOW(0xAA);
							   ADDRESS_HIGH(0x6A);
							   WRITE_FLASH(0x90);

            	               SetAddr(0x00);

						       if (ReadByte() == 0x89)
            			       {
            			         if (BungHW == 1)
								 {
						           //printf("bung =1");
							 	   ADDRESS_LOW(0x02);
 							       if (ReadByte() == 0x14){
                                     printf("Erase 64x64kB blocks\n");
								     if (!EraseNFlash3(32)) ErrorExit();}
								   else if (ReadByte() == 0x15){
								  	 printf("Erase 64x128kB blocks\n");
								     if (!EraseNFlash3(64)) ErrorExit();}
								   else
									 printf("Device not supproted!");
            			         }
							   }
            			       else
            			         printf("Manufacturer not supproted!");
            			       break;
							}
					break;
			case 'T':
					test();
					break;
			case 'A':
                   /* add extension .GB if none present */
         			strcpy(fname, argv[++arg]);
         			if (strchr(fname, '.') == NULL)
            		strcat(fname, ".gb");
 					if (strlen(fname) == 3) {
						read_header(header,ROM,NULL);
						break;
					}
               		if ((fh = _open( fname, _O_RDONLY))  == -1 ) {
                       	printf("Error opening file %s\n", fname);
                       	ErrorExit();
                    }
   				    if (_filelength( fh ) <= 2048L) {
   				       Size = 0; }
   				    else {
   					   Fsize = (log(_filelength( fh )/2048)/log(2));
   					   printf("File size     : %.0f\n",Fsize);
   					   Size = (unsigned char)Fsize;
   					   _close( fh ); }
				  	if ((fp = fopen(fname, "rb")) == NULL) {
                       	printf("Error opening file %s\n", fname);
                       	ErrorExit();
                    }
					read_header(header,File,fp);
                    fclose(fp);
					break;
			case 'S':
                    /* add extension .GB if none present */
         			strcpy(fname, argv[++arg]);
         			if (strchr(fname, '.') == NULL)
            		strcat(fname, ".gb");

					if((fp = fopen(fname, "wb")) == NULL) {
						printf("Error opening file %s\n", fname);
						ErrorExit();
					}
					read_header(header,ROM,NULL);
					read_data(fp, mbc[header[0x47]], rom[header[0x48]]);
					fclose(fp);
					break;
			case 'O':
					if (argv[++arg] == NULL) Overwrite = 0x00;
					else Overwrite = (BYTE)(atoi(argv[arg]));
			        read_header(header,ROM,NULL);
					clear_sram(sram[header[0x47]], ram[header[0x49]], Overwrite);
					break;
			case 'B':
                    /* add extension .SAV if none present */
         			strcpy(fname, argv[++arg]);
         			if (strchr(fname, '.') == NULL)
            		strcat(fname, ".sav");

					if((fp = fopen(fname, "wb")) == NULL) {
            			printf("Error opening file %s\n", fname);
						ErrorExit();
					}
					read_header(header,ROM,NULL);
					read_sram(fp, sram[header[0x47]], ram[header[0x49]]);
					fclose(fp);
					break;
			case 'R':
                    /* add extension .SAV if none present */
         			strcpy(fname, argv[++arg]);
         			if (strchr(fname, '.') == NULL)
            		strcat(fname, ".sav");

       				if((fp = fopen(fname, "rb")) == NULL) {
						printf("Error opening file %s\n", fname);
						ErrorExit();
					}
   					read_header(header,ROM,NULL);
					write_sram(fp, sram[header[0x47]], ram[header[0x49]]);
					fclose(fp);
					break;
			case 'F':
                    /* add extension .SAV if none present */
         			strcpy(fname, argv[++arg]);
         			if (strchr(fname, '.') == NULL)
            		strcat(fname, ".sav");

       				if((fp = fopen(fname, "rb")) == NULL) {
						printf("Error opening file %s\n", fname);
						ErrorExit();
					}
   					read_header(header,ROM,NULL);
					verify_sram(fp, sram[header[0x47]], ram[header[0x49]]);
					fclose(fp);
					break;
			default:
					usage();
                    ErrorExit();
		}
	}
    LED_OFF;
	exit(0);
}


