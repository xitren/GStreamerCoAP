/*
 * GStreamer
 * Copyright (C) 2005 Thomas Vander Stichele <thomas@apestaart.org>
 * Copyright (C) 2005 Ronald S. Bultje <rbultje@ronald.bitfreak.net>
 * Copyright (C) 2013 xitren <<xitren@ya.ru>>
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *
 * Alternatively, the contents of this file may be used under the
 * GNU Lesser General Public License Version 2.1 (the "LGPL"), in
 * which case the following provisions apply instead of the ones
 * mentioned above:
 *
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

/**
 * SECTION:element-ocv
 *
 * FIXME:Describe ocv here.
 *
 * <refsect2>
 * <title>Example launch line</title>
 * |[
 * gst-launch -v -m fakesrc ! ocv ! fakesink silent=TRUE
 * ]|
 * </refsect2>
 */
#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <gst/gst.h>
#include <fcntl.h>	/* open() and O_XXX flags */
#include <sys/stat.h>	/* S_IXXX flags */
#include <sys/types.h>	/* mode_t */
#include <unistd.h>	/* close() */
#include <stdlib.h>
#include <stdio.h>

#include "gstocv.h"

GST_DEBUG_CATEGORY_STATIC (gst_ocv_debug);
#define GST_CAT_DEFAULT gst_ocv_debug


//Structures
/* Filter signals and args */
enum
{
  /* FILL ME */
  LAST_SIGNAL
};

enum
{
  PROP_0,
  PROP_SILENT,
  PROP_PRECISION,
  PROP_PERSENTAGE
};

static GstBuffer * last_one;
static gint images_count;
static gint precision;
static gint percentage;
static gint _midbright;
static gint _width;
static gint _height;

//functions
static GstBuffer * my_filter_calc_map(GstBuffer * buf, const gint width, const gint height);
static void my_filter_discrete(const GstMapInfo * img_map,GstMapInfo * bit_map, const gint width, const gint height, gint midtone);

static GstBuffer * my_filter_calc_map(GstBuffer * buf, const gint width, const gint height)
{
  GstMapInfo info;
  GstMapInfo map_info;
  GstBuffer *map;
  GstMemory *map_mem;

  /* make empty buffer */
  map = gst_buffer_new ();

  /* make memory */
  map_mem = gst_allocator_alloc (NULL, (width*height)/(precision*precision), NULL);

  /* add the the buffer */
  gst_buffer_append_memory (map, map_mem);

  /* get WRITE access to the memory*/
  gst_buffer_map (buf, &info, GST_MAP_WRITE || GST_MAP_READ);
  gst_buffer_map (map, &map_info, GST_MAP_WRITE || GST_MAP_READ);

  //========================================
  my_filter_discrete(&info, &map_info, width, height, 0);
  //========================================

  gst_buffer_unmap (buf, &info);
  gst_buffer_unmap (map, &map_info);
  gst_buffer_unref (map);
  return map;
}
static void my_filter_discrete(const GstMapInfo * img_map,GstMapInfo * bit_map, const gint width, const gint height, gint midtone)
{
  gint i,j;
//  if ((_midbright = midtone) == 0)
//  	  _midbright = midtone = my_filter_get_midtone(img_map,width,height);
  for (j=0;j<height;j++)
  {
	  for (i=0;i<width;i++)
	  {
  		if((*(img_map->data+(i+j*width)*2)) < midtone)
			*(bit_map->data+i/precision+(j/precision)*(width/precision)) = '0';
		else
			*(bit_map->data+i/precision+(j/precision)*(width/precision)) = '1';
	  }
  }
  //g_print("Midtone: %d Images: %d\n",midtone,images_count);
  return;
}
/* the capabilities of the inputs and outputs.
 *
 * describe the real formats here.
 */
static GstStaticPadTemplate sink_factory = GST_STATIC_PAD_TEMPLATE ("sink",
    GST_PAD_SINK,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS ("video/x-raw,format=(string)YUY2")
    );

static GstStaticPadTemplate src_factory = GST_STATIC_PAD_TEMPLATE ("src",
    GST_PAD_SRC,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS ("video/x-raw,format=(string)YUY2")
    );

