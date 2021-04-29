//
// GDBRDP.C
//
// Handler for GDB remote debugger interface commands
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
#include <asm/ioctls.h>
#include "jtagsock.h"
#include "gdbrdp.h"
#include "mips32.h"
#include "readbyte.h"
#include "readhalf.h"
#include "readword.h"
#include "writebyte.h"
#include "writehalf.h"
#include "writeword.h"
#include "singlestepset.h"
#include <myconfig.h>
#include <cmdparser.h>
#define gdb_little_endian(x) (((x>>24)&0xff)|((x>>8)&0xff00)|((x&0xff00)<<8)|((x&0xff)<<24))
//
// TYPEDEFS
//
typedef struct _sw_breakpoint_
{
  unsigned int  ui_address;
  unsigned int  ui_original_memory;
  unsigned char uc_enabled;
} sw_breakpoint;

typedef struct _hw_breakpoint_
{
  unsigned int  ui_address;
  unsigned char uc_enabled;
} hw_breakpoint;

//
// STATIC LOCAL VARS
//
static int           si_inputhandlerstate;
static unsigned char suc_calculatedchecksum;
static unsigned char suc_receivedchecksum;
static unsigned int  sui_numbytesreceived;
static char          sac_commandbuffer[dGDBRDP_MAX_CMD_BUFFER];
static sw_breakpoint swb_array[dGDBRDP_MAX_SW_BREAKPOINTS];
static hw_breakpoint hwb_inst_array[dMIPS32_MAX_HARDWARE_BKPTS];
static hw_breakpoint hwb_data_array[dMIPS32_MAX_HARDWARE_BKPTS];

static unsigned int aui_registers[dMIPS32_NUM_DEBUG_REGISTERS];


static int config_gdbserver_compareasid = 0;
static int config_gdbserver_comparedata = 0;
static int config_gdbserver_comparedata_val = 0;
int config_gdbserver_quitgdb = 0;

myconfig_init("gdbserver.compareasid",config_gdbserver_compareasid,CONFIG_I,"gdbserver compare asid");
myconfig_init("gdbserver.compareadata",config_gdbserver_comparedata,CONFIG_I,"gdbserver compare data");
myconfig_init("gdbserver.compareadata.val",config_gdbserver_comparedata_val,CONFIG_I,"gdbserver compare data val");
myconfig_init("gdbserver.quitgdb",config_gdbserver_quitgdb,CONFIG_I,"quit gdb");

//
// fGDBRDP_Handler
//
// The real 'main'.  This function handles executing the individual
// handlers.  (JTAG and GDB communications)
//
int fGDBRDP_Handler(int i_socket)
{
  char   c_byte;
  int    i_result;
  char   ac_responsebuffer[0x03]; // Only signal ever sent in this buffer

  // Mark all breakpoints disabled - Just use i_result
  for (i_result=0; i_result<dGDBRDP_MAX_SW_BREAKPOINTS; i_result++)
  {
    swb_array[i_result].uc_enabled=0x00;
  }
  for (i_result=0; i_result<dMIPS32_MAX_HARDWARE_BKPTS; i_result++)
  {
    hwb_inst_array[i_result].uc_enabled=0x00;
    hwb_data_array[i_result].uc_enabled=0x00;
  }
  
  // Init the target specific code and all code below it
  //fMIPS32_Init();

  si_inputhandlerstate = dGDBRDP_INPUT_IDLE;

  // This routine does not return until the socket is closed
  for (;;)
  {
  int quit=0;
    i_result = fJTAGSOCK_ReadByte(i_socket, &c_byte);
    if (i_result > 0) 
    {
      if (-1 == fGDBRDP_InputHandler(i_socket, c_byte))
	  quit=1;
    }
    else
    {
     int n;
     char buf[100];

	ioctl(0,FIONREAD,&n);
	if(n)
	{
	read(0,buf,n);
	buf[n] = 0;
	do_cmd(buf);
	}
	if(config_gdbserver_quitgdb > 1) return 0;
    }

    // Run JTAG exception catcher task
    if (-1 == fMIPS32_EJTAG_Machine())
    {
      // -1 == Target entered debug mode - report to GDB a user break (0x12)
      sprintf(ac_responsebuffer, "S%02x", ui_last_signal);
      fGDBRDP_SendToGDB(i_socket, &ac_responsebuffer[0], 0x03);
    }

	if(quit)
    {
      // End of debug statement received - Kill this socket
      // and return back waiting on another GDB session.
      close(i_socket);
      printf("GDB DETACHED.\n");
      return(1);
    }
  }
}

//
// fGDBRDP_InputHandler
//
// This function accumulates inbound commands from GDB and launches
// the GDB command intepreter when a full command is received.
//
int fGDBRDP_InputHandler (int i_socket, char c_byte)
{
  // Input processing machine
  switch(si_inputhandlerstate)
  {
    case dGDBRDP_INPUT_IDLE:
      if (c_byte == dGDBRDP_START_OF_COMMAND) // Start of command
      {
        si_inputhandlerstate = dGDBRDP_RECEIVE_COMMAND;
        sui_numbytesreceived=0; 
        suc_calculatedchecksum=0;
        break;
      }
      else if (c_byte == dGDBRDP_CTRLC)
      {
        if (ui_ejtag_state == dMIPS32_EJTAG_WAIT_FOR_DEBUG_MODE)
        {
          // Make the catcher perform a debug break
          ui_ejtag_state = dMIPS32_EJTAG_FORCE_DEBUG_MODE;
          return(1);
        }
        else
        {
          // Processor was IN debug mode.  Shouldn't be getting this.
          printf("JTAGEMUL: Received CTRL-C while not running.\n");
          ui_ejtag_state = dMIPS32_EJTAG_EXIT_DEBUG_MODE;
          return(-1);
        }
      }
      else if (c_byte == dGDBRDP_END_OF_COMMAND) // Somehow in the middle of a command
      {
        si_inputhandlerstate = dGDBRDP_OUT_OF_SYNC;
        break;
      }
      // All other data just throw away (out of sync)
      break;

    case dGDBRDP_OUT_OF_SYNC:
      // When out of sync we need to receive the 2 checksum
      // bytes and then send a NACK.  This is the first byte.
      si_inputhandlerstate = dGDBRDP_OUT_OF_SYNC_2;
      break;

    case dGDBRDP_OUT_OF_SYNC_2:
      // Last byte of checksum of a command that we are not
      // in sync with.  Just send the NACK.
      fJTAGSOCK_WriteByte(i_socket, dGDBRDP_NACK);
      si_inputhandlerstate = dGDBRDP_INPUT_IDLE;
      break;

    case dGDBRDP_RECEIVE_COMMAND:
      // Receive data into buffer until '#' is received
      if (c_byte == dGDBRDP_END_OF_COMMAND)
      {
        si_inputhandlerstate = dGDBRDP_GET_CHECKSUM;
        break;
      }

      // New data received
      suc_calculatedchecksum = suc_calculatedchecksum + c_byte;
      sac_commandbuffer[sui_numbytesreceived++]=c_byte; 
      break;

    case dGDBRDP_GET_CHECKSUM:
      // Receive the first nibble of the checksum
      suc_receivedchecksum = fGDBRDP_ConvertNibble(c_byte) << 4;
      si_inputhandlerstate = dGDBRDP_GET_CHECKSUM_2;
      break;

    case dGDBRDP_GET_CHECKSUM_2:
      // Receive the second nibble of the checksum and check it.
      suc_receivedchecksum = suc_receivedchecksum + fGDBRDP_ConvertNibble(c_byte);

      // Back to idle regardless of checksum
      si_inputhandlerstate = dGDBRDP_INPUT_IDLE;

      if (suc_receivedchecksum != suc_calculatedchecksum)
      {
        // Bad checksum calculation - Report NACK
        fJTAGSOCK_WriteByte(i_socket, dGDBRDP_NACK);
        break;
      }

      // We have a valid command, ACK it and run the handler.
      fJTAGSOCK_WriteByte(i_socket, dGDBRDP_ACK);
      return(fGDBRDP_CommandInterpreter(i_socket, &sac_commandbuffer[0], sui_numbytesreceived));
      break;
  }

  return(1);
}

