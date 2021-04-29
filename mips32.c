//
// MIPS32.C
//
// Target CPU specific interface code for MIPS32 core
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
//#include <signal.h>
#include "mips32.h"
#include "jtag.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "ejtag.h"
#include <time.h>
#include <string.h>
#include "singlestepclear.h"
#include <sys/time.h>
#include <signal.h>
#include <cmdparser.h>
extern int log_disasm;

void * md_disasm ( char *dest, void *addr, unsigned int inst);
//#include <curses.h>

//
// PROTOTYPES
//
#ifndef EXEC_DEBUG_Z
#define EXEC_DEBUG_Z
#endif

//#undef EXEC_DEBUG_Z
static void         fMIPS32_HandleWrite  (unsigned int ui_address, unsigned int ui_data,int accesstype);
static unsigned int fMIPS32_HandleRead   (unsigned int ui_address);

//
// GLOBALS
//
unsigned int  ui_ejtag_state;
unsigned int  ui_last_signal;
unsigned int  ui_datamem[0x80];
unsigned int  *ui_fifomem;
unsigned int  ui_fifomem_size=0x2000;
unsigned int  uififo_raddr,uififo_waddr;
#define CP0_DEBUG_SST_EN (1<<8)             /* single step enable */

unsigned int  context_reg[dMIPS32_NUM_DEBUG_REGISTERS]={0};
//
// MIPS32_Init
//
// Performs intialization specific to the target CPU MIPS32
//
int Ejtag_boot_flag=0;
static int  fMIPS32_inited=0;
int enable_timer;
unsigned int timer_val=1000;
int ejtag_in_debug_mode = 0;

int check_ejtag(char *buf,int *debug_reg)
{
	unsigned int ui_result,debug_reg1;
	char *p = buf;
	*p=0;

	ui_result = dMIPS32_ECR_CPU_RESET_OCCURED | dMIPS32_ECR_ACCESS_PENDING |
		dMIPS32_ECR_PROBE_ENABLE | dMIPS32_ECR_PROBE_VECTOR;

	fJTAG_Instruction(dJTAG_REGISTER_CONTROL);
	ui_result = fJTAG_Data(ui_result);

	// Check for debug mode
	if (ui_result & dMIPS32_ECR_IN_DEBUG_MODE)
	{

		// Read out the debug register to determine the cause
		static int codebuf[100] =tgt_compile(
				"1:\n" 
				"mtc0	ra, $31;\n" 
				"bal 2f;\n"
				"nop;\n"
				".word 0;\n"  /*v0*/
				".word 0;\n"  /*debug reg*/
				".word 0;\n"  /*depc*/
				".word 0;\n"  /*inst*/
				"2:sw	v0,  0(ra);\n" 
				"mfc0	v0,  $23;\n" 
				"sw	v0,  4(ra);\n" 
				"mfc0	v0, $24;\n" 
				"sw	v0,  8(ra);\n" 
				"mfc0	v0, $24;\n" 
				"lw		v0,(v0)\n"
				"sw	v0,  12(ra);\n" 
				"lw	v0,  0(ra);\n" 
				"mfc0	ra, $31 ;\n" 
				"b 1b;\n" 
				"nop;\n" 
				);

		fMIPS32_ExecuteDebugModule(codebuf,0x8000,1);
		debug_reg1 = codebuf[4];

		p += sprintf(p,"debug:%08x #",debug_reg1);

		if (debug_reg1 & dMIPS32_DEBUG_SW_BKPT)
			p += sprintf(p,"i");

		if (debug_reg1 & dMIPS32_DEBUG_HW_INST_BKPT)
			p += sprintf(p,"I ");

		if (debug_reg1 & dMIPS32_DEBUG_HW_DATA_LD_BKPT)
			p += sprintf(p,"l");

		if (debug_reg1 & dMIPS32_DEBUG_HW_DATA_ST_BKPT)
			p += sprintf(p,"s");

		if (debug_reg1 & dMIPS32_DEBUG_HW_DATA_LD_IMP_BKPT)
			p += sprintf(p,"L");

		if (debug_reg1 & dMIPS32_DEBUG_HW_DATA_ST_IMP_BKPT)
			p += sprintf(p,"S");

		if(debug_reg1 & (dMIPS32_DEBUG_HW_DATA_LD_BKPT|dMIPS32_DEBUG_HW_DATA_ST_BKPT|dMIPS32_DEBUG_HW_DATA_LD_IMP_BKPT|dMIPS32_DEBUG_HW_DATA_ST_IMP_BKPT))
		{
		/*clear data breakpoint*/
			tgt_exec(
					"1:\n" 
					"mtc0    $15, $31;\n" 
					"lui    $15,0xFF30;\n" 
					"sw $0,0x2000($15);\n"
					"mfc0    $15, $31;\n" 
					"b 1b;\n" \
					"nop;\n" \
					,0x80000);
		}

		if (debug_reg1 & dMIPS32_DEBUG_SINGLE_STEP_BKPT)
			p += sprintf(p," step");

		if (debug_reg1 & dMIPS32_DEBUG_JTAG_BKPT)
			p += sprintf(p,"user ");
		if (debug_reg1 & 0x100000)
			p += sprintf(p," exp");

		p += sprintf(p,"\n");
		md_disasm (p, codebuf[5], codebuf[6]);
	*debug_reg = debug_reg1;

	}
	else 
	{
		p += sprintf(p,"not in debug mode");
	*debug_reg = 0;
	}

	
	return ui_result;
}

unsigned int debug_reg = 0;
static void timer_handler(int sig)
{
	static char buf[256];
	static int ui_result;
	int debug_reg1;

	ejtag_in_debug_mode = !!(ui_result & dMIPS32_ECR_IN_DEBUG_MODE);
	if(enable_timer == 1)
	{
		check_ejtag(buf,&debug_reg1);

	if(debug_reg1 != debug_reg )
		printf("\n%s\n",buf);
	debug_reg = debug_reg1;
	}
}

static int set_timer(int ms);

void fMIPS32_Init(void)
{
if(fMIPS32_inited)return;

  fJTAG_Init();

  fMIPS32_inited=1;

  signal(SIGALRM, timer_handler);

  if(enable_timer != -1)
    set_timer(timer_val);

  return;
}

static int set_timer(int ms)
{
	  struct itimerval value,oval;
	  value.it_interval.tv_sec = ms/1000;
	  value.it_interval.tv_usec = (ms%1000)*1000;
	  value.it_value = value.it_interval;
	  setitimer(ITIMER_REAL,&value,&oval);
}

static int cmd_timer(int argc,char **argv)
{
	if(argc != 2)return -1;
	set_timer(strtoul(argv[1],0,0));
	return 0;
}

mycmd_init(timer,cmd_timer,"timer ms","set timer");

//
// fMIPS32_ExecuteDebugModule
//
// This will take an array of 32 bit values and feed them into the 
// processor via the EJTAG port as instructions.
//
  unsigned int ui_stack[32];
  unsigned int ui_stack_index=0;

#define STATE_CONNECTING 0
#define STATE_WAITING 1
#define STATE_RUN 2
//#define USE_FASTDATA

unsigned int force_acc()
{
	unsigned int ui_result;
  	ui_result = dMIPS32_ECR_CPU_RESET_OCCURED | dMIPS32_ECR_ACCESS_PENDING |
              dMIPS32_ECR_PROBE_ENABLE | dMIPS32_ECR_PROBE_VECTOR |
              dMIPS32_ECR_EJTAG_BREAK;
	fJTAG_Instruction(dJTAG_REGISTER_CONTROL);
	ui_result = fJTAG_Data(ui_result);
	return ui_result;
}

unsigned int get_acc(unsigned int *op,int *address,int *data)
{
	unsigned int ui_result;
	unsigned int ui_address;
	unsigned int ui_data;
	int ret;
	int connecting = 0;

	// Read the control register.  Make sure an access is requested, then 
	// do it.
	ui_result = dMIPS32_ECR_CPU_RESET_OCCURED | dMIPS32_ECR_ACCESS_PENDING |
		dMIPS32_ECR_PROBE_ENABLE | dMIPS32_ECR_PROBE_VECTOR;

	fJTAG_Instruction(dJTAG_REGISTER_CONTROL);
	ui_result = fJTAG_Data(ui_result);

	*op = ui_result;

	if(!(ui_result&dMIPS32_ECR_IN_DEBUG_MODE))  {
		return 0;
	}


	if (!(ui_result & dMIPS32_ECR_ACCESS_PENDING))
	{
		return 1;
	}

	fJTAG_Instruction(dJTAG_REGISTER_ADDRESS);
	ui_address = fJTAG_Data(0x00)|0xff000000;
	fJTAG_Data(ui_address);

	*address = ui_address;

	fJTAG_Instruction(dJTAG_REGISTER_DATA);
	ui_data = fJTAG_Data(0x00);
	fJTAG_Data(ui_data);

	*data = ui_data;
	return 3;
}

