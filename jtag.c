//
// JTAG.C
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

//
// INCLUDES
//
#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <netinet/in.h>
#include <netdb.h>
#include "jtag.h"
#include "parport.h"

//
// DEFINES
//
// Define the pins each signal is assigned to on the data port
#define dPARPORT_TAP_RESET_BIT     0x03
#define dPARPORT_TAP_DI_BIT        0x06
#define dPARPORT_TAP_MS_BIT        0x01
#define dPARPORT_TAP_CLK_BIT       0x00
#define dPARPORT_TARGET_RESET_BIT  0x04

// Define the bit in the status register that is our TAP_DI
#define dPARPORT_TAP_DO_BIT        0x80

// Macro to calculate the bit settings when writing to the data port
#define mPARPORT_TRS_BIT(x) (x << dPARPORT_TAP_RESET_BIT   )
#define mPARPORT_TMS_BIT(x) (x << dPARPORT_TAP_MS_BIT      )
#define mPARPORT_TDI_BIT(x) (x << dPARPORT_TAP_DI_BIT      )
#define mPARPORT_TCK_BIT(x) (x << dPARPORT_TAP_CLK_BIT     )
#define mPARPORT_RST_BIT(x) (x << dPARPORT_TARGET_RESET_BIT)

//
// PROTOTYPES
//
static unsigned char fJTAG_Wiggle(unsigned char uc_tms, unsigned char uc_tdo);

//
// fJTAG_Init
//
// Initializes the state of the JTAG hardware
//
void fJTAG_Init(void)
{
  // First we need to init the parallel port
  fPARPORT_Init();
  
  // Wiggle the TAP_RESET bit - Be mindful of the target reset bit
//  fPARPORT_Write((unsigned char)mPARPORT_RST_BIT(1)|mPARPORT_TRS_BIT(0));
//  fPARPORT_Write((unsigned char)mPARPORT_RST_BIT(1)|mPARPORT_TRS_BIT(1));

  // Run through a handful of clock cycles with TMS high to make sure
  // we are in the TEST-LOGIC-RESET state.
  fJTAG_Wiggle(1,0);
  fJTAG_Wiggle(1,0);
  fJTAG_Wiggle(1,0);
  fJTAG_Wiggle(1,0);
  fJTAG_Wiggle(1,0);
  fJTAG_Wiggle(1,0);
  
  // Enter the RUN-TEST-IDLE state
  fJTAG_Wiggle(0,0);


  fJTAG_Instruction(1);
  printf("IDCODE=0x%08x\n",fJTAG_Data(1));
  fJTAG_Instruction(3);
  printf("IMPCODE=0x%08x\n",fJTAG_Data(3));

  return;
}

//
// fJTAG_Wiggle
//
// Sets states of lines out to JTAG controller and returns
// the one input bit from status (which is inverted in hardware)
//
static unsigned char fJTAG_Wiggle(unsigned char uc_tms, unsigned char uc_tdo)
{
  unsigned char uc_dout;
  unsigned char uc_din;

  uc_dout = (unsigned char)(mPARPORT_TRS_BIT(1)|mPARPORT_TMS_BIT(uc_tms)|
                            mPARPORT_TDI_BIT(uc_tdo)|mPARPORT_TCK_BIT(0)|
                            mPARPORT_RST_BIT(1));
  fPARPORT_Write(uc_dout);

  uc_dout = (unsigned char)(mPARPORT_TRS_BIT(1)|mPARPORT_TMS_BIT(uc_tms)|
                            mPARPORT_TDI_BIT(uc_tdo)|mPARPORT_TCK_BIT(1)|
                            mPARPORT_RST_BIT(1));
  fPARPORT_Write(uc_dout);

  // Read in the status bit (inverted in hardware)
  uc_din = fPARPORT_Read() & dPARPORT_TAP_DO_BIT ? 0 : 1;

  return uc_din;
}

//
// fJTAG_Data
//
// Write/Read from/to the JTAG data register
//
unsigned int fJTAG_Data (unsigned int ui_data_out)
{
  unsigned int  ui_data_in;
  unsigned char uc_index;
  unsigned char uc_result;
  
  fJTAG_Wiggle(1, 0);  // enter select-dr-scan
  fJTAG_Wiggle(0, 0);  // enter capture-dr
  fJTAG_Wiggle(0, 0);  // enter shift-dr

  ui_data_in = 0x00;

  // Clock all but last bit out
  for (uc_index=0; uc_index<31; uc_index++)
    {
      uc_result   = fJTAG_Wiggle(0, (ui_data_out & 0x01));
      ui_data_out = ui_data_out >> 1;
      ui_data_in  = ui_data_in | (uc_result << uc_index);
    }
  // On last bit set the TAP_MS
  uc_result   = fJTAG_Wiggle(1, (ui_data_out & 0x01));
  ui_data_in  = ui_data_in | (uc_result << uc_index);

  fJTAG_Wiggle(1, 0);  // enter update-dr
  fJTAG_Wiggle(0, 0);  // enter runtest-idle

  return(ui_data_in);
}