int help_access(unsigned int ui_address,int andlo0,int orlo0,int *entrylo0)
{
	  if(ui_address < 0x80000000 || ui_address >= 0xc0000000)
	  {
	volatile unsigned int codebuf[] = tgt_compile(
			".set noat;\n" 
			".set mips3;\n" 
			"1:\n" 
			"mtc0    ra, $31;\n" 
			"bal 2f;\n"
			"nop;\n"
			".space 60;\n"
			"2:sw    $1, 0(ra);\n" 
			"sw    $2, 4(ra);\n" 

			"mfc0  $2, c0_index;\n"
			"sw	 $2, 8(ra);\n"

			"mfc0  $2, c0_entryhi;\n"
			"sw	 $2, 12(ra);\n"

			"mfc0  $2, c0_entrylo0;\n"
			"sw	 $2, 16(ra);\n"


			"mfc0  $2, c0_entrylo1;\n"
			"sw	 $2, 20(ra);\n"

			"sw $3, 52(ra);\n"
			"mfc0 $3,c0_pagemask;\n"
			"ori $3,0x1fff;\n"

			"mfc0  $2, c0_entryhi;\n"
			"andi   $2,0xff;\n"
			"lw    $1, 24(ra);\n"  /*input address*/
			"or $1,$3;\n"
			"xor $1,$3;\n"
			"or $2,$1;\n"
			"mtc0  $2,c0_entryhi;\n"

			"tlbp;\n"
			"mfc0  $2, c0_index;\n" /*not in tlb,just ignore*/
			"sw	$2, 28(ra);\n"  /*return index*/
			"slt   $2,$2,$0;\n"
			"bnez $2,2f;\n"
			"nop;\n"
			"tlbr;\n"

			"mfc0 $2,c0_entrylo1;\n" 
			"sw	$2, 48(ra);\n" /*return lo1*/

			"mfc0 $2,c0_entrylo0;\n" 
			"sw	$2, 32(ra);\n" /*return lo0*/

			"addiu $3,1;\n"
			"srl $3,1;\n"
			"lw    $1, 24(ra);\n"  /*input address*/
			 "and $3,$1;\n"
			"bnez $3,101f;\n"
			"nop;\n"


			"mfc0 $2,c0_entrylo0;\n" 
			"and $1,$2,2;\n"
			"beqz $1,2f;\n"
			"nop;\n"

			"sw $2,36(ra);\n"  /*return success*/
			"lw $1,40(ra);\n" /*andlo0*/
			"and $2,$1;\n"
			"lw $1,44(ra);\n" /*orlo0*/
			"or $2,$1;\n"
			"mtc0 $2,c0_entrylo0;\n"
			"tlbwi;\n"
			"b 2f;\n"
			"nop;"
			"101:\n"

			"mfc0 $2,c0_entrylo1;\n" 
			"and $1,$2,2;\n"
			"beqz $1,2f;\n"
			"nop;\n"

			"sw $2,36(ra);\n"  /*return success*/
			"lw $1,40(ra);\n" /*andlo0*/
			"and $2,$1;\n"
			"lw $1,44(ra);\n" /*orlo0*/
			"or $2,$1;\n"
			"mtc0 $2,c0_entrylo1;\n"
			"tlbwi;\n"
			
			"2:lw $2, 20(ra);\n"	  
			"mtc0  $2, c0_entrylo0;\n"

			"lw $2, 16(ra);\n"	  
			"mtc0  $2, c0_entrylo0;\n"

			"lw $2, 12(ra);\n"	  
			"mtc0  $2, c0_entryhi;\n"

			"lw $2, 8(ra);\n"	  
			"mtc0  $2, c0_index;\n"

			"lw $3, 52(ra);\n"
			"lw    $2, 4(ra);\n" 
			"lw    $1, 0(ra);\n" 
			"mfc0    ra, $31;\n" 
			"b 1b;\n" 
			"nop;\n" 
			);

/*
 * 10: index
 * 11: lo0
 * 15: lo1  
 * 12: lo
 */
	codebuf[9] = ui_address;
	codebuf[12] = 0; /*lo0 or lo1*/
	codebuf[13] = andlo0;
	codebuf[14] = orlo0;
	codebuf[15] = 12;
	codebuf[10] = 9;
      fMIPS32_ExecuteDebugModule(codebuf,sizeof(codebuf),1);
	printf("index=%x entrylo0=%x entrylo1=%x entrylo=%x\n",codebuf[10],codebuf[11],codebuf[15],codebuf[12]);
	if(entrylo0)
	*entrylo0 = codebuf[12];
	return codebuf[12]?0:1;
	  }
	  else return 0;
}

 
//
// fGDBRDP_CommandInterpreter
//
// Takes a string of ui_size length that has the actual command contents from GDB
// with all the protocol stripped.  Processes the command and calls the required 
// JTAG functions and responds to GDB when necessary.
//
#define Z_DEBUG 
// printf("ZGJ %s : %d\n",__FUNCTION__,__LINE__);
int fGDBRDP_CommandInterpreter(int i_socket, char *pc_command, unsigned int ui_size)
{
  unsigned int ui_responsesize;
  unsigned int ui_address;
  unsigned int ui_count;
  unsigned int count;
  unsigned int ui_index;
  unsigned int entrylo0;
  char         ac_responsebuffer[dGDBRDP_MAX_CMD_BUFFER];
  int num;
  pc_command[ui_size]=0;
  {
  int i;
  printf("mydebug:receive ");
  for(i=0;i<ui_size;i++)
  printf("%c",pc_command[i]);
  printf("\n");
  }

  // Evaluate each command received from GDB  
  switch(*pc_command)
  {
    case 'k': // GDB is exiting - Kill thread
      // We are currently using this as indication that GDB is exiting.
        if (ui_ejtag_state == dMIPS32_EJTAG_WAIT_FOR_CONTINUE)
         ui_ejtag_state = dMIPS32_EJTAG_EXIT_DEBUG_MODE;
        sprintf(ac_responsebuffer, "OK");
        ui_responsesize = 2;
  	//	fGDBRDP_SendToGDB(i_socket, &ac_responsebuffer[0], ui_responsesize);
	//printf("K-K-K-K-K-K-K-K-\n");
      return(-1); // Forces exit
      break;

     case 'D': //detach
        if (ui_ejtag_state == dMIPS32_EJTAG_WAIT_FOR_CONTINUE)
         ui_ejtag_state = dMIPS32_EJTAG_EXIT_DEBUG_MODE;
        sprintf(ac_responsebuffer, "OK");
        ui_responsesize = 2;
  		fGDBRDP_SendToGDB(i_socket, &ac_responsebuffer[0], ui_responsesize);
      return(-1); // Forces exit
      break;

    case '?': // Get last signal
      // This is used to query the debug hardware as to what the last exception was
      // that entered debug mode.  We always use 05 as of now which is ABORT.
      sprintf(ac_responsebuffer, "S%02x", ui_last_signal);
      ui_responsesize=3;
      break;

    case 'g': // Read general registers
      // This command reads all the registers from the processor and returns them

      // Go read all the registers from the processor
	  uififo_raddr=0;
	  uififo_waddr=0;
	  memset(ui_fifomem,0,38*4);
      fMIPS32_ExecuteDebugModule(aui_readregisters_code,0x80000,1);
      
      // Now the register array is accurate, pump it out
      for (ui_responsesize=0; ui_responsesize<(dMIPS32_NUM_DEBUG_REGISTERS*8); ui_responsesize+=8)
      {
      	sprintf(&ac_responsebuffer[ui_responsesize], "%08x", gdb_little_endian(ui_fifomem[ui_responsesize/8]));
      }
	memcpy(aui_registers,ui_fifomem,dMIPS32_NUM_DEBUG_REGISTERS*4);
      break;
	case 'p': //this command is for 64bit regs
	{
	int regnum;
	if(pc_command[2]) regnum=(fGDBRDP_ConvertNibble(pc_command[1])<<4)+fGDBRDP_ConvertNibble(pc_command[2]);
	else  regnum=fGDBRDP_ConvertNibble(pc_command[1]);
      // Go read all the registers from the processor

		if(regnum<31)
		{
	int reg32code[]=tgt_compile(\
		"sw $0,($31);\n" \
		"sw $1,($31);\n" \
		);
		 reg32code[1]=reg32code[0]+regnum*0x10000;
		  tgt_exec(\
		  "mtc0    $31, $31;\n" \
		  "lui    $31,0xFF20;\n" \
		  ,8);
		  fMIPS32_ExecuteDebugModule(&reg32code[1],4,1);
		  tgt_exec("mfc0    $31, $31;\n",4);
		}
		else if(regnum==31)
		tgt_exec(\
		  "mtc0    $2, $31;\n" \
		  "lui    $2,0xFF20;\n" \
		  "sw    $31, ($2);\n" \
		  "mfc0    $2, $31;\n" \
		  ,16);
		else if(regnum<38) //sr
		{
		int cpxbuf[]=tgt_compile(\
		  "mfc0 $2, $12;\n" \
		  "mflo	$2;\n" \
		  "mfhi	$2;\n" \
		  "mfc0	$2, $8;\n" \
		  "mfc0	$2, $13;\n" \
		  "mfc0	$2, $24;\n" \
		  );

		tgt_exec(\
		  "mtc0    $15, $31;\n" \
		  "lui    $15,0xFF20;\n" \
		  "sw    $2, 0x1fc($15);\n" \
		  ,12);
		fMIPS32_ExecuteDebugModule(&cpxbuf[regnum-32],4,1);
		tgt_exec(\
		  "sw    $2, ($15);\n" \
		  "lw    $2, 0x1fc($15);\n" \
		  "mfc0    $15, $31;\n" \
		  ,12);
		}
		  else 
		  ui_datamem[0]=0;

	    if(ui_datamem[0]&0x80000000)
      	sprintf(ac_responsebuffer, "%08xffffffff", gdb_little_endian(ui_datamem[0]));
		else
      	sprintf(ac_responsebuffer, "%08x00000000", gdb_little_endian(ui_datamem[0]));
		ui_responsesize=16;
	   }
	 break;

    case 'P': // Write a single register
      // Format is Pxx=12345678 where xx is the register number
      pc_command++; // Get past the 'P'
      
      // Get the register number
	  if(pc_command[1]=='=') ui_index  = fGDBRDP_ConvertNibble(*pc_command++);
	  else
	  {
      ui_index  = fGDBRDP_ConvertNibble(*pc_command++) << 4;
      ui_index += fGDBRDP_ConvertNibble(*pc_command++);
	  }

	  pc_command++;

      
      if (ui_index < dMIPS32_NUM_DEBUG_REGISTERS)
      {
        // Valid register number so go ahead and update
        ui_datamem[0] = fGDBRDP_ConvertNibble(*pc_command++) << 4;
        ui_datamem[0] += fGDBRDP_ConvertNibble(*pc_command++);
        ui_datamem[0] += fGDBRDP_ConvertNibble(*pc_command++) << 12;
        ui_datamem[0] += fGDBRDP_ConvertNibble(*pc_command++) << 8;
        ui_datamem[0] += fGDBRDP_ConvertNibble(*pc_command++) << 20;
        ui_datamem[0] += fGDBRDP_ConvertNibble(*pc_command++) << 16;
        ui_datamem[0] += fGDBRDP_ConvertNibble(*pc_command++) << 28;
        ui_datamem[0] += fGDBRDP_ConvertNibble(*pc_command++) << 24;
		if(ui_index<31)
		{
	int reg32code[]=tgt_compile(\
		"lw $0,($31);\n" \
		"lw $1,($31);\n" \
		);
		 reg32code[1]=reg32code[0]+ui_index*0x10000;
		  tgt_exec(\
		  "mtc0    $31, $31;\n" \
		  "lui    $31,0xFF20;\n" \
		  ,8);
		  fMIPS32_ExecuteDebugModule(&reg32code[1],4,1);
		  tgt_exec("mfc0    $31, $31;\n",4);
		}
		else if(ui_index==31)
		tgt_exec(\
		  "mtc0    $2, $31;\n" \
		  "lui    $2,0xFF20;\n" \
		  "lw    $31, ($2);\n" \
		  "mfc0    $2, $31;\n" \
		  ,16);
		else if(ui_index<38) //sr
		{
		int cpxbuf[]=tgt_compile(\
		  "mtc0 $2, $12;\n" \
		  "mtlo	$2;\n" \
		  "mthi	$2;\n" \
		  "mtc0	$2, $8;\n" \
		  "mtc0	$2, $13;\n" \
		  "mtc0	$2, $24;\n" \
		  );

		tgt_exec(\
		  "mtc0    $15, $31;\n" \
		  "lui    $15,0xFF20;\n" \
		  "sw    $2, 0x1fc($15);\n" \
		  "lw    $2, ($15);\n" \
		  ,16);
		fMIPS32_ExecuteDebugModule(&cpxbuf[ui_index-32],4,1);
		tgt_exec(\
		  "lw    $2, 0x1fc($15);\n" \
		  "mfc0    $15, $31;\n" \
		  ,8);
		}

        sprintf(ac_responsebuffer, "OK");
        ui_responsesize = 2;
      }
      else
      {
        printf("JTAGEMUL: Invalid register number %d.\n", ui_index);
      }
      break;
    case 'G': // Write general registers
      // This command writes all the registers in the processor
      pc_command++;
      
      for (ui_index=0; ui_index<dMIPS32_NUM_DEBUG_REGISTERS; ui_index++)
      {
        aui_registers[ui_index] = fGDBRDP_ConvertNibble(*pc_command++) << 4;
        aui_registers[ui_index] += fGDBRDP_ConvertNibble(*pc_command++);
        aui_registers[ui_index] += fGDBRDP_ConvertNibble(*pc_command++) << 12;
        aui_registers[ui_index] += fGDBRDP_ConvertNibble(*pc_command++) << 8;
        aui_registers[ui_index] += fGDBRDP_ConvertNibble(*pc_command++) << 20;
        aui_registers[ui_index] += fGDBRDP_ConvertNibble(*pc_command++) << 16;
        aui_registers[ui_index] += fGDBRDP_ConvertNibble(*pc_command++) << 28;
        aui_registers[ui_index] += fGDBRDP_ConvertNibble(*pc_command++) << 24;
      }
	  uififo_raddr=0;
	  uififo_waddr=0;
	memcpy(ui_fifomem,aui_registers,dMIPS32_NUM_DEBUG_REGISTERS*4);
      fMIPS32_ExecuteDebugModule(aui_writeregisters_code,0x80000,1);
      
      sprintf(ac_responsebuffer, "OK");
      ui_responsesize = 2;
      break;

    case 'H': // Set thread
      // For now just say OK!
      sprintf(ac_responsebuffer, "OK");
      ui_responsesize = 2;
      break;

    case 'q': // Get a general query
      // For now only support 'qC' that returns the current thread ID 16bit
      pc_command++;
      switch(*pc_command)
      {
        case 'C':
          // Just return a made up thread ID
          sprintf(ac_responsebuffer, "QC5A5A");
          ui_responsesize = 6;
          break;
          
        default:
          // Unsupported general query
          ui_responsesize = 0;
          break;
      }
      break;

    case 'm': // Read memory
      // OK - This command comes formatted m<addr>,<length> with both components 
      // encoded ascii hex MSB to LSB.
      ui_address = 0x00;
      ui_count   = 0x00;

      // Calculate the address
      for (ui_index=1; *(pc_command+ui_index)!=','; ui_index++)
      {
      	ui_address = ui_address << 4;
      	ui_address = ui_address + fGDBRDP_ConvertNibble(*(pc_command+ui_index));
      }
      
      // Increment the index past the ','
      ui_index++;
      
      // Calculate the count
      for ( ; ui_index<ui_size; ui_index++)
      {
      	ui_count = ui_count << 4;
      	ui_count = ui_count + fGDBRDP_ConvertNibble(*(pc_command+ui_index));
      }

      // Ok, now we have the address and count for the read.  For speed we want to
      // read as much as fast as we can.  We WANT to do 32 bit aligned WORD accesses
      // but we can't always do that.  What we will do is perform BYTE reads until
      // we are WORD aligned.  Then we will perform WORD reads as long as we can.
      // Then we will finish up with more BYTE reads until we have the entire range.
      
      ui_responsesize=0x00;

	  if(ui_address < 0x80000000 || ui_address >= 0xc0000000)
	  {

		  if(help_access(ui_address,-1,0,0) != 0)
		  {
			  sprintf(ac_responsebuffer, "E16");
			  ui_responsesize = 3;
			  break;
		  }
	  }

      
      // Check to see if we are WORD aligned, if not then do BYTE reads
      while (0x00 != ui_address % 0x04) // A word aligned address will result in 0 modulus
      {
        // Non zero means we are not WORD aligned and need to do byte reads
        ui_datamem[0]=ui_address;
        fMIPS32_ExecuteDebugModule(aui_readbyte_code,0x80000,1);
        sprintf(&ac_responsebuffer[ui_responsesize], "%02x", ui_datamem[1]);

        ui_responsesize = ui_responsesize + 2;
        ui_address = ui_address + 1;
        ui_count = ui_count - 1;
        
        // Now we have to check to see if we are finished (for small accesses)
        if (0x00 == ui_count)
        {
          // All done, go ahead and exit
          break;
        }
      }

      // Now we are WORD aligned, we need to read WORDs as long as we have at least
      // WORDSIZE count of bytes left to read.
      while (ui_count > 0x03)
      {
        ui_datamem[0]=ui_address;
        fMIPS32_ExecuteDebugModule(aui_readword_code,0x80000,1);
        sprintf(&ac_responsebuffer[ui_responsesize], "%08x", gdb_little_endian(ui_datamem[1]));

        ui_responsesize = ui_responsesize + 8;
        ui_address = ui_address + 4;
        ui_count = ui_count - 4;
      }

      // Now we need to perform any BYTE reads necessary to finish the read block.
      while(ui_count)
      {
        ui_datamem[0]=ui_address;
        fMIPS32_ExecuteDebugModule(aui_readbyte_code,0x80000,1);
        sprintf(&ac_responsebuffer[ui_responsesize], "%02x", ui_datamem[1]);

        ui_responsesize = ui_responsesize + 2;
        ui_address = ui_address + 1;
        ui_count = ui_count - 1;
      }

      // Finished.  Successful read of the entire block.
      break;
    
    case 'M': // Write memory
      // OK - This command comes formatted M<addr>,<length>:VALUE with all components 
      // encoded ascii hex MSB to LSB.
      ui_address = 0x00;
      ui_count   = 0x00;

      // Calculate the address
      for (ui_index=1; *(pc_command+ui_index)!=','; ui_index++)
      {
      	ui_address = ui_address << 4;
      	ui_address = ui_address + fGDBRDP_ConvertNibble(*(pc_command+ui_index));
      }
      
      // Increment the index past the ','
      ui_index++;
      
      // Calculate the count
      for ( ; *(pc_command+ui_index)!=':'; ui_index++)
      {
      	ui_count = ui_count << 4;
      	ui_count = ui_count + fGDBRDP_ConvertNibble(*(pc_command+ui_index));
      }

	  if(help_access(ui_address,-1,4,&entrylo0) != 0)
	  {
		  sprintf(ac_responsebuffer, "E16");
		  ui_responsesize = 3;
		  break;
          break;
	  }

      // Increment the index past the ':'
      ui_index++;
      while(ui_count)
      {
		  if(((ui_address&3)==0)&&(ui_count>=0x04)) count=0x04;
		  else if(((ui_address&1)==0)&&(ui_count>=0x02)) count=0x02;
		  else count=1;
      
      switch(count)
      {
        case 0x01: // Byte write
          ui_datamem[0]=ui_address;

          ui_datamem[1]  = fGDBRDP_ConvertNibble(*(pc_command+ui_index++)) << 4;
          ui_datamem[1] += fGDBRDP_ConvertNibble(*(pc_command+ui_index++));

          fMIPS32_ExecuteDebugModule(aui_writebyte_code,0x80000,1);
		  ui_address+=1;
		  ui_count-=1;
          break;
          
        case 0x02: // Half word write
          ui_datamem[0]=ui_address;

          ui_datamem[1]  = fGDBRDP_ConvertNibble(*(pc_command+ui_index++)) << 4;
          ui_datamem[1] += fGDBRDP_ConvertNibble(*(pc_command+ui_index++));
          ui_datamem[1] += fGDBRDP_ConvertNibble(*(pc_command+ui_index++)) << 12;
          ui_datamem[1] += fGDBRDP_ConvertNibble(*(pc_command+ui_index++)) << 8;

          fMIPS32_ExecuteDebugModule(aui_writehalf_code,0x80000,1);
		  ui_address+=2;
		  ui_count-=2;
          break;
          
        case 0x04: // Word write
          ui_datamem[0]=ui_address;

          ui_datamem[1]  = fGDBRDP_ConvertNibble(*(pc_command+ui_index++)) << 4;
          ui_datamem[1] += fGDBRDP_ConvertNibble(*(pc_command+ui_index++));
          ui_datamem[1] += fGDBRDP_ConvertNibble(*(pc_command+ui_index++)) << 12;
          ui_datamem[1] += fGDBRDP_ConvertNibble(*(pc_command+ui_index++)) << 8;
          ui_datamem[1] += fGDBRDP_ConvertNibble(*(pc_command+ui_index++)) << 20;
          ui_datamem[1] += fGDBRDP_ConvertNibble(*(pc_command+ui_index++)) << 16;
          ui_datamem[1] += fGDBRDP_ConvertNibble(*(pc_command+ui_index++)) << 28;
          ui_datamem[1] += fGDBRDP_ConvertNibble(*(pc_command+ui_index++)) << 24;

          fMIPS32_ExecuteDebugModule(aui_writeword_code,0x80000,1);
		  ui_address+=4;
		  ui_count-=4;
          break;
          
        default:   // Unsupported
          printf("GDBRDP ERROR: Unsupported write size of %d.\n", ui_count);
          ui_responsesize=0;
          break;
      }          
      }
	  help_access(ui_address,entrylo0&4?-1:~4,0,&entrylo0);
      sprintf(ac_responsebuffer, "OK");
      ui_responsesize = 2;
      break;

    
    case 'S': // This one has signal to step in it (ignore for now, just step)
    case 's': // Step (possible at address)
      // All we need to do is set the single step bit and continue - The chip will
      // only execute 1 instruction and then return to debug mode.  Just execute
      // the code to set the single step bit and then fall through into the continue
      // code that takes care of the rest.
      fMIPS32_ExecuteDebugModule(aui_singlestepset_code,0x80000,1);
	  goto continue_now;
      
      // *** NOTE *** FALL THROUGH ON PURPOSE!
      
    case 'C': // This one has signal to resume in it (ignore for now, just continue)
	 {
	  int signal=(fGDBRDP_ConvertNibble(pc_command[1])<<4)+fGDBRDP_ConvertNibble(pc_command[2]);
	  if(signal==0x14)
	  {
	  sprintf(ac_responsebuffer,"T02"); //send sigint
      ui_responsesize = strlen(ac_responsebuffer);
      break;
	  }
	  }
    case 'c': // Continue (possible at address)
      // Continue is a different animal because it has no response back to GDB
      // until something happens that halts the system.  (such as a breakpoint or
      // signal)  All we need to do is have the machine exit debug mode at this 
      // point.  If it ever comes out on its own the debug mode catcher will 
      // notifiy GDB.
	continue_now:
      if (ui_ejtag_state == dMIPS32_EJTAG_WAIT_FOR_CONTINUE)
      {
      	// Let the catcher take care of exiting debug mode.
        ui_ejtag_state = dMIPS32_EJTAG_EXIT_DEBUG_MODE;
        return(1);
      }
      else
      {
        // Processor was NOT in debug mode.  Shouldn't be getting this.
        printf("JTAGEMUL: Received continue while NOT debugging.\n");
        return(1);
      }
      break;

    case 'z': // The 'z' packet is for removing breakpoints
    case 'Z': // The 'Z' packet is for setting breakpoints
      // Z0 is for software breakpoints, Z1 is for hardware breakpoints
      if ('0' == *(pc_command+1))
      {
        // Handle software breakpoint
        if (1 == fGDBRDP_HandleSoftwareBreakpoint(pc_command, ui_size))
        { 
          // Success       
          sprintf(ac_responsebuffer, "OK");
          ui_responsesize = 2;
          break;
        }
        else
        { 
          // Failure
          sprintf(ac_responsebuffer, "E00");
          ui_responsesize = 3;
          break;
        }
      }
      if ('1' == *(pc_command+1))
      {
        // Handle hardware breakpoint
        if (1 == fGDBRDP_HandleHardwareInstBreakpoint(pc_command, ui_size))
        { 
          // Success       
          sprintf(ac_responsebuffer, "OK");
          ui_responsesize = 2;
          break;
        }
        else
        { 
          // Failure
          sprintf(ac_responsebuffer, "E00");
          ui_responsesize = 3;
          break;
        }
      }
      else if (('2' == *(pc_command+1))||('3' == *(pc_command+1))||('4' == *(pc_command+1)))
      {
        // Handle hardware breakpoint
        if (1 == fGDBRDP_HandleHardwareDataBreakpoint(pc_command, ui_size))
        { 
          // Success       
          sprintf(ac_responsebuffer, "OK");
          ui_responsesize = 2;
          break;
        }
        else
        { 
          // Failure
          sprintf(ac_responsebuffer, "E00");
          ui_responsesize = 3;
          break;
        }
      }
      else
      {
        // Unknown - report error
        sprintf(ac_responsebuffer, "E00");
        ui_responsesize = 3;
        break;
      }
      break;
     case 'a':
	do_cmd(&pc_command[1]);
      
    default:  // Command not handled. 
      // Unhandled commands should be responded with $#00.  What we will do here
      // is set the response size to 0 and call the respond routine.  The respond
      // routine will send the $ first, then the message of nothing, then the # 
      // and the calculated checksum which will be 00.
      ui_responsesize=0;
      break;
  }

  // We always send a response to these processed commands.  If it fell through
  // to default we will send an empty packet which the spec says is correct for
  // unsupported options.
  fGDBRDP_SendToGDB(i_socket, &ac_responsebuffer[0], ui_responsesize);

  return(1);
}

