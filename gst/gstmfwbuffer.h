/*
 * Copyright (C) 2010 Freescale Semiconductor, Inc. All rights reserved.
 *
 */

/*
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

/*
 * Module Name:    gstmfwbuffer.h
 *
 * Description:    Head file of Freescale accelerated hardware buffer
 *                 for gstreamer.
 *
 * Portability:    This code is written for Linux OS and Gstreamer
 */  
 
/*
 * Changelog: 
 *
 */
#ifndef __GST_MFW_BUFFER_H__
#define __GST_MFW_BUFFER_H__

#include <gst/gstminiobject.h>
#include <gst/gstbuffer.h>

G_BEGIN_DECLS

typedef struct _GstMfwBuffer GstMfwBuffer;
typedef struct _GstMfwBufferClass GstMfwBufferClass;





#define GST_MFWBUFFER_TRACE_NAME           "GstMfwBuffer"

#define GST_TYPE_MFWBUFFER                         (gst_mfw_buffer_get_type())
#define GST_IS_MFWBUFFER(obj)                      (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GST_TYPE_MFWBUFFER))
#define GST_IS_MFWBUFFER_CLASS(klass)                    (G_TYPE_CHECK_CLASS_TYPE ((klass), GST_TYPE_MFWBUFFER))
#define GST_MFWBUFFER_GET_CLASS(obj)               (G_TYPE_INSTANCE_GET_CLASS ((obj), GST_TYPE_MFWBUFFER, GstMfwBufferClass))
#define GST_MFWBUFFER(obj)                         (G_TYPE_CHECK_INSTANCE_CAST ((obj), GST_TYPE_MFWBUFFER, GstMfwBuffer))
#define GST_MFWBUFFER_CLASS(klass)                 (G_TYPE_CHECK_CLASS_CAST ((klass), GST_TYPE_MFWBUFFER, GstMfwBufferClass))
#define GST_MFWBUFFER_CAST(obj)                    ((GstMfwBuffer *)(obj))



typedef enum {
    GST_MFW_BUFFER_FLAG_DMABLE = (1<<0),
    GST_MFW_BUFFER_FLAG_DMABLE_MANDATORY  =  (1<<1),
} GstMfwBufferFlag;

#define GST_MFWBUFFER_IS_DMABLE(buf)  (GST_MFWBUFFER_CAST(buf)->flags & GST_MFW_BUFFER_FLAG_DMABLE)
#define GST_MFWBUFFER_SET_DMABLE(buf)  \
    do{\
        GST_MFWBUFFER_CAST(buf)->flags |= GST_MFW_BUFFER_FLAG_DMABLE;\
    }while(0)

#define GST_MFWBUFFER_FLAGS(buf) (GST_MFWBUFFER_CAST(buf)->flags)
#define GST_MFWBUFFER_PHYADDRESS(buf) (GST_MFWBUFFER_CAST(buf)->phy_addr)
#define GST_MFWBUFFER_VIRTADDRESS(buf) (GST_MFWBUFFER_CAST(buf)->virt_addr)
#define GST_MFWBUFFER_DEF_FINALIZE(buf) (GST_MFWBUFFER_CAST(buf)->def_finalize)
#define GST_MFWBUFFER_PRIVOBJ(buf)			(GST_MFWBUFFER_CAST(buf)->priv)
#define GST_MFWBUFFER_SUBBUF(buf)			(GST_MFWBUFFER_CAST(buf)->sub_buf)
#define GST_MFWBUFFER_UID(buf)			(GST_MFWBUFFER_CAST(buf)->uid)





#define GST_GSTMFWBUFFER_SET_FINALIZE_CALLBACK(buf, callback, args)\
    do{\
        GST_BUFFER_CAST(buf)->def_finalize=(callback);\
    }while(0)


typedef void (*mfw_buf_finalize_func) (GstMfwBuffer *);


struct _GstMfwBuffer {
    GstBuffer gst_buf;
    gint uid; /* uniformed id, default -1 */
    guint flags;
    void * virt_addr;
    void * phy_addr;
    void * alloc_obj; /* same as virtaddr when sw buffer, hwbuf desc when hw buffer */
    void * priv; /* caller defined priv */
    mfw_buf_finalize_func def_finalize;
    GstBuffer * sub_buf;
};

struct _GstMfwBufferClass {
  GstBufferClass gstbuf_class;
};

GType       gst_mfw_buffer_get_type (void);

/* allocation */
GstMfwBuffer * gst_mfw_buffer_new               (void);
GstMfwBuffer * gst_mfw_buffer_new_and_alloc     (guint size, guint flags);

void gst_mfw_buffer_make_readonly(GstMfwBuffer * buffer);

void gst_mfw_buffer_replace_sub_buffer(GstMfwBuffer * buffer, GstBuffer * sub_buf);

G_END_DECLS

#endif /* __GstMfwBuffer_H__ */

