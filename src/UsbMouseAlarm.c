/*
 ============================================================================
 Name        : UsbMouseAlarm.c
 Author      : Adnan Akbar
 Version     :
 Copyright   : MIT
 Description : Using a mouse as Alarm Device
 ============================================================================
 */
/**** Reverse Engineered Protocol
 *    Messages Are 6 bytes in Length
 *
 *    1) Mouse Button1 Press Message :
 *    		01 01 00 00 00 00  -- >  Disable Alarm
 *    2) Mouse  Button Release :
 *    		01 00 00 00 00 00  -- >  Enable Alarm
 *    3) Mouse Button2 Press Message :
 *    		01 02 00 00 00 00  -- > Stop Running Alarm
 *    4) Mouse Buton 1 & 2 Press Message :
 *    		01 03 00 00 00 00  -- > Reset Alarm Machine
 * 	  5) Mouse Wheel Button Press Message :
 * 			01 04 00 00 00 00  -- > Code Enter Button
 * 	  6) Mouse Wheel Scroll Up
 *			01 00 00 00 00 01  -- > Up Down
 *	  7) Mouse Wheel Scroll Down
 *	  		01 00 00 00 00 ff  -- > Down Code
 *
 *			Code is the 5  updowns
 *	  8) Movement of mouse
 *	  		01 00 xx xx xx 00
 *
 *	  		If either of the 3 bytes have no zero there is a movement
 *	  		Not concerned right now about the movement up down left
 *	  		right. Will add support for coming and leaving supprt.
 *
 * **/


#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <libusb-1.0/libusb.h>
// use cat /proc/bus/input/devices for the vendor and product id of device or lsusb -v for also interface packet size
#define VENDOR_ID 0x2188
#define PRODUCT_ID 0x0ae1
#define D_PACKET_SIZE 0x6

// exit signal condition
static int do_exit = 0;
static void sighandler(int signum) {
	do_exit = 1;
}

// inits neccessary objects for reading from usb device
libusb_device_handle * init_sub_system(libusb_context* ctx);

// read loop function
void readSyncDeviceLoop(libusb_device_handle *handle);
//static void status(int code);
// just prints the error code to friendly description
static void error(int code);

// perform async operations setup/cleanup
static const int initAsyncStuff(libusb_device_handle *handle);
static void cleanUpAsyncStuff(libusb_context *ctx);

int main(int argc, char* *argv) {
	printf("Using the mouse with Vendor=%x, Product=%x as Alarm Device\n",
			VENDOR_ID, PRODUCT_ID);
	libusb_context *ctx;
	int r; //for return values

	struct sigaction sigact; // for clean exiting
	sigact.sa_handler = sighandler;
	sigemptyset(&sigact.sa_mask);
	sigact.sa_flags = 0;

	r = libusb_init(&ctx); //initialize a library session
	if (r < 0) {
		printf("Init Error %d\n", r); //there was an error
		return 1;
	}
	//set verbosity level to 3, as suggested in the documentation
	libusb_set_debug(ctx, 3);
	libusb_device_handle *handle = init_sub_system(ctx);
	if (handle) {
		if (argc > 1 && initAsyncStuff(handle))
			do_exit = 1;

		//register signal handlers
		sigaction(SIGINT, &sigact, NULL);
		sigaction(SIGTERM, &sigact, NULL);
		sigaction(SIGQUIT, &sigact, NULL);

		printf("Going for A Loop\n");

		while (!do_exit) {
			if (argc < 2)
				readSyncDeviceLoop(handle);
			else {
				if ((r = libusb_handle_events(ctx)) < 0) {
					printf("Events Error returned %d \n", r);
					cleanUpAsyncStuff(ctx);
					break;
				}
			}
		}

		r = libusb_release_interface(handle, 0); //release the claimed interface
		if (r != 0) {
			printf("Unable to Release Interface\n");
		} else {
			printf("Released Interface\n");
		}

		libusb_close(handle);
		printf("Closed Device Handle\n");
	}

	libusb_exit(ctx); //close the session
	return 0;
}

