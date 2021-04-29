#define REG_ADDRESS 0x0
#define CONFIG_BASE 0xaffffe00
    
#define note(x)
#include <stdio.h>

int ddr2_reg_data[] =
{
/*00:*/0x00000101,
/*01:*/0x01000100,
/*02:*/0x00010100,
/*03:*/0x00000000,
/*04:*/0x00000000,
/*05:*/0x01000101,
/*06:*/0x01000000,
/*07:*/0x01010000,
/*08:*/0x00010101,
/*09:*/0x01000202,
/*10:*/0x04040202,
/*11:*/0x00000000,
/*12:*/0x01020203,
/*13:*/0x0a030203,
/*14:*/0x00010708,
/*15:*/0x00000200,
/*16:*/0x01020408,
/*17:*/0x01020408,
/*18:*/0x00000000,
/*19:*/0x00000204,
/*20:*/0x3f070200,
/*21:*/0x1d1d1d3f,
/*22:*/0x1d1d1d1d,
/*23:*/0x5f7f1d1d,
/*24:*/0x05000000,
/*25:*/0x01ff0302,
/*26:*/0x01ff01ff,
/*27:*/0x01ff01ff,
/*28:*/0x01ff01ff,
/*29:*/0x01ff01ff,
/*30:*/0x00000004,
/*31:*/0x00b40020,
/*32:*/0x00000087,
/*33:*/0x000007ff,
/*34:*/0x0000009c,
/*35:*/0x00000000,
/*36:*/0x00000000,
/*37:*/0xffff0000,
/*38:*/0x00c8006b,
/*39:*/0x04b00002,
/*40:*/0x00c8000f,
/*41:*/0x00000000,
/*42:*/0x00030d40,
/*43:*/0x00000000,
/*44:*/0x00000000,
/*45:*/0x00000000,
/*46:*/0x00000000,
/*47:*/0x00000000,
/*48:*/0x00000000,
/*49:*/0x00000000,
/*50:*/0x00000000,
/*51:*/0x00000000,
/*52:*/0x00000000,
/*53:*/0x00000000,
/*54:*/0x00000000,
/*55:*/0x00000000,
/*56:*/0x00000000,
/*57:*/0x00000000,
/*58:*/0x01000100,
/*59:*/0x01010100,
/*60:*/0x01010100
};

int cmd_ddr2_config(int argc,char **argv)
{
FILE *fp;
char *ret;
char buf[256];
int count=0;
char *oldfifo;

if(argc < 2)
{
memcpy(ui_fifomem,ddr2_reg_data,sizeof(ddr2_reg_data));
}
else
{
fp=fopen(argv[1],"r");
if(fp == NULL) { perror("error open file"); return -1; }

do{
char *p;
ret = fgets(buf,256,fp);
if(!ret) break;
if(buf[0] == '/' || buf[0] == '#') continue;
if(!(p = strstr(buf,"0x")))continue;
ui_fifomem[count++] = strtoul(p,0,0);
} while(count < 61);
fclose(fp);
}

	uififo_raddr=0;

tgt_exec(\
"1:mtc0    v0, $31;\n" \
"li    v0,0xff200000;\n" \
"sw    v1,0x1fc(v0);\n" \
"sw    t1,0x1fc(v0);\n" \
"sw    t2,0x1fc(v0);\n" \
"li      t1, 0x1d;\n"
"li      t2, 0xaffffe00;\n" 
"101:\n"
"lw      v1, 0x1f8(v0);\n"
"sw      v1, 0(t2);\n"
"lw      v1, 0x1f8(v0);\n"
"sw      v1, 4(t2);\n"
"subu    t1, t1, 0x1;\n"
"bne     t1, $0, 101b;\n"
"addiu   t2, t2, 0x10;\n"

/*############start##########*/
"li      t2, 0xaffffe00;\n" 
"lw      v1, 0x1f8(v0);\n"
"sw      v1, 0x30(t2);\n"
"lw      v1, 0x1f8(v0);\n"
"sw      v1, 0x34(t2);\n"
"lw      t2, 0x1fc(v0);\n"
"lw      t1, 0x1fc(v0);\n"
"lw      v1, 0x1fc(v0);\n"
"mfc0    v0, $31;\n" \
"b 1b;\n"
"nop\n"
,0x8000);

return 0;
}

mycmd_init(ddr2_config,cmd_ddr2_config,"[ddr2.cfg]","init ddr2");