unsigned int feed_acc(int data)
{
	int ui_result,ui_data = data;
	fJTAG_Instruction(dJTAG_REGISTER_DATA);
	ui_data = fJTAG_Data(ui_data);
	// Clear the access pending bit (let the processor eat!)
	ui_result = dMIPS32_ECR_CPU_RESET_OCCURED | dMIPS32_ECR_PROBE_ENABLE | 
		dMIPS32_ECR_PROBE_VECTOR;

	fJTAG_Instruction(dJTAG_REGISTER_CONTROL);
	ui_result = fJTAG_Data(ui_result);
	return ui_result;
}

unsigned int wait_address(unsigned int address,int write,int waitacc)
{
  unsigned int ui_result;
  unsigned int ui_address;
  unsigned int ui_data;
  int accesstype;
  while (1)
  {
    // Read the control register.  Make sure an access is requested, then 
    // do it.
    ui_result = dMIPS32_ECR_CPU_RESET_OCCURED | dMIPS32_ECR_ACCESS_PENDING |
                dMIPS32_ECR_PROBE_ENABLE | dMIPS32_ECR_PROBE_VECTOR;
  
    fJTAG_Instruction(dJTAG_REGISTER_CONTROL);
    ui_result = fJTAG_Data(ui_result);
    accesstype=(ui_result>>29)&3;
	if(!(ui_result & dMIPS32_ECR_IN_DEBUG_MODE))
	{
      fprintf(stderr,"not in debug mode!\n");
    //  	  printf("not in debug mode!\n");
	  return 0;
	}
    
    if (!(ui_result & dMIPS32_ECR_ACCESS_PENDING))
    {
	  if(!waitacc)return -1;
	  continue;
    }

	
    fJTAG_Instruction(dJTAG_REGISTER_ADDRESS);
    ui_address = fJTAG_Data(0x00)|0xff000000;
    fJTAG_Instruction(dJTAG_REGISTER_DATA);
    ui_data = fJTAG_Data(0x00);
//	printf("ui_address =%08x\n",ui_address);
    if (ui_result & dMIPS32_ECR_ACCESS_RW) // Bit set for a WRITE
    {
        // Clear the access pending bit (let the processor eat!)
        ui_result = dMIPS32_ECR_CPU_RESET_OCCURED | dMIPS32_ECR_PROBE_ENABLE | 
                    dMIPS32_ECR_PROBE_VECTOR;
      	if (ui_address < 0xFF200200)
        fMIPS32_HandleWrite(ui_address, ui_data, accesstype);
  	
		if(address!=ui_address || !write || ui_address < 0xFF200200)
		{
        fJTAG_Instruction(dJTAG_REGISTER_CONTROL);
        ui_result = fJTAG_Data(ui_result);
		}

		if(address==ui_address && write)break;
	}
	else
	{
        // Clear the access pending bit (let the processor eat!)
        ui_result = dMIPS32_ECR_CPU_RESET_OCCURED | dMIPS32_ECR_PROBE_ENABLE | 
                    dMIPS32_ECR_PROBE_VECTOR;

	  fJTAG_Instruction(dJTAG_REGISTER_DATA);
      if (ui_address >= 0xFF200200)
		ui_data = 0;
	  else
      	ui_data = fMIPS32_HandleRead(ui_address);
        ui_data = fJTAG_Data(ui_data);

		if(address!=ui_address || write || ui_address < 0xFF200200)
		{
        fJTAG_Instruction(dJTAG_REGISTER_CONTROL);
        ui_result = fJTAG_Data(ui_result);
		}

		if(address==ui_address && !write)break;
	}

}

    fJTAG_Instruction(dJTAG_REGISTER_ADDRESS);
    fJTAG_Data(ui_address);

	return 0;
  

}

unsigned int fMIPS32_ExecuteDebugModule_safe(unsigned int *pui_module,unsigned int len,unsigned int flags)
{
	/*safe exec jtag code,if exception report,revert depc*/
	unsigned int depc;
	unsigned int volatile codebuf[] = tgt_compile(
			"1:mtc0 ra,$31;\n"
			"bal 2f;\n"
			"nop;\n"
			".space 12;\n"
			"2:sw v0,(ra);\n"
			"mfc0 v0,$24;\n"
			"sw v0,4(ra);\n"
			"lw v0,(ra);\n"
			"mfc0 ra,$31;\n"
			"b 1b;\n"
			"nop;\n"
			);	

	volatile unsigned int codebuf1[] = tgt_compile(
			"1:mtc0 ra,c0_desave;\n"
			"bal 2f;\n"
			"nop;\n"
			".space 16;\n"
			"2:sw v0,(ra);\n"
			"sw v1,4(ra);\n"
			"mfc0 v0,c0_debug;\n"
			"sw v0,8(ra);\n"
			"lui v1,1<<4;\n"
			"and v1,v0;\n"
			"beqz v1,3f;\n"
			"nop;\n"
			"lw v0,12(ra);\n"
			"mtc0 v0,c0_depc;\n"
			"3:lw v1,4(ra);\n"
			"lw v0,(ra);\n"
			"mfc0 ra,c0_desave;\n"
			"b 1b;\n"
			"nop;\n"
			);	

	fMIPS32_ExecuteDebugModule(codebuf,0x8000,1);
	depc = codebuf[4];
	fMIPS32_ExecuteDebugModule(pui_module,len,flags);
	codebuf1[6] = depc;
	fMIPS32_ExecuteDebugModule(codebuf1,0x8000,1);
	if(codebuf1[5]&(1<<20))
		printf("exception in ejtag code!!!\n");

	return (codebuf1[5]&(1<<20))?1:0;
}

