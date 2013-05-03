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
 * Module Name:    gstmfwbuffer.c
 *
 * Description:    Implementation of Freescale accelerated hardware buffer
 *                 for gstreamer.
 *
 * Portability:    This code is written for Linux OS and Gstreamer
 */  
 
/*
 * Changelog: 
 *
 */
#include <stdio.h>
#include <stdlib.h>
#include <dlfcn.h>

#include "gst_private.h"

#include "gstbuffer.h"
#include "gstinfo.h"
#include "gstutils.h"
#include "gstminiobject.h"
#include "gstmfwbuffer.h"

typedef void * (* new_hwbuf_func)(int, void **, void **, int flags);
typedef void (* free_hwbuf_func)(void *);

static new_hwbuf_func g_new_hwbuf_handle = NULL;
static free_hwbuf_func g_free_hwbuf_handle = NULL;
static void * g_dlhandle = NULL;


static void gst_mfw_buffer_finalize (GstMfwBuffer * buffer);
static GType _gst_mfw_buffer_type = 0;

static int g_mfwbuffer_cnt = 0;
static int g_mfwbuffer_max = 0;


void open_allocator_dll()
{
    char * errstr;
    g_dlhandle = dlopen("libmfwba.so", RTLD_LAZY);
    
    if (!g_dlhandle) {
        GST_CAT_ERROR(GST_CAT_MFWBUFFER, "Can not open dll, %s.\n", dlerror());
        goto error;
    }
    
    dlerror();
    g_new_hwbuf_handle = dlsym(g_dlhandle, "mfw_new_hw_buffer");
    if ((errstr = dlerror()) != NULL)  {
        GST_CAT_ERROR(GST_CAT_MFWBUFFER, "Can load symbl for mfw_new_hw_buffer, %s\n", errstr);
        goto error;
    }
    
    dlerror();
    g_free_hwbuf_handle = dlsym(g_dlhandle, "mfw_free_hw_buffer");
    if ((errstr = dlerror()) != NULL)  {
        GST_CAT_ERROR(GST_CAT_MFWBUFFER, "Can load symbl for mfw_free_hw_buffer, %s\n", errstr);
        goto error;
    }

    return;

error:
    if (g_dlhandle){
        dlclose(g_dlhandle);
        g_dlhandle=NULL;
    }
    g_new_hwbuf_handle = NULL;
    g_free_hwbuf_handle = NULL;
    
}

void
_gst_mfw_buffer_initialize (void)
{
  /* the GstMiniObject types need to be class_ref'd once before it can be
   * done from multiple threads;
   * see http://bugzilla.gnome.org/show_bug.cgi?id=304551 */
  g_type_class_ref (gst_mfw_buffer_get_type ());
  open_allocator_dll();
}

#define _do_init \
{ \
  _gst_mfw_buffer_type = g_define_type_id; \
}

G_DEFINE_TYPE_WITH_CODE (GstMfwBuffer, gst_mfw_buffer, GST_TYPE_BUFFER, _do_init);

static void
gst_mfw_buffer_class_init (GstMfwBufferClass * klass)
{
  klass->gstbuf_class.mini_object_class.finalize =
      (GstMiniObjectFinalizeFunction) gst_mfw_buffer_finalize;
}

static void
gst_mfw_buffer_finalize (GstMfwBuffer * buffer)
{
    g_return_if_fail (buffer != NULL);

    GST_CAT_LOG (GST_CAT_MFWBUFFER, "finalize %p", buffer);

    /* custom finalize callback */
    if (G_UNLIKELY (buffer->def_finalize))
        (*buffer->def_finalize)(buffer);

    /* actually free all resource when refcont not added finalize function */
    if (GST_MINI_OBJECT_REFCOUNT(buffer)==0){
        if (buffer->sub_buf){
            gst_buffer_unref(buffer->sub_buf);
            buffer->sub_buf = NULL;
        }
        
        if (buffer->alloc_obj){
            if (buffer->flags&GST_MFW_BUFFER_FLAG_DMABLE){
                (*g_free_hwbuf_handle)(buffer->alloc_obj);
            }else{
                g_free(buffer->alloc_obj);
            }
            buffer->alloc_obj = NULL;
        }
        buffer->flags=0;
        g_mfwbuffer_cnt--;
    }
}

