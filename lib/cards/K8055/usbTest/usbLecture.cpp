// #include <usb.h>
#include "/usr/include/usb.h"
#include <stdio.h>


#define VENDOR_ID 0x066f
#define PRODUCT_ID 0x8000

struct usb_bus *bus;
struct usb_device *dev, *my_dev = NULL;

int main (int argc,char **argv)
{
	int receive = 0;
	int len = 1024;
	char buffer[len];
	usb_dev_handle *device_handle = 0;
 
	usb_init();         // Initiaser libusb
	usb_find_busses();  //Trouver tous les bus 
	usb_find_devices(); //Trouver tous les peripheriques sur tous les bus  find_busses
 
	//sses = usb_get_busses();int usb_bulk_write(usb_dev_handle *dev, int ep, char *bytes, int size, int timeout);
	for (bus = usb_busses; bus; bus = bus->next){
 
 
	for (dev = bus->devices; dev; dev = dev->next)	
	{
	{
	printf("bus : %s Device %s \n id Vendor: %x | id Product : %x \n", bus->dirname,dev->filename ,dev->descriptor.idVendor,dev->descriptor.idProduct);
	}
 
	if ((dev->descriptor.idVendor == VENDOR_ID) && (dev->descriptor.idProduct == PRODUCT_ID))
	dev = dev;
 
 
	}
	}
 
		if (dev == NULL)
		{
			printf("aucun périphérique ne cerrespond");	
		return(0);
		}
			device_handle = usb_open(dev);
			  if (device_handle == NULL) 
			{
			    printf("usb_open : %s\n", usb_strerror());
			    return 1;
                	     }
 
 
return(0); //EXIT_SUCCESS; 
}