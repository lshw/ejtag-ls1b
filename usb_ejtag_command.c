/*
	GS232 USB EJTAG Simulator.
	Author : ZGJ 
	Date : 20090609
	Version :V1.0
	email: zhouguojian@ict.ac.cn
*/
#include <stdio.h>
#include "usb_ejtag_command.h"
//#include "usb_jtag.h"
#include "aaa.c"
#include <myconfig.h>

static int config_usb_ejtag_put_speed = 0;

myconfig_init("usb_ejtag.put_speed",config_usb_ejtag_put_speed,CONFIG_I,"usb ejtag put speed");

#if 0
#define USB_FOUND 1
#define USB_NO_FOUND 0
#define USB_NOT_BULK_WR_FLAG 0
#define USB_CAN_BULK_WR_FLAG 1
#define LS232_USB_EJTAG_VENDOR_ID  0x0547
#define LS232_USB_EJTAG_PRODUCT_ID 0x1002

int usb_can_bulk_wr_flag = USB_NOT_BULK_WR_FLAG;
unsigned short usb_wr_dr_data[3] ={0};
unsigned short dma32_write1_data[DMA32_WR1_MAX+1]={0};
unsigned short dma32_write2_data[DMA32_WR2_MAX+3]={0};
unsigned short signal_echo_data[3] ={0};
unsigned short burst_echo_data[BUSRT_ECHO_MAX+3] ={0};

struct usb_device *current_device;
usb_dev_handle *current_handle;
char *bus_name="001";
char *device_name="001";
#endif
unsigned int usb_ejtag_ID_IP_CODE(void);
struct usb_device *find_device(char *busname, char *devicename)
{
	struct usb_bus *p;
	struct usb_device *q;
	p=usb_busses;
	while(p!=NULL)
	{
        	q=p->devices;
        	if(strcmp(p->dirname, busname))
		{
                	p=p->next;
                	continue;
                }
        	while(q!=NULL)
		{
                	if(!strcmp(q->filename, devicename))
				return q;
                	q=q->next;
                }
        	p=p->next;
        }
	
	return NULL;
}


int dump_busses(void)
{
	struct usb_bus *p;
	struct usb_device *q;
	int found_usb = USB_NO_FOUND ;
	p = usb_busses;
//	printf("Dump of USB subsystem:\n");
	while(p!=NULL)
	{
        	q = p->devices;
        	while(q != NULL)
		{
                	fprintf(stderr," bus %s device %s vendor id=0x%04x product id=0x%04x ",
                        	p->dirname, q->filename, q->descriptor.idVendor, q->descriptor.idProduct);
			if((q->descriptor.idVendor == LS232_USB_EJTAG_VENDOR_ID) && (q->descriptor.idProduct == LS232_USB_EJTAG_PRODUCT_ID))
			{
				bus_name = p->dirname ;
				device_name = q->filename ;
				found_usb = USB_FOUND;
				fprintf(stderr," : (LS232 FX2 USB-Ejtag-Simulator is found)\n");
				return found_usb;
			}
                	q = q->next;
			fprintf(stderr,"\n");
                }
        	p = p->next;
        }
	return found_usb;
}

int usb_tap_init(void)
{
	int found_usb = USB_NO_FOUND;
	usb_init();		//Init the USB driver data structure
	usb_find_busses();	//search the system USB bus
	usb_find_devices();	//find the USB bus devices
	found_usb = dump_busses();	//search the USB EJTAG-EMU is or not
	if(!found_usb)
		return USB_NO_FOUND;

	current_device=find_device(bus_name,device_name);  //get the USB-ejtag-simulator device structure

	if(current_device == NULL)
	{
                fprintf(stderr,"Cannot find device %s on bus %s\n", device_name, bus_name);
                found_usb = USB_NO_FOUND;
	} 

	return found_usb;
}

