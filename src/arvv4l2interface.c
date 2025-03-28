/* Aravis - Digital camera library
 *
 * Copyright © 2009-2025 Emmanuel Pacaud <emmanuel.pacaud@free.fr>
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
 * Author: Emmanuel Pacaud <emmanuel@gnome.org>
 */

/**
 * SECTION: arvv4l2interface
 * @short_description: V4l2 interface
 */

#include <arvv4l2interfaceprivate.h>
#include <arvv4l2deviceprivate.h>
#include <arvv4l2miscprivate.h>
#include <arvinterfaceprivate.h>
#include <arvv4l2device.h>
#include <arvdebugprivate.h>
#include <arvmisc.h>
#include <fcntl.h>
#include <gudev/gudev.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <linux/videodev2.h>
#include <linux/media.h>

struct _ArvV4l2Interface {
	ArvInterface	interface;

	GHashTable *devices;
	GUdevClient *udev;
};

struct _ArvV4l2InterfaceClass {
	ArvInterfaceClass parent_class;
};

G_DEFINE_TYPE (ArvV4l2Interface, arv_v4l2_interface, ARV_TYPE_INTERFACE)

typedef struct {
	char *id;
        char *model;
        char *driver;
	char *bus;
	char *device_file;
	char *version;
        char *serial_nbr;

	volatile gint ref_count;
} ArvV4l2InterfaceDeviceInfos;

static ArvV4l2InterfaceDeviceInfos *
arv_v4l2_interface_device_infos_new (const char *device_file, const char *name)
{
	ArvV4l2InterfaceDeviceInfos *infos = NULL;

	g_return_val_if_fail (device_file != NULL, NULL);

	if (strncmp ("/dev/vbi", device_file,  8) != 0) {
		int fd;
                struct stat st;

                if (stat(device_file, &st) == -1)
                        return NULL;

                if (!S_ISCHR(st.st_mode))
                        return NULL;

                fd = open (device_file, O_RDWR, 0);
                if (fd != -1) {
                        struct v4l2_capability cap;

			if (ioctl (fd, VIDIOC_QUERYCAP, &cap) != -1 &&
			    ((cap.capabilities & V4L2_CAP_VIDEO_CAPTURE) != 0) &&
			    ((cap.capabilities & V4L2_CAP_STREAMING) != 0)) {
                                unsigned int i;
                                gboolean found = FALSE;
                                struct media_device_info mdinfo = {0};

                                for (i = 0; TRUE; i++) {
                                        struct v4l2_fmtdesc format = {0};

                                        format.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
                                        format.index = i;
                                        if (ioctl(fd, VIDIOC_ENUM_FMT, &format) == -1)
                                                break;

                                        if (arv_pixel_format_from_v4l2(format.pixelformat) != 0) {
                                                found = TRUE;
                                                break;
                                        }
                                }

                                if (found) {
                                        int media_fd = arv_v4l2_get_media_fd(fd, (char *) cap.bus_info);

                                        infos = g_new0 (ArvV4l2InterfaceDeviceInfos, 1);

                                        infos->ref_count = 1;
                                        infos->bus = g_strdup ((char *) cap.bus_info);
                                        infos->driver = g_strdup ((char *) cap.driver);
                                        infos->device_file = g_strdup (device_file);
                                        infos->model = g_strdup ((char *) cap.card);
                                        infos->version = g_strdup_printf ("%d.%d.%d",
                                                                          (cap.version >> 16) & 0xff,
                                                                          (cap.version >>  8) & 0xff,
                                                                          (cap.version >>  0) & 0xff);

                                        if (media_fd != -1 &&
                                            ioctl (media_fd, MEDIA_IOC_DEVICE_INFO, &mdinfo) != -1) {
                                                infos->id = g_strdup_printf ("%s-%s-%s",
                                                                             (char *) cap.driver,
                                                                             (char *) cap.card,
                                                                             mdinfo.serial);
                                                infos->serial_nbr = g_strdup (mdinfo.serial);
                                        } else {
                                                infos->id = g_strdup_printf ("%s-%s-%s",
                                                                             (char *) cap.driver,
                                                                             (char *) cap.card,
                                                                             name);
                                                infos->serial_nbr = g_strdup (device_file);
                                        }

                                        if (media_fd != -1)
                                                close (media_fd);

                                        close (fd);

                                        return infos;
                                }

                                arv_info_interface ("No suitable pixel format found for v4l2 device '%s'",
                                                       device_file);
                        }
                        close (fd);
                }
        }

	return NULL;
}

static ArvV4l2InterfaceDeviceInfos *
arv_v4l2_interface_device_infos_ref (ArvV4l2InterfaceDeviceInfos *infos)
{
	g_return_val_if_fail (infos != NULL, NULL);
	g_return_val_if_fail (g_atomic_int_get (&infos->ref_count) > 0, NULL);

	g_atomic_int_inc (&infos->ref_count);

	return infos;
}

static void
arv_v4l2_interface_device_infos_unref (ArvV4l2InterfaceDeviceInfos *infos)
{
	g_return_if_fail (infos != NULL);
	g_return_if_fail (g_atomic_int_get (&infos->ref_count) > 0);

	if (g_atomic_int_dec_and_test (&infos->ref_count)) {
		g_free (infos->id);
                g_free (infos->model);
                g_free (infos->driver);
		g_free (infos->bus);
		g_free (infos->device_file);
		g_free (infos->version);
                g_free (infos->serial_nbr);
		g_free (infos);
	}
}

