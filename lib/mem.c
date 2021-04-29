
static union commondata{
		unsigned char data1;
		unsigned short data2;
		unsigned int data4;
		unsigned int data8[2];
		unsigned char c[8];
}mydata,*pmydata;

static int __syscall1(int type,unsigned long long addr,union commondata *mydata);
static int __syscall2(int type,unsigned long long addr,union commondata *mydata);
static int (*syscall1)(int type,unsigned long long addr,union commondata *mydata);
static int (*syscall2)(int type,unsigned long long addr,union commondata *mydata);


static int __syscall1(int type,unsigned long long addr,union commondata *mydata)
{

switch(type)
{
case 1:
	  mydata->data1=jtag_inb(addr);
	   break;
case 2:
	  mydata->data2=jtag_inw(addr);
	   break;
case 4:
	  mydata->data4=jtag_inl(addr);
	   break;
case 8:
	  mydata->data8[0]=jtag_inl(addr);
	  mydata->data8[1]=jtag_inl(addr+4);
	   break;
}

return 0;
}

static int __syscall2(int type,unsigned long long addr,union commondata *mydata)
{

switch(type)
{
case 1:
	 jtag_outb(addr,mydata->data1);
	   break;
case 2:
	  jtag_outw(addr,mydata->data2);
	  break;
case 4:
	  jtag_outl((long)addr,mydata->data4);
	    break;
case 8:
	   jtag_outl(addr,mydata->data8[0]);
	   jtag_outl(addr+4,mydata->data8[1]);
	   break;
}

return 0;
}



static int (*syscall1)(int type,unsigned long long addr,union commondata *mydata)=&__syscall1;
static int (*syscall2)(int type,unsigned long long addr,union commondata *mydata)=&__syscall2;

static unsigned long lastaddr=0;
static int dump(int argc,char **argv)
{
		char type=4;
static	unsigned long addr,count=1;
		int i,j,k;
		char memdata[16];
//		char opts[]="bhwd";
		if(argc>3){return -1;}

		switch(argv[0][1])
		{
				case '1':	type=1;break;
				case '2':	type=2;break;
				case '4':	type=4;break;
				case '8':	type=8;break;
		}

		if(argc>1)addr=strtoul(argv[1],0,0);
		else addr=lastaddr;
		if(argc>2)count=strtoul(argv[2],0,0);
		else if(count<=0||count>=1024) count=1;
		for(j=0;j<count;j=j+16/type,addr=addr+16/syscall_addrwidth)
		{
		printf("%08lx: ",addr);

		pmydata=(void *)memdata;
		for(i=0;type*i<16;i++)
		{
		if(syscall1(type,addr+i*type/syscall_addrwidth,pmydata)<0){printf("read address %p error\n",addr+i*type/syscall_addrwidth);return -1;}
		pmydata=(void *)((char *)pmydata+type);
		if(j+i+1>=count)break;
		}
		
		pmydata=(void *)memdata;
		for(i=0;type*i<16;i++)
		{
		switch(type)
		{
		case 1:	printf("%02x ",pmydata->data1);break;
		case 2: printf("%04x ",pmydata->data2);break;
		case 4: printf("%08x ",pmydata->data4);break;
		case 8: printf("%08x%08x ",pmydata->data8[1],pmydata->data8[0]);break;
		}
		if(j+i+1>=count){int k;for(i=i+1;type*i<16;i++){for(k=0;k<type;k++)printf("  ");printf(" ");}break;}
		pmydata=(void *)((char *)pmydata+type);
		}
		
		pmydata=(void *)memdata;
		#define CPMYDATA ((char *)pmydata)
		for(k=0;k<16;k++)
		{
		printf("%c",(CPMYDATA[k]<0x20 || CPMYDATA[k]>0x7e)?'.':CPMYDATA[k]);
		if(j+(k+1)/type>=count)break;
		}
		printf("\n");
		}
		lastaddr=addr+count*type;
		return 0;
}

static int getdata(char *str)
{
	static char buf[17];
	char *pstr;
	int sign=1;
	int radix=10;
	pstr=strsep(&str," \t\x0a\x0d");

		if(pstr)
		{
		if(pstr[0]=='q')return -1;
		memset(buf,'0',16); buf[17]=0;
		if(pstr[0]=='-')
		{
		sign=-1;
		pstr++;
		}
		else if(pstr[0]=='+')
		{
			pstr++;
		}
		
		if(pstr[0]!='0'){radix=10;}
		else if(pstr[1]=='x'){radix=16;pstr=pstr+2;}

		memcpy(buf+16-strlen(pstr),pstr,strlen(pstr));
		pstr=buf;
		pstr[16]=pstr[8];pstr[8]=0;
		mydata.data8[1]=strtoul(pstr,0,radix);
		pstr[8]=pstr[16];pstr[16]=0;
		mydata.data8[0]=strtoul(&pstr[8],0,radix);
		if(sign==-1)
		{
		long x=mydata.data8[0];
			mydata.data8[0]=-mydata.data8[0];
			if(x<0)
			mydata.data8[1]=-mydata.data8[1];
			else mydata.data8[1]=~mydata.data8[1];
			
		}
		return 1;
		}
		return 0;

}

static int modify(int argc,char **argv)
{
		char type=4;
		unsigned long addr;
//		char opts[]="bhwd";
		char str[100];
		int i;

		if(argc<2){return -1;}

		switch(argv[0][1])
		{
				case '1':	type=1;break;
				case '2':	type=2;break;
				case '4':	type=4;break;
				case '8':	type=8;break;
		}
		addr=strtoul(argv[1],0,0);
		if(argc>2)
		{
		 i=2;
	          while(i<argc)
		 {
	       	   getdata(argv[i]);
		   if(syscall2(type,addr,&mydata)<0)
		   {printf("write address %p error\n",addr);return -1;};
		   addr=addr+type/syscall_addrwidth;
		 i++;
		 }
		  return 0;
		}


		while(1)
		{
		if(syscall1(type,addr,&mydata)<0){printf("read address %p error\n",addr);return -1;};
		printf("%08lx:",addr);
		switch(type)
		{
		case 1:	printf("%02x ",mydata.data1);break;
		case 2: printf("%04x ",mydata.data2);break;
		case 4: printf("%08x ",mydata.data4);break;
		case 8: printf("%08x%08x ",mydata.data8[1],mydata.data8[0]);break;
		}
		memset(str,0,100);
		read(0,str,100);
	        i=getdata(str);
		if(i<0)break;	
		else if(i>0) 
		{
		if(syscall2(type,addr,&mydata)<0)
		{printf("write address %p error\n",addr);return -1;};
		}
	addr=addr+type/syscall_addrwidth;
		}	
		lastaddr=addr;	
		return 0;
}
static int cmd_mems(int argc,char **argv)
{
syscall1=&__syscall1;
syscall2=&__syscall2;
syscall_addrwidth=1;
	return 0;
}

mycmd_init(mems,cmd_mems,"mems","select memory");
mycmd_init(d1,dump,"d1 [addr] [count]","dump memory (byte)");
mycmd_init(d2,dump,"d2 [addr] [count]","dump memory (half word)");
mycmd_init(d4,dump,"d4 [addr] [count]","dump memory (word)");
mycmd_init(d8,dump,"d8 [addr] [count]","modify memory (double word)");
mycmd_init(m1,modify,"m1 addr","modify memory (byte)");
mycmd_init(m2,modify,"m2 addr","modify memory (half word)");
mycmd_init(m4,modify,"m4 addr","modify memory (word)");
mycmd_init(m8,modify,"m8 addr","modify memory (double word)");
