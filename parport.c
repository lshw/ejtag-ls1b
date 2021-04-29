//
// PARPORT.C
//
// Handles all access to the parallel port
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
#include <err.h>
//#include <machine/pio.h>
//#include <machine/sysarch.h>
#include <sys/io.h>
#include "parport.h"

//
// DEFINES
//
// Port addresses
#define	dPARPORT_DATA    0x378
#define	dPARPORT_STATUS  0x379
#define	dPARPORT_CONTROL 0x37A

//
// PROTOTYPES
//
static inline void fPARPORT_SetAccess(u_long * map, u_int bit, int allow);

//
// fPARPORT_SetAccess
//
// Sets the direct access permissions for the parallel port
//
static inline void fPARPORT_SetAccess(u_long * map, u_int bit, int allow)
{
  unsigned int  word;
  unsigned int  shift;
  unsigned long mask;

  word = bit / 32;
  shift = bit - (word * 32);

  mask = 0x000000001 << shift;
  if (allow)
    map[word] &= ~mask;
  else
    map[word] |= mask;
}

//
// fPARPORT_Init
//
// Initializes the parallel port
//
void fPARPORT_Init(void)
{
  unsigned long iomap[32];
	
	iopl(3);
	return;
}

//
// fPARPORT_Write
//
// Writes a byte to the parallel port data register
//
void inline fPARPORT_Write(unsigned char uc_byte)
{
  outb(uc_byte,dPARPORT_DATA);
  
  return;
}

//
// fPARPORT_Read
//
// Read a byte from the parallel port status register
//
unsigned char inline fPARPORT_Read(void)
{
  return(inb(dPARPORT_STATUS));
}

#include <cmdparser.h>

static int ppcmd(int argc,char **argv)
{
	int i;
if (!strcmp(argv[0],"ppinit")) fPARPORT_Init();
else if (!strcmp(argv[0],"ppread")) printf("0x%02x\n",fPARPORT_Read());
else if (!strcmp(argv[0],"ppwrite")) {
	if (argc != 2) return -1;
	fPARPORT_Write(strtoul(argv[1],0,0));
 }
return 0;	
}

mycmd_init(ppinit,ppcmd,"","init parport");
mycmd_init(ppread,ppcmd,"","read parport");
mycmd_init(ppwrite,ppcmd,"data","write parport");

