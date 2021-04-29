/*
 * put filename  addr 
 */
#include <stdlib.h>
#include <flash.h>
#include <myconfig.h>

static unsigned int code_program_sst16[]=
tgt_compile(\
"1:li v0,0xbfc05554;\n" \
"li v1,0xaa\n" \
"sh v1,0x5556(v0);\n" \
"li v1,0x55;\n" \
"sh v1,(v0);\n" \
"li v1,0xa0\n" \
"sh v1,0x5556(v0);\n" \
"lh v0,(a0);\n" \
"sh v0,(a1);\n" \
"2:\n" \
"lh v0,(a1);\n" \
"lh v1,(a1);\n" \
"bne v0,v1,2b;\n" \
"nop;\n" \
"addiu a0,2;\n" \
"addiu a1,2;\n" \
"addiu a2,-2;\n" \
"bnez a2,1b;\n" \
"nop;\n" \
"li ra,0xff200200 ;\n" \
"jr ra ;\n" \
"nop;\n" \
);

static unsigned int code_erase_sst16[]=
tgt_compile(\
"1:mtc0    v0, $31;\n" \
"li    v0,0xff200000;\n" \
"sw    v1,0x1fc(v0);\n" \
"li v0,0xbfc05554;\n" \
"li v1,0xaa;\n" \
"sh v1,0x5556(v0);\n" \
"li v1,0x55;\n" \
"sh v1,0(v0);\n" \
"li v1,0x80;\n" \
"sh v1,0x5556(v0);\n" \
"li v1,0xaa;\n" \
"sh v1,0x5556(v0);\n" \
"li v1,0x55;\n" \
"sh v1,0(v0);\n" \
"li v1,0x10;\n" \
"sh v1,0x5556(v0);\n" \
"li    v0,0xff200000;\n" \
"lw    v1,0x1fc(v0);\n" \
"mfc0    v0, $31;\n" \
"b 1b;\n" \
"nop;\n"
);

static unsigned int code_program_sst8[]=
tgt_compile(\
"1:li v0,0xbfc02aaa;\n" \
"li v1,0xaa\n" \
"sb v1,0x2aab(v0);\n" \
"li v1,0x55;\n" \
"sb v1,(v0);\n" \
"li v1,0xa0\n" \
"sb v1,0x2aab(v0);\n" \
"lb v0,(a0);\n" \
"sb v0,(a1);\n" \
"2:\n" \
"lb v0,(a1);\n" \
"lb v1,(a1);\n" \
"bne v0,v1,2b;\n" \
"nop;\n" \
"addiu a0,1;\n" \
"addiu a1,1;\n" \
"addiu a2,-1;\n" \
"bnez a2,1b;\n" \
"nop;\n" \
"li ra,0xff200200 ;\n" \
"jr ra ;\n" \
"nop;\n" \
);

static unsigned int code_erase_sst8[]=
tgt_compile(\
"1:mtc0    v0, $31;\n" \
"li    v0,0xff200000;\n" \
"sw    v1,0x1fc(v0);\n" \
"li v0,0xbfc02aaa;\n" \
"li v1,0xaa;\n" \
"sb v1,0x2aab(v0);\n" \
"li v1,0x55;\n" \
"sb v1,0(v0);\n" \
"li v1,0x80;\n" \
"sb v1,0x2aab(v0);\n" \
"li v1,0xaa;\n" \
"sb v1,0x2aab(v0);\n" \
"li v1,0x55;\n" \
"sb v1,0(v0);\n" \
"li v1,0x10;\n" \
"sb v1,0x2aab(v0);\n" \
"li    v0,0xff200000;\n" \
"lw    v1,0x1fc(v0);\n" \
"mfc0    v0, $31;\n" \
"b 1b;\n" \
"nop;\n"
);



static struct flash_function
flash_functions[]
={
	{"sst16",code_program_sst16,sizeof(code_program_sst16),code_erase_sst16},
	{"sst8",code_program_sst8,sizeof(code_program_sst8),code_erase_sst8},
	{0}
};

struct flash_function *flash_func_head;
struct flash_function *flash_func_tail;
extern struct flash_function begin_flash_func;
extern struct flash_function end_flash_func;

static void init_flash_func(void) __attribute__ ((constructor));
static void init_flash_func(void)
{
	int i;
	struct flash_function *pflash;
	flash_func_head = flash_functions;
	flash_func_tail = flash_functions;

	for(pflash = flash_functions + 1; pflash->name; pflash++)
	{
	flash_func_tail->next = pflash;
	flash_func_tail = pflash;
	}

	for(pflash = &begin_flash_func + 1; pflash != &end_flash_func; pflash++)
	{
		flash_func_tail->next = pflash;
		flash_func_tail = pflash;
	}

}

char *dupstr (int);