//
// fGDBRDP_SendToGDB
//
// Send a message to GDB.  Takes a string to send (pc_message) of uc_length size.
// Generates all the protocol required including checksum to send the data.
//
void fGDBRDP_SendToGDB(int i_socket, char *pc_message, unsigned int ui_length)
{
  unsigned char uc_calculatedchecksum;
  
  // Start checksum at 0
  uc_calculatedchecksum = 0;
  
Z_DEBUG
  // Send the leader
  fJTAGSOCK_WriteByte(i_socket, dGDBRDP_START_OF_COMMAND);
  
Z_DEBUG
  while(ui_length--)
  {
Z_DEBUG
    // Send each byte in the message while calculating checksum.
    fJTAGSOCK_WriteByte(i_socket, *pc_message);
Z_DEBUG
    uc_calculatedchecksum = uc_calculatedchecksum + *pc_message++;
  }
  
Z_DEBUG
  // Send the trailer
  fJTAGSOCK_WriteByte(i_socket, dGDBRDP_END_OF_COMMAND);
Z_DEBUG
  
  // Send the upper nibble of the checksum
  fJTAGSOCK_WriteByte(i_socket, fGDBRDP_CalculateNibble(uc_calculatedchecksum >> 4));
Z_DEBUG
  
  // Send the lower nibble of the checksum
  fJTAGSOCK_WriteByte(i_socket, fGDBRDP_CalculateNibble(uc_calculatedchecksum & 0x0F));
Z_DEBUG

  // Forget about the ACK or NACK - Hope this is good enough.  Time will tell.  The input 
  // handler will throw out the ack/nack while waiting on a leader.
  return;
}