unsigned int fMIPS32_ExecuteDebugModule(unsigned int *pui_module,unsigned int len,unsigned int flags)
{
  unsigned int ui_result;
  unsigned int ui_address;
  unsigned int ui_data;
  unsigned int ui_finished;
  unsigned int start_address;
  unsigned int end_address;
  unsigned int state=STATE_CONNECTING;
  unsigned int download_size=0;
  unsigned int init_do=0;
    //time_t *time_start=malloc(sizeof(time_t));
    //time_t *time_end=malloc(sizeof(time_t));
    time_t time_start;
    time_t time_end;
    time_t  time_deta=0;
	int runonce=flags&1;
	int waitacc=!(flags&2);
	int allowbefore=flags&4;
    

  fprintf(stderr,"JTAGEMUL: Executing a module.\n");
  fMIPS32_Init();

  fprintf(stderr,"connecting...\n");
#if 0
  if(!Ejtag_boot_flag)
  {
        fprintf(stderr,"===============dJTAG_REGISTER_EJTAG_BOOT ==========\n");
  	Ejtag_boot_flag=1;	
  	fJTAG_Instruction(dJTAG_REGISTER_EJTAG_BOOT);
  }
         fprintf(stderr,"dJTAG_REGISTER_EJTAG_BOOT done\n");
#endif
  
while(1)
{
  ui_result = dMIPS32_ECR_CPU_RESET_OCCURED|dMIPS32_ECR_ACCESS_PENDING|dMIPS32_ECR_PROBE_ENABLE | dMIPS32_ECR_PROBE_VECTOR ;
  fJTAG_Instruction(dJTAG_REGISTER_CONTROL);
  ui_result = fJTAG_Data(ui_result);

if(!(ui_result&dMIPS32_ECR_IN_DEBUG_MODE))  {
    ui_result = dMIPS32_ECR_PROBE_ENABLE | dMIPS32_ECR_PROBE_VECTOR |dMIPS32_ECR_EJTAG_BREAK;
  fJTAG_Instruction(dJTAG_REGISTER_CONTROL);
  ui_result = fJTAG_Data(ui_result);
  continue;
  }
  fprintf(stderr,"connected\n");
  state=STATE_RUN;
  break;
}

    fJTAG_Instruction(dJTAG_REGISTER_ADDRESS);
    start_address = fJTAG_Data(0x00)|0xff000000;
	end_address=start_address+len;
    ui_finished    = 0x00;
//	printf ("lxy: start_address = 0x%x !\n", start_address);
	  fprintf(stderr,"start_address=%x,end_address=%x\n",start_address,end_address);


  // Feed that chip
  while (1)
  {
    // Read the control register.  Make sure an access is requested, then 
    // do it.
    ui_result = dMIPS32_ECR_CPU_RESET_OCCURED | dMIPS32_ECR_ACCESS_PENDING |
                dMIPS32_ECR_PROBE_ENABLE | dMIPS32_ECR_PROBE_VECTOR;
  
    fJTAG_Instruction(dJTAG_REGISTER_CONTROL);
    ui_result = fJTAG_Data(ui_result);
	if(!(ui_result & dMIPS32_ECR_IN_DEBUG_MODE))
	{
      fprintf(stderr,"not in debug mode!\n");
    //  	  printf("not in debug mode!\n");
	  return 0;
	}
    
    if (!(ui_result & dMIPS32_ECR_ACCESS_PENDING))
    {
	  if(!waitacc)return -1;
	  if(state!=STATE_WAITING)printf("waiting\n");
	  state=STATE_WAITING;
	  continue;
    }
	state=STATE_RUN;

	
    fJTAG_Instruction(dJTAG_REGISTER_ADDRESS);
    ui_address = fJTAG_Data(0x00)|0xff000000;
//	printf ("lxy: ui_address = 0x%x !\n", ui_address);
   
     if(ui_address<0xff200000|| ui_address>=0xff300000)
     {
     fprintf(stderr,"JTAGEMUL: %s to unknown location 0x%08X.\n", (ui_result & dMIPS32_ECR_ACCESS_RW)?"write":"read",ui_address);
     return 0;
     }

   //fprintf(stderr,"%s ui_address=%x,accesstype=%x\n",(ui_result & dMIPS32_ECR_ACCESS_RW)?"write":"read",ui_address,(ui_result>>29)&3);
    // Check for read or write
    if (ui_result & dMIPS32_ECR_ACCESS_RW) // Bit set for a WRITE
    {
      unsigned data0,data1,mask;
      int accesstype=(ui_result>>29)&3;
      // Read the data out
#ifdef USE_FASTDATA
      fJTAG_Instruction(dJTAG_REGISTER_FASTDATA);
      ui_data = fJTAG_FastData(0x00);
#else
	  fJTAG_Instruction(dJTAG_REGISTER_DATA);
      ui_data = fJTAG_Data(0x00);
      // Clear the access pending bit (let the processor eat!)
      ui_result = dMIPS32_ECR_CPU_RESET_OCCURED | dMIPS32_ECR_PROBE_ENABLE | 
                  dMIPS32_ECR_PROBE_VECTOR;
  
      fJTAG_Instruction(dJTAG_REGISTER_CONTROL);
      ui_result = fJTAG_Data(ui_result);
#endif

	if(ui_address >= 0xFF200200)
	{
       if(ui_address>=end_address|| ( ui_address<start_address && !allowbefore))return ui_address;
       data0=pui_module[(ui_address - start_address) / 4];
       data1=ui_data;

        switch(accesstype)
		{
			case 0:
			mask=0xff<<((ui_address&3)*8); break;
			case 1:
			mask=0xffff<<(((ui_address>>1)&1)*16); break;
			case 2: mask=0xffffffff;break;
			default: mask=0xffffff<<((ui_address&1)*8);break;
		}

		ui_data=(data1&mask)|(data0&~mask);
	}

      // Processor is writing to us
      if (ui_address >= 0xFF200200)
        pui_module[(ui_address - start_address) / 4]=ui_data;
      else
      {
        // Data out to the real world
        //fprintf(stderr,"JTAGEMUL: Write 0x%08X to address 0x%08X.\n", ui_data, ui_address);
        fMIPS32_HandleWrite(ui_address, ui_data, accesstype);
      }
    }
    else
    {
      // Check to see if its reading at the debug vector.  The first pass through
      // the module is always read at the vector, so the first one we allow.  When
      // the second read from the vector occurs we are done and just exit.
      if ((start_address == ui_address)&&runonce)
      {
        if (ui_finished++) // Allows ONE pass
        {
		ui_finished=0;
         // fprintf(stderr,"JTAGEMUL: Finished module.\n");
          return ui_address;
        }
      }

      // Processor is reading from us
      if (ui_address >= 0xFF200200)
      {
	    if(ui_address>=end_address|| ( ui_address<start_address && !allowbefore))return ui_address;
        // Reading an instruction from our module

        // Fetch the instruction from the module
        //fprintf(stderr,"JTAGEMUL: Instruction read at 0x%08X ", ui_address); fflush(stdout);
        ui_data = (ui_address - start_address) / 4;
        //fprintf(stderr,"offset -> %04d ", ui_data); fflush(stdout);
        ui_data = *(unsigned int *)(pui_module + ui_data);
        //fprintf(stderr,"data -> 0x%08X\n", ui_data); fflush(stdout);
	
	if(log_disasm)
	{
	char buf[256];
	md_disasm (buf, ui_address, ui_data);
	fprintf(stderr,"%s\n",buf);
	}

        // Send the data out
#ifdef USE_FASTDATA
        fJTAG_Instruction(dJTAG_REGISTER_FASTDATA);
        ui_data = fJTAG_Data(ui_data);
#else
	  fJTAG_Instruction(dJTAG_REGISTER_DATA);
      ui_data = fJTAG_Data(ui_data);
        // Clear the access pending bit (let the processor eat!)
        ui_result = dMIPS32_ECR_CPU_RESET_OCCURED | dMIPS32_ECR_PROBE_ENABLE | 
                    dMIPS32_ECR_PROBE_VECTOR;
  
        fJTAG_Instruction(dJTAG_REGISTER_CONTROL);
        ui_result = fJTAG_Data(ui_result);
#endif
      }
      else
      {
      	// A read from dmseg virtual registers
      	ui_data = fMIPS32_HandleRead(ui_address);
      

        //fprintf(stderr,"JTAGEMUL: Read address 0x%08X result = 0x%08X.\n", ui_address, ui_data);

        // Send the data out
#ifdef USE_FASTDATA
        fJTAG_Instruction(dJTAG_REGISTER_FASTDATA);
        ui_data = fJTAG_Data(ui_data);
#else
	  fJTAG_Instruction(dJTAG_REGISTER_DATA);
      ui_data = fJTAG_Data(ui_data);
        // Clear the access pending bit (let the processor eat!)
        ui_result = dMIPS32_ECR_CPU_RESET_OCCURED | dMIPS32_ECR_PROBE_ENABLE | 
                    dMIPS32_ECR_PROBE_VECTOR;
  
        fJTAG_Instruction(dJTAG_REGISTER_CONTROL);
        ui_result = fJTAG_Data(ui_result);
#endif
      }
    }    
  }  
}

unsigned int fMIPS32_Transfer(unsigned int *pui_module,unsigned int len)
{
  unsigned int ui_result;
  unsigned int ui_address;
  unsigned int ui_data;
  unsigned int ui_finished;
  unsigned int start_address;
  unsigned int end_address;
  unsigned int state=STATE_CONNECTING;
  int i;

  fMIPS32_Init();

  
while(1)
{
  ui_result = dMIPS32_ECR_CPU_RESET_OCCURED|dMIPS32_ECR_ACCESS_PENDING|dMIPS32_ECR_PROBE_ENABLE | dMIPS32_ECR_PROBE_VECTOR ;
  fJTAG_Instruction(dJTAG_REGISTER_CONTROL);
  ui_result = fJTAG_Data(ui_result);

if(!(ui_result&dMIPS32_ECR_IN_DEBUG_MODE))  {
    ui_result = dMIPS32_ECR_PROBE_ENABLE | dMIPS32_ECR_PROBE_VECTOR |dMIPS32_ECR_EJTAG_BREAK;
  fJTAG_Instruction(dJTAG_REGISTER_CONTROL);
  ui_result = fJTAG_Data(ui_result);
  continue;
  }
  fprintf(stderr,"connected\n");
  state=STATE_RUN;
  break;
}


  // Feed that chip
  for(i=0;len;)
  {
    // Read the control register.  Make sure an access is requested, then 
    // do it.
    ui_result = dMIPS32_ECR_CPU_RESET_OCCURED | dMIPS32_ECR_ACCESS_PENDING |
                dMIPS32_ECR_PROBE_ENABLE | dMIPS32_ECR_PROBE_VECTOR;
  
    fJTAG_Instruction(dJTAG_REGISTER_CONTROL);
    ui_result = fJTAG_Data(ui_result);
	if(!(ui_result & dMIPS32_ECR_IN_DEBUG_MODE))
	{
      fprintf(stderr,"not in debug mode!\n");
	  return 0;
	}
    
    if (!(ui_result & dMIPS32_ECR_ACCESS_PENDING))
    {
	  if(state!=STATE_WAITING)printf("waiting\n");
	  state=STATE_WAITING;
	  continue;
    }
	state=STATE_RUN;

#if 0
    fJTAG_Instruction(dJTAG_REGISTER_ADDRESS);
    start_address = fJTAG_Data(0x00)|0xff000000;
    printf("start_address=0x%x\n",start_address);
#endif
	
    if (ui_result & dMIPS32_ECR_ACCESS_RW) // Bit set for a WRITE
    {
      // Read the data out
#ifdef USE_FASTDATA
      fJTAG_Instruction(dJTAG_REGISTER_FASTDATA);
      ui_data = fJTAG_FastData(0x00);
#else
	  fJTAG_Instruction(dJTAG_REGISTER_DATA);
      ui_data = fJTAG_Data(0x00);
      // Clear the access pending bit (let the processor eat!)
      ui_result = dMIPS32_ECR_CPU_RESET_OCCURED | dMIPS32_ECR_PROBE_ENABLE | 
                  dMIPS32_ECR_PROBE_VECTOR;
  
      fJTAG_Instruction(dJTAG_REGISTER_CONTROL);
      ui_result = fJTAG_Data(ui_result);
#endif

        pui_module[i++]=ui_data;
	len-=4;
    }
    else
    {
        ui_data = pui_module[i++];
	len -= 4;
        // Send the data out
#ifdef USE_FASTDATA
        fJTAG_Instruction(dJTAG_REGISTER_FASTDATA);
        ui_data = fJTAG_Data(ui_data);
#else
	  fJTAG_Instruction(dJTAG_REGISTER_DATA);
      ui_data = fJTAG_Data(ui_data);
        // Clear the access pending bit (let the processor eat!)
        ui_result = dMIPS32_ECR_CPU_RESET_OCCURED | dMIPS32_ECR_PROBE_ENABLE | 
                    dMIPS32_ECR_PROBE_VECTOR;
  
        fJTAG_Instruction(dJTAG_REGISTER_CONTROL);
        ui_result = fJTAG_Data(ui_result);
#endif
      }
    }    
}