/* Generator function for command completion.  STATE lets us
   know whether to start from scratch; without any state
   (i.e. STATE == 0), then we start at the top of the list. */
static char * flash_complete (text, state)
     const char *text;
     int state;
{
  static int list_index, len;
  char *name;
 static struct flash_function *pflash;

  /* If this is a new word to complete, initialize now.  This
     includes saving the length of TEXT for efficiency, and
     initializing the index variable to 0. */
  if (!state)
    {
	  pflash = flash_func_head;
      len = strlen (text);
    }

  /* Return the next name which partially matches from the
     command list. */



  while (pflash && (name = pflash->name))
    {
	  pflash = pflash->next;

      if (strncmp (name, text, len) == 0)
        return (dupstr(name));
    }

  /* If no names matched, then return NULL. */
  return ((char *)NULL);

}


static char *config_flash_type = "sst8";

myconfig_init1("flash.type",config_flash_type,CONFIG_S,"flash type",flash_complete);

int cmd_program(int argc,char **argv)
{
	char *type;
	struct flash_function *pflash;
	int context_reg[dMIPS32_NUM_DEBUG_REGISTERS];
	if(!(type=config_flash_type))
	goto unfind;
	for(pflash=flash_func_head;pflash;pflash=pflash->next)
	if(!strcmp(pflash->name,type))break;
	if(!pflash)goto unfind;

	if(pflash->init) pflash->init(pflash);
    
	context_save(context_reg);

	jtag_putmem(pflash->program,help_addr,pflash->program_code_size);
	if(!(help_addr & 0x20000000)) cacheflush( help_addr, sizeof(pflash->program_code_size));

 {
unsigned int miro_inst_2[]=
tgt_compile(\
"bal 1f;\n" \
"nop;\n" \
".space	16 \n" \
"1:\n" \
"lw v0,(ra);\n" \
"lw a0,4(ra);\n" \
"lw a1,8(ra);\n" \
"lw a2,12(ra);\n" \
"jr v0;\n" \
"nop ;\n" \
);
	    miro_inst_2[2]=help_addr;
	    miro_inst_2[3]=strtoul(argv[1],0,0); //ram address
	    miro_inst_2[4]=strtoul(argv[2],0,0); //flash address
	    miro_inst_2[5]=strtoul(argv[3],0,0);
    fMIPS32_ExecuteDebugModule(miro_inst_2,sizeof(miro_inst_2),1);
}

    context_restore(context_reg);


    return 0;
unfind:
	printf("unfind flash,doest not setevn flash,or unsuppoted flashtype.\nmaybe run  setenv flash sst8\n");
	return -1;
}

int cmd_erase(int argc,char **argv)
{
	char *type;
	struct flash_function *pflash;

	if(!(type=config_flash_type))
	goto unfind;
	for(pflash=flash_func_head;pflash;pflash=pflash->next)
	if(!strcmp(pflash->name,type))break;
	if(!pflash)goto unfind;

	if(pflash->init) pflash->init(pflash);
    fMIPS32_ExecuteDebugModule(pflash->erase,0x8000,1);
return 0;
unfind:
	printf("unfind flash,doest not setevn flash,or unsuppoted flashtype.\nmaybe run  setenv flash sst8\n");
	return -1;

}

int cmd_erase_area(int argc,char **argv)
{
	char *type;
	struct flash_function *pflash;
	unsigned int start_addr,end_addr,sector_size;

	if(argc != 4) return -1;

	if(!(type=config_flash_type))
	goto unfind;
	for(pflash=flash_func_head;pflash;pflash=pflash->next)
	if(!strcmp(pflash->name,type))break;
	if(!pflash)goto unfind;
	if(!pflash->erase_area)goto unsupport;
	if(pflash->init) pflash->init(pflash);
	start_addr = strtoul(argv[1],0,0);
	end_addr = strtoul(argv[2],0,0);
	sector_size = strtoul(argv[3],0,0);
	ui_datamem[0] = start_addr;
	ui_datamem[1] = end_addr;
	ui_datamem[2] = sector_size;
	printf ("start_addr = 0x%x, end_addr = 0x%x, sector_size =  0x%x !\n", start_addr, end_addr, sector_size);
    fMIPS32_ExecuteDebugModule(pflash->erase_area,0x8000,1);
return 0;
unfind:
	printf("unfind flash,doest not setevn flash,or unsuppoted flashtype.\nmaybe run  setenv flash sst8\n");
	return -1;
unsupport:
	printf("unsupport erase flash area\n");
	return -2;
}

mycmd_init(program,cmd_program,"program ramaddr flashaddr size ","program  address,\tenv: flash");
mycmd_init(erase,cmd_erase,"erase","erase whole chip,\tenv: flash");
mycmd_init(erase_area,cmd_erase_area,"erase_area start end sectorsize","erase sectors,\tenv: flash");

