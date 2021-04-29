// JTAGSOCK.C
//
// Handles the socket interface for the JTAG emalation
// project.
//
// Based in large on the BSD sockets primer.
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

#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <netinet/in.h>
#include <netdb.h>
#include <asm/ioctls.h>
#include "jtagsock.h"
extern int config_gdbserver_quitgdb;

//
// fJTAGSOCK_Establish
//
// Attaches process to a TCP/IP port but does not wait
// for a connection.
//
int fJTAGSOCK_Establish (unsigned short us_portnum)
{ 
  char                c_myname[dJTAGSOCK_MAXHOSTNAME+1];
  int                 i_socket;
  struct sockaddr_in  st_sa;
  struct hostent     *st_hp;

  // Clear the address structure
  memset(&st_sa, 0, sizeof(struct sockaddr_in));

  // Get our own machine name
  gethostname(c_myname, dJTAGSOCK_MAXHOSTNAME);

  // Retrieve our address information
  st_hp=gethostbyname(c_myname);
  if (st_hp == NULL) return(-1);        // Error condition

  st_sa.sin_family = st_hp->h_addrtype;    // Our host address
  st_sa.sin_port   = htons(us_portnum); // Our port number

  // Create the socket
  i_socket = socket(AF_INET, SOCK_STREAM, 0);
  if (i_socket < 0) return(-1);         // Failed to create socket

  // Bind to the socket to our port
  if (bind(i_socket, (struct sockaddr *)&st_sa, sizeof(struct sockaddr_in)) < 0) 
  {
    // Failed to bind
    close(i_socket);
    return(-1);
  }

  // Set listen for maximum number of connections to queue
  listen(i_socket, dJTAGSOCK_NUM_CONNECTIONS);
  return(i_socket);
}

//
// fJTAGSOCK_GetConnection
//
// wait for a connection to occur on a socket created with fJTAGSOCK_Establish()
//
int fJTAGSOCK_GetConnection(int i_socket)
{ 
  int i_newsocket; // Handle for new socket

  // Attempt to accept a connection
  do{
     int n;
     char buf[100];
  i_newsocket = accept(i_socket,NULL,NULL);
	ioctl(0,FIONREAD,&n);
	if(n)
	{
	read(0,buf,n);
	buf[n] = 0;
	do_cmd(buf);
	if(config_gdbserver_quitgdb)return -1;
	}
  }
  while(i_newsocket == -1);

  if (i_newsocket < 0) return(-1); // Failed to accept

  return(i_newsocket);
}

//
// fJTAGSOCK_ReadByte
//
// Reads 1 byte of data from the socket
//
int fJTAGSOCK_ReadByte (int i_socket, char *pc_byte)
{ 
  return(read(i_socket, pc_byte, 0x01));
}

//
// fJTAGSOCK_WriteByte
//
// Writes 1 byte of data to the socket
//
int fJTAGSOCK_WriteByte (int i_socket, char c_byte)
{
	#if 1
  while(write(i_socket, &c_byte, 0x01) != 0x01);
	#else
	volatile int test=0;
	printf("haha-1\n");
	test = write(i_socket, &c_byte, 0x01);
	printf("haha-2 : test : %d\n",test);
	while(test != 0x01)
	{
		printf("haha-3\n");	
		test = write(i_socket, &c_byte, 0x01); 
		printf("haha-4 test : %d\n",test);	
	}
#endif
  return(1);
}