int usb_fx2_reset(void)
{
	current_handle = usb_open(current_device); //open the ejtag simulator,return the device discription

#if 1
        usb_clear_halt(current_handle,2);
//      usb_clear_halt(current_handle,4);
	usb_clear_halt(current_handle,0x86);
//      usb_clear_halt(current_handle,8);
#endif
        if(usb_detach_kernel_driver_np(current_handle, 0)<0)
        {
        	fprintf(stderr," %s\n", usb_strerror());
                return;
        }
        if(usb_set_configuration(current_handle,1))
        {
                fprintf(stderr," %s\n", usb_strerror());
                return;
        }
	
        while(usb_claim_interface(current_handle,0)<0)
        {
                fprintf(stderr,"usb_claim_interface() failed,need to usb_detach_kernel_driver_np!\n");
                if(usb_detach_kernel_driver_np(current_handle, 0)<0)
                {
			usb_can_bulk_wr_flag = USB_NOT_BULK_WR_FLAG ;
                        fprintf(stderr," %s\n", usb_strerror());
                        exit(-1);
                }
        }
        fprintf(stderr,"usb_claim_interface() Successful !\n");

	usb_ejtag_ID_IP_CODE();

	usb_can_bulk_wr_flag = USB_CAN_BULK_WR_FLAG ;

	return usb_can_bulk_wr_flag ;
}


void usb_stop(void)
{
	if(usb_can_bulk_wr_flag == USB_CAN_BULK_WR_FLAG)
	{
		usb_release_interface(current_handle,0);
		usb_close(current_handle);
	}
	else
		usb_close(current_handle);
	fprintf(stderr,"Close the USB\n");
}

unsigned short tap_reset(void)
{
	unsigned short tap_reset_data=0;
	
	tap_reset_data = OP_TAP_RST << OP_EJTAG_SHIFT ;

	return tap_reset_data;

}

unsigned short target_reset(void)
{

	unsigned short target_reset_data=0;
	
	target_reset_data = OP_TARGET_RST << OP_EJTAG_SHIFT ;

	return target_reset_data;

}

/* Bit0 ----- led_on_flag : 1, LED on ;led_on_flag : 0, LED off */
/* Bit1 ----- VIO : Display the Simulator the power is normal */
unsigned short fpga_info_ejtag(unsigned short led_on_flag )
{
	unsigned short fpga_info_ejtag_data=0;

	fpga_info_ejtag_data = OP_FPGA_INFO << OP_EJTAG_SHIFT | led_on_flag ;

	return fpga_info_ejtag_data;

}

unsigned int usb_wr_ir(unsigned short wb,unsigned  short i_data ,unsigned char new_ir_data)
{

        int a=0,count = 2 ;
	unsigned short usb_wr_ir_data=0;
	unsigned char buf[2]={0,0};
	unsigned char buf_read[2]={0,0};
	unsigned int usb_wr_ir_result=0;

	usb_wr_ir_data = OP_WR_IR << OP_EJTAG_SHIFT |  wb << OP_EJTAG_W_SHIFT |i_data << OP_EJTAG_I_SHIFT | new_ir_data;	

        memset(buf, 0, 2);
        memset(buf_read, 0, 2);

        buf[0] = usb_wr_ir_data & 0xFF;
        buf[1] = (usb_wr_ir_data & 0xFF00) >>8;

#ifdef DEBUG_USB_EJTAG
        for(i=0 ; i<count ; i++)
        {
                if((i%16 == 0) && i>0 )
                        fprintf(stderr,"\n");
                fprintf(stderr,"0x%2x ",buf[i]);
        }
        fprintf(stderr,"\n====================================================================================\n");
#endif

        endpoint = 2 ; // Output Endpoint
        a = usb_bulk_write(current_handle, endpoint, buf, count, 1000);

#ifdef DEBUG_USB_EJTAG
        printf("usb_bulk_write end\n");
#endif
        if(a<0)
        {
                fprintf(stderr,"Request for bulk write failed: %s\n", usb_strerror());
                usb_release_interface(current_handle, 0);
                return;
        }

	if(wb == 1)
	{
        	endpoint = 6 ; // Input Endpoint
        	a = usb_bulk_read(current_handle, endpoint, buf_read, count, 1000);

        	if(a<0)
        	{
                	fprintf(stderr,"Request for Burst bulk read failed: %s\n", usb_strerror());
        		//      usb_release_interface(current_handle, 0);
        		//      return;
        	}
	

//zgj        usb_release_interface(current_handle, 0);

	#ifdef DEBUG_USB_EJTAG
        	for(i=0;i<count;i++)
        	{
                	if((i%16 == 0) && i>0 )
                        	printf("\n");
                	printf("0x%x",buf_read[i]);
        	}
        	printf("\n");
	#endif
		usb_wr_ir_result = buf_read[1] << 8 |buf_read[0] ;
	}

	return usb_wr_ir_result;

}