//
// fGDBRDP_ConvertNibble
//
// Takes one ascii encoded hex nibble and converts it to its
// decimal equivelent.
//
int fGDBRDP_ConvertNibble (char c_nibble)
{
  switch(c_nibble)
  {
    case '0': return(0);
    case '1': return(1);
    case '2': return(2);
    case '3': return(3);
    case '4': return(4);
    case '5': return(5);
    case '6': return(6);
    case '7': return(7);
    case '8': return(8);
    case '9': return(9);
    case 'a': return(10);
    case 'A': return(10);
    case 'b': return(11);
    case 'B': return(11);
    case 'c': return(12);
    case 'C': return(12);
    case 'd': return(13);
    case 'D': return(13);
    case 'e': return(14);
    case 'E': return(14);
    case 'f': return(15);
    case 'F': return(15);
    default:  return(0);
  }
}
    
//
// fGDBRDP_CalculateNibble
//
// Takes one decimal nibble and converts it to the ascii
// encoded hex equivelent.
//
char fGDBRDP_CalculateNibble (int i_nibble)
{
  switch(i_nibble)
  {
    case 0x00: return('0');
    case 0x01: return('1');
    case 0x02: return('2');
    case 0x03: return('3');
    case 0x04: return('4');
    case 0x05: return('5');
    case 0x06: return('6');
    case 0x07: return('7');
    case 0x08: return('8');
    case 0x09: return('9');
    case 0x0A: return('a');
    case 0x0B: return('b');
    case 0x0C: return('c');
    case 0x0D: return('d');
    case 0x0E: return('e');
    case 0x0F: return('f');
    default:   return('0');
  }
}

