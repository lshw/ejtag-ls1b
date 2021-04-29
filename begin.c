#include <cmdparser.h>
#include <flash.h>
#include <myconfig.h>

static int begin(int argc,char **argv)
{
return 0;
}
struct flash_function begin_flash_func __attribute__((section(".flash.init")));
mycmd_init(begin,begin,"","");
mysymbol_export(begin);
myconfig_init("begin",begin,CONFIG_I,"begin");