#if 1

#define PACK_SIZE 0x100000
unsigned int fMIPS32_USB_DownloadModule(unsigned int *pui_module,unsigned int len)
{
  unsigned int ui_result;
  unsigned int ui_address;
  unsigned int ui_data;
  unsigned int ui_finished;
  unsigned int start_address;
  unsigned int end_address;
  unsigned int download_ram_size=0;
  unsigned int init_do=0;
  unsigned int pack_num=0;
  unsigned int pack_num_1=0;
  unsigned int i=0;
    //time_t *time_start=malloc(sizeof(time_t));
    //time_t *time_end=malloc(sizeof(time_t));
    time_t time_start;
    time_t time_end;
    time_t  time_deta=0;
    

  fprintf(stderr,"JTAGEMUL: Executing a module.\n");
  fMIPS32_Init();


  fprintf(stderr,"connecting...\n");
#if 1
        if(!init_do)
        {
                time_start = (time(&time_start))&0xffffffff;
                printf("time_start : %x \n",time_start);
                init_do = 1;
                download_ram_size=0;
        }
        else
        {
                if(!(download_ram_size%0x10000))
                {
                time_end = (time(&time_end))&0xffffffff;
                printf("time_end : %x \n",time_end);
                time_deta= time_end - time_start;
		if(time_deta == 0)
			time_deta++ ;
                printf("time : 0x%x, download_size : %x, download rate=%d B/S\n",time_deta,download_ram_size,((download_ram_size/time_deta)));
                }
        }
#endif
	pack_num = len/PACK_SIZE ;
	printf("len : %d , pack_num : %d \n",len,pack_num);
	if(len % PACK_SIZE)
		pack_num_1=1;
        //zgj-20090627  fJTAG_Instruction(dJTAG_REGISTER_DBTX);
        fJTAG_Instruction(dJTAG_REGISTER_FASTDATA);
         // ui_data = fJTAG_DBTX(ui_data);
#if 1
	for(i=0;i<pack_num;i++)//20090628
	{ //20090628
		printf("pack_num : %d \n",i);
		printf("pui_module addr 0x%x :%x \n", pui_module,*pui_module);
         	fJTAG_DBTX(PACK_SIZE,pui_module+i*(PACK_SIZE/4));
         	//fJTAG_DBTX(len,pui_module);
	}//20090628
	if(pack_num_1)
	{	
		printf("LAST pui_module addr 0x%x :%x  i= %d \n", pui_module+i*PACK_SIZE,*(pui_module+i*PACK_SIZE/4),i);
		fJTAG_DBTX(len - i*PACK_SIZE , pui_module + i*(PACK_SIZE/4));
	}
#else
	printf("pui_module addr 0x%x :%x \n", pui_module,*pui_module);
        fJTAG_DBTX(len,pui_module);
#endif    
         download_ram_size = len;
                time_end = (time(&time_end))&0xffffffff;
                printf("=<<<==time_end : %x \n",time_end);
                time_deta= time_end - time_start;
		if(time_deta == 0)
			time_deta++ ;
                printf("=<<<==time : %d, download_size : %x, download rate=%d B/S\n",time_deta,download_ram_size,((download_ram_size/time_deta)));
}
#endif




//
// fMIPS32_HandleWrite
//
// This routine takes the data and address from a write and puts it in the 
// correct location.
//
static void fMIPS32_HandleWrite(unsigned int ui_address, unsigned int ui_data, int accesstype)
{
  unsigned int data0,data1,mask;
  int index;
  switch(ui_address)
  {
    case dMIPS32_PSEUDO_SERIAL_ADDR:   /*simple put char*/
	    putchar(ui_data);
	    break;
    case dMIPS32_PSEUDO_SERIAL_ADDR+1:  /*printf predefined strings with argument*/
	    index = (ui_data>>8)&0xff;
	    printf(ui_datamem[index],ui_datamem[index+1],ui_datamem[index+2],ui_datamem[index+3],ui_datamem[index+4],ui_datamem[index+5],ui_datamem[index+6],ui_datamem[index+7],ui_datamem[index+8],ui_datamem[index+9],ui_datamem[index+10]);
//		printf ("lxy: index = 0x%x, ui_datamem[index] = 0x%x, 0x%x, 0x%x, 0x%x !\n",ui_datamem[index],ui_datamem[index+1],ui_datamem[index+2],ui_datamem[index+3]);
//		printf ("lxy: index = 0x%x, ui_datamem = 0x%x, 0x%x, 0x%x, 0x%x !\n",ui_datamem[0],ui_datamem[1],ui_datamem[2],ui_datamem[3]);
		break;
    case dMIPS32_PSEUDO_FIFO_ADDR: 
	    ui_fifomem[uififo_waddr++]=ui_data;        break;
    case dMIPS32_PSEUDO_STACK_ADDR:
        	ui_stack[ui_stack_index] = ui_data;
		ui_stack_index=(ui_stack_index+1)&0x1f;
		break;
    default:
		data0=ui_datamem[(ui_address-dMIPS32_DATAMEM)>>2];
		data1=ui_data;

		switch(accesstype)
		{
			case 0:
				mask=0xff<<((ui_address&3)*8); break;
			case 1:
				mask=0xffff<<(((ui_address>>1)&1)*16); break;
			case 2: mask=0xffffffff;break;
			default: mask=0xffffff<<((ui_address&1)*8);break;
		}

		ui_data=(data1&mask)|(data0&~mask);
		ui_datamem[(ui_address-dMIPS32_DATAMEM)>>2]=ui_data;
      break;
  }
}

//
// fMIPS32_HandleRead
//
// This routine takes the address from a read and returns the correct data
//
static unsigned int fMIPS32_HandleRead(unsigned int ui_address)
{
  switch(ui_address)
  {
    case dMIPS32_PSEUDO_FIFO_ADDR: return ui_fifomem[uififo_raddr++];        break;
    case dMIPS32_PSEUDO_STACK_ADDR: return ui_stack[(--ui_stack_index)&0x1f]; break;
    default:
	  if((ui_address>=dMIPS32_DATAMEM)&&(ui_address<dMIPS32_DATAMEM+0x400))
	    return ui_datamem[(ui_address-dMIPS32_DATAMEM)>>2];
	   else
      fprintf(stderr,"JTAGEMUL: Read to unknown location 0x%08X.\n", ui_address);
      break;
  }
  
  return(0);
}

//
// fMIPS32_ExitDebugMode
//
// Exits debug mode using the DERET instruction
static unsigned int fMIPS32_ExitDebugMode (void)
{
  unsigned int ui_result;

  // Feed the processor a DERET instruction.

  // Read the control register and make sure processor is attempting to 
  // read from dmseg.
  ui_result = dMIPS32_ECR_CPU_RESET_OCCURED | dMIPS32_ECR_ACCESS_PENDING |
              dMIPS32_ECR_PROBE_ENABLE | dMIPS32_ECR_PROBE_VECTOR;

  fJTAG_Instruction(dJTAG_REGISTER_CONTROL);
  ui_result = fJTAG_Data(ui_result);

  if (ui_result & dMIPS32_ECR_ACCESS_PENDING)
  {
    // Make sure processor is not writing
    if (ui_result & dMIPS32_ECR_ACCESS_RW) // Bit set for a WRITE
    {
      // Chip is writing, need to clear the write
      printf("EJTAGEMUL ERROR: Processor writing on debug exit.\n");  
      return(0);
    }

    // Chip is reading from instruction space - feed it a DERET
    // Write the DERET instruction into the data register            
    fJTAG_Instruction(dJTAG_REGISTER_DATA);
    fJTAG_Data(dMIPS32_DERET_INSTRUCTION);

    // Now finish the processor access by clearing the access pending bit
    fJTAG_Instruction(dJTAG_REGISTER_CONTROL);

    ui_result = dMIPS32_ECR_CPU_RESET_OCCURED | dMIPS32_ECR_PROBE_ENABLE | 
                dMIPS32_ECR_PROBE_VECTOR;

    ui_result = fJTAG_Data(ui_result);

    // At this point we are done.  Even if the chip didn't exit debug mode
    // the debug mode catcher will grab it and report the reason to GDB.
//    printf("JTAGEMUL: Processor exited debug mode.\n");
    return(1);
  }
  else
  {
    // ERROR!  What to do if the processor is not accessing?
    printf("EJTAGEMUL ERROR: Processor not accessing for debug exit.\n");
    return(0);
  }

  // Should never get here!  
  return(0);
}

