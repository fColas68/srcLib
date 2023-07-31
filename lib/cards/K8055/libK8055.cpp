/*
 * Click nbfs://nbhost/SystemFileSystem/Templates/Licenses/license-default.txt to change this license
 * Click nbfs://nbhost/SystemFileSystem/Templates/cpplite/CPPTemplate.cpp to edit this template
 */
#include <usb.h>
#include <iostream>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <math.h>

#define STR_BUFF 256
#define PACKET_LEN 8
#define USB_TIMEOUT 20
#define K8055_ERROR -1
#define K8055_OK 1

#define K8055_IPID 0x5500
#define VELLEMAN_VENDOR_ID 0x10cf
#define K8055_MAX_DEV 4

#define USB_OUT_EP 0x01	/* USB output endpoint */
#define USB_INP_EP 0x81 /* USB Input endpoint */

#define DIGITAL_INP_OFFSET 0
#define DIGITAL_OUT_OFFSET 1
#define ANALOG_1_OFFSET 2
#define ANALOG_2_OFFSET 3
#define COUNTER_1_OFFSET 4
#define COUNTER_2_OFFSET 6

#define CMD_RESET 0x00
#define CMD_SET_DEBOUNCE_1 0x01
#define CMD_SET_DEBOUNCE_2 0x01
#define CMD_RESET_COUNTER_1 0x03
#define CMD_RESET_COUNTER_2 0x04
#define CMD_SET_ANALOG_DIGITAL 0x05

/*---------------------------------------*/
/* variables pour la bibliothèque USB    */
/*---------------------------------------*/
static struct usb_bus *bus, *busses;
static struct usb_device *dev;

/*---------------------------------------*/
/* globales pour echange de données      */
/*---------------------------------------*/
struct k8055_dev {
    unsigned char data_in[PACKET_LEN+1];
    unsigned char data_out[PACKET_LEN+1];
    struct usb_dev_handle *device_handle;
    int DevNo;
};
static struct k8055_dev k8055d[K8055_MAX_DEV];
static struct k8055_dev *CurrDev;

/* Keep these globals for now */
unsigned char *data_in, *data_out;

/* set debug to 0 to not print excess info */
int DEBUG = 1;

/* ============================================================================================= */
/* ############################################################################################# */
/* #                                                                                           # */
/* #                             GESTION DES DRIVERS USB                                       # */
/* #                                                                                           # */
/* ############################################################################################# */
/* ============================================================================================= */

/* ====================================================================== */
/*  init_usb                                                              */
/* ---------------------------------------------------------------------- */
/*  Initialise la bibliothèque USB -  une fois                            */
/* ====================================================================== */
static void init_usb(void)
{
    static int Done = 0;            // Seulement si pas déjà fait
    if (!Done)
    {
        usb_init();                 // usb.h : Initialisation de la librairie (par example determine le chemin du repertoire des bus et peripheriques)
        usb_find_busses();          // usb.h :  Enumère tous les bus USB du systemes
        usb_find_devices();         // usb.h :  Enumère tous les peripheriques sur tous les Bus présents)
        busses = usb_get_busses();
        Done = 1;
    }
}

/* ==================================================================================== */
/*  takeover_device                                                                     */
/* ------------------------------------------------------------------------------------ */
/* If device is owned by some kernel driver, try to disconnect it and claim the device  */
/* ==================================================================================== */
static int takeover_device(usb_dev_handle * udev, int interface)
{
    char driver_name[STR_BUFF];

    memset(driver_name, 0, STR_BUFF);
    int ret = K8055_ERROR;

    assert(udev != NULL);
    ret = usb_get_driver_np(udev, interface, driver_name, sizeof(driver_name));
    if (ret == 0)
    {
        if (DEBUG)
            std::cout<<"Nom du driver : "<<driver_name<<std::endl;

        if (0 > usb_detach_kernel_driver_np(udev, interface))
        {
            if (DEBUG) {std::cout<<"Déconnecter le pilote : "<<usb_strerror()<<std::endl;}

        }
        else if (DEBUG) {std::cout<<"Pilote déconnecté : "<<usb_strerror()<<std::endl;}


    }
    else if (DEBUG) {std::cout<<"Récupération du driver : "<<usb_strerror()<<std::endl;}

        /* claim interface */
    if (usb_claim_interface(udev, interface) < 0)
    {
        if (DEBUG) {std::cout<<"Erreur de l'interface de réclamation : "<<usb_strerror()<<std::endl;}
        return K8055_ERROR;
    }
    else
        usb_set_altinterface(udev, interface);
    usb_set_configuration(udev, 1);

    if (DEBUG)
        {
        std::cout<<"Interface trouvée : "<<interface<<std::endl;
        std::cout<<"Prise en charge de l'appareil"<<std::endl;
        }
    return 0;
}

