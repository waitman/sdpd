/*
 * hid.c
 * Waitman Gobble <ns@waitman.net> (Based on code from -->
 * Copyright (c) 2004 Maksim Yevmenkin <m_evmenkin@yahoo.com>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
//  * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 */

#include <sys/queue.h>
#include <bluetooth.h>
#include <sdp.h>
#include <string.h>
#include <syslog.h>
#include "profile.h"
#include "provider.h"


#define	SDP_SERVICE_CLASS_PNP_DEVICE	0x1200
#define SDP_ATTR_PNP_SPECIFICATION_ID	0x0200
#define SDP_ATTR_PNP_VENDOR_ID		0x0201
#define	SDP_ATTR_PNP_PRODUCT_ID		0x0202
#define	SDP_ATTR_PNP_VERSION		0x0203
#define SDP_ATTR_PNP_PRIMARY_RECORD	0x0204
#define	SDP_ATTR_PNP_VENDOR_ID_SOURCE	0x0205

#define SDP_DATA_BOOL8			0x28
#define SDP_DATA_FALSE			0x0
#define SDP_DATA_TRUE			0x1
#define SDP_NO_LOCAL 			0x0000
#define SDP_IMA_KEYBOARD		0x0001
#define USB_HID_VERSION			0x0111
#define SDP_ATTR_USB_HID_VERSION	0x0201
#define SDP_ATTR_HID_DEVICE_SUBCLASS	0x0202
#define SDP_ATTR_PROFILE_COUNTRY	0x0203
#define SDP_ATTR_HID_VIRTUAL_CABLE	0x0204
#define SDP_ATTR_AUTO_RECONNECT		0x0205
#define SDP_ATTR_HID_DESCRIPTOR		0x0206
#define SDP_ATTR_LANG_ID		0x0207
#define SDP_ATTR_BOOT_DEVICE		0x020e

struct sdp_hid_profile
{
        uint8_t server_channel;
        uint8_t supported_formats_size;
        uint8_t supported_formats[30];
};
typedef struct sdp_hid_profile        sdp_hid_profile_t;
typedef struct sdp_hid_profile *      sdp_hid_profile_p;

struct sdp_pnp_profile
{
        uint8_t server_channel;
        uint8_t supported_formats_size;
        uint8_t supported_formats[30];
};
typedef struct sdp_pnp_profile        sdp_pnp_profile_t;
typedef struct sdp_pnp_profile *      sdp_pnp_profile_p;


static int32_t
hid_profile_create_service_class_id_list(
		uint8_t *buf, uint8_t const * const eob,
		uint8_t const *data, uint32_t datalen)
{
	static uint16_t	service_classes[] = {
		SDP_SERVICE_CLASS_HUMAN_INTERFACE_DEVICE
	};

	return (common_profile_create_service_class_id_list(
			buf, eob,
			(uint8_t const *) service_classes,
			sizeof(service_classes)));
}

static int32_t
pnp_profile_create_service_class_id_list(
		uint8_t *buf, uint8_t const * const eob,
		uint8_t const *data, uint32_t datalen)
{
	static uint16_t	service_classes[] = {
		SDP_SERVICE_CLASS_PNP_DEVICE
	};

	return (common_profile_create_service_class_id_list(
			buf, eob,
			(uint8_t const *) service_classes,
			sizeof(service_classes)));
}

static int32_t
hid_profile_create_bluetooth_profile_descriptor_list(
		uint8_t *buf, uint8_t const * const eob,
		uint8_t const *data, uint32_t datalen)
{
	static uint16_t	profile_descriptor_list[] = {
		SDP_SERVICE_CLASS_HUMAN_INTERFACE_DEVICE,
		0x0100
	};

	return (common_profile_create_bluetooth_profile_descriptor_list(
			buf, eob,
			(uint8_t const *) profile_descriptor_list,
			sizeof(profile_descriptor_list)));
}

static int32_t
pnp_profile_create_bluetooth_profile_descriptor_list(
		uint8_t *buf, uint8_t const * const eob,
		uint8_t const *data, uint32_t datalen)
{
	static uint16_t	profile_descriptor_list[] = {
		SDP_SERVICE_CLASS_PNP_DEVICE,
		0x0100
	};

	return (common_profile_create_bluetooth_profile_descriptor_list(
			buf, eob,
			(uint8_t const *) profile_descriptor_list,
			sizeof(profile_descriptor_list)));
}
      
