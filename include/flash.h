#ifndef __FLASH_H__
#define __FLASH_H__
struct flash_function{
char *name;
int *program;
int program_code_size;
int *erase;
void (*init)(void *);
int *erase_area;
struct flash_function *next;
};

extern struct flash_function *flash_func_head;
extern struct flash_function *flash_func_tail;

static inline void *register_flash(struct flash_function *flashfunc)
{
struct flash_function *p;
flash_func_tail->next = flashfunc;
flash_func_tail = flashfunc;
}
#endif
