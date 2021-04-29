#ifndef __MYCONFIG_H__
#define __MYCONFIG_H__
struct myconfig{
char *name;
void *config;
int type;
char *note;
char *(*complete)(char *text,int state);
struct mysymbol *next;
};
#define CONFIG_I 0
#define CONFIG_S 1


#define myconfig_init(name,config,type,note)                              \
	    struct myconfig  __myconfig_##config __attribute__ ((unused,__section__ (".myconfig"))) = {name,&config,type,note,0,0};
#define myconfig_init1(name,config,type,note,complete)                              \
	    struct myconfig  __myconfig_##config __attribute__ ((unused,__section__ (".myconfig"))) = {name,&config,type,note,complete,0};
#endif