//
// Write/Read from/to the JTAG Fastdata register
//
unsigned int fJTAG_FastData (unsigned int ui_data_out)
{
  unsigned int  ui_data_in;
  unsigned char uc_index;
  unsigned char uc_result;
  
  fJTAG_Wiggle(1, 0);  // enter select-dr-scan
  fJTAG_Wiggle(0, 0);  // enter capture-dr
  fJTAG_Wiggle(0, 0);  // enter shift-dr

  ui_data_in = 0x00;

      fJTAG_Wiggle(0, 0);  // write a 0 to the Fastdata register
  // Clock all but last bit out
  for (uc_index=0; uc_index<31; uc_index++)
    {
      uc_result   = fJTAG_Wiggle(0, (ui_data_out & 0x01));
      ui_data_out = ui_data_out >> 1;
      ui_data_in  = ui_data_in | (uc_result << uc_index);
    }
  // On last bit set the TAP_MS
  uc_result   = fJTAG_Wiggle(1, (ui_data_out & 0x01));
  ui_data_in  = ui_data_in | (uc_result << uc_index);

  fJTAG_Wiggle(1, 0);  // enter update-dr
  fJTAG_Wiggle(0, 0);  // enter runtest-idle

  return(ui_data_in);
}

//
// Write/Read from/to the JTAG DBTX register
//
unsigned int fJTAG_DBTX(unsigned int ui_data_out)
{
  unsigned int  ui_data_in;
  unsigned char uc_index;
  unsigned char uc_result;
  
  fJTAG_Wiggle(1, 0);  // enter select-dr-scan
  fJTAG_Wiggle(0, 0);  // enter capture-dr
  fJTAG_Wiggle(0, 0);  // enter shift-dr

  ui_data_in = 0x00;

      fJTAG_Wiggle(0, 0);  // write a 0 to the Fastdata register
  // Clock all but last bit out
  for (uc_index=0; uc_index<31; uc_index++)
    {
      uc_result   = fJTAG_Wiggle(0, (ui_data_out & 0x01));
      ui_data_out = ui_data_out >> 1;
      ui_data_in  = ui_data_in | (uc_result << uc_index);
    }
  // On last bit set the TAP_MS
  uc_result   = fJTAG_Wiggle(1, (ui_data_out & 0x01));
  ui_data_in  = ui_data_in | (uc_result << uc_index);

  fJTAG_Wiggle(1, 0);  // enter update-dr
  fJTAG_Wiggle(0, 0);  // enter runtest-idle

  return(ui_data_in);
}

#if 0
//
// Read from the JTAG DBDX register
//
unsigned int fJTAG_DBDX_TEST(unsigned int ui_data_out)
{
  unsigned int  ui_data_in;
  unsigned char uc_index;
  unsigned int uc_result=0x00;
  unsigned int uc_result1=0x00;
  
  fJTAG_Wiggle(1, 0);  // enter select-dr-scan
  fJTAG_Wiggle(0, 0);  // enter capture-dr
  fJTAG_Wiggle(0, 0);  // enter shift-dr

  ui_data_in = 0x00;

      uc_result1=fJTAG_Wiggle(0, 1);  // write a 1 to the JTAG DBDX register
  // Clock all but last bit out
  for (uc_index=0; uc_index<31; uc_index++)
    {
      uc_result   = fJTAG_Wiggle(0, (ui_data_out & 0x01));
      ui_data_out = ui_data_out >> 1;
      ui_data_in  = ui_data_in | (uc_result << uc_index);
    }
  // On last bit set the TAP_MS
  uc_result   = fJTAG_Wiggle(1, (ui_data_out & 0x01));
  ui_data_in  = ui_data_in | (uc_result << uc_index);

  fJTAG_Wiggle(1, 0);  // enter update-dr
  fJTAG_Wiggle(0, 0);  // enter runtest-idle
//  if(uc_result1)
//	printf("fJTAG_DBDX_TEST : 0x%x\n",uc_result1);
  return(uc_result1);
}
#endif