//
// fMIPS32_EJTAG_Machine
//
// State machine that manages the the EJTAG processor module.
// This state machine hands instructions and memory accesses
// over the EJTAG port.
//
int fMIPS32_EJTAG_Machine(void)
{
  unsigned int ui_result;

  // Need to handle resets here on each execution.
  ui_result = dMIPS32_ECR_CPU_RESET_OCCURED | dMIPS32_ECR_ACCESS_PENDING |
              dMIPS32_ECR_PROBE_ENABLE | dMIPS32_ECR_PROBE_VECTOR;

  fJTAG_Instruction(dJTAG_REGISTER_CONTROL);
  ui_result = fJTAG_Data(ui_result);

  // Check for reset
  if (ui_result & dMIPS32_ECR_CPU_RESET_OCCURED)
  {
    // Reset occured - Must acknowlege this before EJTAG will work
    printf("JTAGEMUL: Processor was reset.\n");
    
    // Clear the reset and set our bits
    ui_result = dMIPS32_ECR_ACCESS_PENDING | dMIPS32_ECR_PROBE_ENABLE | 
                dMIPS32_ECR_PROBE_VECTOR;

    ui_result = fJTAG_Data(ui_result);
  }
  
  switch(ui_ejtag_state)
  {
    case dMIPS32_EJTAG_WAIT_FOR_DEBUG_MODE:
      // Check to see if we have entered debug mode

      ui_result = dMIPS32_ECR_CPU_RESET_OCCURED | dMIPS32_ECR_ACCESS_PENDING |
                  dMIPS32_ECR_PROBE_ENABLE | dMIPS32_ECR_PROBE_VECTOR;

      fJTAG_Instruction(dJTAG_REGISTER_CONTROL);
      ui_result = fJTAG_Data(ui_result);
      
      // Check for debug mode
      if (ui_result & dMIPS32_ECR_IN_DEBUG_MODE)
      {
      	// We have entered debug mode.
      	ui_ejtag_state = dMIPS32_EJTAG_WAIT_FOR_CONTINUE;
      	
        printf("JTAGEMUL: Debug exception!\n");

        // Turn on the debug mode LED
//        fMIPS32_ExecuteDebugModule(aui_led_code,0x80000);

        // Read out the debug register to determine the cause
 tgt_exec(\
"1:\n" \
"	mtc0	$15, $31;\n" \
"	lui	$15, 0xFF20;\n" \
"	sw	$2,  0x1fc($15);\n" \
"	mfc0	$2,  $23;\n" \
"	sw	$2,  0($15);\n" \
"	lw	$2,  0x1fc($15);\n" \
"	mfc0	$15, $31 ;\n" \
"	b 1b;\n" \
"	nop;\n" \
,0x8000);

        // We're done with ui_result at this point - Use it as a count
        // of how many exceptions matched.  We only ever want to match
        // one exception at any given time.
        ui_result = 0x00;
		printf("ui_datamem[0]=%x\n",ui_datamem[0]);

        if (ui_datamem[0] & dMIPS32_DEBUG_SW_BKPT)
        {
          ui_result++;
          ui_last_signal=SIGTRAP; // Breakpoint
        }
        if (ui_datamem[0] & dMIPS32_DEBUG_HW_INST_BKPT)
        {
          ui_result++;
          ui_last_signal=SIGTRAP; // Breakpoint
        }
        if (ui_datamem[0] & dMIPS32_DEBUG_HW_DATA_LD_BKPT)
        {
          ui_result++;
          ui_last_signal=SIGTRAP; // Breakpoint
        }
        if (ui_datamem[0] & dMIPS32_DEBUG_HW_DATA_ST_BKPT)
        {
          ui_result++;
          ui_last_signal=SIGTRAP; // Breakpoint
        }
        if (ui_datamem[0] & dMIPS32_DEBUG_HW_DATA_LD_IMP_BKPT)
        {
          ui_result++;
          ui_last_signal=SIGTRAP; // Breakpoint
        }
        if (ui_datamem[0] & dMIPS32_DEBUG_HW_DATA_ST_IMP_BKPT)
        {
          ui_result++;
          ui_last_signal=SIGTRAP; // Breakpoint
        }
        if (ui_datamem[0] & dMIPS32_DEBUG_SINGLE_STEP_BKPT)
        {
          ui_result++;
          ui_last_signal=SIGTRAP; // Breakpoint
          
          // We need to clear the single step bit
          fMIPS32_ExecuteDebugModule(aui_singlestepclear_code,0x80000,1);
        }
        if (ui_datamem[0] & dMIPS32_DEBUG_JTAG_BKPT)
        {
          ui_result++;
          ui_last_signal=SIGTSTP; // User stop
        }

        // Check for AU1500 specific hardware breakpoint
        ui_datamem[0] = ui_datamem[0] >> 10;
        ui_datamem[0] &= 0x1F;
        if (dMIPS32_AU1500_HW_BKPT == ui_datamem[0])
        {
          ui_result++;
          ui_last_signal=SIGTRAP; // Breakpoint
        }

        if (ui_result == 0x00)
        {
          printf("JTAGEMUL ERROR: Unknown debug exception = 0x%08X\n", ui_datamem[0]);
        }
        if (ui_result > 0x01)
        {
          printf("JTAGEMUL ERROR: Multiple debug exceptions = 0x%08X\n", ui_datamem[0]);
        }

        return(-1); // Tell GDBRDP we entered debug mode
      }
      
      break;

    case dMIPS32_EJTAG_WAIT_FOR_CONTINUE:
      // Check to see if we have stayed in debug mode

      ui_result = dMIPS32_ECR_CPU_RESET_OCCURED | dMIPS32_ECR_ACCESS_PENDING |
                  dMIPS32_ECR_PROBE_ENABLE | dMIPS32_ECR_PROBE_VECTOR;

      fJTAG_Instruction(dJTAG_REGISTER_CONTROL);
      ui_result = fJTAG_Data(ui_result);
      
      // Check for debug mode
      if (!(ui_result & dMIPS32_ECR_IN_DEBUG_MODE))
      {
        // Program got away from us and is RUNNING!?!?!?! ARGH!
        printf("JTAGEMUL: Program got away, currently running!\n");

        ui_ejtag_state = dMIPS32_EJTAG_WAIT_FOR_DEBUG_MODE;
      }
      
      break;
      
    case dMIPS32_EJTAG_EXIT_DEBUG_MODE:
      if (!fMIPS32_ExitDebugMode()) 
      {
        printf("JTAGEMUL ERROR: Could not resume!\n");
      }

      ui_ejtag_state = dMIPS32_EJTAG_WAIT_FOR_DEBUG_MODE;
      break;

    case dMIPS32_EJTAG_FORCE_DEBUG_MODE:
      // Force debug break  
      ui_result = dMIPS32_ECR_CPU_RESET_OCCURED | dMIPS32_ECR_ACCESS_PENDING |
                  dMIPS32_ECR_PROBE_ENABLE | dMIPS32_ECR_PROBE_VECTOR |
                  dMIPS32_ECR_EJTAG_BREAK;
  
      fJTAG_Instruction(dJTAG_REGISTER_CONTROL);
      ui_result = fJTAG_Data(ui_result);

      ui_ejtag_state = dMIPS32_EJTAG_WAIT_FOR_DEBUG_MODE;
      break;
    
    default:
      // Should never get here
      ui_ejtag_state = dMIPS32_EJTAG_WAIT_FOR_DEBUG_MODE;
      printf("JTAGEMUL ERROR: Corrupted debug mode catcher state!\n");
      break;  
  }
  
  return(1);
}

unsigned char jtag_inb(unsigned long addr)
{
      ui_datamem[0]=addr;
	  tgt_exec(\
	  ".set noat;\n" \
	  "1:\n" \
	  "mtc0    $15, $31;\n" \
	  "li    $15,0xFF200000;\n" \
	  "sw    $2, 0x1fc($15);\n" \
	  "sw    $3, 0x1fc($15);\n" \
	  "lw    $2, ($15);\n" \
	  "lb    $3, ($2);\n" \
	  "sb    $3, 4($15);\n" \
	  "lw    $3, 0x1fc($15);\n" \
	  "lw    $2, 0x1fc($15);\n" \
	  "mfc0    $15, $31;\n" \
	  "b 1b;\n" \
	  "nop;\n" \
	  ,0x80000);
	  return (unsigned char)(ui_datamem[1]&0xff);
}

unsigned short jtag_inw(unsigned long addr)
{
      ui_datamem[0]=addr;
	  tgt_exec(\
	  ".set noat;\n" \
	  "1:\n" \
	  "mtc0    $15, $31;\n" \
	  "li    $15,0xFF200000;\n" \
	  "sw    $2, 0x1fc($15);\n" \
	  "sw    $3, 0x1fc($15);\n" \
	  "lw    $2, ($15);\n" \
	  "lh    $3, ($2);\n" \
	  "sh    $3, 4($15);\n" \
	  "lw    $3, 0x1fc($15);\n" \
	  "lw    $2, 0x1fc($15);\n" \
	  "mfc0    $15, $31;\n" \
	  "b 1b;\n" \
	  "nop;\n" \
	  ,0x80000);
	  return (unsigned short)(ui_datamem[1]&0xffff);
}