// inits neccessary objects for reading from usb device
libusb_device_handle * init_sub_system(libusb_context* ctx) {

	libusb_device **devs; //pointer to pointer of device, used to retrieve a list of devices
	ssize_t cnt; //holding number of devices in list
	int r; //for return values

	cnt = libusb_get_device_list(ctx, &devs); //get the list of devices
	libusb_free_device_list(devs, 1); //free the list, unref the devices in it
	if (cnt < 0) {
		printf("Get Device Error %ld\n", cnt); //there was an error
		return NULL;
	}
	// open usb mouse
	libusb_device_handle *dev_handle; //a device handle
	dev_handle = libusb_open_device_with_vid_pid(ctx, VENDOR_ID, PRODUCT_ID);
	//open mouse
	if (dev_handle == NULL) {
		printf("Cannot open device\n");
	} else {
		printf("Device Opened\n");
		if (libusb_kernel_driver_active(dev_handle, 0) == 1) { //find out if kernel driver is attached
			printf("Kernel Driver Active \n");
			if (libusb_detach_kernel_driver(dev_handle, 0) == 0) //detach it
				printf("Kernel Driver Detached!\n");
		}

		r = libusb_claim_interface(dev_handle, 0); //claim interface 0 (the first) of device (mine had jsut 1)
		if (r < 0) {
			printf("Cannot Claim Interface\n");
			libusb_close(dev_handle);
			dev_handle = NULL;
		} else {
			printf("Interface claimed\n");
		}
	}

	return dev_handle;
}

///////////////////////////////

void readSyncDeviceLoop(libusb_device_handle *dev_handle) {
	int size;
	int j = 0;
	unsigned char datain[1024];

	int rr = libusb_bulk_transfer(dev_handle, (1 | LIBUSB_ENDPOINT_IN), datain,
			1024, &size, 1000); // millis timeout 0 means on input
	printf("libusb_bulk_transfer rr:%d \n", rr);
	if (rr) {
		error(rr);
		return;
	}
	printf("size: %d\n", size);
	printf("data: ");
	for (j = 0; j < size; j++)
		printf("%02x ", (unsigned char) (datain[j]));
	printf("\n");

}

static void error(int code) {
	switch (code) {
	case LIBUSB_ERROR_IO:
		fprintf(stdout, "Error: LIBUSB_ERROR_IO\n");
		break;
	case LIBUSB_ERROR_INVALID_PARAM:
		fprintf(stdout, "Error: LIBUSB_ERROR_INVALID_PARAM\n");
		break;
	case LIBUSB_ERROR_ACCESS:
		fprintf(stdout, "Error: LIBUSB_ERROR_ACCESS\n");
		break;
	case LIBUSB_ERROR_NO_DEVICE:
		fprintf(stdout, "Error: LIBUSB_ERROR_NO_DEVICE\n");
		break;
	case LIBUSB_ERROR_NOT_FOUND:
		fprintf(stdout, "Error: LIBUSB_ERROR_NOT_FOUND\n");
		break;
	case LIBUSB_ERROR_BUSY:
		fprintf(stdout, "Error: LIBUSB_ERROR_BUSY\n");
		break;
	case LIBUSB_ERROR_TIMEOUT:
		fprintf(stdout, "Error: LIBUSB_ERROR_TIMEOUT\n");
		break;
	case LIBUSB_ERROR_OVERFLOW:
		fprintf(stdout, "Error: LIBUSB_ERROR_OVERFLOW\n");
		break;
	case LIBUSB_ERROR_PIPE:
		fprintf(stdout, "Error: LIBUSB_ERROR_PIPE\n");
		break;
	case LIBUSB_ERROR_INTERRUPTED:
		fprintf(stdout,
				"Error:LIBUSB_ERROR_INTERRUPTED : System call interrupted (perhaps due to signal).\n");
		break;
	case LIBUSB_ERROR_NO_MEM:
		fprintf(stdout, "Error: LIBUSB_ERROR_NO_MEM\n");
		break;
	case LIBUSB_ERROR_NOT_SUPPORTED:
		fprintf(stdout,
				"Error: LIBUSB_ERROR_NOT_SUPPORTED\nOperation not supported or unimplemented on this platform.\n");
		break;
	case LIBUSB_ERROR_OTHER:
		fprintf(stdout, "Error: LIBUSB_ERROR_OTHER\n");
		break;
	default:
		fprintf(stdout, "Error: unkown error\n");
		break;
	}
}


static void LIBUSB_CALL cb_data(struct libusb_transfer *transfer);
static struct libusb_transfer *data_transfer = NULL;
static unsigned char databuf[D_PACKET_SIZE];


// perform async operations
static const int initAsyncStuff(libusb_device_handle *handle) {
	printf("initAsyncStuff\n");

	data_transfer = libusb_alloc_transfer(0);
	if (!data_transfer)
		return -ENOMEM;

	libusb_fill_bulk_transfer(data_transfer, handle, (1 | LIBUSB_ENDPOINT_IN),
			databuf, sizeof(databuf), cb_data, NULL, 0);

	if (libusb_submit_transfer(data_transfer) < 0)
		return 1;
	printf("ASynch Setup Completed\n");
	return 0;
}

