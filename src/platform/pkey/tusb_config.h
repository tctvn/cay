#ifndef _TUSB_CONFIG_H_
#define _TUSB_CONFIG_H_

#ifdef __cplusplus
 extern "C" {
#endif

//--------------------------------------------------------------------
// COMMON CONFIGURATION
//--------------------------------------------------------------------
#define CFG_TUSB_RHPORT0_MODE       OPT_MODE_DEVICE

//--------------------------------------------------------------------
// DEVICE CONFIGURATION
//--------------------------------------------------------------------
#define CFG_TUD_ENDPOINT0_SIZE      64

//------------- CLASS -------------//
#define CFG_TUD_HID               2
#define CFG_TUD_CDC               0
#define CFG_TUD_MSC               0
#define CFG_TUD_MIDI              0
#define CFG_TUD_VENDOR            0

// HID buffer size Should be sufficient to hold ID (if any) + Data
#define CFG_TUD_HID_EP_BUFSIZE    64

//--------------------------------------------------------------------
// HOST CONFIGURATION
//--------------------------------------------------------------------
#define CFG_TUH_ENABLED           1
#define CFG_TUH_RPI_PIO_USB       1

// Size of buffer to hold up to 1 MaxPacketSize of control endpoint
#define CFG_TUH_ENDPOINT0_SIZE    64

#define CFG_TUH_ENUMERATION_BUFSIZE 256

#define CFG_TUH_HUB               0
#define CFG_TUH_HID               1
#define CFG_TUH_HID_EPIN_BUFSIZE  64
#define CFG_TUH_HID_EPOUT_BUFSIZE 64

#ifndef BOARD_TUH_RHPORT
#define BOARD_TUH_RHPORT          1
#endif

#ifndef CFG_TUSB_RHPORT1_MODE
#define CFG_TUSB_RHPORT1_MODE     OPT_MODE_HOST
#endif

#ifdef __cplusplus
 }
#endif

#endif /* _TUSB_CONFIG_H_ */
