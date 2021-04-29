#!/usr/bin/perl
use File::Temp;
while(<>)
{
if(/(.*)tgt_compile\s*\(\s*(".*")\s*\)(.*)/)
{
$head=$1;
$self=$2;
$tail=$3;
$tmpname=File::Temp::tempnam( "/tmp", "mycpp_");

print qq(${head}\{\n);
	open F,">$tmpname.S";
	print F ".set noreorder;\n.set noat;\n";
	while($self=m/"([^"]*)"/g)
	{
	($a=$1)=~s/deret/.word 0x4200001f;\n/g;
	$a=~s/\\n/\n/g;
	print F "$a";
	}
	close F;
	if(index(`uname -m`,"mips")>=0)
	{
	system("gcc -include include/regdef.h -c -o $tmpname.o $tmpname.S && objcopy  -j .text -O binary $tmpname.o $tmpname.bin");
	}
	else
	{
	system("mipsel-linux-gcc -include include/regdef.h -c -o $tmpname.o $tmpname.S && mipsel-linux-objcopy  -j .text -O binary $tmpname.o $tmpname.bin");
	}
	if($?)
	{
	print qq(#error mycpp\n);
	}
	else
	{
		unlink qq($tmpname.o);
		unlink qq($tmpname.S);
	open FS,"$tmpname.bin";
	my ($x,$i,$d);
	$i=0;
	while(!eof(FS))
	{
	printf  ("/*%08x:*/",$i*4) if($i%4==0);
	read FS,$d,4;
	$x=unpack "I",$d;
	printf  ("0x%08x",$x);
	$i++;
	if(!eof(FS))
	{
	printf  (","); 
	}
	}
	close(FS);
		unlink qq($tmpname.bin);
	}

print qq(\n\}$tail\n);

}
else { print; }
}
