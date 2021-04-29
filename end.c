#include <cmdparser.h>
#include <flash.h>
#include <myconfig.h>

static int end(int argc,char **argv)
{
return 0;
}

struct flash_function end_flash_func __attribute__((section(".flash.init")));
mycmd_init(end,end,"","");
mysymbol_export(end);
myconfig_init("end",end,CONFIG_I,"end");