unsigned int jtag_inl(unsigned long addr)
{
      ui_datamem[0]=addr;
	  tgt_exec(\
	  ".set noat;\n" \
	  "1:\n" \
	  "mtc0    $15, $31;\n" \
	  "li    $15,0xFF200000;\n" \
	  "sw    $2, 0x1fc($15);\n" \
	  "sw    $3, 0x1fc($15);\n" \
	  "lw    $2, ($15);\n" \
	  "lw    $3, ($2);\n" \
	  "sw    $3, 4($15);\n" \
	  "lw    $3, 0x1fc($15);\n" \
	  "lw    $2, 0x1fc($15);\n" \
	  "mfc0    $15, $31;\n" \
	  "b 1b;\n" \
	  "nop;\n" \
	  ,0x80000);
	  return (unsigned int)ui_datamem[1];
}

void jtag_outb(unsigned long addr,unsigned char val)
{
      ui_datamem[0]=addr;
      ui_datamem[1]=val;

	  tgt_exec(\
	  ".set noat;\n" \
	  "1:\n" \
	  "mtc0    $15, $31;\n" \
	  "li    $15,0xFF200000;\n" \
	  "sw    $2, 0x1fc($15);\n" \
	  "sw    $3, 0x1fc($15);\n" \
	  "lw    $2, ($15);\n" \
	  "lw    $3, 0x4($15);\n" \
	  "sb    $3, ($2);\n" \
	  "lw    $3, 0x1fc($15);\n" \
	  "lw    $2, 0x1fc($15);\n" \
	  "mfc0    $15, $31;\n" \
	  "b 1b;\n" \
	  "nop;\n" \
	  ,0x80000);
}

void jtag_outw(unsigned long addr,unsigned short val)
{
      ui_datamem[0]=addr;
      ui_datamem[1]=val;
	  tgt_exec(\
	  ".set noat;\n" \
	  "1:\n" \
	  "mtc0    $15, $31;\n" \
	  "li    $15,0xFF200000;\n" \
	  "sw    $2, 0x1fc($15);\n" \
	  "sw    $3, 0x1fc($15);\n" \
	  "lw    $2, ($15);\n" \
	  "lw    $3, 0x4($15);\n" \
	  "sh    $3, ($2);\n" \
	  "lw    $3, 0x1fc($15);\n" \
	  "lw    $2, 0x1fc($15);\n" \
	  "mfc0    $15, $31;\n" \
	  "b 1b;\n" \
	  "nop;\n" \
	  ,0x80000);
}

void jtag_outl(unsigned long addr,unsigned int val)
{
      ui_datamem[0]=addr;
      ui_datamem[1]=val;

	  tgt_exec(\
	  ".set noat;\n" \
	  "1:\n" \
	  "mtc0    $15, $31;\n" \
	  "li    $15,0xFF200000;\n" \
	  "sw    $2, 0x1fc($15);\n" \
	  "sw    $3, 0x1fc($15);\n" \
	  "lw    $2, ($15);\n" \
	  "lw    $3, 0x4($15);\n" \
	  "sw    $3, ($2);\n" \
	  "lw    $3, 0x1fc($15);\n" \
	  "lw    $2, 0x1fc($15);\n" \
	  "mfc0    $15, $31;\n" \
	  "b 1b;\n" \
	  "nop;\n" \
	  ,0x80000);

}

#define min(a,b) ((a)<(b)?(a):(b))
void jtag_getmem(char *dst,unsigned long addr,unsigned int len)
{
	  int once;
     while(len)
     {
	  uififo_raddr=0;
	  uififo_waddr=0;
	  once = min(((len + (addr&3)) + 3) & ~3,ui_fifomem_size);
      ui_datamem[0]=addr&~3;
      ui_datamem[1]=ui_datamem[0] + once -4;
      printf("once=0x%x,addr=0x%x,dst=0x%x\n",once,addr,dst);

	  tgt_exec(\
	  ".set noat;\n" \
	  ".set noreorder;\n" \
	  "1:\n" \
	  "mtc0    $15, $31;\n" \
	  "li    $15,0xFF200000;\n" \
	  "sw    $1, 0x1FC($15);\n" \
	  "sw    $2, 0x1FC($15);\n" \
	  "sw    $3, 0x1FC($15);\n" \
	  "lw    $1, ($15);\n" \
	  "lw    $2, 0x4($15);\n" \
	  "2:\n" \
	  "lw    $3, ($1);\n" \
	  "sw    $3, 0x1F8($15);\n" \
	  "bne   $1,$2,2b;\n" \
	  "addu  $1, 4;\n" \
	  "lw    $3, 0x1FC($15);\n" \
	  "lw    $2, 0x1FC($15);\n" \
	  "lw    $1, 0x1FC($15);\n" \
	  "mfc0    $15, $31;\n" \
	  "b 1b;\n" \
	  "nop;\n" \
	  ,0x80000);
	  memcpy(dst,(char *)ui_fifomem+(addr&3),min(once - (addr&3),len));
	  dst+=min(once - (addr&3),len);
	  len-=min(once - (addr&3),len);
	  addr+=once - (addr&3);
   }

}

void jtag_putmem(char *src,unsigned long addr,unsigned int len)
{
	  int once;
     while(len)
     {
	  uififo_raddr=0;
	  uififo_waddr=0;

	  once = min(((len + (addr&3)) + 3) & ~3,ui_fifomem_size);

      printf("addr=%x,len=%x,once=%d\n",addr,len,once);

      if(addr&3) ui_fifomem[0] = jtag_inl(addr&~3);
      if((once - (addr&3) > len) && !((addr&3) && once ==4)) ui_fifomem[once/4-1] = jtag_inl((addr&~3) + once - 4);
      memcpy((char *)ui_fifomem+(addr&3),src,min(once - (addr&3),len));

      ui_datamem[0]=addr&~3;
      ui_datamem[1]=ui_datamem[0] + once -4;

	  tgt_exec(\
	  ".set noat;\n" \
	  ".set noreorder;\n" \
	  "1:\n" \
	  "mtc0    $15, $31;\n" \
	  "li    $15,0xFF200000;\n" \
	  "sw    $1, 0x1FC($15);\n" \
	  "sw    $2, 0x1FC($15);\n" \
	  "sw    $3, 0x1FC($15);\n" \
	  "lw    $1, ($15);\n" \
	  "lw    $2, 0x4($15);\n" \
	  "2:\n" \
	  "lw    $3, 0x1F8($15);\n" \
	  "sw    $3, ($1);\n" \
	  "bne   $1,$2,2b;\n" \
	  "addu  $1, 4;\n" \
	  "lw    $3, 0x1FC($15);\n" \
	  "lw    $2, 0x1FC($15);\n" \
	  "lw    $1, 0x1FC($15);\n" \
	  "mfc0    $15, $31;\n" \
	  "b 1b;\n" \
	  "nop;\n" \
	  ,0x80000);
	  src+=min(once - (addr&3),len);
	  len-=min(once - (addr&3),len);
	  addr+=once - (addr&3);
	}
}

void jtag_quit()
{
tgt_exec("deret;",4); 
debug_reg = 0;
}

#include "readregisters.h"
#include "writeregisters.h"

#if 1

unsigned int code_inst_download_crc[100]=
tgt_compile(\
".set mips32\n" \
"mfc0 t5,$16,6;\n" \
"li t7,512;\n" \
"nor t7,t7,zero;\n" \
"and t5,t5,t7;\n" \
"1:mtc0 t5,$16,6;\n" \
"lui t5,0xa530;\n" \
"mfc0 t6,$16,6;\n" \
"andi t6,t6,0x200;\n" \
"bnez t6,2f;\n" \
"nop ;\n" \
"mfc0 t6,$16,6;\n" \
"andi t6,t6,0x100;\n" \
"beq zero,t6,1b;\n" \
"nop ;\n" \
"mfc0 t7,$22,6;\n" \
"sw t7,0(t5);\n" \
"addi t5,t5,4;\n" \
"mfc0 t6,$16,6;\n" \
"li t7,256;\n" \
"nor t7,t7,zero;\n" \
"and t6,t6,t7;\n" \
"mtc0 t6,$16,6;\n" \
"nop ;\n" \
"mfc0 t6,$16,6;\n" \
"andi t6,t6,0x200;\n" \
"beqz t6,1b;\n" \
"nop ;\n" \
"2:mfc0 t5,$16,6;\n" \
"li t7,512;\n" \
"nor t7,t7,zero;\n" \
"and t5,t5,t7;\n" \
"mtc0 t5,$16,6;\n" \
"nop ;\n" \
"nop ;\n" \
"mfc0 t7,c0_taghi;\n" \
"jr t7;\n" \
"nop ;\n" \
);


void jtag_self_down_crc(unsigned long addr_download)
{
	printf("download self download to %p\n",addr_download);
	jtag_putmem(code_inst_download_crc,addr_download,sizeof(code_inst_download_crc));
	printf("jtag client len and crc download over!\n");
}
#endif

#if 1

void jtag_download_crc_checksum(unsigned long addr_download,unsigned int buf_crc_checksum)
{
		printf("download buf CRC32 checksum download to %p , CRC :0x%x \n",addr_download,buf_crc_checksum);
		jtag_outl(addr_download,buf_crc_checksum);
}
#endif

void jtag_download_length(unsigned long addr_download,unsigned int buf_length)
{
		jtag_outl(addr_download,buf_length);
}

/*
 * param +0: dest address
 * param +4: length
 *
 */