#if 1

unsigned int usb_wr_upload(unsigned char *buf_read,unsigned  int len)
{
        int i,a,count = 8 ;
        unsigned char buf[8];

	unsigned int usb_wr_dr_result=0;
//	short usb_wr_dr_data[3] ={0};
	usb_wr_dr_data[0] = OP_WR_UPLOAD <<OP_EJTAG_SHIFT | 0x0 ;
	fast_write2_data[1] = config_usb_ejtag_put_speed;
	usb_wr_dr_data[2] = (len/4) & 0xFFFF;
	usb_wr_dr_data[3] = ((len/4) & 0xFFFF0000) >> 16;
	

fprintf(stderr,"len : 0x%x\n",len);
        if(count > 8)
        {
                fprintf(stderr, "Cannot transfer more than 8 bytes at a time.\n");
                exit(-1);
        }

        memset(buf, 0, 8);
        memset(buf_read, 0, len);

        for(i=0;i<count;i=i+2)
        {
                buf[i] = usb_wr_dr_data[i/2] & 0xFF;
                buf[i+1] = (usb_wr_dr_data[i/2] & 0xFF00) >>8;
        }

#ifdef DEBUG_USB_EJTAG
        for(i=0 ; i<count ; i++)
        {
                if((i%16 == 0) && i>0 )
                        printf("\n");
                printf("0x%2x ",buf[i]);
        }
        printf("\n========================================================================================\n");
#endif
#if 1
        endpoint = 2 ; // Output Endpoint
        a = usb_bulk_write(current_handle, endpoint, buf, count, 1000);

#ifdef DEBUG_USB_EJTAG
        printf("usb_bulk_write end\n");
#endif
        if(a<0)
        {
                fprintf(stderr,"Request for bulk write failed: %s\n", usb_strerror());
                usb_release_interface(current_handle, 0);
                return;
        }

        endpoint = 6 ; // Input Endpoint
	count=len ;
fprintf(stderr,"haha-1\n");
        a = usb_bulk_read(current_handle, endpoint, buf_read, len, 15000);
fprintf(stderr,"haha-2\n");

        if(a<0)
        {
                fprintf(stderr,"Request for Burst bulk read failed: %s\n", usb_strerror());
        //      usb_release_interface(current_handle, 0);
        //      return;
        }
#endif

//zgj        usb_release_interface(current_handle, 0);

//#ifdef DEBUG_USB_EJTAG
#if 0
        for(i=0;i<count;i++)
        {
                if((i%8 == 0) && i>0 )
		{
                        printf("\n");
			printf(" 0x%x ==",i);
		}
                printf("0x%x ",buf_read[i]);
        }
        printf("\n");
#endif

	usb_wr_dr_result = buf_read[3] << 24 |buf_read[2] << 16 |buf_read[1] << 8 |buf_read[0] ;
	return usb_wr_dr_result;

}
#endif

