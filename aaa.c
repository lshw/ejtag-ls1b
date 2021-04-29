#include "usb_jtag.h"
#include"mips32.h"
#define EP_IN 0x6
#define EP_OUT 0x2
static unsigned char last_ir ;
static unsigned char updated ;
unsigned int usb_wr_ir_nwb(unsigned char new_ir_data);
unsigned int usb_wr_dr_nwb(unsigned int new_dr_data);


void usb_hardware_send(unsigned char*ptr,unsigned int len)
{
    int a ;
    a=usb_bulk_write(current_handle,EP_OUT,ptr,len,5000);
    if(a!=len)
    {
        printf("Request for bulk write failed: %s\n");
        return ;
    }
}

//make sure the length is sufficent
void usb_hardware_rcv(unsigned char*ptr,unsigned int len)
{
    int a=usb_bulk_read(current_handle,EP_IN,ptr,len,5000);
    if(a!=len)
    {
        printf("Request for bulk write failed: %s\n");
        return ;
    }
}

#define PACK_BUFF_SIZE 1024*1024*10
int pack_buff_cntr=0 ;
unsigned char pack_buffer[PACK_BUFF_SIZE];

// static unsigned char *pack_buffer;
int init_my_1=0;
#define MY_DEBUG 
//printf("%s :%d \n",__FUNCTION__,__LINE__);
unsigned char *usb_wr_cache(unsigned char*ptr,unsigned int len,int rd_len)
{
    int i,j,k,a ;

     //	if(ptr != NULL)
     //   	usb_hardware_send(ptr,len);
#if 1
    int do_send_now=a=0; //  (0==rd_len)?0:1 ;
/*
    if(!init_my_1)
    { 
	init_my_1 = 1;
    	pack_buffer = malloc(PACK_BUFF_SIZE);
    }
*/
    if(ptr==NULL)
	do_send_now=1 ;
    if(len==0)
	do_send_now=1 ;
    
    if(PACK_BUFF_SIZE<(pack_buff_cntr+len+64))
    {	
	do_send_now=1 ;
	a=1;
    }
    else 
	for(i=0;i<len;++i)
	//	pack_buff_cntr++;
		pack_buffer[pack_buff_cntr++]=ptr[i];
    MY_DEBUG
    // if(do_send_now&&(pack_buff_cntr!=0))
    if(do_send_now)
    {
    MY_DEBUG
        //printf("sending a packdge ,which size is %d\n",pack_buff_cntr);
        usb_hardware_send(pack_buffer,pack_buff_cntr);
        pack_buff_cntr=0 ;
	if(a)
	{
		for(i=0;i<len;++i)
	//	pack_buff_cntr++;
		 pack_buffer[pack_buff_cntr++]=ptr[i];
	}
    }
    MY_DEBUG
#endif

#if 0
    if(rd_len!=0)
    {
    MY_DEBUG
    	usb_hardware_rcv(ptr,rd_len);
    MY_DEBUG
    }
    MY_DEBUG
#endif
    return ptr ;
}


unsigned int lw_feed_data(unsigned int data)
{
    unsigned int ui_result ;
    // printf("Info : feed 0x%08x with 0x%08x\n",lw_get_ejtag_addr(),data);
    usb_wr_ir_nwb(dJTAG_REGISTER_DATA);
    usb_wr_dr_nwb(data);
    
    ui_result=dMIPS32_ECR_CPU_RESET_OCCURED|dMIPS32_ECR_PROBE_ENABLE|
    dMIPS32_ECR_PROBE_VECTOR ;
    usb_wr_ir_nwb(dJTAG_REGISTER_CONTROL);
    usb_wr_dr_nwb(ui_result);
    //	usb_wr_cache(NULL,0,0);
}

unsigned int usb_wr_ir_nwb(unsigned char new_ir_data)
{
    int a ;
    unsigned short usb_wr_ir_data ;
    //=0xff&(new_ir_data <<8 )+0x10;
    new_ir_data&=0x1f ;
    usb_wr_ir_data=new_ir_data+1<<4 ;
    if(last_ir==new_ir_data)return ;
    last_ir=new_ir_data ;
    updated=1 ;
}

unsigned int usb_wr_dr_nwb(unsigned int new_dr_data)
{
    unsigned short usb_wr_dr_data[20];
    static unsigned char buf[32];
    static unsigned char tmp[4];
    int i,a,count=6 ;
    unsigned int usb_wr_dr_result=0 ;
//    int i_data=(wb==0)?0:1 ;
//    wb=i_data ;
    if(updated)
    {
        tmp[0]=last_ir ;
        //usb_wr_ir_data&0xff;
        tmp[1]=0x10 ;
        //no need to write back.
        usb_wr_cache(tmp,2,0);
        updated=0 ;
    }
    usb_wr_dr_data[0]=OP_WR_DR<<OP_EJTAG_SHIFT ;
    // if (wb)	usb_wr_dr_data[0]|= 1 << OP_EJTAG_W_SHIFT ;
    //	if (wb)	usb_wr_dr_data[0]|= 1 << OP_EJTAG_I_SHIFT ;
    usb_wr_dr_data[1]=new_dr_data&0xFFFF ;
    usb_wr_dr_data[2]=(new_dr_data&0xFFFF0000)>>16 ;
    for(i=0;i<count;i=i+2)
    {
        buf[i]=usb_wr_dr_data[i/2]&0xFF ;
        buf[i+1]=(usb_wr_dr_data[i/2]&0xFF00)>>8 ;
    }
    
    usb_wr_cache(buf,6,0);
    
    return 0 ;
}

