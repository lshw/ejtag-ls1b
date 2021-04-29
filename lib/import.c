#include <dlfcn.h>
static int cmd_import(int argc,char **argv)
{
 void *handle;
 void (*initcmd)(void *);
 char *error;

 handle = dlopen(argv[1], RTLD_NOW|RTLD_GLOBAL);
 if (!handle) {
	 fprintf(stderr, "%s\n", dlerror());
	 exit(EXIT_FAILURE);
 }

 dlerror();    /* Clear any existing error */

 /* Writing: cosine = (double (*)(double)) dlsym(handle, "cos");
	would seem more natural, but the C99 standard leaves
	casting from "void *" to a function pointer undefined.
	The assignment used below is the POSIX.1-2003 (Technical
	Corrigendum 1) workaround; see the Rationale for the
	POSIX specification of dlsym(). */

 initcmd = dlsym(handle, "initcmd");

 if ((error = dlerror()) != NULL)  {
	 fprintf(stderr, "%s\n", error);
	 exit(EXIT_FAILURE);
 }

 initcmd(mysymbol_head);

// dlclose(handle);
 return 0;
}

mycmd_init(import,cmd_import,"cmds","echo");