unsigned int code_inst_download[]=
tgt_compile(\
"bal 1f;\n" \
"nop;\n" \
"param:\n" \
".space	40 \n" \
"1:\n" \
"lw t4,0(ra);\n" \
"lw t5,4(ra);\n" \
"move t3,zero;\n" \
"lui t6,0xff20;\n" \
"ori t6,t6,0xfe04;\n" \
"1:lw t7,0(t6);\n" \
"sw t7,0(t5);\n" \
"addu t3,t3,4;\n" \
"slt at,t3,t4;\n" \
"bnez at,1b;\n" \
"addi t5,t5,4;\n" \
"nop ;\n" \
);

void jtag_self_down(unsigned long addr_download)
{
	int i=0;
	unsigned long ui_address=addr_download;
	printf("download self download to %p\n",addr_download);
	jtag_putmem(code_inst_download,addr_download,sizeof(code_inst_download));
	printf("jtag client download over!\n");
}


unsigned int code_inst_upload[100]=
tgt_compile(\
"lui t6,0xa530;\n" \
"lw t4,0(t6);\n" \
"lui t5,0xbfc0;\n" \
"add t4,t4,t5;\n" \
"lui t6,0xff2f;\n" \
"ori t6,t6,0xfe04;\n" \
"1:lw t7,0(t5);\n" \
"sw t7,0(t6);\n" \
"bne t4,t5,1b;\n" \
"addi t5,t5,4;\n" \
"nop ;\n" \
);


void jtag_self_down_up(unsigned long addr_download)
{
	printf("download self download to %p\n",addr_download);
	jtag_putmem(code_inst_upload,addr_download,sizeof(code_inst_upload));

	printf("jtag client download over!\n");
}

#if 1

unsigned int code_inst_crc[100]=
tgt_compile(\
".set mips32\n" \
"lui k0,0x3333;\n" \
"ori k0,k0,0x3333;\n" \
"lui k1,0x0;\n" \
"ori k1,k1,0x0;\n" \
"move a0,zero;\n" \
"lui a1,0xa020;\n" \
"ori a1,a1,0x0;\n" \
"lui t7,0xa530;\n" \
"ori t7,t7,0x0;\n" \
"lw a2,0(t7);\n" \
"addu a2,a1,a2;\n" \
"sltu v0,a1,a2;\n" \
"beqz v0,1f;\n" \
"nor a3,zero,a0;\n" \
"lui v0,0xa500;\n" \
"ori t0,v0,0x100;\n" \
"2:lbu v0,0(a1);\n" \
"addiu a1,a1,1;\n" \
"xor v0,a3,v0;\n" \
"andi v0,v0,0xff;\n" \
"sll v0,v0,0x2;\n" \
"addu v0,v0,t0;\n" \
"lw a0,0(v0);\n" \
"sltu v1,a1,a2;\n" \
"srl v0,a3,0x8;\n" \
"bnez v1,2b;\n" \
"xor a3,a0,v0;\n" \
"1:nor v0,zero,a3;\n" \
"lui k0,0x8680;\n" \
"ori k0,k0,0x1125;\n" \
"move k1,v0;\n" \
"lui t7,0xa030;\n" \
"ori t7,t7,0x0;\n" \
"nop ;\n" \
"sdbbp ;\n" \
"nop ;\n" \
"nop ;\n" \
);

unsigned int code_inst_crc_old[100]=
tgt_compile(\
".set mips32\n" \
"lui k0,0x3333;\n" \
"ori k0,k0,0x3333;\n" \
"lui t7,0xa530;\n" \
"ori t7,t7,0x0;\n" \
"lw t6,0(t7);\n" \
"lui t5,0xa030;\n" \
"ori t5,t5,0x0;\n" \
"lui a3,0xa500;\n" \
"ori a3,a3,0x100;\n" \
"li a0,0;\n" \
"nor a1,a0,zero;\n" \
"add t4,t5,t6;\n" \
"sltu t0,t4,t5;\n" \
"bnez t0,1f;\n" \
"nop ;\n" \
"2:lw t3,0(t5);\n" \
"addi t3,t3,1;\n" \
"xor a0,a1,t3;\n" \
"andi a0,a0,0xff;\n" \
"sll a0,a0,0x2;\n" \
"add a0,a0,a3;\n" \
"lw t2,0(a0);\n" \
"sltu t1,t4,t5;\n" \
"srl a1,a1,0x3;\n" \
"beqz t1,2b;\n" \
"xor a1,a1,t2;\n" \
"1:lui k0,0x8680;\n" \
"ori k0,k0,0x1125;\n" \
"move k1,a1;\n" \
"nop ;\n" \
"lui t7,0xa030;\n" \
"ori t7,t7,0x0;\n" \
"nop ;\n" \
);

void jtag_crc_down(unsigned long addr_download)
{
	int i=0;
	unsigned long ui_address=addr_download;
	printf("download self download to %p\n",addr_download);
	jtag_putmem(code_inst_crc,addr_download,sizeof(code_inst_crc));
	printf("jtag CRC code download over!\n");
}

#endif

#if 1

/* Table computed with Mark Adler's makecrc.c utility.  */
static const unsigned int crc32_table_down[256] =
{
  0x00000000, 0x77073096, 0xee0e612c, 0x990951ba, 0x076dc419,
  0x706af48f, 0xe963a535, 0x9e6495a3, 0x0edb8832, 0x79dcb8a4,
  0xe0d5e91e, 0x97d2d988, 0x09b64c2b, 0x7eb17cbd, 0xe7b82d07,
  0x90bf1d91, 0x1db71064, 0x6ab020f2, 0xf3b97148, 0x84be41de,
  0x1adad47d, 0x6ddde4eb, 0xf4d4b551, 0x83d385c7, 0x136c9856,
  0x646ba8c0, 0xfd62f97a, 0x8a65c9ec, 0x14015c4f, 0x63066cd9,
  0xfa0f3d63, 0x8d080df5, 0x3b6e20c8, 0x4c69105e, 0xd56041e4,
  0xa2677172, 0x3c03e4d1, 0x4b04d447, 0xd20d85fd, 0xa50ab56b,
  0x35b5a8fa, 0x42b2986c, 0xdbbbc9d6, 0xacbcf940, 0x32d86ce3,
  0x45df5c75, 0xdcd60dcf, 0xabd13d59, 0x26d930ac, 0x51de003a,
  0xc8d75180, 0xbfd06116, 0x21b4f4b5, 0x56b3c423, 0xcfba9599,
  0xb8bda50f, 0x2802b89e, 0x5f058808, 0xc60cd9b2, 0xb10be924,
  0x2f6f7c87, 0x58684c11, 0xc1611dab, 0xb6662d3d, 0x76dc4190,
  0x01db7106, 0x98d220bc, 0xefd5102a, 0x71b18589, 0x06b6b51f,
  0x9fbfe4a5, 0xe8b8d433, 0x7807c9a2, 0x0f00f934, 0x9609a88e,
  0xe10e9818, 0x7f6a0dbb, 0x086d3d2d, 0x91646c97, 0xe6635c01,
  0x6b6b51f4, 0x1c6c6162, 0x856530d8, 0xf262004e, 0x6c0695ed,
  0x1b01a57b, 0x8208f4c1, 0xf50fc457, 0x65b0d9c6, 0x12b7e950,
  0x8bbeb8ea, 0xfcb9887c, 0x62dd1ddf, 0x15da2d49, 0x8cd37cf3,
  0xfbd44c65, 0x4db26158, 0x3ab551ce, 0xa3bc0074, 0xd4bb30e2,
  0x4adfa541, 0x3dd895d7, 0xa4d1c46d, 0xd3d6f4fb, 0x4369e96a,
  0x346ed9fc, 0xad678846, 0xda60b8d0, 0x44042d73, 0x33031de5,
  0xaa0a4c5f, 0xdd0d7cc9, 0x5005713c, 0x270241aa, 0xbe0b1010,
  0xc90c2086, 0x5768b525, 0x206f85b3, 0xb966d409, 0xce61e49f,
  0x5edef90e, 0x29d9c998, 0xb0d09822, 0xc7d7a8b4, 0x59b33d17,
  0x2eb40d81, 0xb7bd5c3b, 0xc0ba6cad, 0xedb88320, 0x9abfb3b6,
  0x03b6e20c, 0x74b1d29a, 0xead54739, 0x9dd277af, 0x04db2615,
  0x73dc1683, 0xe3630b12, 0x94643b84, 0x0d6d6a3e, 0x7a6a5aa8,
  0xe40ecf0b, 0x9309ff9d, 0x0a00ae27, 0x7d079eb1, 0xf00f9344,
  0x8708a3d2, 0x1e01f268, 0x6906c2fe, 0xf762575d, 0x806567cb,
  0x196c3671, 0x6e6b06e7, 0xfed41b76, 0x89d32be0, 0x10da7a5a,
  0x67dd4acc, 0xf9b9df6f, 0x8ebeeff9, 0x17b7be43, 0x60b08ed5,
  0xd6d6a3e8, 0xa1d1937e, 0x38d8c2c4, 0x4fdff252, 0xd1bb67f1,
  0xa6bc5767, 0x3fb506dd, 0x48b2364b, 0xd80d2bda, 0xaf0a1b4c,
  0x36034af6, 0x41047a60, 0xdf60efc3, 0xa867df55, 0x316e8eef,
  0x4669be79, 0xcb61b38c, 0xbc66831a, 0x256fd2a0, 0x5268e236,
  0xcc0c7795, 0xbb0b4703, 0x220216b9, 0x5505262f, 0xc5ba3bbe,
  0xb2bd0b28, 0x2bb45a92, 0x5cb36a04, 0xc2d7ffa7, 0xb5d0cf31,
  0x2cd99e8b, 0x5bdeae1d, 0x9b64c2b0, 0xec63f226, 0x756aa39c,
  0x026d930a, 0x9c0906a9, 0xeb0e363f, 0x72076785, 0x05005713,
  0x95bf4a82, 0xe2b87a14, 0x7bb12bae, 0x0cb61b38, 0x92d28e9b,
  0xe5d5be0d, 0x7cdcefb7, 0x0bdbdf21, 0x86d3d2d4, 0xf1d4e242,
  0x68ddb3f8, 0x1fda836e, 0x81be16cd, 0xf6b9265b, 0x6fb077e1,
  0x18b74777, 0x88085ae6, 0xff0f6a70, 0x66063bca, 0x11010b5c,
  0x8f659eff, 0xf862ae69, 0x616bffd3, 0x166ccf45, 0xa00ae278,
  0xd70dd2ee, 0x4e048354, 0x3903b3c2, 0xa7672661, 0xd06016f7,
  0x4969474d, 0x3e6e77db, 0xaed16a4a, 0xd9d65adc, 0x40df0b66,
  0x37d83bf0, 0xa9bcae53, 0xdebb9ec5, 0x47b2cf7f, 0x30b5ffe9,
  0xbdbdf21c, 0xcabac28a, 0x53b39330, 0x24b4a3a6, 0xbad03605,
  0xcdd70693, 0x54de5729, 0x23d967bf, 0xb3667a2e, 0xc4614ab8,
  0x5d681b02, 0x2a6f2b94, 0xb40bbe37, 0xc30c8ea1, 0x5a05df1b,
  0x2d02ef8d
};

