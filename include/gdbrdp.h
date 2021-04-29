//
// GDBRDP.H
//
// Definitions for the GDB remote debugger interface commands
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

#ifndef GDBRDP_H
#define GDBRDP_H

//
// PROTOTYPES
//
int  fGDBRDP_Handler                  (int i_socket);
int  fGDBRDP_InputHandler             (int i_socket, char   c_byte);
int  fGDBRDP_CommandInterpreter       (int i_socket, char *pc_command, unsigned int ui_size);
void fGDBRDP_SendToGDB                (int i_socket, char *pc_message, unsigned int ui_length);
int  fGDBRDP_ConvertNibble            (char c_nibble);
char fGDBRDP_CalculateNibble          (int i_nibble);
int  fGDBRDP_HandleHardwareBreakpoint (char *pc_command, unsigned int ui_size);
int  fGDBRDP_HandleSoftwareBreakpoint (char *pc_command, unsigned int ui_size);

//
// DEFINES
//
#define dGDBRDP_INPUT_IDLE         0x01
#define dGDBRDP_OUT_OF_SYNC        0x02
#define dGDBRDP_OUT_OF_SYNC_2      0x03
#define dGDBRDP_RECEIVE_COMMAND    0x04
#define dGDBRDP_GET_CHECKSUM       0x05
#define dGDBRDP_GET_CHECKSUM_2     0x06

#define dGDBRDP_START_OF_COMMAND   '$'
#define dGDBRDP_END_OF_COMMAND     '#'
#define dGDBRDP_ACK                '+'
#define dGDBRDP_NACK               '-'
#define dGDBRDP_CTRLC              0x03

#define dGDBRDP_MAX_CMD_BUFFER     384

#define dGDBRDP_MAX_SW_BREAKPOINTS 25

extern unsigned int  ui_ejtag_state;
extern unsigned int  ui_last_signal;
#endif // #ifndef GDBRDP_H
