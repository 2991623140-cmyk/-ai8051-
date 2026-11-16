#ifndef __AI_USB_H
#define __AI_USB_H

typedef unsigned char BYTE;
typedef bit BOOL;

#define DEVSTATE_ATTACHED       0
#define DEVSTATE_POWERED        1
#define DEVSTATE_DEFAULT        2
#define DEVSTATE_ADDRESS        3
#define DEVSTATE_CONFIGURED     4
#define DEVSTATE_SUSPENDED      5

void usb_init(void);
void usb_OUT_done(void);
void USB_SendData(BYTE *dat, int size);

void set_usb_ispcmd(char *cmd);
void USB_SetIntPriority(BYTE prio);

int printf_usb(const char *fmt, ...);

extern BYTE xdata UsbFeatureBuffer[64];
extern BYTE xdata UsbInBuffer[64];
extern BYTE xdata UsbOutBuffer[64];
extern BOOL bUsbFeatureReady;
extern BOOL bUsbInBusy;
extern BOOL bUsbOutReady;
extern BYTE DeviceState;
extern BYTE OutNumber;

#endif