int set_inst_breakpoint(unsigned int ui_address)
{
unsigned int ui_index;
unsigned int tmpibc,asid;
    // Set breakpoint - First check to make sure this address doesn't already
    // have a software breakpoint.  We only allow 1 breakpoint per address.
    for(ui_index=0; ui_index<dMIPS32_MAX_HARDWARE_BKPTS; ui_index++)
    {
      if (hwb_inst_array[ui_index].uc_enabled)
      {
        // Breakpoint is enabled, check to see the address
        if (hwb_inst_array[ui_index].ui_address == ui_address)
        {
          // Bad bad bad!  Already have a breakpoint at this address!
          printf("DUPLICATE HARDWARE BREAKPOINT!\n");
          return(-1);
        }
      }
    }
    
    // OK - Breakpoint is NOT a duplicate - Find an open slot
    for(ui_index=0; ui_index<dMIPS32_MAX_HARDWARE_BKPTS; ui_index++)
    {
      if (!(hwb_inst_array[ui_index].uc_enabled))
      {
        // Ok - This is an open slot - Mark enabled - Read old value - 
        // Write new value - Store address
        hwb_inst_array[ui_index].uc_enabled = 0x01;
        hwb_inst_array[ui_index].ui_address = ui_address;

	if(config_gdbserver_compareasid)
	{
	  tgt_exec(\
	  ".set noat;\n" \
	  "1:\n" \
	  "mtc0    $15, $31;\n" \
	  "lui    $15,0xFF20;\n" \
	  "sw    $2, 0x1fc($15);\n" \
	  "mfc0    $2, $10;\n" \
	  "sw    $2, ($15);\n" \
	  "lw    $2, 0x1fc($15);\n" \
	  "mfc0    $15, $31;\n" \
	  "b 1b;\n" \
	  "nop;\n" \
	  ,0x80000);

	 asid=ui_datamem[0]&0xff;
	}

		tmpibc = 1;
		if(config_gdbserver_compareasid)
		 tmpibc |= 1<<23;
        
        // Now we need to set the enable bit in the watch register which is
        // in bit 2 with the address.  Just OR this bit right into the register.
        ui_datamem[0] =dMIPS32_INST_BKPT_ADDR(ui_index); //set 1 to this address to set BE
        ui_datamem[1] = ui_address;
        ui_datamem[2] = asid;
        ui_datamem[3] = tmpibc;
        
        // Write out the breakpoint registers
		  tgt_exec(\
		  ".set noat;\n" \
		  "1:\n" \
		  "mtc0    $15, $31;\n" \
		  "lui    $15,0xFF20;\n" \
		  "sw    $2, 0x1fc($15);\n" \
		  "sw    $3, 0x1fc($15);\n" \
		  "lw    $2, ($15);\n" \
		  "lw    $3, 4($15);\n" \
		  "sw    $3, ($2);\n" \
		  "lw    $3, 8($15);\n" \
		  "sw    $3, 0x10($2);\n" \
		  "lw    $3, 12($15);\n" \
		  "sw    $3, 0x18($2);\n" \
		  "lw    $3, 0x1fc($15);\n" \
		  "lw    $2, 0x1fc($15);\n" \
		  "mfc0    $15, $31;\n" \
		  "b 1b;\n" \
		  "nop;\n" \
		  ,0x80000);

        return(1); // Success!
      }
    }

    // No open slots!
    printf("NO OPEN HARDWARE BREAKPOINT SLOTS!\n");
    return(-1);
}