unsigned int usb_wr_dr(unsigned short wb, unsigned short i_data ,unsigned  int new_dr_data)
{
        int i,a,count = 6 ;
        unsigned char buf[6];
        unsigned char buf_read[6];

	unsigned int usb_wr_dr_result=0;
//	short usb_wr_dr_data[3] ={0};
	usb_wr_dr_data[0] = OP_WR_DR << OP_EJTAG_SHIFT | wb << OP_EJTAG_W_SHIFT | i_data << OP_EJTAG_I_SHIFT ;
	usb_wr_dr_data[1] = new_dr_data & 0xFFFF;
	usb_wr_dr_data[2] = (new_dr_data & 0xFFFF0000) >> 16;

        if(count > 6)
        {
                fprintf(stderr, "Cannot transfer more than 6 bytes at a time.\n");
                exit(-1);
        }

        memset(buf, 0, 6);
        memset(buf_read, 0, 6);

        for(i=0;i<count;i=i+2)
        {
                buf[i] = usb_wr_dr_data[i/2] & 0xFF;
                buf[i+1] = (usb_wr_dr_data[i/2] & 0xFF00) >>8;
        }

#ifdef DEBUG_USB_EJTAG
        for(i=0 ; i<count ; i++)
        {
                if((i%16 == 0) && i>0 )
                        printf("\n");
                printf("0x%2x ",buf[i]);
        }
        printf("\n========================================================================================\n");
#endif
#if 1
        endpoint = 2 ; // Output Endpoint
        a = usb_bulk_write(current_handle, endpoint, buf, count, 1000);

#ifdef DEBUG_USB_EJTAG
        printf("usb_bulk_write end\n");
#endif
        if(a<0)
        {
                fprintf(stderr,"Request for bulk write failed: %s\n", usb_strerror());
                usb_release_interface(current_handle, 0);
                return;
        }

        endpoint = 6 ; // Input Endpoint
	count=4;
        a = usb_bulk_read(current_handle, endpoint, buf_read, count, 1000);

        if(a<0)
        {
                fprintf(stderr,"Request for Burst bulk read failed: %s\n", usb_strerror());
        //      usb_release_interface(current_handle, 0);
        //      return;
        }
#endif

//zgj        usb_release_interface(current_handle, 0);

#ifdef DEBUG_USB_EJTAG
        for(i=0;i<count;i++)
        {
                if((i%16 == 0) && i>0 )
                        printf("\n");
                printf("0x%x",buf_read[i]);
        }
        printf("\n");
#endif
	usb_wr_dr_result = buf_read[3] << 24 |buf_read[2] << 16 |buf_read[1] << 8 |buf_read[0] ;
	return usb_wr_dr_result;

}

// unsigned int usb_ejtag_ID_IP_CODE(unsigned char test_ir)
unsigned int usb_ejtag_ID_IP_CODE(void)
{
unsigned int input_e=0,output_e=0;
usb_wr_ir(1,1,0x01);
input_e=0xffffffff;
output_e=usb_wr_dr(1,1,0x01);
fprintf(stderr,"ID-CODE : 0x%x\n",output_e);

usb_wr_ir(1,1,0x03);
input_e=0xffffffff;
output_e=usb_wr_dr(1,1,0x01);
fprintf(stderr,"IP-CODE : 0x%x\n",output_e);

return output_e;
}
//DMA32
unsigned short * dma32_write1(unsigned short count)
{

	unsigned short i=0;
//	short dma32_write1_data[1025]={0};
	if(count > DMA32_WR1_MAX)
	{
		printf("dma32_write1 write size MAX 1K is overflow!\n");
		return -1;
	}
	dma32_write1_data[0] = OP_DMA32_WRITE1 << OP_EJTAG_SHIFT | count ;
	for(i=1;i<count;i++)
		dma32_write1_data[i]=(count - i);
	return dma32_write1_data;

}

unsigned short * dma32_write2(unsigned int dma32_count,unsigned int * dma32_write2_data)
{

	unsigned int i=0;
	// short burst_echo_data[BUSRT_ECHO_MAX+3] ={0};

	dma32_write2_data[0] = OP_DMA32_WRITE2 << OP_EJTAG_SHIFT ;
	dma32_write2_data[1] = dma32_count & 0xFFFF;
	dma32_write2_data[2] = (dma32_count & 0xFFFF0000) >> 16;

	if(dma32_count > DMA32_WR2_MAX)
	{
		printf("burst_count is overflow , MAX size is 0x%x \n",DMA32_WR2_MAX);
		dma32_count = DMA32_WR2_MAX;
	//	return -1;
	}

	for(i=3;i<dma32_count;i++)
		dma32_write2_data[i] = ((i%256)<<8 | ((i%256)+0x30));
#if 0
	for(i=0;i<burst_count;i++)
	{
		if(i%16 == 0)
			printf("\n");
		printf("0x%4x ",dma32_write2_data[i]);
	}
#endif
	return dma32_write2_data;

}

#if 1
unsigned short * fast_write_3(unsigned int fast_count,unsigned int * fast_write2_data_input)
{
	int i=0;
//	static int j=0;
	int fast_count_size = fast_count/4;
	if(fast_count_size%4)
		fast_count_size++;

	for( ;i<fast_count_size; i++)
	{
		lw_feed_data(fast_write2_data_input[i]);	
	}
#if 0
	j++;
	if(j>82)
	{
	   printf("j = %d \n",j);
		getchar();
		getchar();
	}
#endif
	// printf("usb_wr_cache before \n");
	usb_wr_cache(NULL,0,0);
	// printf("usb_wr_cache after \n");
//	free(pack_buffer);
}
#endif

