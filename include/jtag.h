//
// JTAG.H
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

#ifndef dJTAG_H
#define dJTAG_H

//
// PROTOTYPES
//
void          fJTAG_Init       (void);
unsigned int  fJTAG_Data       (unsigned int  ui_data_out);
unsigned char fJTAG_Instruction(unsigned char uc_data_out);

//
// DEFINES
//
#define dJTAG_REGISTER_EXTEST       0x00
#define dJTAG_REGISTER_IDCODE       0x01
#define dJTAG_REGISTER_SAMPLE       0x02
#define dJTAG_REGISTER_IMPCODE      0x03
#define dJTAG_REGISTER_ADDRESS      0x08
#define dJTAG_REGISTER_DATA         0x09
#define dJTAG_REGISTER_CONTROL      0x0A
#define dJTAG_REGISTER_ALL          0x0B
#define dJTAG_REGISTER_EJTAG_BOOT   0x0C
#define dJTAG_REGISTER_NORMAL_BOOT  0x0D
#define dJTAG_REGISTER_FASTDATA     0x0E
#define dJTAG_REGISTER_DBTX	    0x1C
#define dJTAG_REGISTER_EJWATCH      0x1C
#define dJTAG_REGISTER_DBDX	    0x1D
#define dJTAG_REGISTER_BYPASS       0x1F

#endif // #ifndef dJTAG_H