static void
gst_mfw_buffer_init (GstMfwBuffer * buffer)
{
  GST_CAT_LOG (GST_CAT_MFWBUFFER, "init %p", buffer);

  buffer->phy_addr = NULL;
  buffer->virt_addr = NULL;
  buffer->alloc_obj = NULL;
  buffer->priv = NULL;
  buffer->sub_buf = NULL;
  buffer->def_finalize = NULL;
  buffer->flags = 0;
  buffer->uid = -1;
  g_mfwbuffer_cnt++;
  if (g_mfwbuffer_cnt>g_mfwbuffer_max){
    GST_CAT_LOG (GST_CAT_MFWBUFFER, "max mfwbuffer count %d\n", g_mfwbuffer_cnt);
    g_mfwbuffer_max = g_mfwbuffer_cnt;
  }
}

GstMfwBuffer *
gst_mfw_buffer_new (void)
{
  GstMfwBuffer *newbuf;

  newbuf = (GstMfwBuffer *) gst_mini_object_new (_gst_mfw_buffer_type);

  GST_CAT_LOG (GST_CAT_MFWBUFFER, "new %p", newbuf);

  return newbuf;
}

GstMfwBuffer *
gst_mfw_buffer_new_and_alloc (guint size, guint flags )
{
    GstMfwBuffer *newbuf = NULL;

    if ((newbuf = gst_mfw_buffer_new())==NULL){
        GST_CAT_ERROR(GST_CAT_MFWBUFFER, "Can not new gst_mfw_buffer\n");
        goto error;
    }

    if (flags&GST_MFW_BUFFER_FLAG_DMABLE){
        if (g_new_hwbuf_handle){
            newbuf->alloc_obj = 
                (*g_new_hwbuf_handle)(size, &newbuf->phy_addr, &newbuf->virt_addr, 0);
        }
        if ((newbuf->alloc_obj==NULL) && (flags&GST_MFW_BUFFER_FLAG_DMABLE_MANDATORY)){
            GST_CAT_ERROR(GST_CAT_MFWBUFFER, "Can not create mandatory hwbuf size %d\n", size);
            goto error;
        }

        if (newbuf->alloc_obj)
            newbuf->flags |= GST_MFW_BUFFER_FLAG_DMABLE;
    }

    if (newbuf->alloc_obj==NULL){
        newbuf->alloc_obj = g_malloc(size);
        if (newbuf->alloc_obj==NULL){
            GST_CAT_ERROR(GST_CAT_MFWBUFFER, "Can not create sw buf size %d\n", size);
            goto error;
        }
        newbuf->virt_addr = newbuf->alloc_obj;
    }

    GST_BUFFER_SIZE(newbuf) = size;
    GST_BUFFER_DATA(newbuf) = newbuf->virt_addr;

    return newbuf;

error:
    if (newbuf){
        gst_mini_object_unref(GST_MINI_OBJECT_CAST(newbuf));
        newbuf = NULL;    
    }
    return newbuf;
}

void gst_mfw_buffer_make_readonly(GstMfwBuffer * buffer)
{
    g_return_if_fail (buffer != NULL);
    
    GST_MINI_OBJECT_FLAG_SET(buffer, GST_MINI_OBJECT_FLAG_READONLY);
}

void gst_mfw_buffer_replace_sub_buffer(GstMfwBuffer * buffer, GstBuffer * sub_buf)
{
    g_return_if_fail (buffer != NULL);

    if (buffer->sub_buf){
        gst_buffer_unref(buffer->sub_buf);
    }

    if (sub_buf){
        gst_buffer_ref(sub_buf);
    }

    buffer->sub_buf = sub_buf;
}