int unset_inst_breakpoint(unsigned int ui_address)
{
unsigned int ui_index;
    // Search for the breakpoint
    for(ui_index=0; ui_index<dMIPS32_MAX_HARDWARE_BKPTS; ui_index++)
    {
      if (hwb_inst_array[ui_index].uc_enabled)
      {
        if (hwb_inst_array[ui_index].ui_address == ui_address)
        {
          // Found the breakpoint - Mark unused - Delete - Disable
          hwb_inst_array[ui_index].uc_enabled = 0x00;

        ui_datamem[0] =dMIPS32_INST_BKPT_CONTROL(ui_index); //set 1 to this address to set BE
        ui_datamem[1] = 0;
        ui_datamem[2] = dMIPS32_INST_BKPT_ADDR(ui_index);
        ui_datamem[3] = ui_address;
          // Write out the breakpoint registers
	  tgt_exec(\
	  ".set noat;\n" \
	  "1:\n" \
	  "mtc0    $15, $31;\n" \
	  "lui    $15,0xFF20;\n" \
	  "sw    $2, 0x1fc($15);\n" \
	  "sw    $3, 0x1fc($15);\n" \
	  "lw    $2, ($15);\n" \
	  "lw    $3, 0x4($15);\n" \
	  "sw    $3, ($2);\n" \
	  "lw    $2, 8($15);\n" \
	  "lw    $3, 0xc($15);\n" \
	  "sw    $3, ($2);\n" \
	  "lw    $3, 0x1fc($15);\n" \
	  "lw    $2, 0x1fc($15);\n" \
	  "mfc0    $15, $31;\n" \
	  "b 1b;\n" \
	  "nop;\n" \
	  ,0x80000);

          return(1);
        }
      }
    }
    
    // Never found the breakpoint
    printf("BREAKPOINT NOT FOUND!\n");
    return(-1);
}

//
// fGDBRDP_HandleHardwareBreakpoint
//
// Sets or clears a hardware breakpoint
//
int  fGDBRDP_HandleHardwareInstBreakpoint (char *pc_command, unsigned int ui_size)
{
  unsigned int ui_index;
  unsigned int ui_address;
  
  // Calculate the address now - Its the same location for all packets.
  // Packet is formated <z/Z><0/1><,><ADDR><,><LENGTH>
  ui_address = 0x00;
  for (ui_index=3; *(pc_command+ui_index) != ','; ui_index++)
  {
    ui_address = ui_address << 4;
    ui_address = ui_address + fGDBRDP_ConvertNibble(*(pc_command+ui_index));
  }

  // Make sure address is 32bit aligned
  if (0x00 != (ui_address % 4))
  {
    // Unaligned, not allowed
    printf("BREAKPOINT NOT ALIGNED!\n");
    return(-1);
  }

  // Check to see if it is a SET or REMOVE breakpoint command
  if ('Z' == *pc_command) // Then its a set command
  return set_inst_breakpoint(ui_address);
  else if ('z' == *pc_command) // Then its a remove command
  return unset_inst_breakpoint(ui_address);

  // Should never get here
  return (-1);
}

int set_data_breakpoint(unsigned int ui_address,unsigned int ui_addrsize,int action)
{
  // action:
  // 1: watch  wrote
  // 2: rwatch read
  // 3: accessed
#define ACTION_WRITE 1
#define ACTION_READ 2
#define ACTION_ALL 3

unsigned int ui_index;
    // Set breakpoint - First check to make sure this address doesn't already
    // have a software breakpoint.  We only allow 1 breakpoint per address.
    for(ui_index=0; ui_index<dMIPS32_MAX_HARDWARE_BKPTS; ui_index++)
    {
      if (hwb_data_array[ui_index].uc_enabled)
      {
        // Breakpoint is enabled, check to see the address
        if (hwb_data_array[ui_index].ui_address == ui_address)
        {
          // Bad bad bad!  Already have a breakpoint at this address!
          printf("DUPLICATE HARDWARE BREAKPOINT!\n");
          return(-1);
        }
      }
    }
    
    // OK - Breakpoint is NOT a duplicate - Find an open slot
    for(ui_index=0; ui_index<dMIPS32_MAX_HARDWARE_BKPTS; ui_index++)
    {
      if (!(hwb_data_array[ui_index].uc_enabled))
      {
	   unsigned int tmpdbs,tmpdbcn,tmpdbm,value,asid;
        // Ok - This is an open slot - Mark enabled - Read old value - 
        // Write new value - Store address
        hwb_data_array[ui_index].uc_enabled = 0x01;
        hwb_data_array[ui_index].ui_address = ui_address;

	if(config_gdbserver_compareasid)
	{
	  tgt_exec(\
	  ".set noat;\n" \
	  "1:\n" \
	  "mtc0    $15, $31;\n" \
	  "lui    $15,0xFF20;\n" \
	  "sw    $2, 0x1fc($15);\n" \
	  "mfc0    $2, $10;\n" \
	  "sw    $2, ($15);\n" \
	  "lw    $2, 0x1fc($15);\n" \
	  "mfc0    $15, $31;\n" \
	  "b 1b;\n" \
	  "nop;\n" \
	  ,0x80000);

	 asid=ui_datamem[0]&0xff;
	}


	value = config_gdbserver_comparedata_val;


	tmpdbcn=1;	//set BE
	tmpdbm=0x00000000; //all address bit are compared
	switch (ui_addrsize) {
	case 4:
		break;
	case 2:
		switch (ui_address%4){
		case 0:
			tmpdbcn=tmpdbcn|((0xf^0x3)<<14);	//BAI; ignore access on which bytes 1100
			tmpdbcn=tmpdbcn|((0xf^0x3)<<4);	//BLM
			break;
		case 2:
			tmpdbcn=tmpdbcn|((0xf^0xc)<<14);	//BAI;
			tmpdbcn=tmpdbcn|((0xf^0xc)<<4);	//BLM
			break;
		default:
			printf("half word insert db went error!\n");
			return 0;
		}
		break;
	case 1:
		switch (ui_address%4) {
		case 0:
			tmpdbcn=tmpdbcn|((0xf^0x1)<<14);	//BAI
			tmpdbcn=tmpdbcn|((0xf^0x1)<<4);	//BLM
			break;
		case 1:
			tmpdbcn=tmpdbcn|((0xf^0x2)<<14);	//BAI
			tmpdbcn=tmpdbcn|((0xf^0x2)<<4);	//BLM
			//value = value << 8;
			break;
		case 2:
			tmpdbcn=tmpdbcn|((0xf^0x4)<<14);	//BAI
			tmpdbcn=tmpdbcn|((0xf^0x4)<<4);	//BLM
			//value = value << 16;
			break;
		case 3:
			tmpdbcn=tmpdbcn|((0xf^0x8)<<14);	//BAI
			tmpdbcn=tmpdbcn|((0xf^0x8)<<4);	//BLM
			//value = value << 24;
			break;
		default:
			printf("something went error\n");
		}
		break;
	default:
		printf("insert db went wrong! \n");
		break;
	};

		if(config_gdbserver_compareasid)
		 tmpdbcn |= (1<<23); //comare asid also
		if(!config_gdbserver_comparedata)
	     tmpdbcn |= (0xff<<4); //BAI, byte access ignore data bus
		if(action == ACTION_WRITE)
		 tmpdbcn |= (1<<12); //NOLB
        else if(action == ACTION_READ)
		 tmpdbcn |= (1<<13); //NOSB
        
        // Now we need to set the enable bit in the watch register which is
        // in bit 2 with the address.  Just OR this bit right into the register.
        ui_datamem[0] = dMIPS32_DATA_BKPT_ADDR(ui_index);
        ui_datamem[1] = ui_address;
        ui_datamem[2] = tmpdbm;
	    ui_datamem[3] = asid;
	    ui_datamem[4] = tmpdbcn;
	    ui_datamem[5] = value;

	  tgt_exec(\
	  ".set noat;\n" \
	  "1:\n" \
	  "mtc0    $15, $31;\n" \
	  "lui    $15,0xFF20;\n" \
	  "sw    $2, 0x1fc($15);\n" \
	  "sw    $3, 0x1fc($15);\n" \
	  "lw    $2, ($15);\n" \
	  "lw    $3, 0x4($15);\n" \
	  "sw    $3, ($2);\n" \
	  "lw    $3, 8($15);\n" \
	  "sw    $3, 0x8($2);\n" \
	  "lw    $3, 0xc($15);\n" \
	  "sw    $3, 0x10($2);\n" \
	  "lw    $3, 0x10($15);\n" \
	  "sw    $3, 0x18($2);\n" \
	  "lw    $3, 0x14($15);\n" \
	  "sw    $3, 0x20($2);\n" \
	  "lw    $3, 0x1fc($15);\n" \
	  "lw    $2, 0x1fc($15);\n" \
	  "mfc0    $15, $31;\n" \
	  "b 1b;\n" \
	  "nop;\n" \
	  ,0x80000);
        return(1); // Success!
      }
    }

    // No open slots!
    printf("NO OPEN HARDWARE BREAKPOINT SLOTS!\n");
    return(-1);
}