#if 1

unsigned short * fast_write2(unsigned int fast_count,unsigned int * fast_write2_data_input)
{

	unsigned int i=0;
	unsigned int j=0;
	unsigned int count=0;
	int a=0;
	unsigned int download_size_fast=fast_count/4;
	unsigned char *buf ;
// [FAST_WR2_MAX];

	// short burst_echo_data[BUSRT_ECHO_MAX+3] ={0};
        if(fast_count%4)
		download_size_fast++;
	printf("fast_count : 0x%x , download_size_fast : 0x%x\n",fast_count,download_size_fast);
	fast_write2_data[0] = OP_FAST_WRITE2 << OP_EJTAG_SHIFT ;
	fast_write2_data[1] = config_usb_ejtag_put_speed;
	fast_write2_data[2] = (download_size_fast& 0xFFFF); // fast_count & 0xFFFF;
	fast_write2_data[3] =(download_size_fast & 0xFFFF0000) >> 16 ; // (fast_count & 0xFFFF0000) >> 16;

	if(fast_count > FAST_WR2_MAX)
	{
		printf("fast_count is overflow , MAX size is 0x%x \n",FAST_WR2_MAX);
		fast_count = FAST_WR2_MAX;
	//	return -1;
	}
	
	buf=malloc(FAST_WR2_MAX);

	for(i=4;j<download_size_fast;j++)
	{
		fast_write2_data[i++] = (fast_write2_data_input[j]& 0xFFFF);
		fast_write2_data[i++] = (fast_write2_data_input[j]& 0xFFFF0000) >> 16;
	}
                // ((i%256)<<8 | ((i%256)+0x30));
		//fast_write2_data[i] = ((i%256)<<8 | ((i%256)+0x30));
	memset(buf,0,FAST_WR2_MAX);

	count = fast_count + 8 ;
        for(i=0;i<count;i=i+2)
        {
                buf[i] = fast_write2_data[i/2] & 0xFF;
                buf[i+1] = (fast_write2_data[i/2] & 0xFF00) >>8;
        }

// #ifdef DEBUG_USB_EJTAG
#if 1
        // for(i=0 ; i<count ; i++)
        for(i=0 ; i<16 ; i++)
        {
                if((i%16 == 0) && i>0 )
                        printf("\n");
                printf("0x%2x ",buf[i]);
        }
        printf("\n========================================================================================\n");
#endif
        
	endpoint = 2 ; // Output Endpoint
        // a = usb_bulk_write(current_handle, endpoint, buf, count, 1000);
        a = usb_bulk_write(current_handle, endpoint, buf, count, 15000000);

#ifdef DEBUG_USB_EJTAG
        printf("usb_bulk_write end\n");
#endif
	printf("USB write size is : 0x%x , a is %d \n",a, a);
        if(a != count )
        {
                fprintf(stderr,"Request for bulk write failed: %s\n", usb_strerror());
                usb_release_interface(current_handle, 0);
                return;
        }
#if 0
	for(i=0;i<burst_count;i++)
	{
		if(i%16 == 0)
			printf("\n");
		printf("0x%4x ",dma32_write2_data[i]);
	}
#endif

        free(buf);
	return fast_write2_data;

}
#endif


//Test Command
unsigned short * signal_echo(unsigned short wb, unsigned short i_data , unsigned int echo_data)
{

	//short signal_echo_data[3] ={0};
	signal_echo_data[0] = OP_SIGNAL_ECHO << OP_EJTAG_SHIFT | wb << OP_EJTAG_W_SHIFT | i_data << OP_EJTAG_I_SHIFT ;
	signal_echo_data[1] = echo_data & 0xFFFF;
	signal_echo_data[2] = (echo_data & 0xFFFF0000) >> 16;

	return signal_echo_data;

}


