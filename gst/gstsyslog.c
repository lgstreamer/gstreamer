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

#include <unistd.h>
#include <string.h>
#include <syslog.h>
#include <stdarg.h>
#include <gst_private.h>
#include "gstsyslog.h"

struct _GstDebugMessage
{
  gchar *message;
  const gchar *format;
  va_list arguments;
};

void
gst_sys_debug_log (GstDebugCategory * category, GstDebugLevel level,
    const gchar * file, const gchar * function, gint line,
    GObject * object, const gchar * format, ...)
{
#ifdef DUMP_SYSLOG
  gint pid;
  GstDebugMessage message;
  const gchar *file_name;
  gchar *obj = NULL;
  gchar **split = NULL;
  gint split_length;

  g_return_if_fail (category != NULL);
  g_return_if_fail (file != NULL);
  g_return_if_fail (function != NULL);
  g_return_if_fail (format != NULL);

  va_start (message.arguments, format);
  pid = getpid ();
  if (object)
    obj = gst_object_get_name ((GstObject *) object);
  else
    obj = (gchar *) "";

  if (file != NULL) {
    split = g_strsplit (file, "/", -1);
    split_length = g_strv_length (split);
    file_name = split[split_length - 1];
  } else {
    file_name = "<Unknown Filename>";
  }

  message.message = NULL;
  message.format = format;
  syslog (LOG_MAKEPRI (LOG_USER, LOG_INFO), "%d %p %s %s:%d:%s:<%s> %s", pid,
      g_thread_self (), gst_debug_level_get_name (level), file_name, line,
      function, obj, gst_debug_message_get (&message));
  if (split)
    g_strfreev (split);

  if (object != NULL)
    g_free (obj);
  g_free (message.message);
  va_end (message.arguments);
#endif

  if (G_UNLIKELY (level <= _gst_debug_min)) {
    va_list var_args;
    va_start (var_args, format);
    gst_debug_log_valist (category, level, file, function, line, object, format,
        var_args);
    va_end (var_args);
  }
}
