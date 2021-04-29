//
// JTAGEMUL.C
//
// Emulates a JTAG hardware interface via the parallel port
// and a JTAG/parallel port dongle to target hardware that
// interfaces to GDB via the remote debugging protocol.
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
#include <fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <netinet/in.h>
#include <netdb.h>
#include "jtagsock.h"
#include "gdbrdp.h"
#include "cmdparser.h"
#include "mips32.h"

//
// DEFINES
//
#define PORTNUM                 50010

extern int config_gdbserver_quitgdb;

//
// main
//
// The heart of it all.
//
void ctrl_c_op(int signo)
{
	printf("kkkkkkkkkkkkkkkkkkkk\n");
	signal(SIGPIPE,SIG_IGN);
}

int cmd_gdbserver(int argc,char **argv)
{
	int i_socket, i_currentsocket, i_flags;
	int port = argc > 1 ? strtoul(argv[1],0,0) :PORTNUM;

//	signal(SIGPIPE,ctrl_c_op);
	setbuffer(stdout,0,0);
	setbuffer(stderr,0,0);

	fMIPS32_Init();
	// Establish connection to port
	i_socket = fJTAGSOCK_Establish(port);
	if (i_socket < 0)
	{
		// Establish failed.
		perror("fJTAGSOCK_Establish failed.");
		exit(1);
	}


	config_gdbserver_quitgdb = 0;
	// Forever
	for (;;)
	{
		if(config_gdbserver_quitgdb)break;
		printf("Attached to port %d, waiting for gdb.\nyou can also input command on console\n",port);

		i_flags = fcntl(i_socket, F_GETFL, 0); 
		i_flags |= O_NONBLOCK;
		fcntl(i_socket, F_SETFL, i_flags); 
		// Wait for a connection to occur
		i_currentsocket = fJTAGSOCK_GetConnection(i_socket);
		if (i_currentsocket < 0)
		{
			perror("fJTAGSOCK_GetConnection failed.");
			close(i_socket);
			return 2;
		}

		// Change the socket to non-blocking
		i_flags = fcntl(i_currentsocket, F_GETFL, 0); 
		i_flags |= O_NONBLOCK;
		fcntl(i_currentsocket, F_SETFL, i_flags); 

		printf("GDB successfully attached.\r\n");

		force_acc();
		ui_ejtag_state=dMIPS32_EJTAG_WAIT_FOR_CONTINUE;
		ui_last_signal=SIGTRAP;

		// Run the GDB Remote Debugging Protocol handler (the guts)
		fGDBRDP_Handler(i_currentsocket);
	}

	// Should never reach here
	close(i_socket);
	close(i_currentsocket);
	return(1);
}
static int cmd_quitgdb(int argc,char **argv)
{
extern int config_gdbserver_quitgdb;
if(argc > 1)
 config_gdbserver_quitgdb = strtoul(argv[1],0,0);
else
 config_gdbserver_quitgdb = 1;
 return 0;
}
mycmd_init(gdbserver,cmd_gdbserver,"gdbserver","gdbserver [port],\tenv:compareasid,comparedata,quitgdb");
mycmd_init(quitgdb,cmd_quitgdb,"quitgdb [n]","quitgdb n>1 force quit");