#define gst_ocv_parent_class parent_class
G_DEFINE_TYPE (Gstocv, gst_ocv, GST_TYPE_ELEMENT);

static void gst_ocv_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec);
static void gst_ocv_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec);

static gboolean gst_ocv_sink_event (GstPad * pad, GstObject * parent, GstEvent * event);
static GstFlowReturn gst_ocv_chain (GstPad * pad, GstObject * parent, GstBuffer * buf);

/* GObject vmethod implementations */

/* initialize the ocv's class */
static void
gst_ocv_class_init (GstocvClass * klass)
{
  GObjectClass *gobject_class;
  GstElementClass *gstelement_class;

  gobject_class = (GObjectClass *) klass;
  gstelement_class = (GstElementClass *) klass;

  gobject_class->set_property = gst_ocv_set_property;
  gobject_class->get_property = gst_ocv_get_property;

  g_object_class_install_property (gobject_class, PROP_SILENT,
      g_param_spec_boolean ("silent", "Silent", "Produce verbose output ?",
          FALSE, G_PARAM_READWRITE));

  gst_element_class_set_details_simple(gstelement_class,
    "ocv",
    "Open Contest Video Plugin",
    "Made to RTSoft linux contest",
    "xitren <<xitren@ya.ru>>");

  gst_element_class_add_pad_template (gstelement_class,
      gst_static_pad_template_get (&src_factory));
  gst_element_class_add_pad_template (gstelement_class,
      gst_static_pad_template_get (&sink_factory));
}

/* initialize the new element
 * instantiate pads and add them to element
 * set pad calback functions
 * initialize instance structure
 */
static void
gst_ocv_init (Gstocv * filter)
{
  filter->sinkpad = gst_pad_new_from_static_template (&sink_factory, "sink");
  gst_pad_set_event_function (filter->sinkpad,
                              GST_DEBUG_FUNCPTR(gst_ocv_sink_event));
  gst_pad_set_chain_function (filter->sinkpad,
                              GST_DEBUG_FUNCPTR(gst_ocv_chain));
  GST_PAD_SET_PROXY_CAPS (filter->sinkpad);
  gst_element_add_pad (GST_ELEMENT (filter), filter->sinkpad);

  filter->srcpad = gst_pad_new_from_static_template (&src_factory, "src");
  GST_PAD_SET_PROXY_CAPS (filter->srcpad);
  gst_element_add_pad (GST_ELEMENT (filter), filter->srcpad);

  filter->silent = FALSE;
}