int unset_data_breakpoint(unsigned int ui_address)
{
unsigned int ui_index;
    // Search for the breakpoint
    for(ui_index=0; ui_index<dMIPS32_MAX_HARDWARE_BKPTS; ui_index++)
    {
      if (hwb_data_array[ui_index].uc_enabled)
      {
        if (hwb_data_array[ui_index].ui_address == ui_address)
        {
          // Found the breakpoint - Mark unused - Delete - Disable
          hwb_data_array[ui_index].uc_enabled = 0x00;

        ui_datamem[0] = dMIPS32_DATA_BKPT_STATUS; //clear BS[14:0] on DBS
        ui_datamem[1] = dMIPS32_DATA_BKPT_CONTROL(ui_index); //set 0 to this address to disable BE
          // Write out the breakpoint registers
	  tgt_exec(\
	  ".set noat;\n" \
	  "1:\n" \
	  "mtc0    $15, $31;\n" \
	  "lui    $15,0xFF20;\n" \
	  "sw    $2, 0x1fc($15);\n" \
	  "lw    $2, ($15);\n" \
	  "sw    $0, ($2);\n" \
	  "lw    $2, 4($15);\n" \
	  "sw    $0, ($2);\n" \
	  "lw    $2, 0x1fc($15);\n" \
	  "mfc0    $15, $31;\n" \
	  "b 1b;\n" \
	  "nop;\n" \
	  ,0x80000);

          return(1);
        }
      }
    }
    
    // Never found the breakpoint
    printf("BREAKPOINT NOT FOUND!\n");
    return(-1);
}

int  fGDBRDP_HandleHardwareDataBreakpoint (char *pc_command, unsigned int ui_size)
{
  unsigned int ui_index;
  unsigned int ui_address;
  unsigned int ui_addrsize;
  
  // Calculate the address now - Its the same location for all packets.
  // Packet is formated <z/Z><0/1><,><ADDR><,><LENGTH>
  // Z2: watch  wrote
  // Z3: rwatch read
  // Z4: accessed
  ui_address = 0x00;
  for (ui_index=3;*(pc_command+ui_index) != ','; ui_index++)
  {
    ui_address = ui_address << 4;
    ui_address = ui_address + fGDBRDP_ConvertNibble(*(pc_command+ui_index));
  }

  ui_addrsize = fGDBRDP_ConvertNibble(pc_command[ui_index+1]);

  // Check to see if it is a SET or REMOVE breakpoint command
  if ('Z' == *pc_command) // Then its a set command
	return set_data_breakpoint(ui_address,ui_addrsize,pc_command[1] - '1');
  else if ('z' == *pc_command) // Then its a remove command
    return unset_data_breakpoint(ui_address);

  // Should never get here
  return (-1);
}


int set_soft_breakpoint(unsigned ui_address)
{
	unsigned int ui_index;
    // Set breakpoint - First check to make sure this address doesn't already
    // have a software breakpoint.  We only allow 1 breakpoint per address.
    for(ui_index=0; ui_index<dGDBRDP_MAX_SW_BREAKPOINTS; ui_index++)
    {
      if (swb_array[ui_index].uc_enabled)
      {
        // Breakpoint is enabled, check to see the address
        if (swb_array[ui_index].ui_address == ui_address)
        {
          // Bad bad bad!  Already have a breakpoint at this address!
          printf("DUPLICATE BREAKPOINT!\n");
          return(-1);
        }
      }
    }
    
    // OK - Breakpoint is NOT a duplicate - Find an open slot
    for(ui_index=0; ui_index<dGDBRDP_MAX_SW_BREAKPOINTS; ui_index++)
    {
      if (!(swb_array[ui_index].uc_enabled))
      {
        // Ok - This is an open slot - Mark enabled - Read old value - 
        // Write new value - Store address
        swb_array[ui_index].uc_enabled = 0x01;
        
{
unsigned int codebuf[] = \
	  tgt_compile(\
	  ".set noat;\n" \
	  ".set mips3;\n" \
	  "1:\n" \
	  "mtc0    $15, $31;\n" \
	  "lui    $15,0xFF20;\n" \
	  "sw    $1, 0x1fc($15);\n" \
	  "sw    $2, 0x1fc($15);\n" \
	  "lw    $2, ($15);\n" \
	  "lw	 $1, ($2);\n" \
	  "sw    $1, 8($15);\n" \
	  "lw    $1, 4($15);\n" \
	  "sw    $1, ($2);\n" \
	  "cache    0x15,($2);\n" \
	  "cache    1, 0($2);\n" \
	  "cache    1, 1($2);\n" \
	  "cache    1, 2($2);\n" \
	  "cache    1, 3($2);\n" \
	  "cache    0x10,($2);\n" \
	  "cache    0, ($2);\n" \
	  "cache    0, 1($2);\n" \
	  "cache    0, 2($2);\n" \
	  "cache    0, 3($2);\n" \
	  "lw    $2, 0x1fc($15);\n" \
	  "lw    $1, 0x1fc($15);\n" \
	  "mfc0    $15, $31;\n" \
	  "b 1b;\n" \
	  "nop;\n");
if(ui_address >= 0x80000000 && ui_address < 0xc0000000)
{
        ui_datamem[0]=ui_address;
        ui_datamem[1] = dMIPS32_SW_BREAKPOINT_INSTRUCTION;
      fMIPS32_ExecuteDebugModule(codebuf,sizeof(codebuf),1);
      swb_array[ui_index].ui_original_memory = ui_datamem[2];
      swb_array[ui_index].ui_address = ui_address;
}
else
{
  unsigned int entrylo0;
	if(help_access(ui_address,-1,4,&entrylo0) == 0)
	{
	 
        ui_datamem[0]=ui_address;
        ui_datamem[1] = dMIPS32_SW_BREAKPOINT_INSTRUCTION;
      fMIPS32_ExecuteDebugModule(codebuf,sizeof(codebuf),1);
      swb_array[ui_index].ui_original_memory = ui_datamem[2];
      swb_array[ui_index].ui_address = ui_address;
	
	  help_access(ui_address,entrylo0&4?-1:~4,0,&entrylo0);
	}
	else
	{
		return -1;
	}
}
}


	return(1); // Success!
   }
 }
    printf("NO OPEN SLOTS!\n");
	return -2;
}