//short * burst_echo(short wb, short i_data ,int burst_count, int * echo_data)
unsigned short * burst_echo(unsigned int burst_count,unsigned int * echo_data)
{

	unsigned int i=0;
	// short burst_echo_data[BUSRT_ECHO_MAX+3] ={0};

	burst_echo_data[0] = OP_BURST_ECHO << OP_EJTAG_SHIFT ;
	burst_echo_data[1] = burst_count & 0xFFFF;
	burst_echo_data[2] = (burst_count & 0xFFFF0000) >> 16;

	if(burst_count > BUSRT_ECHO_MAX)
	{
		printf("burst_count is overflow , MAX size is 0x%x \n",BUSRT_ECHO_MAX);
		burst_count = BUSRT_ECHO_MAX;
	//	return -1;
	}

	for(i=3;i<burst_count;i++)
		burst_echo_data[i] = ((i%256)<<8 | ((i%256)+0x30));
#if 0
	for(i=0;i<burst_count;i++)
	{
		if(i%16 == 0)
			printf("\n");
		printf("0x%4x ",burst_echo_data[i]);
	}
#endif
	return burst_echo_data;

}

#if 0
int main()
{
	printf("USB Ejtag test\n");
	return 0;
}
#endif

#if 1

//unsigned short * fast_read(unsigned int fast_count,unsigned int * fast_read_data_input)
unsigned short * fast_read(unsigned int fast_count )
{

	unsigned int i=0;
	unsigned int j=0;
	unsigned int count=0;
	int a=0;
	unsigned int download_size_fast = fast_count/4;
	unsigned char *buf ;
	unsigned char *buf_read ;
// [FAST_WR2_MAX];

	// short burst_echo_data[BUSRT_ECHO_MAX+3] ={0};
        if(fast_count%4)
		download_size_fast++;
	printf("fast_count : 0x%x , download_size_fast : 0x%x\n",fast_count,download_size_fast);

	fast_read_data[0] = OP_FAST_READ << OP_EJTAG_SHIFT ;
	fast_read_data[1] = config_usb_ejtag_put_speed;
	fast_read_data[2] = (download_size_fast& 0xFFFF); // fast_count & 0xFFFF;
	fast_read_data[3] =(download_size_fast & 0xFFFF0000) >> 16 ; // (fast_count & 0xFFFF0000) >> 16;

#if 0
	if(fast_count > FAST_WR2_MAX)
	{
		printf("fast_count is overflow , MAX size is 0x%x \n",FAST_WR2_MAX);
		fast_count = FAST_WR2_MAX;
	//	return -1;
	}
#endif
	buf=malloc(8);

	buf_read = malloc(FAST_WR2_MAX);

#if 0
	for(i=4;j<download_size_fast;j++)
	{
		fast_read_data[i++] = (fast_read_data_input[j]& 0xFFFF);
		fast_read_data[i++] = (fast_read_data_input[j]& 0xFFFF0000) >> 16;
	}
#endif
                // ((i%256)<<8 | ((i%256)+0x30));
		//fast_write2_data[i] = ((i%256)<<8 | ((i%256)+0x30));
	memset(buf,0,8);
	memset(buf_read,0,FAST_WR2_MAX);

//	count = fast_count + 8 ;

	count = 8 ;
        for(i=0;i<count;i=i+2)
        {
                buf[i] = fast_read_data[i/2] & 0xFF;
                buf[i+1] = (fast_read_data[i/2] & 0xFF00) >>8;
        }

// #ifdef DEBUG_USB_EJTAG
#if 1
        // for(i=0 ; i<count ; i++)
        for(i=0 ; i< 8 ; i++)
        {
                if((i%16 == 0) && i>0 )
                        printf("\n");
                printf("0x%2x ",buf[i]);
        }
        printf("\n========================================================================================\n");
#endif
        
	endpoint = 2 ; // Output Endpoint
        // a = usb_bulk_write(current_handle, endpoint, buf, count, 1000);
        a = usb_bulk_write(current_handle, endpoint, buf, count, 5000);

#ifdef DEBUG_USB_EJTAG
        printf("usb_bulk_write end\n");
#endif
	printf("USB write size is : 0x%x , a is %d \n",a, a);
        if(a != count )
        {
                fprintf(stderr,"Request for bulk write failed: %s\n", usb_strerror());
                usb_release_interface(current_handle, 0);
                return;
        }


#if 1
        endpoint = 6 ; // Input Endpoint
//        count=;
        a = usb_bulk_read(current_handle, endpoint, buf_read, fast_count, 1500000);

        if(a != fast_count)
        {
                fprintf(stderr,"Request for Burst bulk read failed: %s\n", usb_strerror());
        //      usb_release_interface(current_handle, 0);
        //      return;
        }
#endif

//zgj        usb_release_interface(current_handle, 0);

//#ifdef DEBUG_USB_EJTAG
#if 1
        for(i=0;i < fast_count ;i++)
        {
                if((i%16 == 0) && i>0 )
                        printf("\n");
                printf("0x%x ",buf_read[i]);
        }
        printf("\n");
#endif

#if 0
	for(i=0;i<burst_count;i++)
	{
		if(i%16 == 0)
			printf("\n");
		printf("0x%4x ",dma32_write2_data[i]);
	}
#endif

        free(buf);
        free(buf_read);
	return fast_read_data;

}
#endif