/* =================================================================================== */
/*  ReadK8055Data                                                                      */
/* ----------------------------------------------------------------------------------- */
/* Actual read of data from the device endpoint, retry 3 times if not responding ok    */
/* =================================================================================== */
static int ReadK8055Data(void)
{
    int read_status = 0, i = 0;

    if (CurrDev->DevNo == 0) return K8055_ERROR;

    for(i=0; i < 3; i++)
        {
        read_status = usb_interrupt_read(CurrDev->device_handle, USB_INP_EP, (char *)CurrDev->data_in, PACKET_LEN, USB_TIMEOUT);
        if ((read_status == PACKET_LEN) && (CurrDev->data_in[1] == CurrDev->DevNo )) return 0;
        if (DEBUG) {std::cout<<"Re tentative de lecture"<<std::endl;}
        }
    return K8055_ERROR;
}

/* ==================================================================================== */
/*  WriteK8055Data                                                                      */
/* ------------------------------------------------------------------------------------ */
/* Actual write of data to the device endpont, retry 3 times if not reponding correctly */
/* ==================================================================================== */
static int WriteK8055Data(unsigned char cmd)
{
    int write_status = 0, i = 0;
    if (CurrDev->DevNo == 0) return K8055_ERROR;
    CurrDev->data_out[0] = cmd;
    for(i=0; i < 3; i++)
        {
        write_status = usb_interrupt_write(CurrDev->device_handle, USB_OUT_EP, (char *)CurrDev->data_out, PACKET_LEN, USB_TIMEOUT);
        if (write_status == PACKET_LEN) return 0;
        if (DEBUG) {std::cout<<"Re tentative d'écriture"<<std::endl;}
        }
    return K8055_ERROR;
}

/* ========================================================================= */
/*  SetCurrentDevice                                                         */
/* --------------------------------------------------------------------------*/
/* New function in version 2 of Velleman DLL, should return deviceno if OK   */
/* ========================================================================= */
long SetCurrentDevice(long deviceno)
{
    if (deviceno >= 0 && deviceno < K8055_MAX_DEV)
    {
        if (k8055d[deviceno].DevNo != 0)
        {
            CurrDev  = &k8055d[deviceno];
            data_in  = CurrDev->data_in;
            data_out = CurrDev->data_out;
            return deviceno;
        }
    }
    return K8055_ERROR;
}

/* ============================================================================================= */
/* ############################################################################################# */
/* #                                                                                           # */
/* #                             GESTION DE LA CARTE                                           # */
/* #                                                                                           # */
/* ############################################################################################# */
/* ============================================================================================= */

