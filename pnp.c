/*
 * pnp.c
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
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
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

/* pnp info TEST */

struct sdp_pnp_profile
{
        uint8_t server_channel;
        uint8_t supported_formats_size;
        uint8_t supported_formats[30];
};
typedef struct sdp_pnp_profile        sdp_pnp_profile_t;
typedef struct sdp_pnp_profile *      sdp_pnp_profile_p;


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



static int32_t
pnp_profile_vendor_id_source(
		uint8_t *buf, uint8_t const * const eob,
		uint8_t const *data, uint32_t datalen)
{
/*
 *	1	Bluetooth SIG assigned Company Identifier value from the Assigned Numbers document
 *	2	USB Implementer's Forum assigned Vendor ID value
 */

    SDP_PUT8(SDP_DATA_UINT8, buf);	//2
    SDP_PUT8(0x1, buf);
    return (2);
}

/*
 *           keyboard:
              Address: 00-25-bc-fc-29-01
              Type: Keyboard
              Firmware Version: 0x50
              Services: Apple Wireless Keyboard
              Paired: Yes
              Favorite: No
              Connected: Yes
              Manufacturer: Apple (0x3, 0x31c)
              Vendor ID: 0x5ac
              Product ID: 0x239
              EDR Supported: No
              eSCO Supported: No
              */

static int32_t
pnp_profile_vendor_id(
		uint8_t *buf, uint8_t const * const eob,
		uint8_t const *data, uint32_t datalen)
{
  
    /* Identifies the product vendor from the namespace in the Vendor ID Source
     * value here is for TESTING only - not for prodution use!!
     */
    
    SDP_PUT8(SDP_DATA_UINT16, buf);	//3
    SDP_PUT16(0x05ac, buf);
    return (3);
}

static int32_t
pnp_profile_product_id(
		uint8_t *buf, uint8_t const * const eob,
		uint8_t const *data, uint32_t datalen)
{
  
    /* Identifies the product id, managed by mfg
     * value here is for TESTING only - not for prodution use!!
     */
    
    SDP_PUT8(SDP_DATA_UINT16, buf);	//3
    SDP_PUT16(0x0239, buf);
    return (3);
}

static int32_t
pnp_profile_product_version(
		uint8_t *buf, uint8_t const * const eob,
		uint8_t const *data, uint32_t datalen)
{
  
    /* Identifies the product version, managed by mfg
     * value here is for TESTING only - not for prodution use!!
     */
    
    SDP_PUT8(SDP_DATA_UINT16, buf);	//3
    SDP_PUT16(0x0050, buf);
    return (3);
}

static int32_t
pnp_profile_specification_id(
		uint8_t *buf, uint8_t const * const eob,
		uint8_t const *data, uint32_t datalen)
{
  
    /* Identifies the Bluetooth specification version
     * value here is for TESTING only - not for prodution use!!
     */
    
    SDP_PUT8(SDP_DATA_UINT16, buf);	//3
    SDP_PUT16(0x0102, buf);
    return (3);
}

static int32_t
pnp_profile_primary_record(
		uint8_t *buf, uint8_t const * const eob,
		uint8_t const *data, uint32_t datalen)
{
  
    /* this is the primary record */
    
    SDP_PUT8(SDP_DATA_BOOL8, buf);	//2
    SDP_PUT8(SDP_DATA_TRUE, buf);
    return (2);
}

int32_t
pnp_profile_data_valid(uint8_t const *data, uint32_t datalen)
{
	//sdp_hid_profile_p	hid = (sdp_hid_profile_p) data;
	/* validate data here*/
	/* TODO: validate the data, for now just return 'OK' */
	return (1);
}


static attr_t	pnp_profile_attrs[] = {
	{ SDP_ATTR_SERVICE_RECORD_HANDLE, 			common_profile_create_service_record_handle }, 
	{ SDP_ATTR_SERVICE_CLASS_ID_LIST, 			pnp_profile_create_service_class_id_list },
	{ SDP_ATTR_PROTOCOL_DESCRIPTOR_LIST, 			pnp_profile_create_protocol_descriptor_list },
	{ SDP_ATTR_BLUETOOTH_PROFILE_DESCRIPTOR_LIST,		pnp_profile_create_bluetooth_profile_descriptor_list },
	{ SDP_ATTR_PNP_SPECIFICATION_ID,			pnp_profile_specification_id },
	{ SDP_ATTR_PNP_VENDOR_ID,				pnp_profile_vendor_id },
	{ SDP_ATTR_PNP_PRODUCT_ID,				pnp_profile_product_id },
	{ SDP_ATTR_PNP_VERSION,					pnp_profile_product_version },
	{ SDP_ATTR_PNP_PRIMARY_RECORD,				pnp_profile_primary_record },
	{ SDP_ATTR_PNP_VENDOR_ID_SOURCE,			pnp_profile_vendor_id_source },

	{ 0, NULL } /* end entry */
};

profile_t	pnp_profile_descriptor = {
	SDP_SERVICE_CLASS_PNP_DEVICE,
	sizeof(sdp_pnp_profile_t),
	pnp_profile_data_valid,
	(attr_t const * const) &pnp_profile_attrs
};

/*
Attribute Identifier : 0x0 - ServiceRecordHandle
  Integer : 0x10001
Attribute Identifier : 0x1 - ServiceClassIDList
  Data Sequence
    UUID16 : 0x1200 - PnPInformation
Attribute Identifier : 0x4 - ProtocolDescriptorList
  Data Sequence
    Data Sequence
      UUID16 : 0x0100 - L2CAP
      Channel/Port (Integer) : 0x1
    Data Sequence
      UUID16 : 0x0001 - SDP
Attribute Identifier : 0x9 - BluetoothProfileDescriptorList
  Data Sequence
    Data Sequence
      UUID16 : 0x1200 - PnPInformation
      Version (Integer) : 0x100
Attribute Identifier : 0x200 - SpecificationID
  Integer : 0x100
Attribute Identifier : 0x201 - VendorID
  Integer : 0x004C 
Attribute Identifier : 0x202 - ProductID
  Integer : 0x255
Attribute Identifier : 0x203 - Version
  Integer : 0x50
Attribute Identifier : 0x204 - PrimaryRecord
  Integer : 0x1
Attribute Identifier : 0x205 - VendorIDSource
  Integer : 0x2
  
*/