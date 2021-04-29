/* 
	GS232 USB Ejtag Simulator Command List 
	Author : ZGJ 
	Date : 20090609
	Version :V1.0
	email: zhouguojian@ict.ac.cn
*/

#include <usb.h>

#define BUSRT_ECHO_MAX 	0x10000
#define DMA32_WR1_MAX 	0x400
#define DMA32_WR2_MAX 	0x100000 // 0x100000000
#define FAST_WR2_MAX 	0x1000000 // 0x100000000

#define OP_EJTAG_SHIFT 	0xA
#define OP_EJTAG_W_SHIFT 0x9          //Write Back
#define OP_EJTAG_I_SHIFT 0x8          //Immediately Write Back

//control command
#define OP_TAP_RST 	0x1
#define OP_TARGET_RST	0x2
#define OP_FPGA_INFO	0x3


// Simple read-write TAP IR or DR register command
/*
15          10  9   8   7      5   4    0
(OP_TYPE =) 4 | W | I | RESERVED | NEW_IR
*/
#define OP_WR_IR	0x4

/*
15          10  9   8   7      5   4    0
(OP_TYPE =) 5 | W | I | RESERVED 
		bits [15: 0]
		bits [31:16]
*/
#define OP_WR_DR	0x5


// ACCelerate Download Rate

#define OP_DMA32_WRITE1	0x6
#define OP_DMA32_WRITE2	0x7

#define OP_FAST_WRITE1	0xB
#define OP_FAST_WRITE2	0xC
#define OP_WR_UPLOAD	0xF
#define OP_FAST_READ	0x10

// Test command

#define OP_SIGNAL_ECHO	0x8
#define OP_BURST_ECHO	0x9

//Extern command
//OP_


#define USB_FOUND 1
#define USB_NO_FOUND 0
#define USB_NOT_BULK_WR_FLAG 0
#define USB_CAN_BULK_WR_FLAG 1
#define LS232_USB_EJTAG_VENDOR_ID  0x2961 // 0x0547
#define LS232_USB_EJTAG_PRODUCT_ID 0x6688  // 0x1002

int usb_can_bulk_wr_flag = USB_NOT_BULK_WR_FLAG;
unsigned short usb_wr_dr_data[4] ={0};
unsigned short dma32_write1_data[DMA32_WR1_MAX+1]={0};
unsigned short dma32_write2_data[DMA32_WR2_MAX+3]={0};
unsigned short fast_write2_data[FAST_WR2_MAX+3]={0};
unsigned short fast_read_data[FAST_WR2_MAX+3]={0};
unsigned short signal_echo_data[3] ={0};
unsigned short burst_echo_data[BUSRT_ECHO_MAX+3] ={0};

struct usb_device *current_device;
usb_dev_handle *current_handle;
char *bus_name="001";
char *device_name="001";
int endpoint = 0; 

