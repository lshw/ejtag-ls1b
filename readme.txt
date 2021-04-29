-w[w|h|b]reg=val -uaddress=filename -daddress=xxx:size=xxx:filename=xxx

可以采用mydebug的方法将命令放到一个文件里面，依次执行。
reg read write
memory upload,download(wmem,rmem)
reg读写,内存读写可以采用gdbdaemon扩展



gdbdaemon用有读写寄存器的功能