#if 0

unsigned int usb_fast_upload2(unsigned  int fast_count)
{
        int i,a,count = 6 ;
        unsigned char *buf;
        unsigned char *buf_read;

	unsigned int usb_wr_dr_result=0;
         unsigned int download_size_fast = fast_count/4;

        if(fast_count%4)
                download_size_fast++;
        printf("fast_count : 0x%x , download_size_fast : 0x%x\n",fast_count,download_size_fast);

        if(fast_count > FAST_WR2_MAX)
        {
                printf("fast_count is overflow , MAX size is 0x%x \n",FAST_WR2_MAX);
                fast_count = FAST_WR2_MAX;
        //      return -1;
        }

//	short usb_wr_dr_data[3] ={0};
	usb_fast_read_data[0] = OP_FAST_READ <<OP_EJTAG_SHIFT | 0x0 ;
	usb_fast_read_data[1] = fast_count & 0xFFFF;
	usb_fast_read_data[2] = (fast_count & 0xFFFF0000) >> 16;

        buf=malloc(FAST_WR2_MAX);	         
	buf_read=malloc(FAST_WR2_MAX);

        if(count > FAST_WR2_MAX)
        {
                fprintf(stderr, "Cannot transfer more than 6 bytes at a time.\n");
                exit(-1);
        }

        memset(buf, 0, FAST_WR2_MAX);
        memset(buf_read, 0, 600);

        for(i=0;i<count;i=i+2)
        {
                buf[i] = usb_wr_dr_data[i/2] & 0xFF;
                buf[i+1] = (usb_wr_dr_data[i/2] & 0xFF00) >>8;
        }

// #ifdef DEBUG_USB_EJTAG
#if 1
        for(i=0 ; i<count ; i++)
        {
                if((i%16 == 0) && i>0 )
                        printf("\n");
                printf("0x%2x ",buf[i]);
        }
        printf("\n========================================================================================\n");
#endif
#if 1
        endpoint = 2 ; // Output Endpoint
        a = usb_bulk_write(current_handle, endpoint, buf, count, 1000);

//#ifdef DEBUG_USB_EJTAG
#if 1
        printf("usb_bulk_write end\n");
#endif
        if(a<0)
        {
                fprintf(stderr,"Request for bulk write failed: %s\n", usb_strerror());
                usb_release_interface(current_handle, 0);
                return;
        }

        endpoint = 6 ; // Input Endpoint
	count=600;
        a = usb_bulk_read(current_handle, endpoint, buf_read, 60, 15000);

        if(a<0)
        {
                fprintf(stderr,"Request for Burst bulk read failed: %s\n", usb_strerror());
        //      usb_release_interface(current_handle, 0);
        //      return;
        }
#endif

//zgj        usb_release_interface(current_handle, 0);

//#ifdef DEBUG_USB_EJTAG
#if 1
        for(i=0;i<count;i++)
        {
                if((i%16 == 0) && i>0 )
                        printf("\n");
                printf("0x%x ",buf_read[i]);
        }
        printf("\n");
#endif

	usb_wr_dr_result = buf_read[3] << 24 |buf_read[2] << 16 |buf_read[1] << 8 |buf_read[0] ;

	return usb_wr_dr_result;

}
#endif