static void
_discover (ArvV4l2Interface *v4l2_interface, GArray *device_ids)
{
	GList *devices, *elem;

	g_hash_table_remove_all (v4l2_interface->devices);

	devices = g_udev_client_query_by_subsystem (v4l2_interface->udev, "video4linux");

	for (elem = g_list_first (devices); elem; elem = g_list_next (elem)) {
		ArvV4l2InterfaceDeviceInfos *device_infos;

		device_infos = arv_v4l2_interface_device_infos_new (g_udev_device_get_device_file (elem->data),
                                                                    g_udev_device_get_name(elem->data));
		if (device_infos != NULL) {
			ArvInterfaceDeviceIds *ids;

			g_hash_table_replace (v4l2_interface->devices,
					      device_infos->id,
					      arv_v4l2_interface_device_infos_ref (device_infos));
			g_hash_table_replace (v4l2_interface->devices,
					      device_infos->bus,
					      arv_v4l2_interface_device_infos_ref (device_infos));
			g_hash_table_replace (v4l2_interface->devices,
					      device_infos->device_file,
					      arv_v4l2_interface_device_infos_ref (device_infos));

			if (device_ids != NULL) {
				ids = g_new0 (ArvInterfaceDeviceIds, 1);

				ids->device = g_strdup (device_infos->id);
				ids->physical = g_strdup (device_infos->bus);
				ids->address = g_strdup (device_infos->device_file);
				ids->vendor = g_strdup (device_infos->driver);
				ids->model = g_strdup (device_infos->model);
				ids->serial_nbr = g_strdup (device_infos->serial_nbr);
                                ids->protocol = "V4L2";

				g_array_append_val (device_ids, ids);
			}

			arv_v4l2_interface_device_infos_unref (device_infos);
		}

		g_object_unref (elem->data);
	}

	g_list_free (devices);
}

static void
arv_v4l2_interface_update_device_list (ArvInterface *interface, GArray *device_ids)
{
	ArvV4l2Interface *v4l2_interface = ARV_V4L2_INTERFACE (interface);

	g_assert (device_ids->len == 0);

	_discover (v4l2_interface, device_ids);
}

static ArvDevice *
_open_device (ArvInterface *interface, const char *device_id, GError **error)
{
	ArvV4l2Interface *v4l2_interface = ARV_V4L2_INTERFACE (interface);
	ArvV4l2InterfaceDeviceInfos *device_infos;

	if (device_id == NULL) {
		GList *device_list;

		device_list = g_hash_table_get_values (v4l2_interface->devices);
		device_infos = device_list != NULL ? device_list->data : NULL;
		g_list_free (device_list);
	} else
		device_infos = g_hash_table_lookup (v4l2_interface->devices, device_id);

	if (device_infos != NULL)
		return arv_v4l2_device_new (device_infos->device_file, error);

	return NULL;
}

static ArvDevice *
arv_v4l2_interface_open_device (ArvInterface *interface, const char *device_id, GError **error)
{
	ArvDevice *device;
	GError *local_error = NULL;

	device = _open_device (interface, device_id, error);
	if (ARV_IS_DEVICE (device) || local_error != NULL) {
		if (local_error != NULL)
			g_propagate_error (error, local_error);
		return device;
	}

	_discover (ARV_V4L2_INTERFACE (interface), NULL);

	return _open_device (interface, device_id, error);
}

static ArvInterface *arv_v4l2_interface = NULL;
static GMutex arv_v4l2_interface_mutex;

/**
 * arv_v4l2_interface_get_instance:
 *
 * Gets the unique instance of the v4l2 interface.
 *
 * Returns: (transfer none): a #ArvInterface singleton.
 */

ArvInterface *
arv_v4l2_interface_get_instance (void)
{
	g_mutex_lock (&arv_v4l2_interface_mutex);

	if (arv_v4l2_interface == NULL)
		arv_v4l2_interface = g_object_new (ARV_TYPE_V4L2_INTERFACE, NULL);

	g_mutex_unlock (&arv_v4l2_interface_mutex);

	return ARV_INTERFACE (arv_v4l2_interface);
}

void
arv_v4l2_interface_destroy_instance (void)
{
	g_mutex_lock (&arv_v4l2_interface_mutex);

	if (arv_v4l2_interface != NULL) {
		g_object_unref (arv_v4l2_interface);
		arv_v4l2_interface = NULL;
	}

	g_mutex_unlock (&arv_v4l2_interface_mutex);
}

static void
arv_v4l2_interface_init (ArvV4l2Interface *v4l2_interface)
{
	const gchar *const subsystems[] = {"video4linux", NULL};

	v4l2_interface->udev = g_udev_client_new (subsystems);

	v4l2_interface->devices = g_hash_table_new_full (g_str_hash, g_str_equal, NULL,
							 (GDestroyNotify) arv_v4l2_interface_device_infos_unref);
}

static void
arv_v4l2_interface_finalize (GObject *object)
{
	ArvV4l2Interface *v4l2_interface = ARV_V4L2_INTERFACE (object);

	g_hash_table_unref (v4l2_interface->devices);
	v4l2_interface->devices = NULL;

	G_OBJECT_CLASS (arv_v4l2_interface_parent_class)->finalize (object);
}

static void
arv_v4l2_interface_class_init (ArvV4l2InterfaceClass *v4l2_interface_class)
{
	GObjectClass *object_class = G_OBJECT_CLASS (v4l2_interface_class);
	ArvInterfaceClass *interface_class = ARV_INTERFACE_CLASS (v4l2_interface_class);

	object_class->finalize = arv_v4l2_interface_finalize;

	interface_class->update_device_list = arv_v4l2_interface_update_device_list;
	interface_class->open_device = arv_v4l2_interface_open_device;
}