static int32_t
hid_profile_create_service_name(
		uint8_t *buf, uint8_t const * const eob,
		uint8_t const *data, uint32_t datalen)
{
	static char	service_name[] = "HID Device";

	return (common_profile_create_string8(
			buf, eob,
			(uint8_t const *) service_name, strlen(service_name)));
}

/*
 * seq8 len8			- 2 bytes
 *	seq8 len8		- 2 bytes
 *		uuid16 value16	- 3 bytes
 *		uint16 value16	- 3 bytes
 *	seq8 len8		- 2 bytes
 *		uuid16 value16	- 3 bytes
 */


static int32_t
hid_profile_create_protocol_descriptor_list(
		uint8_t *buf, uint8_t const * const eob,
		uint8_t const *data, uint32_t datalen)
{
	provider_p		provider = (provider_p) data;
	sdp_hid_profile_p	hid = (sdp_hid_profile_p) provider->data;
	

      /*
       * Create a protocol descriptor list. 
       * HID profile uses L2CAP with a control channel and interrupt channel. 
       * Control channel is set by calling function. 
       * Here we assume that interrupt channel is control channel + 2
       * TODO: maybe set interrupt channel in calling function
       */
	
	SDP_PUT8(SDP_DATA_SEQ8, buf); 		//2
	SDP_PUT8(13, buf);
	SDP_PUT8(SDP_DATA_SEQ8, buf); 		//2
	SDP_PUT8(6, buf);
	SDP_PUT8(SDP_DATA_UUID16, buf); 	//3
	SDP_PUT16(SDP_UUID_PROTOCOL_L2CAP, buf); 
	SDP_PUT8(SDP_DATA_UINT16, buf);		//3
	SDP_PUT16(*(uint16_t const *)&hid->server_channel, buf); 	/* PSM */
	SDP_PUT8(SDP_DATA_SEQ8, buf); 		//2
	SDP_PUT8(3, buf);
	SDP_PUT8(SDP_DATA_UUID16, buf); 	//3
	SDP_PUT16(SDP_UUID_PROTOCOL_HIDP, buf); 

	return (15);
}

static int32_t
pnp_profile_create_protocol_descriptor_list(
		uint8_t *buf, uint8_t const * const eob,
		uint8_t const *data, uint32_t datalen)
{
	

      /*
       * Create a protocol descriptor list. 
       * HID profile uses L2CAP with a control channel and interrupt channel. 
       * Control channel is set by calling function. 
       * Here we assume that interrupt channel is control channel + 2
       * TODO: maybe set interrupt channel in calling function
       */
	
	SDP_PUT8(SDP_DATA_SEQ8, buf); 		//2
	SDP_PUT8(13, buf);
	SDP_PUT8(SDP_DATA_SEQ8, buf); 		//2
	SDP_PUT8(6, buf);
	SDP_PUT8(SDP_DATA_UUID16, buf); 	//3
	SDP_PUT16(SDP_UUID_PROTOCOL_L2CAP, buf); 
	SDP_PUT8(SDP_DATA_UINT16, buf);		//3
	SDP_PUT16(1, buf); 	/* PSM */
	SDP_PUT8(SDP_DATA_SEQ8, buf); 		//2
	SDP_PUT8(3, buf);
	SDP_PUT8(SDP_DATA_UUID16, buf); 	//3
	SDP_PUT16(SDP_UUID_PROTOCOL_SDP, buf); 

	return (15);
}


/*
 * seq8 len8				- 2 bytes
 * 	seq8 len8			- 2 bytes
 *		seq8 len8		- 2 bytes
 *			uuid16 value16	- 3 bytes
 *			uint16 value16	- 3 bytes
 *		seq8 len8		- 2 bytes
 *			uuid16 value16	- 3 bytes
 */


