//
// MIPS32.H
//
// Target specific EJTAG code for MIPS32
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

#ifndef dMIPS32_H
#define dMIPS32_H

// 
// PROTOTYPES
//
int  fMIPS32_EJTAG_Machine     (void);
unsigned int fMIPS32_ExecuteDebugModule(unsigned int *pui_module,unsigned int len,unsigned int runonce);

//
// DEFINES
//
// ECR register definition
//
#define dMIPS32_ECR_CPU_RESET_OCCURED  0x00000000 // Set to 1 if a reset occured
#define dMIPS32_ECR_ACCESS_SIZE        0x60000000 // See below note 1
#define dMIPS32_ECR_DOZE               0x00400000 // Set to 1 if processor is in low power mode
#define dMIPS32_ECR_HALT               0x00200000 // Set to 1 if system bus clock is stopped
#define dMIPS32_ECR_PERIPHERAL_RESET   0x00100000 // Set to 1 to reset peripherals (optional)
#define dMIPS32_ECR_ACCESS_RW          0x00080000 // Set to 1 if pending access is write, 0 for read
#define dMIPS32_ECR_ACCESS_PENDING     0x00040000 // Set to 1 to indicate access pending
#define dMIPS32_ECR_CPU_RESET          0x00010000 // Set to 1 to reset CPU (optional)
#define dMIPS32_ECR_PROBE_ENABLE       0x00008000 // Set to 1 to enable remote hosted memory
#define dMIPS32_ECR_PROBE_VECTOR       0x00004000 // Set to 1 to use dmseg location for remote memory
#define dMIPS32_ECR_EJTAG_BREAK        0x00001000 // Set to 1 to request a debug interrupt exception
#define dMIPS32_ECR_IN_DEBUG_MODE      0x00000008 // Set to 1 to indicate in debug mode

// Note 1
//
// Possible values of dMIPS32_ECR_ACCESS_SIZE
#define dMIPS32_ECR_ACCESS_BYTE        0x00000000 // 0 = 1 byte
#define dMIPS32_ECR_ACCESS_HWORD       0x20000000 // 1 = 2 bytes
#define dMIPS32_ECR_ACCESS_WORD        0x40000000 // 2 = 4 bytes
#define dMIPS32_ECR_ACCESS_TRIPLE      0x60000000 // 3 = ? bytes

// Instruction for return from debug handler
#define dMIPS32_DERET_INSTRUCTION      0x4200001F // Big endian format

// A slight note about EJTAG hosted memory.  Nothing is designated as ROM
// or RAM its all virtual definitions we create for being able to identify
// accesses for instructions or accesses for data.  The begining of EJTAG
// hosted memory is designated for ROM up to where RAM begins.  All 
// virtual registers (RAM) will live above the ROM section.  This is useful
// when exiting debug mode to send the correct DERET instruction only when
// retrieving instructions not data.
//
// Our pseudo stack location
#define dMIPS32_PSEUDO_STACK_ADDR      0xFF2001FC
 
#define dMIPS32_NUM_DEBUG_REGISTERS    38
 
// Define the memory access registers
#define dMIPS32_PSEUDO_FIFO_ADDR         0xFF2001F8
#define dMIPS32_PSEUDO_SERIAL_ADDR       0xFF2001F4
#define dMIPS32_DATAMEM         0xFF200000

//
// EXTERNS - For sharing the register array
extern unsigned int  ui_datamem[];
extern unsigned int  context_reg[];
extern unsigned int  uififo_raddr,uififo_waddr,*ui_fifomem,aui_readregisters_code[],ui_fifomem_size;
extern unsigned int  ui_ejtag_state;
extern unsigned int  ui_last_signal;
extern unsigned int aui_writeregisters_code[];

// Define the register locations of the DRSEG hardware breakpoint registers
// These are not implemented on the au1500 (suck!)
#define dMIPS32_INST_BKPT_STATUS       0xFF301000
#define dMIPS32_INST_BKPT_ADDR(X)      0xFF301100 + (X * 0x100)
#define dMIPS32_INST_BKPT_ADDR_MSK(X)  0xFF301108 + (X * 0x100)
#define dMIPS32_INST_BKPT_ASID(X)      0xFF301110 + (X * 0x100)
#define dMIPS32_INST_BKPT_CONTROL(X)   0xFF301118 + (X * 0x100)

#define dMIPS32_DATA_BKPT_STATUS       0xFF302000
#define dMIPS32_DATA_BKPT_ADDR(X)      0xFF302100 + (X * 0x100)
#define dMIPS32_DATA_BKPT_ADDR_MSK(X)  0xFF302108 + (X * 0x100)
#define dMIPS32_DATA_BKPT_ASID(X)      0xFF302110 + (X * 0x100)
#define dMIPS32_DATA_BKPT_CONTROL(X)   0xFF302118 + (X * 0x100)
#define dMIPS32_DATA_BKPT_VALUE(X)     0xFF302120 + (X * 0x100)

