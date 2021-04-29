OPTIONS_CC = -g -DHAVE_NVENV -DNVRAM_IN_FLASH
OPTIONS_LD = -g -ldl -lreadline -L/usr/lib -lusb
CC=gcc

all: ejtag_debug_usb ejtag_debug_pp

gdb.o :lib/jtagemul.o lib/jtagsock.o lib/gdbrdp.o
	ld -r -o $@ $^
ejtag_debug_usb: begin.o main.o mips32.o usb_jtag.o usb_ejtag_command.o gdb.o disassemble.o end.o
	${CC} -g -o ejtag_debug_usb $^ -I . ${OPTIONS_LD}

ejtag_debug_pp: begin.o main.o mips32.o jtag.o parport.o gdb.o disassemble.o end.o
	${CC} -g  -o ejtag_debug_pp $^ -I . ${OPTIONS_LD}

usr: 
	make all;ar cr /tmp/ejtag.a *.o
	cp Makefile.usr /tmp/Makefile
	tar -C /tmp -rf /tmp/1.tar ejtag.a Makefile
	tar rf /tmp/1.tar include example.c  mycc  mycpp.pl 
	./ldd-pack ./ejtag_debug_usb |sh -
	./ldd-pack ./ejtag_debug_pp | sh -
	

main.o: main.c lib/env.c lib/put.c lib/sput.c

%.o:%.c
	./mycc -c -I. -I./lib -o $@ $< -I ./include ${OPTIONS_CC}
clean:
	rm -f *.out *.a *.o dev/*.o ejtag_debug_usb  ejtag_debug_pp *.tmp.c lib/*.o lib/*.tmp.c