int unset_soft_breakpoint(unsigned ui_address)
{
 unsigned int ui_index;
    // Search for the breakpoint
    for(ui_index=0; ui_index<dGDBRDP_MAX_SW_BREAKPOINTS; ui_index++)
    {
      if (swb_array[ui_index].uc_enabled)
      {
        if (swb_array[ui_index].ui_address == ui_address)
        {
          // Found the breakpoint - Replace memory - Mark unused
          ui_datamem[0] = ui_address;
          ui_datamem[1] = swb_array[ui_index].ui_original_memory;

if(ui_address >= 0x80000000 && ui_address < 0xc0000000)
{
	  tgt_exec(\
	  ".set noat;\n" \
	  ".set mips3;\n" \
	  "1:\n" \
	  "mtc0    $15, $31;\n" \
	  "lui    $15,0xFF20;\n" \
	  "sw    $1, 0x1fc($15);\n" \
	  "sw    $2, 0x1fc($15);\n" \
	  "lw    $2, ($15);\n" \
	  "lw    $1, 4($15);\n" \
	  "sw    $1, ($2);\n" \
	  "cache    0x15,($2);\n" \
	  "cache    1, 0($2);\n" \
	  "cache    1, 1($2);\n" \
	  "cache    1, 2($2);\n" \
	  "cache    1, 3($2);\n" \
	  "cache    0x10,($2);\n" \
	  "cache    0, ($2);\n" \
	  "cache    0, 1($2);\n" \
	  "cache    0, 2($2);\n" \
	  "cache    0, 3($2);\n" \
	  "lw    $2, 0x1fc($15);\n" \
	  "lw    $1, 0x1fc($15);\n" \
	  "mfc0    $15, $31;\n" \
	  "b 1b;\n" \
	  "nop;\n" \
	  ,0x80000);
}
else
{
	  uififo_raddr = 0;
	  tgt_exec(
	  ".set noat;\n" 
	  ".set mips3;\n" 
	  "1:\n" 
	  "mtc0    $15, $31;\n" 
	  "lui    $15,0xFF20;\n" 
	  "sw    $1, 0x1fc($15);\n" 
	  "sw    $2, 0x1fc($15);\n" 

	  "mfc0  $2, c0_index;\n"
	  "sw	 $2, 0x1fc($15);\n"

	  "mfc0  $2, c0_entryhi;\n"
	  "sw	 $2, 0x1fc($15);\n"

	  "mfc0  $2, c0_entrylo0;\n"
	  "sw	 $2, 0x1fc($15);\n"


	  "mfc0  $2, c0_entrylo1;\n"
	  "sw	 $2, 0x1fc($15);\n"

	  "mfc0  $2, c0_entryhi;\n"
	  "andi   $2,0xff;\n"
	  "lw    $1, ($15);\n" 
	  "ori $1,0x1fff;\n"
	  "xori $1,0x1fff;\n"
	  "or $2,$1;\n"
	  "mtc0  $2,c0_entryhi;\n"
	  "tlbp;\n"

	  "mfc0  $2, c0_index;\n" /*not in tlb,just ignore*/
	  "sw	$2, 0x1f8($15);\n"
	  "slt   $2,$2,$0;\n"
	  "bnez $2,2f;\n"
	  "nop;\n"
	  "tlbr;\n"
	  "mfc0 $2,c0_entrylo0;\n" 
	  "sw	$2, 0x1f8($15);\n"
	  "and $1,$2,2;\n"
	  "beqz $1,2f;\n"
	  "nop;\n"
	  "sw $2, 0x1fc($15);\n"
	  "or $2,4;\n"
	  "mtc0 $2,c0_entrylo0;\n"
	  "tlbwi;\n"

	  "lw    $2, ($15);\n" 
	  "lw    $1, 4($15);\n" 
	  "sw    $1, ($2);\n" 
	  "cache    0x15,($2);\n" 
	  "cache    1, 0($2);\n" 
	  "cache    1, 1($2);\n" 
	  "cache    1, 2($2);\n" 
	  "cache    1, 3($2);\n" 
	  "cache    0x10,($2);\n" 
	  "cache    0, ($2);\n" 
	  "cache    0, 1($2);\n" 
	  "cache    0, 2($2);\n" 
	  "cache    0, 3($2);\n" 

	  "lw $2, 0x1fc($15);\n"
	  "mtc0  $2, c0_entrylo0;\n"
	  "tlbwi;\n"

	  "2:lw $2, 0x1fc($15);\n"	  
	  "mtc0  $2, c0_entrylo1;\n"

	  "lw $2, 0x1fc($15);\n"	  
	  "mtc0  $2, c0_entrylo0;\n"

	  "lw $2, 0x1fc($15);\n"	  
	  "mtc0  $2, c0_entryhi;\n"

	  "lw $2, 0x1fc($15);\n"	  
	  "mtc0  $2, c0_index;\n"

	  "lw    $2, 0x1fc($15);\n" 
	  "lw    $1, 0x1fc($15);\n" 
	  "mfc0    $15, $31;\n" 
	  "b 1b;\n" 
	  "nop;\n" 
	  ,0x80000);
	printf("index=%x,c0_datalo0=%x\n",ui_fifomem[0],ui_fifomem[1]);
}

          swb_array[ui_index].uc_enabled = 0x00;

          return(1);
        }
      }
    }
    
    // Never found the breakpoint
    printf("BREAKPOINT NOT FOUND!\n");
    return(-1);
}

//
// fGDBRDP_HandleHardwareBreakpoint
//
// Sets or clears a software breakpoint
//
int  fGDBRDP_HandleSoftwareBreakpoint (char *pc_command, unsigned int ui_size)
{
  unsigned int ui_index;
  unsigned int ui_address;
  
  // Calculate the address now - Its the same location for all packets.
  // Packet is formated <z/Z><0/1><,><ADDR><,><LENGTH>
  ui_address = 0x00;
  for (ui_index=3; *(pc_command+ui_index) != ','; ui_index++)
  {
    ui_address = ui_address << 4;
    ui_address = ui_address + fGDBRDP_ConvertNibble(*(pc_command+ui_index));
  }

  // Make sure address is 32bit aligned
  if (0x00 != (ui_address % 4))
  {
    // Unaligned, not allowed
    printf("BREAKPOINT NOT ALIGNED!\n");
    return(-1);
  }

  // Check to see if it is a SET or REMOVE breakpoint command
  if ('Z' == *pc_command) // Then its a set command
	return set_soft_breakpoint(ui_address);
  else if ('z' == *pc_command) // Then its a remove command
	return unset_soft_breakpoint(ui_address);
  else // Should never reach this
  {
    // Error
    return(-1);
  }
  
  // Should never reach here
  return(-1);
}

static int cmd_hb(int argc,char **argv)
{
if(argv[0][0] != 'u')
 set_inst_breakpoint(strtoul(argv[1],0,0));
else
 unset_inst_breakpoint(strtoul(argv[1],0,0));
return 0;
}

static int cmd_b(int argc,char **argv)
{
if(argv[0][0] != 'u')
 set_soft_breakpoint(strtoul(argv[1],0,0));
else
 unset_soft_breakpoint(strtoul(argv[1],0,0));
return 0;
}

static int cmd_watch(int argc,char **argv)
{
int size;
if(argc > 3) size = strtoul(argv[2],0,0);
else size = 4;

if(argv[0][0] != 'u')
 set_data_breakpoint(strtoul(argv[1],0,0),size,ACTION_WRITE);
else
 unset_data_breakpoint(strtoul(argv[1],0,0));
return 0;
}

static int cmd_rwatch(int argc,char **argv)
{
int size;
if(argc > 3) size = strtoul(argv[2],0,0);
else size = 4;

if(argv[0][0] != 'u')
 set_data_breakpoint(strtoul(argv[1],0,0),size,ACTION_READ);
else
 unset_data_breakpoint(strtoul(argv[1],0,0));
return 0; 
}

static int cmd_awatch(int argc,char **argv)
{
int size;
if(argc > 3) size = strtoul(argv[2],0,0);
else size = 4;

if(argv[0][0] != 'u')
 set_data_breakpoint(strtoul(argv[1],0,0),size,ACTION_ALL);
else
 unset_data_breakpoint(strtoul(argv[1],0,0));
return 0; 
}

extern int check_ejtag(char *buf,int *debug_reg);

static int cmd_si(int argc,char **argv)
{
extern unsigned int aui_singlestepclear_code[];
int count=1;
int debug_reg = 0;
char buf[256];
if(argc > 1) count = strtoul(argv[1],0,0);

if(argv[0][0] != 'u')
{
      fMIPS32_ExecuteDebugModule(aui_singlestepset_code,0x80000,1);
	while(count--)
	{
	  jtag_quit();
	  if(count)
	  {
	  do
	  check_ejtag(buf,&debug_reg);
	  while ((debug_reg & dMIPS32_DEBUG_SINGLE_STEP_BKPT) == 0);
	  printf("%s\n",buf);
	  }
	}
}
else
      fMIPS32_ExecuteDebugModule(aui_singlestepclear_code,0x80000,1);
return 0;
}

static int cmd_taccess(int argc,char **argv)
{
help_access(strtoul(argv[1],0,0),-1,0,0);
return 0;
}

mycmd_init(taccess,cmd_taccess,"b addr","check address access");

mycmd_init(b,cmd_b,"b addr","set softbeakpoint on addr");
mycmd_init(unb,cmd_b,"unb addr","unset softbeakpoint on addr");
mycmd_init(hb,cmd_hb,"bh addr","set hardbeakpoint on addr");
mycmd_init(unhb,cmd_hb,"unbh addr","unset hardbeakpoint on addr");
mycmd_init(watch,cmd_watch,"watch addr [size]","set databeakpoint on addr for write access");
mycmd_init(unwatch,cmd_watch,"unwatch addr [size]","unset databeakpoint on addr for write access");
mycmd_init(rwatch,cmd_rwatch,"rwatch addr [size]","set databeakpoint on addr for read access");
mycmd_init(unrwatch,cmd_rwatch,"unrwatch addr [size]","unset databeakpoint on addr for read access");
mycmd_init(awatch,cmd_awatch,"awatch addr [size]","set databeakpoint on addr for all access");
mycmd_init(unawatch,cmd_awatch,"uawatch addr [size]","unset databeakpoint on addr for all access");
mycmd_init(si,cmd_si,"si","single step");
mycmd_init(unsi,cmd_si,"unsi","unset single step");