static int32_t
hid_profile_create_additional_protocol_descriptor_list(
		uint8_t *buf, uint8_t const * const eob,
		uint8_t const *data, uint32_t datalen)
{
	provider_p		provider = (provider_p) data;
	sdp_hid_profile_p	hid = (sdp_hid_profile_p) provider->data;
	uint16_t intrchan = *(uint16_t const *)&hid->server_channel+2;


      /*
      * the L2CAP interrupt channel goes in the the 
      * 'additional profile descriptor list'
      */
	
	SDP_PUT8(SDP_DATA_SEQ8, buf); 		//2
	SDP_PUT8(15, buf);
	SDP_PUT8(SDP_DATA_SEQ8, buf); 		//2
	SDP_PUT8(11, buf);
	SDP_PUT8(SDP_DATA_SEQ8, buf); 		//2
	SDP_PUT8(6, buf);
	SDP_PUT8(SDP_DATA_UUID16, buf); 	//3
	SDP_PUT16(SDP_UUID_PROTOCOL_L2CAP, buf); 
	SDP_PUT8(SDP_DATA_UINT16, buf);		//3
	SDP_PUT16(intrchan, buf); 	/* PSM */
	SDP_PUT8(SDP_DATA_SEQ8, buf); 		//2
	SDP_PUT8(3, buf);
	SDP_PUT8(SDP_DATA_UUID16, buf); 	//3
	SDP_PUT16(SDP_UUID_PROTOCOL_HIDP, buf); 


	return (17);
}


static int32_t
hid_profile_create_service_id(
		uint8_t *buf, uint8_t const * const eob,
		uint8_t const *data, uint32_t datalen)
{
	if (buf + 3 > eob)
		return (-1);

	/*
	 * The ServiceID is a UUID that universally and uniquely identifies 
	 * the service instance described by the service record. This service
	 * attribute is particularly useful if the same service is described
	 * by service records in more than one SDP server
	 */

	SDP_PUT8(SDP_DATA_UUID16, buf);
	SDP_PUT16(SDP_UUID_PROTOCOL_SDP, buf); /* XXX ??? */

	return (3);
}



static int32_t
hid_profile_create_browse_group_list(
		uint8_t *buf, uint8_t const * const eob,
		uint8_t const *data, uint32_t datalen)
{
	if (buf + 5 > eob)
		return (-1);

	SDP_PUT8(SDP_DATA_SEQ8, buf);
	SDP_PUT8(3, buf);

	/*
	 * The top-level browse group ID, called PublicBrowseRoot and
	 * representing the root of the browsing hierarchy, has the value
	 * 00001002-0000-1000-8000-00805F9B34FB (UUID16: 0x1002) from the
	 * Bluetooth Assigned Numbers document
	 */

	SDP_PUT8(SDP_DATA_UUID16, buf);
	SDP_PUT16(SDP_SERVICE_CLASS_PUBLIC_BROWSE_GROUP, buf);

	return (5);
}

static int32_t
hid_profile_create_version_number_list(
		uint8_t *buf, uint8_t const * const eob,
		uint8_t const *data, uint32_t datalen)
{
	if (buf + 5 > eob)
		return (-1);

	SDP_PUT8(SDP_DATA_SEQ8, buf);
	SDP_PUT8(3, buf);

	/* 
	 * The VersionNumberList is a data element sequence in which each 
	 * element of the sequence is a version number supported by the SDP
	 * server. A version number is a 16-bit unsigned integer consisting
	 * of two fields. The higher-order 8 bits contain the major version
	 * number field and the low-order 8 bits contain the minor version
	 * number field. The initial version of SDP has a major version of
	 * 1 and a minor version of 0
	 */

	SDP_PUT8(SDP_DATA_UINT16, buf);
	SDP_PUT16(0x0100, buf);

	return (5);
}


static int32_t
hid_profile_create_service_database_state(
		uint8_t *buf, uint8_t const * const eob,
		uint8_t const *data, uint32_t datalen)
{
	uint32_t	change_state = provider_get_change_state();

	if (buf + 5 > eob)
		return (-1);

	SDP_PUT8(SDP_DATA_UINT32, buf);
	SDP_PUT32(change_state, buf);

	return (5);
}

int32_t
hid_profile_data_valid(uint8_t const *data, uint32_t datalen)
{
	//sdp_hid_profile_p	hid = (sdp_hid_profile_p) data;
	/* validate data here*/
	/* TODO: validate the data, for now just return 'OK' */
	return (1);
}



static int32_t
hid_profile_usb_hid_version(
		uint8_t *buf, uint8_t const * const eob,
		uint8_t const *data, uint32_t datalen)
{
  
    /* the USB HID version supported, 1.11 */
    
    SDP_PUT8(SDP_DATA_UINT16, buf);	//3
    SDP_PUT16(USB_HID_VERSION, buf);
    return (3);
}

