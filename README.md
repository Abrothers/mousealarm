mousealarm
==========

Mouse alarm with libusb1.0. Modify the top of the main.c file. 
use cat /proc/bus/input/devices for the vendor and product id of device or 
lsusb -v for also interface packet size that is D_PACKET_SIZE for my case it was 6.
Recompile the project either or console or eclipse what ever is preffered. 


 *VENDOR_ID 0x2188
 *PRODUCT_ID 0x0ae1
 *D_PACKET_SIZE 0x6


/*
 * ============================================================================
 * Name        : UsbMouseAlarm.c
 * Author      : Adnan Akbar
 * Version     :
 * Copyright   : MIT
 * Description : Using a mouse as Alarm Device
 * ============================================================================
 */
* /**** Reverse Engineered Protocol
 *    Messages Are 6 bytes in Length
 *
 *    1) Mouse Button1 Press Message :
     		01 01 00 00 00 00  -- >  Disable Alarm
 *    2) Mouse  Button Release :
     		01 00 00 00 00 00  -- >  Enable Alarm
 *    3) Mouse Button2 Press Message :
     		01 02 00 00 00 00  -- > Stop Running Alarm
 *    4) Mouse Buton 1 & 2 Press Message :
     		01 03 00 00 00 00  -- > Reset Alarm Machine
 * 	  5) Mouse Wheel Button Press Message :
  			01 04 00 00 00 00  -- > Code Enter Button
 * 	  6) Mouse Wheel Scroll Up
 			  01 00 00 00 00 01  -- > Up Down
 *	  7) Mouse Wheel Scroll Down
 	  		01 00 00 00 00 ff  -- > Down Code
 
 			Code is the 5  updowns
 *	  8) Movement of mouse
 	  		01 00 xx xx xx 00
 
 	  		If either of the 3 bytes have no zero there is a movement
 	  		Not concerned right now about the movement up down left
 	  		right. Will add support for coming and leaving supprt.
 
 * **/

