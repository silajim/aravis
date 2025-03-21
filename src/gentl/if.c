/* Aravis - Digital camera library
 *
 * Copyright © 2023 Václav Šmilauer
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, see <http://www.gnu.org/licenses/>.
 *
 * Authors: Václav Šmilauer <eu@doxos.eu>
 */

#include "private.h"
#include <arvinterface.h>
#include <arvcamera.h>

#define _IF_CHECK_HANDLE GENTL_ENSURE_INIT; if(hIface==NULL || !(ARV_IS_INTERFACE(hIface))) return GC_ERR_INVALID_HANDLE;

GC_API
IFClose (IF_HANDLE hIface)
{
	_IF_CHECK_HANDLE;
	arv_trace_gentl("%s (hIface=%s[%p])",__FUNCTION__,G_OBJECT_TYPE_NAME(hIface),hIface);

	/* ArvInterface* is singleton, no need to free it */
	return GC_ERR_SUCCESS;
}

GC_API
IFGetInfo (IF_HANDLE hIface, INTERFACE_INFO_CMD iInfoCmd, INFO_DATATYPE *piType, void *pBuffer, size_t *piSize)
{
	_IF_CHECK_HANDLE;
	arv_trace_gentl("%s (hIface=%s[%p], iInfoCmd=%d)",__FUNCTION__,G_OBJECT_TYPE_NAME(hIface),hIface,iInfoCmd);

	switch (iInfoCmd) {
		case INTERFACE_INFO_ID:
		case INTERFACE_INFO_DISPLAYNAME:
		case INTERFACE_INFO_TLTYPE:
			return gentl_to_buf(INFO_DATATYPE_STRING,pBuffer,
                                            arv_interface_get_device_protocol(hIface,/*unused*/0),piSize,piType);
		default:
			return GC_ERR_INVALID_PARAMETER;
	}
}

GC_API
IFGetNumDevices (IF_HANDLE hIface, uint32_t *piNumDevices)
{
	_IF_CHECK_HANDLE;
	arv_trace_gentl("%s (hIface=%s[%p], piNumDevices=%p)",
                        __FUNCTION__,G_OBJECT_TYPE_NAME(hIface),hIface,piNumDevices);

	if(piNumDevices==NULL)
                return GC_ERR_INVALID_PARAMETER;

	*piNumDevices=arv_interface_get_n_devices(hIface);
	arv_trace_gentl("    (returning %d)",*piNumDevices);

	return GC_ERR_SUCCESS;
}

GC_API
IFGetDeviceID (IF_HANDLE hIface, uint32_t iIndex, char *sIDeviceID, size_t *piSize)
{
	_IF_CHECK_HANDLE;
	arv_trace_gentl("%s (hIface=%s[%p], iIndex=%d, sIDeviceId=%p, piSize=%p)",
                        __FUNCTION__,G_OBJECT_TYPE_NAME(hIface),hIface,iIndex,sIDeviceID,piSize);

	return gentl_to_buf(INFO_DATATYPE_STRING,sIDeviceID,arv_interface_get_device_id(hIface,iIndex),piSize,NULL);
}

GC_API
IFUpdateDeviceList (IF_HANDLE hIface, bool8_t *pbChanged, uint64_t iTimeout)
{
	_IF_CHECK_HANDLE;
	arv_trace_gentl("%s (hIface=%s[%p], pbChanged=%p, iTimeout=%ld)",
                        __FUNCTION__,G_OBJECT_TYPE_NAME(hIface),hIface,pbChanged,iTimeout);

	/* TODO: should say whether the list changed */
	if(pbChanged)
                pbChanged=0;
	arv_interface_update_device_list(hIface);

	return GC_ERR_SUCCESS;
}

