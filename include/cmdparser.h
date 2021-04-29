#ifndef __CMDPARSER_H 
#define __CMDPARSER_H
struct mycmd{
char *name;
int (*func)(int argc,char **argv);
char *usage;
char *note;
char *(*complete)(int argc,char **av,char *text,int start,int end);
struct mycmd *next;
};

struct myinit{
int (*func)(void);
};
#define mycmd_init(name,func,usage,note)                              \
	    struct mycmd  __mycmd_##name __attribute__ ((unused,__section__ (".mycmd"))) = {#name,func,usage,note,0,0};
#define mycmd_init1(name,func,usage,note,complete)                              \
	    struct mycmd  __mycmd_##name __attribute__ ((unused,__section__ (".mycmd"))) = {#name,func,usage,note,complete,0};


#endif