static int32_t
hid_profile_keyboard(
		uint8_t *buf, uint8_t const * const eob,
		uint8_t const *data, uint32_t datalen)
{
    
    /* Announce that I'm a keyboard */
    
    SDP_PUT8(SDP_DATA_UINT8, buf);	//2
    SDP_PUT8(SDP_IMA_KEYBOARD, buf);
    return (2);
}

static int32_t
hid_profile_country(
		uint8_t *buf, uint8_t const * const eob,
		uint8_t const *data, uint32_t datalen)
{
  
    /* Set country / localization. Here it is not localized */
    
    SDP_PUT8(SDP_DATA_UINT8, buf);	//2
    SDP_PUT8(SDP_NO_LOCAL, buf);
    return (2);
}

static int32_t
hid_profile_virtual_cable(
		uint8_t *buf, uint8_t const * const eob,
		uint8_t const *data, uint32_t datalen)
{
  
    /* Virtual cable is set to false. Read the BT HID spec */
    
    SDP_PUT8(SDP_DATA_BOOL8, buf);	//2
    SDP_PUT8(SDP_DATA_TRUE, buf);
    return (2);
}


static int32_t
hid_profile_reconnect_auto(
		uint8_t *buf, uint8_t const * const eob,
		uint8_t const *data, uint32_t datalen)
{

    /* Auto Reconnect set to false. Read the BT HID spec. */
    
    SDP_PUT8(SDP_DATA_BOOL8, buf);	//2
    SDP_PUT8(SDP_DATA_TRUE, buf);
    return (2);
}

static int32_t
hid_profile_boot_device(
		uint8_t *buf, uint8_t const * const eob,
		uint8_t const *data, uint32_t datalen)
{
  
    /* Boot Device set to false, Read the BT HID spec. */
    
    SDP_PUT8(SDP_DATA_BOOL8, buf);	//2
    SDP_PUT8(SDP_DATA_FALSE, buf);
    return (2);
}

  /* For now, this is the descriptor code advertised 
   *  by an Apple keyboard, provided by a scan 
   *  from Iain Hibbert (see ml post)
   *  TODO: review and update. This descriptor 
   *  references LEDs, etc. things that may not
   *  actually exist in this system.
   */
  
static const uint8_t base_hid_descriptor[] =  {
    0x05, 0x01, 0x09, 0x06, 0xa1, 0x01, 0x85, 0x01,
    0x05, 0x07, 0x19, 0xe0, 0x29, 0xe7, 0x15, 0x00,
    0x25, 0x01, 0x75, 0x01, 0x95, 0x08, 0x81, 0x02,
    0x75, 0x08, 0x95, 0x01, 0x81, 0x01, 0x75, 0x01,
    0x95, 0x05, 0x05, 0x08, 0x19, 0x01, 0x29, 0x05,
    0x91, 0x02, 0x75, 0x03, 0x95, 0x01, 0x91, 0x01,
    0x75, 0x08, 0x95, 0x06, 0x15, 0x00, 0x26, 0xff,
    0x00, 0x05, 0x07, 0x19, 0x00, 0x2a, 0xff, 0x00,
    0x81, 0x00, 0x75, 0x01, 0x95, 0x01, 0x15, 0x00,
    0x25, 0x01, 0x05, 0x0c, 0x09, 0xb8, 0x81, 0x06,
    0x09, 0xe2, 0x81, 0x06, 0x09, 0xe9, 0x81, 0x02,
    0x09, 0xea, 0x81, 0x02, 0x75, 0x01, 0x95, 0x04,
    0x81, 0x01, 0xc0,
};

static int32_t
hid_descriptor(
		uint8_t *buf, uint8_t const * const eob,
		uint8_t const *data, uint32_t datalen)
{
  
    /* set the HID descriptor. seq, seq, 
     * and string of data 
     above */
    
  SDP_PUT8(SDP_DATA_SEQ8, buf); 		//2
  SDP_PUT8(105, buf);
  SDP_PUT8(SDP_DATA_SEQ8, buf); 		//2
  SDP_PUT8(103, buf);
  SDP_PUT8(SDP_DATA_UINT8, buf);	//2
  SDP_PUT8(0x22, buf);
  SDP_PUT8(SDP_DATA_STR8, buf);
  SDP_PUT8(sizeof(base_hid_descriptor), buf);
  memcpy(buf, base_hid_descriptor, sizeof(base_hid_descriptor));
  return (107); //should be

}

