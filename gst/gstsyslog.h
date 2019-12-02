/* GStreamer
 *
 * Copyright (C) <2018> HoonHee Lee <hoonhee.lee@lge.com>
 *                      DongYun Seo <dongyun.seo@lge.com>
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
 * Free Software Foundation, Inc., 51 Franklin St, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#ifndef __GSTSYSDEBUG_H_
#define __GSTSYSDEBUG_H_

#include <gst/gstinfo.h>

#define GST_SYS_CAT_LEVEL_LOG(cat,level,object,...) G_STMT_START{   \
  gst_sys_debug_log ((cat), (level), __BASE_FILE__, GST_FUNCTION, __LINE__,  \
      (GObject *) (object), __VA_ARGS__);       \
} G_STMT_END

#define GST_SYS_LOG_OBJECT(obj,...) GST_SYS_CAT_LEVEL_LOG (GST_CAT_DEFAULT, GST_LEVEL_LOG, obj, ##__VA_ARGS__)
#define GST_SYS_DEBUG_OBJECT(obj,...) GST_SYS_CAT_LEVEL_LOG (GST_CAT_DEFAULT, GST_LEVEL_DEBUG, obj, ##__VA_ARGS__)
#define GST_SYS_INFO_OBJECT(obj,...) GST_SYS_CAT_LEVEL_LOG (GST_CAT_DEFAULT, GST_LEVEL_INFO, obj, ##__VA_ARGS__)
#define GST_SYS_FIXME_OBJECT(obj,...) GST_SYS_CAT_LEVEL_LOG (GST_CAT_DEFAULT, GST_LEVEL_FIXME, obj, ##__VA_ARGS__)
#define GST_SYS_WARNING_OBJECT(obj,...) GST_SYS_CAT_LEVEL_LOG (GST_CAT_DEFAULT, GST_LEVEL_WARNING, obj, ##__VA_ARGS__)
#define GST_SYS_ERROR_OBJECT(obj,...) GST_SYS_CAT_LEVEL_LOG (GST_CAT_DEFAULT, GST_LEVEL_ERROR, obj, ##__VA_ARGS__)

#define GST_SYS_LOG(...) GST_SYS_CAT_LEVEL_LOG (GST_CAT_DEFAULT, GST_LEVEL_LOG, NULL, ##__VA_ARGS__)
#define GST_SYS_DEBUG(...) GST_SYS_CAT_LEVEL_LOG (GST_CAT_DEFAULT, GST_LEVEL_DEBUG, NULL, ##__VA_ARGS__)
#define GST_SYS_INFO(...) GST_SYS_CAT_LEVEL_LOG (GST_CAT_DEFAULT, GST_LEVEL_INFO, NULL, ##__VA_ARGS__)
#define GST_SYS_FIXME(...) GST_SYS_CAT_LEVEL_LOG (GST_CAT_DEFAULT, GST_LEVEL_FIXME, NULL, ##__VA_ARGS__)
#define GST_SYS_WARNING(...) GST_SYS_CAT_LEVEL_LOG (GST_CAT_DEFAULT, GST_LEVEL_WARNING, NULL, ##__VA_ARGS__)
#define GST_SYS_ERROR(...) GST_SYS_CAT_LEVEL_LOG (GST_CAT_DEFAULT, GST_LEVEL_ERROR, NULL, ##__VA_ARGS__)

#define GST_SYS_CAT_LOG(cat,...) GST_SYS_CAT_LEVEL_LOG (cat, GST_LEVEL_LOG, NULL, ##__VA_ARGS__)
#define GST_SYS_CAT_DEBUG(cat,...) GST_SYS_CAT_LEVEL_LOG (cat, GST_LEVEL_DEBUG, NULL, ##__VA_ARGS__)
#define GST_SYS_CAT_INFO(cat,...) GST_SYS_CAT_LEVEL_LOG (cat, GST_LEVEL_INFO, NULL, ##__VA_ARGS__)
#define GST_SYS_CAT_FIXME(cat,...) GST_SYS_CAT_LEVEL_LOG (cat, GST_LEVEL_FIXME, NULL, ##__VA_ARGS__)
#define GST_SYS_CAT_WARNING(cat,...) GST_SYS_CAT_LEVEL_LOG (cat, GST_LEVEL_WARNING, NULL, ##__VA_ARGS__)
#define GST_SYS_CAT_ERROR(cat,...) GST_SYS_CAT_LEVEL_LOG (cat, GST_LEVEL_ERROR, NULL, ##__VA_ARGS__)

#define GST_SYS_CAT_LOG_OBJECT(cat,obj,...) GST_SYS_CAT_LEVEL_LOG (cat, GST_LEVEL_LOG, obj, ##__VA_ARGS__)
#define GST_SYS_CAT_DEBUG_OBJECT(cat,obj,...) GST_SYS_CAT_LEVEL_LOG (cat, GST_LEVEL_DEBUG, obj, ##__VA_ARGS__)
#define GST_SYS_CAT_INFO_OBJECT(cat,obj,...) GST_SYS_CAT_LEVEL_LOG (cat, GST_LEVEL_INFO, obj, ##__VA_ARGS__)
#define GST_SYS_CAT_FIXME_OBJECT(cat,obj,...) GST_SYS_CAT_LEVEL_LOG (cat, GST_LEVEL_FIXME, obj, ##__VA_ARGS__)
#define GST_SYS_CAT_WARNING_OBJECT(cat,obj,...) GST_SYS_CAT_LEVEL_LOG (cat, GST_LEVEL_WARNING, ##__VA_ARGS__)
#define GST_SYS_CAT_ERROR_OBJECT(cat,obj,...) GST_SYS_CAT_LEVEL_LOG (cat, GST_LEVEL_ERROR, obj, ##__VA_ARGS__)


GST_API
void
gst_sys_debug_log (GstDebugCategory * category, GstDebugLevel level,
    const gchar * file, const gchar * function, gint line,
    GObject * object, const gchar * format, ...);

#endif /* __GSTSYSDEBUG_H_ */