// AU1500 supports 1 instruction and 1 data breakpoint
#define dMIPS32_MAX_HARDWARE_BKPTS     0x10

// AU1500 hardware breakpoints put the following value into the
// debug register CP0 #23 sel 0 in the DExcCode bits
#define dMIPS32_AU1500_HW_BKPT         0x17

// States of the debug exception catcher
#define dMIPS32_EJTAG_WAIT_FOR_DEBUG_MODE    0x01
#define dMIPS32_EJTAG_WAIT_FOR_CONTINUE      0x02
#define dMIPS32_EJTAG_EXIT_DEBUG_MODE        0x03
#define dMIPS32_EJTAG_FORCE_DEBUG_MODE       0x04

// Software breakpoint instruction
#define dMIPS32_SW_BREAKPOINT_INSTRUCTION    0x7000003F

// Bits in the debug register (CP0 register 23 select 0)
#define dMIPS32_DEBUG_SW_BKPT                0x00000002
#define dMIPS32_DEBUG_HW_INST_BKPT           0x00000010
#define dMIPS32_DEBUG_HW_DATA_LD_BKPT        0x00000004
#define dMIPS32_DEBUG_HW_DATA_ST_BKPT        0x00000008
#define dMIPS32_DEBUG_HW_DATA_LD_IMP_BKPT    0x00040000
#define dMIPS32_DEBUG_HW_DATA_ST_IMP_BKPT    0x00080000
#define dMIPS32_DEBUG_SINGLE_STEP_BKPT       0x00000001
#define dMIPS32_DEBUG_JTAG_BKPT              0x00000020


//
// EXTERNS - For sharing the register array

#define DRSEG_BASE	0xff300000
/* instruction breakpoint status */
#define IBS_OFFSET	0x1000			
/* instruction breakpoint address n */
#define IBAN_OFFSET(x)	(0x1100+0x100*x)
/* instruction breakpoint address mask n */
#define IBMN_OFFESET(x)	(0x1108+0x100*x)
/* instruction breakpoint asid n */
#define IBASIDN_OFFSET(x)	(0x1110+0x100*x)
/* instruction breakpoint control n */
#define	IBCN_OFFSET(x)	(0x1118+0x100*x)
#define IBS	(DRSEG_BASE+IBS_OFFSET)
#define IBAN(x)		(DRSEG_BASE+IBAN_OFFSET(x))
#define IBMN(x)		(DRSEG_BASE+IBMN_OFFSET(x))
#define IBASIDN(x)	(DRSEG_BASE+IBASIDN_OFFSET(x))
#define IBCN(x)		(DRSEG_BASE+IBCN_OFFSET(x))

/* data breakpoint status */
#define DBS_OFFSET	0x2000
/* data breakpoint address n */
#define DBAN_OFFSET(x)	(0x2100+0x100*x)
/* data breakpoint address mask n */
#define DBMN_OFFSET(x)	(0x2108+0x100*x)
/* data breakpoint asid n */
#define DBASIDN_OFFSET(x)	(0x2110+0x100*x)
/* data breakpoint control n */
#define DBCN_OFFSET(x)	(0x2118+0x100*x)
/* data breakpoint value n */
#define DBVN_OFFSET(x)	(0x2120+0x100*x)
#define DBS	(DRSEG_BASE+DBS_OFFSET)
#define DBAN(x)	(DRSEG_BASE+DBAN_OFFSET(x))
#define DBMN(x)	(DRSEG_BASE+DBMN_OFFSET(x))
#define DBASIDN(x)	(DRSEG_BASE+DBASIDN_OFFSET(x))
#define DBCN(x)	(DRSEG_BASE+DBCN_OFFSET(x))
#define DBVN(x)	(DRSEG_BASE+DBVN_OFFSET(x))
#define HI_HALF_MASK 0x0000000c
#define LO_HALF_MASK 0x00000003
#define min(a,b) ((a)<(b)?(a):(b))
#define tgt_compile(s) tgt_compile(s)
#define tgt_exec(s,len) ({unsigned int codebuf[]=tgt_compile(s);fMIPS32_ExecuteDebugModule(codebuf,min(len,sizeof(codebuf)),1);})
#define tgt_exec_safe(s,len) ({unsigned int codebuf[]=tgt_compile(s);fMIPS32_ExecuteDebugModule_safe(codebuf,min(len,sizeof(codebuf)),1);})
unsigned char jtag_inb(unsigned long addr);
unsigned short jtag_inw(unsigned long addr);
unsigned int jtag_inl(unsigned long addr);
void jtag_outb(unsigned long addr,unsigned char val);
void jtag_outw(unsigned long addr,unsigned short val);
void jtag_outl(unsigned long addr,unsigned int val);

#endif // #ifndef dMIPS32_H