static int32_t
hid_langid(
		uint8_t *buf, uint8_t const * const eob,
		uint8_t const *data, uint32_t datalen)
{
  
  /*  from ml post by Iain Hibbert
   * seq
      seq
	uint16    0x0409
	uint16    0x0100

	where 0x0409 translates as "English (US)" and 0x0100 is the offset of the
	default language attributes, from LanguageBaseAttributeIDList
    */

  SDP_PUT8(SDP_DATA_SEQ8, buf); 		//2
  SDP_PUT8(8, buf);
  SDP_PUT8(SDP_DATA_SEQ8, buf); 		//2
  SDP_PUT8(6, buf);
  SDP_PUT8(SDP_DATA_UINT16, buf);	//3
  SDP_PUT16(0x0409, buf);
  SDP_PUT8(SDP_DATA_UINT16, buf);	//3
  SDP_PUT16(0x0100, buf);
  return(10);
}

/*
 
 list of essential mandatory attributes
 provided by Maksim Yevmenkin (see ml)
 read BT HID spec...
 
1) Service Class ID List 0x0001
2) Protocol Descriptor List  0x0004
3) LanguageBaseAttributeIDL 0x0006
4) AdditionalProtocolDescriptorList 0x000d
5) BluetoothProfile DescriptorList  0x0009
6) HIDParserVersion 0x0201
7) HIDDeviceSubclass 0x0202
8) HIDCountryCode 0x0203
9) HIDVirtualCable 0x0204
10) HIDReconnectInitiate 0x0205
11) HIDDescriptorList 0x0206
12) HIDLANGIDBaseList 0x0207
13) HIDBootDevice 0x020e

					
*/

static attr_t	hid_profile_attrs[] = {
  { SDP_ATTR_SERVICE_RECORD_HANDLE, 			common_profile_create_service_record_handle }, 
  { SDP_ATTR_SERVICE_CLASS_ID_LIST, 			hid_profile_create_service_class_id_list },			/* 1 set service class id */
  { SDP_ATTR_PRIMARY_LANGUAGE_BASE_ID + SDP_ATTR_SERVICE_NAME_OFFSET,							/* xxx - set service name */
	  hid_profile_create_service_name },
  { SDP_ATTR_PROTOCOL_DESCRIPTOR_LIST, 			hid_profile_create_protocol_descriptor_list },			/* 2 protocol descriptor list */
  { SDP_ATTR_LANGUAGE_BASE_ATTRIBUTE_ID_LIST, 		common_profile_create_language_base_attribute_id_list },	/* 3 language base id */
  { SDP_ATTR_ADDITIONAL_PROTOCOL_DESCRIPTOR_LISTS,	hid_profile_create_additional_protocol_descriptor_list },	/* 4 l2cap interrupt channel */
  { SDP_ATTR_BLUETOOTH_PROFILE_DESCRIPTOR_LIST,		hid_profile_create_bluetooth_profile_descriptor_list },		/* 5 bt profile descriptor list */
  { SDP_ATTR_USB_HID_VERSION,				hid_profile_usb_hid_version }, 					/* 6 - usb hid version */
  { SDP_ATTR_HID_DEVICE_SUBCLASS, 			hid_profile_keyboard }, 					/* 7 - keyboard */
  { SDP_ATTR_PROFILE_COUNTRY, 				hid_profile_country }, 						/* 8 - country - not localized */
  { SDP_ATTR_HID_VIRTUAL_CABLE, 			hid_profile_virtual_cable }, 					/* 9 - virtual cable - false  */
  { SDP_ATTR_AUTO_RECONNECT, 				hid_profile_reconnect_auto }, 					/* 10 - reconnect initiate - false */
  { SDP_ATTR_HID_DESCRIPTOR, 				hid_descriptor },						/* 11 - hid descriptor */
  { SDP_ATTR_LANG_ID,					hid_langid },							/* 12 - hid langid base list */
  { SDP_ATTR_BOOT_DEVICE, 				hid_profile_boot_device }, 					/* 13 - boot device - false */



	{ 0, NULL } /* end entry */
};


profile_t	hid_profile_descriptor = {
	SDP_SERVICE_CLASS_HUMAN_INTERFACE_DEVICE,
	sizeof(sdp_hid_profile_t),
	hid_profile_data_valid,
	(attr_t const * const) &hid_profile_attrs
};



