#include <cmdparser.h>

int cmd_mytest(int argc,char **argv)
{
printf("mytest\n");
return 0;
}


mycmd_init(mytest,cmd_mytest,"cmds","mytest");

extern void add_cmd(struct mycmd *mycmd);
extern struct mycmd *mycmd_head;

void initcmd(struct mysymbol *head)
{
void (*add_cmd)(struct mycmd *mycmd);
add_cmd = find_mysymbol(head,"add_cmd");
add_cmd( &__mycmd_mytest);
}