/* ====================================================================== */
/*  openDevice                                                            */
/* ====================================================================== */
int openDevice(long BoardAddress)
{
    bool lTrouvee = false;

    /* start looping through the devices to find the correct one */
    for (bus = busses; bus; bus = bus->next)
    {
        for (dev = bus->devices; dev; dev = dev->next)
        {

            if (DEBUG)
            {
                //=========================================================================
                //fprintf(stderr,"Port USB Adresse : %s    fabricant 0x0%x   ID : 0x0%x\n",
                //                dev->filename, dev->descriptor.idVendor,
                //                dev->descriptor.idProduct);
                //=========================================================================
                std::cout<< "===================" << std::endl;
                std::cout<< dev->filename << std::endl;
                std::cout<< dev->descriptor.idVendor << std::endl;
                std::cout<< dev->descriptor.idProduct << std::endl;
            }   // de if (DEBUG)

            if ((dev->descriptor.idVendor == VELLEMAN_VENDOR_ID) &&
                (dev->descriptor.idProduct == K8055_IPID ))
            {
                lTrouvee = true;
                if (DEBUG) {std::cout<< "Carte k8055 trouvée !!!" << "à l'adresse "<< dev->filename << std::endl;}

                CurrDev = &k8055d[BoardAddress];
                CurrDev->device_handle = usb_open(dev);

                if (takeover_device(CurrDev->device_handle, 0) < 0)
                {
                    if (DEBUG) {std::cout<<"Impossible de prendre le contrôle de la carte K8055 depuis le driver système !!!"<<std::endl;}
                    usb_close(CurrDev->device_handle);      /* close usb if we fail */
                    return K8055_ERROR;                     /* throw K8055_ERROR to show that OpenDevice failed */
                } // de if (takeover_device(C
                else
                {
                    CurrDev->DevNo = BoardAddress + 1;      /* Mark as open and valid */
                    SetCurrentDevice(BoardAddress);
                    memset(CurrDev->data_out,0,PACKET_LEN);	/* Write cmd 0, read data */
                    WriteK8055Data(CMD_RESET);
                    if (ReadK8055Data() == 0)
                       return BoardAddress;		            /* This function should return board address */
                    else
                       return K8055_ERROR;
                } // de else if (takeover_device(C

            }   // de if ((dev->descriptor.idVendor == VELLEMAN_VENDOR_ID) &&
        }   // de for (dev = bus->devices; dev; dev = dev->next)
    }   // de for (bus = busses; bus; bus = bus->next)

    //##########################################################
    // La carte n'a pas été trouvée
    //##########################################################
    if (!lTrouvee)
    {
        if (DEBUG) {std::cout<< "Carte k8055 non trouvée." << std::endl;}
        return K8055_ERROR;
    }
    return 0;
}

/* ====================================================================== */
/* CloseDevice                                                            */
/* Close the Current device                                               */
/* ====================================================================== */
int CloseDevice()
{
     int rc;

     if (CurrDev->DevNo == 0)
     {
         if (DEBUG)
             fprintf(stderr, "Current device is not open\n" );
         return 0;
     }
     rc = usb_close(CurrDev->device_handle);
     if (rc >= 0)
     {
         CurrDev->DevNo = 0;  /* Not active nay more */
         CurrDev->device_handle = NULL;
     }
     return rc;
}
/* ============================================================================================= */
/* ############################################################################################# */
/* #                                                                                           # */
/* #                             ECHANGES AVEC LA CARTE                                        # */
/* #                                                                                           # */
/* ############################################################################################# */
/* ============================================================================================= */
long ReadAnalogChannel(long Channel)
{
    if (Channel == 1 || Channel == 2)
    {
        if ( ReadK8055Data() == 0)
        {
            if (Channel == 2)
                return CurrDev->data_in[ANALOG_2_OFFSET];
            else
                return CurrDev->data_in[ANALOG_1_OFFSET];
        }
        else
            return K8055_ERROR;
    }
    else
        return K8055_ERROR;
}

/* ============================================================================================= */
/* ############################################################################################# */
/* #                                                                                           # */
/* #                                     MAIN pour tests                                       # */
/* #                                                                                           # */
/* ############################################################################################# */
/* ============================================================================================= */
int main(void)
{
    int ok;

    init_usb();
    ok = openDevice(0);

    if (ok != K8055_ERROR)
        {
        int i = 0;
        int nMax = 2;
        long lec=0;
        do
            {
            i++;
            lec = ReadAnalogChannel(1);
            std::cout<< "lecture : "<< i << " valeur : " << lec <<std::endl;
            } while (i < nMax);
        CloseDevice();
    }
    return ok;
}