#if 1
//
// Read from the JTAG DBDX register
//
unsigned int fJTAG_DBDX_TEST(unsigned int ui_data_out)
{
  unsigned int  ui_data_in;
//  unsigned char uc_index;
  unsigned int uc_result=0x00;
  
  fJTAG_Wiggle(1, 0);  // enter select-dr-scan
  fJTAG_Wiggle(0, 0);  // enter capture-dr
  fJTAG_Wiggle(0, 0);  // enter shift-dr

//  ui_data_in = 0x00;

//      fJTAG_Wiggle(0, 0);  // write a 0 to the JTAG DBDX register
  // Clock all but last bit out
//  for (uc_index=0; uc_index<31; uc_index++)
//    {
//      uc_result   = fJTAG_Wiggle(0, (ui_data_out & 0x01));
//      ui_data_out = ui_data_out >> 1;
//      ui_data_in  = ui_data_in | (uc_result << uc_index);
//    }
  // On last bit set the TAP_MS
  uc_result   = fJTAG_Wiggle(1, (ui_data_out & 0x01));
//  ui_data_in  = ui_data_in | (uc_result << uc_index);

  fJTAG_Wiggle(1, 0);  // enter update-dr
  fJTAG_Wiggle(0, 0);  // enter runtest-idle
  if(uc_result)
	printf("fJTAG_DBDX_TEST : 0x%x\n",uc_result);
  return(uc_result);
}
#endif

//
// Read from the JTAG DBDX register
//
unsigned int fJTAG_DBDX(unsigned int ui_data_out)
{
  unsigned int  ui_data_in;
  unsigned char uc_index;
  unsigned char uc_result;
  
  fJTAG_Wiggle(1, 0);  // enter select-dr-scan
  fJTAG_Wiggle(0, 0);  // enter capture-dr
  fJTAG_Wiggle(0, 0);  // enter shift-dr

  ui_data_in = 0x00;

      fJTAG_Wiggle(0, 0);  // write a 0 to the JTAG DBDX register
  // Clock all but last bit out
  for (uc_index=0; uc_index<31; uc_index++)
    {
      uc_result   = fJTAG_Wiggle(0, (ui_data_out & 0x01));
      ui_data_out = ui_data_out >> 1;
      ui_data_in  = ui_data_in | (uc_result << uc_index);
    }
  // On last bit set the TAP_MS
  uc_result   = fJTAG_Wiggle(1, (ui_data_out & 0x01));
  ui_data_in  = ui_data_in | (uc_result << uc_index);

  fJTAG_Wiggle(1, 0);  // enter update-dr
  fJTAG_Wiggle(0, 0);  // enter runtest-idle
//zgj printf("fJTAG_DBDX===data : 0x%x\n",ui_data_in);
  return(ui_data_in);
}

//
// fJTAG_Instruction
//
// Read/Write from/to the instruction register
//
unsigned char fJTAG_Instruction(unsigned char uc_data_out)
{
  unsigned char uc_data_in;
  unsigned char uc_result;
  unsigned char uc_index;

  fJTAG_Wiggle(1, 0);  // enter select-dr-scan
  fJTAG_Wiggle(1, 0);  // enter select-ir-scan
  fJTAG_Wiggle(0, 0);  // enter capture-ir
  fJTAG_Wiggle(0, 0);  // enter shift-ir (dummy)

  uc_data_in=0;
  
  // Clock all but last bit out
  for (uc_index=0; uc_index<4; uc_index++)
    {
      uc_result   = fJTAG_Wiggle(0, (uc_data_out & 0x01));
      uc_data_out = uc_data_out >> 1;
      uc_data_in  = uc_data_in | (uc_result << uc_index);
    }
  // On last bit set the TAP_MS
  uc_result   = fJTAG_Wiggle(1, (uc_data_out & 0x01));
  uc_data_in  = uc_data_in | (uc_result << uc_index);
  
  fJTAG_Wiggle(1, 0);  // enter update-ir
  fJTAG_Wiggle(0, 0);  // enter runtest-idle

  return(uc_data_in);
}

unsigned int fJTAG_FAST_UPLOAD(unsigned int upload_count)
{
 printf("error no this method fJTAG_FAST_UPLOAD\n");
 exit(-1);
 return -1;
}

void fJTAG_unInit(void)
{
}


unsigned int fJTAG_UPLOAD(unsigned char *buf,unsigned int upload_count)
{
 printf("error no this method fJTAG_UPLOAD\n");
 exit(-1);
 return -1;
}