void jtag_crc_table_down(unsigned long addr_download)
{
	int i=0;
	unsigned long ui_address=addr_download;
	printf("download self download to %p\n",addr_download);
	jtag_putmem(crc32_table_down,addr_download,sizeof(crc32_table_down));
	printf("jtag CRC code download over!\n");
}
#endif

#if 1
unsigned int jtag_wait_k0_flag(unsigned long addr_readreg)
{
	unsigned int k0_val=0;
	unsigned int k1_val=0;
	int i=0;
	unsigned long ui_address = addr_readreg;

	printf("======>>>>k0_val : 0x%x \n",k0_val);
	do
	{
	tgt_exec(\
	  ".set noat;\n" \
	  "1:\n" \
	  "mtc0    $15, $31;\n" \
	  "li    $15,0xFF200000;\n" \
	  "sw    k0, ($15);\n" \
	  "sw    k1,4($15);\n" \
	  "mfc0    $15, $31;\n" \
	  "b 1b;\n" \
	  "nop;\n" \
	  ,0x80000);
	  jtag_quit();

      k0_val=ui_datamem[0];
		sleep(2);
		printf("======>>>>>>>>>>>>>>>>k0_val : 0x%x \n",k0_val);
	}while(k0_val != 0x86801125) ;

	k1_val = ui_datamem[1];
	return k1_val;
}
#endif

unsigned int context_save(int *context_reg)
{
	int i=0;
	  uififo_raddr=0;
	  uififo_waddr=0;
	  memset(ui_fifomem,0,38*4);
	fMIPS32_ExecuteDebugModule(aui_readregisters_code,0x80000,1);
	
	for(i=0;i<dMIPS32_NUM_DEBUG_REGISTERS;i++)
	{
		context_reg[i]=ui_fifomem[i];
	}
	return 0;
	
}

unsigned int context_restore(int *context_reg)
{
	int i=0;
	  uififo_raddr=0;
	  uififo_waddr=0;
	for(i=0;i<dMIPS32_NUM_DEBUG_REGISTERS;i++)
	{
		ui_fifomem[i]=context_reg[i];
	}
	fMIPS32_ExecuteDebugModule(aui_writeregisters_code,0x80000,1);
	return 0;
	
}


unsigned int get_DR2(void)
{
      unsigned int ui_data=0 ;
      unsigned int ui_result=0 ;

#if 1
      // Clear the access pending bit (let the processor eat!)

      ui_result = dMIPS32_ECR_CPU_RESET_OCCURED | dMIPS32_ECR_PROBE_ENABLE | dMIPS32_ECR_PROBE_VECTOR;
      fJTAG_Instruction(dJTAG_REGISTER_CONTROL);
      ui_result = fJTAG_Data(ui_result);
#endif
}

unsigned int get_DR(unsigned int upload_len)
{
      unsigned int ui_data=0 ;
      unsigned int ui_result=0 ;

#if 1 //20090703-zgj

      // Clear the access pending bit (let the processor eat!)

      ui_result = dMIPS32_ECR_CPU_RESET_OCCURED | dMIPS32_ECR_PROBE_ENABLE | dMIPS32_ECR_PROBE_VECTOR;

      fJTAG_Instruction(dJTAG_REGISTER_CONTROL);
      ui_result = fJTAG_Data(ui_result);
#endif

       fJTAG_Instruction(dJTAG_REGISTER_DATA);
       ui_data = fJTAG_Data(0x00);


      return ui_data;
}

void USB_UPLOAD_FAST(unsigned int upload_len)
{
        fJTAG_Instruction(dJTAG_REGISTER_FASTDATA);
        fJTAG_FAST_UPLOAD(upload_len);
}

unsigned int miro_inst[]=
tgt_compile(\
".set mips32\n" \
"1:mtc0 t7,c0_desave;\n" \
"mfc0 t7,$16,6;\n" \
"xori t7,t7,0x220;\n" \
"mtc0 t7,$16,6;\n" \
"mfc0 t7,c0_desave;\n" \
"nop ;\n" \
"b 1b;\n" \
"nop ;\n" \
);


unsigned int miro_inst_2[]=
tgt_compile(\
".set mips32;\n" \
"mtc0 t7,c0_desave;\n" \
"lui t7,0xa500;\n" \
"mtc0 t7,c0_depc;\n" \
"nop ;\n" \
"mfc0 t7,c0_desave;\n" \
"mfc0 t7,c0_depc;\n" \
"nop ;\n" \
"jr t7;\n" \
"nop ;\n" \
);


unsigned int miro_initsdram_inst[]=
tgt_compile(\
"1:mtc0 t7,c0_desave;\n" \
"lui a2,0xbf00;\n" \
"lui a1,0xc40;\n" \
"ori a1,a1,0x2d4;\n" \
"sw a1,0(a2);\n" \
"lui a1,0x2098;\n" \
"ori a1,a1,0xcd;\n" \
"sw a1,4(a2);\n" \
"mfc0 t7,c0_desave;\n" \
"nop ;\n" \
"b 1b;\n" \
"nop ;\n" \
"nop ;\n" \
);


unsigned int miro_initsdram_inst_2[]=
tgt_compile(\
"mtc0 t7,c0_desave;\n" \
"mfc0 t7,c0_depc;\n" \
"mtc0 t7,c0_taghi;\n" \
"lui t7,0xa020;\n" \
"mtc0 t7,c0_depc;\n" \
"nop ;\n" \
"mfc0 t7,c0_desave;\n" \
"nop ;\n" \
"deret ;\n" \
"nop ;\n" \
);


unsigned int miro_download_os_ret_inst[]=
tgt_compile(\
"mtc0 t7,c0_desave;\n" \
"mfc0 t7,c0_taghi;\n" \
"mtc0 t7,c0_depc;\n" \
"nop ;\n" \
"mfc0 t7,c0_desave;\n" \
"nop ;\n" \
"deret ;\n" \
"nop ;\n" \
);

unsigned int miro_savedepc_inst[]=
tgt_compile(\
"mtc0 t7,c0_desave;\n" \
"mfc0 t7,c0_depc;\n" \
"mtc0 t7,c0_taghi;\n" \
"nop ;\n" \
"mfc0 t7,c0_desave;\n" \
"nop ;\n" \
"deret ;\n" \
"nop ;\n" \
);

unsigned int miro_runcrc_inst[]=
tgt_compile(\
"mtc0 t7,c0_desave;\n" \
"lui t7,0xa540;\n" \
"mtc0 t7,c0_depc;\n" \
"nop ;\n" \
"mfc0 t7,c0_desave;\n" \
"nop ;\n" \
"deret ;\n" \
"nop ;\n" \
);


unsigned int miro_initcp0_inst[]=
tgt_compile(\
"1:mtc0 zero,c0_status;\n" \
"mtc0 zero,c0_cause;\n" \
"lui t0,0x40;\n" \
"mtc0 t0,c0_status;\n" \
"lui sp,0x8100;\n" \
"addiu sp,sp,-16384;\n" \
"lui gp,0x8106;\n" \
"addiu gp,gp,-11248;\n" \
"b 1b;\n" \
"nop ;\n" \
);

