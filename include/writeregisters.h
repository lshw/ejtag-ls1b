//
// writeregisters.c
//
// DO NOT EDIT THIS FILE.  This file was generated automatically
// by a script that converts an assembler listing into a C structure
// containing the hex values of the instructions in the listing.
// You must edit the assembler source directly and execute the build
// again.
//

unsigned int aui_writeregisters_code[] = 
tgt_compile(\
".set noat\n" \
"1:lui t7,0xff20;\n" \
"lw zero,0x1f8(t7);\n" \
"lw at,0x1f8(t7);\n" \
"mtc0 at,c0_desave;\n" \
"lui at,0xff20;\n" \
"lw v0,0x1f8(at);\n" \
"lw v1,0x1f8(at);\n" \
"lw a0,0x1f8(at);\n" \
"lw a1,0x1f8(at);\n" \
"lw a2,0x1f8(at);\n" \
"lw a3,0x1f8(at);\n" \
"lw t0,0x1f8(at);\n" \
"lw t1,0x1f8(at);\n" \
"lw t2,0x1f8(at);\n" \
"lw t3,0x1f8(at);\n" \
"lw t4,0x1f8(at);\n" \
"lw t5,0x1f8(at);\n" \
"lw t6,0x1f8(at);\n" \
"lw t7,0x1f8(at);\n" \
"lw s0,0x1f8(at);\n" \
"lw s1,0x1f8(at);\n" \
"lw s2,0x1f8(at);\n" \
"lw s3,0x1f8(at);\n" \
"lw s4,0x1f8(at);\n" \
"lw s5,0x1f8(at);\n" \
"lw s6,0x1f8(at);\n" \
"lw s7,0x1f8(at);\n" \
"lw t8,0x1f8(at);\n" \
"lw t9,0x1f8(at);\n" \
"lw k0,0x1f8(at);\n" \
"lw k1,0x1f8(at);\n" \
"lw gp,0x1f8(at);\n" \
"lw sp,0x1f8(at);\n" \
"lw s8,0x1f8(at);\n" \
"lw ra,0x1f8(at);\n" \
"sw v0,0x1fc(at);\n" \
"lw v0,0x1f8(at);\n" \
"mtc0 v0,c0_status;\n" \
"lw v0,0x1f8(at);\n" \
"mtlo v0;\n" \
"lw v0,0x1f8(at);\n" \
"mthi v0;\n" \
"lw v0,0x1f8(at);\n" \
"mtc0 v0,c0_badvaddr;\n" \
"lw v0,0x1f8(at);\n" \
"mtc0 v0,c0_cause;\n" \
"lw v0,0x1f8(at);\n" \
"mtc0 v0,c0_depc;\n" \
"lui at,0xff20;\n" \
"lw v0,0x1fc(at);\n" \
"mfc0 at,c0_desave;\n" \
"nop ;\n" \
"b 1b;\n" \
"nop ;\n" \
);

