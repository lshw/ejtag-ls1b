static int cmd_memtest(int argc,char **argv)
{
int i;
unsigned int start_address;
unsigned int end_address;
 start_address = strtoul(argv[1],0,0);
 end_address = strtoul(argv[2],0,0);
 ui_datamem[0] = start_address & ~3;
 ui_datamem[1] = end_address & ~3;
 ui_datamem[2] = "address %08x:write %08x read %08x error %08x all error %08x\n";
 tgt_exec(
"1:mtc0 a0,c0_desave;\n"
"lui a0,0xff20;\n"
"sw v0,0x1fc(a0);\n"
"sw v1,0x1fc(a0);\n"
"sw t0,0x1fc(a0);\n"
"sw t1,0x1fc(a0);\n"
"sw t2,0x1fc(a0);\n"
"sw t3,0x1fc(a0);\n"
"sw ra,0x1fc(a0);\n"
"lw t0,(a0);\n"
"lw t1,4(a0);\n"
"move t3,zero;\n"
"2:li v0,0;\n"
"sw v0,(t0);\n"
"lw v1,(t0);\n"
"beq v0,v1,3f;\n"
"nop;\n"
"bal 11f;\n"
"nop;\n"
"3:not v0;\n"
"sw v0,(t0);\n"
"lw v1,(t0);\n"
"beq v0,v1,4f;\n"
"nop;\n"
"bal 11f;\n"
"nop;\n"
"4:addu t0,4;\n"
"bne t0,t1,2b;\n"
"nop;\n"
"lw ra,0x1fc(a0);\n"
"lw t3,0x1fc(a0);\n"
"lw t2,0x1fc(a0);\n"
"lw t1,0x1fc(a0);\n"
"lw t0,0x1fc(a0);\n"
"lw v1,0x1fc(a0);\n"
"lw v0,0x1fc(a0);\n"
"mfc0 a0,c0_desave;\n"
"b 1b;\n"
"nop;\n"
"11:xor t2,v0,v1;\n"
"or t3,t2;\n"
"sw t0,12(a0);\n"
"sw v0,16(a0);\n"
"sw v1,20(a0);\n"
"sw t2,24(a0);\n"
"sw t3,28(a0);\n"
"li v1,2;\n"
"sb v1,0x1f5(a0);\n"
"jr ra;\n"
"nop;\n"
,0x8000);
return 0;	
}


static int cmd_memtest1(int argc,char **argv)
{
int i;
unsigned int start_address;
unsigned int end_address;
unsigned int start_val,incval;
 start_address = strtoul(argv[1],0,0);
 end_address = strtoul(argv[2],0,0);
 start_val = strtoul(argv[3],0,0);
 incval = strtoul(argv[4],0,0);
 ui_datamem[0] = start_address & ~3;
 ui_datamem[1] = end_address & ~3;
 ui_datamem[2] = start_val;
 ui_datamem[3] = incval;
 ui_datamem[7] = "address %08x:write %08x read %08x error %08x all error %08x\n";
 tgt_exec(
"1:mtc0 a0,c0_desave;\n"
"lui a0,0xff20;\n"
"sw v0,0x1fc(a0);\n"
"sw v1,0x1fc(a0);\n"
"sw t0,0x1fc(a0);\n"
"sw t1,0x1fc(a0);\n"
"sw t2,0x1fc(a0);\n"
"sw t3,0x1fc(a0);\n"
"sw t4,0x1fc(a0);\n"
"sw ra,0x1fc(a0);\n"
"lw t0,(a0);\n"
"lw t1,4(a0);\n"
"move t3,zero;\n"
"lw t4,12(a0);\n"
"lw v0,8(a0);\n"
"2:\n"
"sw v0,(t0);\n"
"addu v0,t4;\n"
"addu t0,4;\n"
"bne t0,t1,2b;\n"
"nop;\n"
"lw v0,8(a0);\n"
"lw t0,(a0);\n"
"3:\n"
"lw v1,(t0);\n"
"beq v0,v1,4f;\n"
"nop;\n"
"bal 11f;\n"
"nop;\n"
"4:\n"
"addu t0,4;\n"
"addu v0,t4;\n"
"bne t0,t1,3b;\n"
"nop;\n"
"lw ra,0x1fc(a0);\n"
"lw t4,0x1fc(a0);\n"
"lw t3,0x1fc(a0);\n"
"lw t2,0x1fc(a0);\n"
"lw t1,0x1fc(a0);\n"
"lw t0,0x1fc(a0);\n"
"lw v1,0x1fc(a0);\n"
"lw v0,0x1fc(a0);\n"
"mfc0 a0,c0_desave;\n"
"b 1b;\n"
"nop;\n"
"11:xor t2,v0,v1;\n"
"or t3,t2;\n"
"sw t0,0x20(a0);\n"
"sw v0,0x24(a0);\n"
"sw v1,0x28(a0);\n"
"sw t2,0x2c(a0);\n"
"sw t3,0x30(a0);\n"
"li v1,7;\n"
"sb v1,0x1f5(a0);\n"
"jr ra;\n"
"nop;\n"
,0x8000);
return 0;	
}

mycmd_init(memtest,cmd_memtest,"memtest startaddr endaddr","test memory rw one");
mycmd_init(memtest1,cmd_memtest1,"memtest startaddr endaddr initval incval","test memory rw some ");