static void cleanUpAsyncStuff(libusb_context *ctx){
	int r = 0;
	if (data_transfer) {
			printf("cancelling transfers\n");
			struct libusb_transfer *data_transferCpy = data_transfer;
			if(( r =  libusb_cancel_transfer(data_transfer)) < 0 ){
				printf("Cancelled transfers \n");
			}else{
				printf("Checking for last messages in loop\n");
				while (data_transfer && libusb_handle_events(ctx) > -1);
			}
			printf("Freeing transfer\n");
			libusb_free_transfer(data_transferCpy);
		}
}

// alarmHandling function
static void alarmHandleMessage(const unsigned char* data,const int siz);

static void LIBUSB_CALL cb_data(struct libusb_transfer *transfer) {
	int j = 0;


	if (transfer->status == LIBUSB_TRANSFER_COMPLETED) {


		alarmHandleMessage(transfer->buffer,transfer->length);
		// go for next input here
		if (libusb_submit_transfer(data_transfer) < 0){
			data_transfer = NULL;
			do_exit = 2;
		}
	}else{
		printf("Data callback status = %d\n",transfer->status);
		printf("Actual Length : %d\n",transfer->actual_length);
		printf("Length : %d\n",transfer->length);
		printf("ISO Packets : %d\n",transfer->num_iso_packets);
		printf("data: ");
		for (j = 0; j < transfer->length; j++)
			printf("%02x ", (unsigned char) (transfer->buffer[j]));
		printf("\n");
		printf(" Unknown or partial event i guess\n");
		//if (transfer->status == LIBUSB_TRANSFER_NO_DEVICE)
		data_transfer=NULL;
			do_exit = 3;
	}
}
//handler functions declerations
void disableAlarm();
void stopRunningAlarm();
void resetAlarmMachine();
void codeEnterStart();
void codeUpNext();
void codeDownNext();
void handleActivityDetected();
void enableAlarmFlag();

int codeVerified = 0; // flag to indicate if the code was verified

// see top of the file for logic regarding this function.
static void alarmHandleMessage(const unsigned char* data,const int siz){


	if(!data || siz < D_PACKET_SIZE) return;
	// reset flag after the next packet
	if( codeVerified == 2){ codeVerified =0; }
	if( codeVerified) ++codeVerified;

	if(data[1]){
		switch(data[1]){
		  case 1:
			  disableAlarm();
			  break;
		  case 2:
			  stopRunningAlarm();
			  break;
		  case 3:
			  resetAlarmMachine();
			  break;
		  case 4:
			  codeEnterStart();
			  break;
		  default:
			  break;

		}
		return;
	}

	if(data[5]){
		if( data[5] == 1){
			codeUpNext();
		}else if ( data[5] == 0xff){
			codeDownNext();
		}
		return;
	}

	if( data[2] || data[3] || data[4]){
		handleActivityDetected();
	}

	// default one
	enableAlarmFlag();
}

// flag to decide alarm should start
int alarmFlag = 0;
#define CODE_MAX 6
int cCodeAt = 0 ;
char code[CODE_MAX], password[]={0,0,0,1,1,1}; // code input buffer


void disableAlarm(){
	alarmFlag = 0;
}
void stopRunningAlarm(){
	if( codeVerified){
		printf("Stoping running alarm\n");
	}else{
		printf("Please enter the code first to verify\n");
	}
}
void resetAlarmMachine(){
	if( codeVerified){
		printf("Reset alarm machine\n");
	}else{
		printf("Please enter the code first to verify\n");
	}
}
void codeEnterStart(){
	printf("Please input %d unit up/down code now \n", CODE_MAX);
	cCodeAt = 0;
}
void verifyCode(){
	printf("Verifying code now \n");
	codeVerified =0;
	cCodeAt = 0;
	int i = 0;
	for(;i< CODE_MAX;++i ){
		if( code[i] != password[i]) return;
	}
	codeVerified = 1;
	printf("Code Verified now you can reset and stop Alarm\n");
}

void codeUpNext(){
	printf("Called Up\n");
	code[cCodeAt] = 0;++cCodeAt;
	if( cCodeAt == CODE_MAX){
		verifyCode();
	}
}
void codeDownNext(){
	printf("Called Down\n");
	code[cCodeAt] = 1;++cCodeAt;
	if( cCodeAt == CODE_MAX){
		verifyCode();
	}
}
void handleActivityDetected(){
	if(alarmFlag){
		printf("Intruder Detected ------ Wake! Wake! Wake! *********** \n");
	}
}
void enableAlarmFlag(){
	alarmFlag = 1;
}