static void
gst_ocv_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec)
{
  Gstocv *filter = GST_OCV (object);

  switch (prop_id) {
    case PROP_SILENT:
      filter->silent = g_value_get_boolean (value);
      break;
    case PROP_PRECISION:
      precision = g_value_get_int(value);
      break;
    case PROP_PERSENTAGE:
      percentage = g_value_get_int(value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void
gst_ocv_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec)
{
  Gstocv *filter = GST_OCV (object);

  switch (prop_id) {
    case PROP_SILENT:
      g_value_set_boolean (value, filter->silent);
      break;
    case PROP_PRECISION:
      g_value_set_int (value, precision);
      break;
    case PROP_PERSENTAGE:
      g_value_set_int (value, percentage);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

/* GstElement vmethod implementations */

/* this function handles sink events */
static gboolean
gst_ocv_sink_event (GstPad * pad, GstObject * parent, GstEvent * event)
{
  gboolean ret;
  Gstocv *filter;
  gint x1,x2,y1,y2,time,type;
  GstMapInfo info;

  filter = GST_OCV (parent);
              g_print ("Event\n");

  switch (GST_EVENT_TYPE (event)) {
    case GST_EVENT_CAPS:
    {
      GstCaps * caps;

      gst_event_parse_caps (event, &caps);
      /* do something with the caps */

      /* and forward */
      ret = gst_pad_event_default (pad, parent, event);
      break;
    }
    case GST_EVENT_CUSTOM_UPSTREAM:
    {
      type = g_value_get_int(gst_structure_get_value(gst_event_get_structure(event),"TYPE"));
      if (type == 1){
	      x1 = g_value_get_int(gst_structure_get_value(gst_event_get_structure(event),"X1"));
	      x2 = g_value_get_int(gst_structure_get_value(gst_event_get_structure(event),"X2"));
	      y1 = g_value_get_int(gst_structure_get_value(gst_event_get_structure(event),"Y1"));
	      y2 = g_value_get_int(gst_structure_get_value(gst_event_get_structure(event),"Y2"));
	      time = g_value_get_int(gst_structure_get_value(gst_event_get_structure(event),"TIME"));
	      if ((x1 >= x2) || (y1 >= y2))
			break;
	      gst_buffer_map (last_one, &info, GST_MAP_WRITE || GST_MAP_READ);
	      //my_filter_get_inside(&info, x1, y1, x2, y2, _width, _height, time);
              g_print ("New image from (%d;%d) to (%d;%d)\n",x1,y1,x2,y2);
      }
      else if (type == 0){
	      //my_filter_images_free(images_maps);
              g_print ("Images free\n");
      }
      else if (type == 2){
	      precision = g_value_get_int(gst_structure_get_value(gst_event_get_structure(event),"PRECISION"));
              g_print ("Precision: %d\n",precision);
      }
      else if (type == 3){
	      percentage = g_value_get_int(gst_structure_get_value(gst_event_get_structure(event),"PERSENTAGE"));
              g_print ("Percentage: %d\n",percentage);
      }
      //my_filter_write_file(DEFAULT_PATH,images_maps);
      break;
    }
    default:
      ret = gst_pad_event_default (pad, parent, event);
      break;
  }
  return ret;
}

/* chain function
 * this function does the actual processing
 */
static GstFlowReturn
gst_ocv_chain (GstPad * pad, GstObject * parent, GstBuffer * buf)
{
  gint width, height;
  gchar* type;
  const GstStructure *str;
  Gstocv *filter;
  GstCaps *cap;

  filter = GST_OCV (parent);
  last_one = gst_buffer_copy(buf);

  if (NULL != (cap=gst_pad_get_current_caps (pad)))
  {
	  g_return_if_fail (gst_caps_is_fixed (cap));

	  str = gst_caps_get_structure (cap, 0);
  	  type = gst_structure_get_string (str, "format");
	  if (!gst_structure_get_int (str, "width", &width) ||
	      !gst_structure_get_int (str, "height", &height)/* ||
	      !gst_structure_get_int (str, "variant", &bpp)*/) {
	    g_print ("No width/height available\n");
	    return gst_pad_push (filter->srcpad, buf);
	  _width = width;
	  _height = height;
	  //g_print ("The vide
	  }
	  _width = width;
	  _height = height;
	  //g_print ("The video %s size %d of this set of capabilities is %dx%d depth=%d\n", type, gst_buffer_get_size(buf), width, height, bpp);
	  my_filter_calc_map(buf, width, height);
  }
  /* just push out the incoming buffer without touching it */
  return gst_pad_push (filter->srcpad, buf);
}


/* entry point to initialize the plug-in
 * initialize the plug-in itself
 * register the element factories and other features
 */
static gboolean
ocv_init (GstPlugin * ocv)
{
  precision = 20;
  percentage = 100;
  /* debug category for fltering log messages
   *
   * exchange the string 'Template ocv' with your description
   */
  GST_DEBUG_CATEGORY_INIT (gst_ocv_debug, "ocv",
      0, "Template ocv");

  return gst_element_register (ocv, "ocv", GST_RANK_NONE,
      GST_TYPE_OCV);
}

/* PACKAGE: this is usually set by autotools depending on some _INIT macro
 * in configure.ac and then written into and defined in config.h, but we can
 * just set it ourselves here in case someone doesn't use autotools to
 * compile this code. GST_PLUGIN_DEFINE needs PACKAGE to be defined.
 */
#ifndef PACKAGE
#define PACKAGE "myfirstocv"
#endif

/* gstreamer looks for this structure to register ocvs
 *
 * exchange the string 'Template ocv' with your ocv description
 */
GST_PLUGIN_DEFINE (
    GST_VERSION_MAJOR,
    GST_VERSION_MINOR,
    ocv,
    "Template ocv",
    ocv_init,
    VERSION,
    "LGPL",
    "GStreamer",
    "http://gstreamer.net/"
)
