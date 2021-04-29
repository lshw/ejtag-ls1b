//
// USB_JTAG.C
//
// Low level JTAG access code.  Handles all clocking of data in and
// out of target hardware.
//
// Copyright (c) 2002, Jason Riffel - TotalEmbedded LLC.
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without 
// modification, are permitted provided that the following conditions 
// are met:
//
// Redistributions of source code must retain the above copyright 
// notice, this list of conditions and the following disclaimer. 
//
// Redistributions in binary form must reproduce the above copyright
// notice, this list of conditions and the following disclaimer in 
// the documentation and/or other materials provided with the
// distribution. 
//
// Neither the name of TotalEmbedded nor the names of its 
// contributors may be used to endorse or promote products derived 
// from this software without specific prior written permission.
// 
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS 
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT 
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS 
// FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE 
// COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, 
// INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, 
// BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; 
// LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER 
// CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT 
// LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN 
// ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE 
// POSSIBILITY OF SUCH DAMAGE.
//
// Modifed by ZGJ on the 20090609
//
// INCLUDES
//
#include <errno.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/types.h>
#include "usb_jtag.h"
//#include "usb_ejtag_command.h"

//
// DEFINES
//



//
// PROTOTYPES
//

//
// fJTAG_Init
//
// Initializes the state of the JTAG hardware
//
extern int usb_can_bulk_wr_flag;
void fJTAG_Init(void)
{
	int found_usb = USB_NO_FOUND;
	found_usb = usb_tap_init();
	if(found_usb == USB_FOUND)
	{
		usb_fx2_reset();
	}
	else
	{
		fprintf(stderr,"usb device not found!\n");
		exit(-1);
	}
	
	if(usb_can_bulk_wr_flag == USB_CAN_BULK_WR_FLAG)
		tap_reset();
	else
	{
		fprintf(stderr,"usb ejtag simulator not BULK read/write!\n");
		exit(-1);
	}
}


//
// fJTAG_Data
//
// Write/Read from/to the JTAG data register
//
unsigned int fJTAG_Data (unsigned int ui_data_out)
{
  	unsigned int  ui_data_in;

	ui_data_in = usb_wr_dr(1,1,ui_data_out);
	
  	return(ui_data_in);
}

//
// fJTAG_Instruction
//
// Read/Write from/to the instruction register
//
unsigned char fJTAG_Instruction(unsigned char uc_data_out)
{
  	unsigned char uc_data_in=0;
#if 0
	uc_data_in = usb_wr_ir(1,1,uc_data_out);
#else
	uc_data_in = usb_wr_ir(0,0,uc_data_out);
#endif
  	return(uc_data_in);
}

//
// Write/Read from/to the JTAG Fastdata register
//
unsigned int fJTAG_FastData (unsigned int ui_data_out)
{
  	unsigned int  ui_data_in;

	ui_data_in = usb_wr_dr(1,1,ui_data_out);
	
  	return(ui_data_in);
}

//
// Write/Read from/to the JTAG DBTX register
//
unsigned int fJTAG_DBTX_test(unsigned int ui_data_out)
{
  	unsigned int  ui_data_in=0;

	ui_data_in = usb_wr_dr(1,1,ui_data_out);
	//fast_write2(fast_count,fast_write2_data_input);
	
  	return(ui_data_in);
}
//
// Write/Read from/to the JTAG DBTX register
//
unsigned int fJTAG_DBTX(unsigned int fast_count,unsigned int * fast_write2_data_input)
{
  	unsigned int  ui_data_in=0;

	// ui_data_in = usb_wr_dr(1,1,ui_data_out);
	//20090628-zgj fast_write2(fast_count,fast_write2_data_input);
	fast_write2(fast_count,fast_write2_data_input);
	//20090628-normal-lw-240KB/S  fast_write_3(fast_count,fast_write2_data_input);
	
  	return(ui_data_in);
}

//
// Read from the JTAG DBDX register
//
unsigned int fJTAG_DBDX(unsigned int ui_data_out)
{
  	unsigned int  ui_data_in;

	ui_data_in = usb_wr_dr(1,1,ui_data_out);
	
  	return(ui_data_in);
}

//
// Read from the JTAG DBDX register
//
unsigned int fJTAG_DBDX_TEST(unsigned int ui_data_out)
{
  	unsigned int  ui_data_in;

	ui_data_in = usb_wr_dr(1,1,ui_data_out);
	
  	return(ui_data_in);
}


//
// Write/Read from/to the JTAG DBTX register
//
unsigned int fJTAG_UPLOAD(unsigned char *buf,unsigned int upload_count)
{
  	unsigned int  ui_data_in=0;
#if 1
	unsigned int download_ram_size=0;
	unsigned int init_do=0;
    	time_t time_start;
    	time_t time_end;
    	time_t  time_deta=0;
	int i=0;

        if(!init_do)
        {
                time_start = (time(&time_start))&0xffffffff;
                fprintf(stderr,"time_start : %x \n",time_start);
                init_do = 1;
                download_ram_size=0;
        }
#endif
	// ui_data_in = usb_wr_dr(1,1,ui_data_out);
	//20090628-zgj fast_write2(fast_count,fast_write2_data_input);
//	for(i=0 ; i< 0x10000; i++ )
	{
	//	getchar();
	//	getchar();
		fprintf(stderr,"======================= %d\n",i);
		usb_wr_upload(buf,upload_count);
	}
	//20090628-normal-lw-240KB/S  fast_write_3(fast_count,fast_write2_data_input);

#if 1
         download_ram_size = upload_count;
                time_end = (time(&time_end))&0xffffffff;
                fprintf(stderr,"=<<<==time_end : %x \n",time_end);
                time_deta= time_end - time_start;
                if(time_deta == 0)
                        time_deta++ ;
                fprintf(stderr,"=<<<==time : %d, download_size : %x, download rate=%d B/S\n",time_deta,download_ram_size,((download_ram_size/time_deta)));
#endif
  	return(ui_data_in);
}

//
// Write/Read from/to the JTAG DBTX register
//
unsigned int fJTAG_FAST_UPLOAD(unsigned int upload_count)
{
  	unsigned int  ui_data_in=0;

	// ui_data_in = usb_wr_dr(1,1,ui_data_out);
	//20090628-zgj fast_write2(fast_count,fast_write2_data_input);
	// usb_wr_upload(upload_count);
	fast_read(upload_count);
	//20090628-normal-lw-240KB/S  fast_write_3(fast_count,fast_write2_data_input);
	
  	return(ui_data_in);
}

void fJTAG_unInit(void)
{
	usb_stop();
}
