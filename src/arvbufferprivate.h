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
 * Author: Emmanuel Pacaud <emmanuel.pacaud@free.fr>
 */

#ifndef ARV_BUFFER_PRIVATE_H
#define ARV_BUFFER_PRIVATE_H

#include <arvbuffer.h>
#include <arvgvspprivate.h>
#include <stddef.h>

G_BEGIN_DECLS

typedef struct {
        ptrdiff_t data_offset;
        size_t size;
        guint component_id;
        ArvBufferPartDataType data_type;
	ArvPixelFormat pixel_format;
	guint32 width;
	guint32 height;
	guint32 x_offset;
	guint32 y_offset;
	guint32 x_padding;
	guint32 y_padding;
} ArvBufferPartInfos;

typedef struct {
	size_t allocated_size;
	gboolean is_preallocated;
	unsigned char *data;

	void *user_data;
	GDestroyNotify user_data_destroy_func;

	ArvBufferStatus status;
        size_t received_size;

	ArvBufferPayloadType payload_type;
        gboolean has_chunks;

	guint32 chunk_endianness;

	guint64 frame_id;
	guint64 timestamp_ns;
	guint64 system_timestamp_ns;

        guint n_parts;
        ArvBufferPartInfos *parts;

	gboolean has_gendc;
	guint32 gendc_descriptor_size;
	guint64 gendc_data_size;
	guint64 gendc_data_offset;
} ArvBufferPrivate;

struct _ArvBuffer {
	GObject	object;

	ArvBufferPrivate *priv;
};

struct _ArvBufferClass {
	GObjectClass parent_class;
};

void            arv_buffer_set_n_parts                  (ArvBuffer* buffer, guint n_parts);

G_END_DECLS

#endif