GC_API
IFGetDeviceInfo (IF_HANDLE hIface, const char *sDeviceID, DEVICE_INFO_CMD iInfoCmd, INFO_DATATYPE *piType,
                 void *pBuffer, size_t *piSize)
{
	gint i, iDev;

	_IF_CHECK_HANDLE;
	arv_trace_gentl("%s (hIface=%s[%p], sDeviceID=%s, iInfoCmd=%d, piSize=%p)",
                        __FUNCTION__,G_OBJECT_TYPE_NAME(hIface),hIface,sDeviceID,iInfoCmd,piSize);

	iDev=-1;
	for (i=0; i<arv_interface_get_n_devices(hIface); i++) {
		if (g_strcmp0(arv_interface_get_device_id(hIface,i),sDeviceID)==0) {
                        iDev=i;
                        break;
                }
	}
	if (iDev<0)
                return GC_ERR_INVALID_ID;

	#if 0
		/* do we need device objects for some of the data (like version, access status, timestamp frequency? must be freed again. */
		ArvDevice* dev=arv_interface_open_device(hIface,sDeviceID,&gentl_err);
		if(!dev) return GC_ERR_INVALID_ID;
	#endif

	switch(iInfoCmd) {
		case DEVICE_INFO_ID:
                        return gentl_to_buf(INFO_DATATYPE_STRING,pBuffer,
                                            arv_interface_get_device_id(hIface,iDev),piSize,piType);
		case DEVICE_INFO_VENDOR:
                        return gentl_to_buf(INFO_DATATYPE_STRING,pBuffer,
                                            arv_interface_get_device_vendor(hIface,iDev),piSize,piType);
		case DEVICE_INFO_MODEL:
                        return gentl_to_buf(INFO_DATATYPE_STRING,pBuffer,
                                            arv_interface_get_device_model(hIface,iDev),piSize,piType);
		case DEVICE_INFO_TLTYPE:
                        return gentl_to_buf(INFO_DATATYPE_STRING,pBuffer,
                                            arv_interface_get_device_protocol(hIface,iDev),piSize,piType);
		case DEVICE_INFO_DISPLAYNAME:
                        return gentl_to_buf(INFO_DATATYPE_STRING,pBuffer,
                                            /* ?? */arv_interface_get_device_id(hIface,iDev),piSize,piType);
		case DEVICE_INFO_USER_DEFINED_NAME:
                        return gentl_to_buf(INFO_DATATYPE_STRING,pBuffer,
                                            /* ?? */arv_interface_get_device_id(hIface,iDev),piSize,piType);
		case DEVICE_INFO_SERIAL_NUMBER:
                        return gentl_to_buf(INFO_DATATYPE_STRING,pBuffer,
                                            arv_interface_get_device_serial_nbr(hIface,iDev),piSize,piType);
		/* TODO: where is this supposed to come from? */
		case DEVICE_INFO_VERSION:
                        return gentl_to_buf(INFO_DATATYPE_STRING,pBuffer,"1.0",piSize,piType);
		case DEVICE_INFO_ACCESS_STATUS:
                        {
                                int32_t rw=DEVICE_ACCESS_STATUS_READWRITE;
                                return gentl_to_buf(INFO_DATATYPE_INT32,pBuffer,&rw,piSize,piType);
                        }
		case DEVICE_INFO_TIMESTAMP_FREQUENCY:
		default:
			GENTL_NYI_DETAIL("iInfoCmd=%d",iInfoCmd);
	}
}

GC_API
IFOpenDevice (IF_HANDLE hIface, const char *sDeviceID, DEVICE_ACCESS_FLAGS iOpenFlag, DEV_HANDLE *phDevice)
{
	ArvCamera* cam;

	_IF_CHECK_HANDLE;
	arv_trace_gentl("%s (hIface=%s[%p], sDeviceID=%s, iOpenFlag=%d)",
                        __FUNCTION__,G_OBJECT_TYPE_NAME(hIface),hIface,sDeviceID,iOpenFlag);

	cam=arv_camera_new(sDeviceID,&gentl_err);
	if(!cam)
                return GC_ERR_IO;

	*phDevice=cam;
	return GC_ERR_SUCCESS;
}

/* GenTL v1.4 */
GC_API
IFGetParentTL (IF_HANDLE hIface, TL_HANDLE *phSystem)
{
        GENTL_NYI;
}

