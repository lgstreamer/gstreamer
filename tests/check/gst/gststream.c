/* GStreamer
 * Copyright (C) <2015> Edward Hervey <edward@centricular.com>
 *
 * gststructure.c: Unit tests for GstStream and GstStreamCollection
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
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <gst/gst.h>
#include <gst/check/gstcheck.h>

static GstStream *
new_stream (const gchar * stream_id, const gchar * caps_str,
    GstStreamType stype, GstStreamFlags sflags)
{
  GstCaps *caps = gst_caps_from_string (caps_str);
  GstStream *stream = gst_stream_new (stream_id, caps, stype, sflags);
  gst_caps_unref (caps);
  return stream;
}

GST_START_TEST (test_stream_creation)
{
  GstStream *stream;
  GstCaps *caps;
  GstCaps *caps2;
  GstTagList *tags, *tags2;

  caps = gst_caps_from_string ("some/caps");
  stream = gst_stream_new ("stream-id", caps, GST_STREAM_TYPE_AUDIO, 0);
  fail_unless (stream != NULL);

  fail_unless_equals_string (gst_stream_get_stream_id (stream), "stream-id");
  caps2 = gst_stream_get_caps (stream);
  fail_unless (gst_caps_is_equal (caps, caps2));
  gst_caps_unref (caps2);

  fail_unless (gst_stream_get_stream_type (stream) == GST_STREAM_TYPE_AUDIO);

  gst_caps_unref (caps);

  tags = gst_tag_list_new (GST_TAG_ALBUM, "test-album", NULL);
  g_object_set (stream, "tags", tags, NULL);
  tags2 = gst_stream_get_tags (stream);
  fail_unless (gst_tag_list_is_equal (tags, tags2));
  gst_tag_list_unref (tags);
  gst_tag_list_unref (tags2);

  gst_object_unref (stream);
}

GST_END_TEST;

GST_START_TEST (test_stream_event)
{
  GstEvent *event;
  GstStream *stream, *stream2 = NULL;
  GstCaps *caps;
  GstCaps *caps2;

  event = gst_event_new_stream_start ("here/we/go");
  /* By default a stream-start event has no stream */
  gst_event_parse_stream (event, &stream2);
  fail_if (stream2 != NULL);

  /* Create and set stream on event */
  caps = gst_caps_from_string ("some/caps");
  stream = gst_stream_new ("here/we/go", caps, GST_STREAM_TYPE_AUDIO, 0);
  fail_unless (stream != NULL);
  gst_event_set_stream (event, stream);

  /* Parse and check it's the same */
  gst_event_parse_stream (event, &stream2);
  fail_unless (stream2 != NULL);
  fail_unless_equals_string (gst_stream_get_stream_id (stream2), "here/we/go");
  caps2 = gst_stream_get_caps (stream);
  fail_unless (gst_caps_is_equal (caps, caps2));
  fail_unless (gst_stream_get_stream_type (stream) == GST_STREAM_TYPE_AUDIO);
  gst_caps_unref (caps2);

  gst_event_unref (event);
  gst_caps_unref (caps);
  gst_object_unref (stream);
  gst_object_unref (stream2);
}

GST_END_TEST;

GST_START_TEST (test_stream_components)
{
  GstStream *stream;
  GstStream *c1, *c2, *c3;
  const gchar *stream_id;

  /* Create a stream 'shvc' made of 2 components 'base' and 'layer' */
  stream =
      new_stream ("shvc", "video/x-shvc", GST_STREAM_TYPE_VIDEO,
      GST_STREAM_FLAG_NONE);
  fail_unless (stream != NULL);

  /* At this point there should be no components */
  fail_unless_equals_int (gst_stream_get_components_size (stream), 0);

  /* Create and add the components */
  c1 = new_stream ("base", "video/x-h265", GST_STREAM_TYPE_VIDEO,
      GST_STREAM_FLAG_NONE);
  fail_unless (c1);

  c2 = new_stream ("layer", "video/x-lhvc", GST_STREAM_TYPE_VIDEO,
      GST_STREAM_FLAG_NONE);
  fail_unless (c2);

  /* Add the components and check size */
  gst_stream_add_component (stream, c1);
  fail_unless_equals_int (gst_stream_get_components_size (stream), 1);

  gst_stream_add_component (stream, c2);
  fail_unless_equals_int (gst_stream_get_components_size (stream), 2);

  /* Check behaviour of gst_stream_get_components_idx */
  c3 = gst_stream_get_component_idx (stream, 0);
  stream_id = gst_stream_get_stream_id (c3);
  fail_if (g_strcmp0 (stream_id, "base") && g_strcmp0 (stream_id, "layer"));

  c3 = gst_stream_get_component_idx (stream, 1);
  stream_id = gst_stream_get_stream_id (c3);
  fail_if (g_strcmp0 (stream_id, "base") && g_strcmp0 (stream_id, "layer"));

  fail_if (gst_stream_get_component_idx (stream, 3));

  /* Check behaviour of gst_stream_has_component_by_name */
  fail_unless (gst_stream_has_component_by_name (stream, "base"));
  fail_unless (gst_stream_has_component_by_name (stream, "layer"));
  fail_if (gst_stream_has_component_by_name (stream, "nope"));
  fail_if (gst_stream_has_component_by_name (stream, "shvc"));

  /* Check behaviour of gst_stream_has_component */
  fail_unless (gst_stream_has_component (stream, c1));
  fail_unless (gst_stream_has_component (stream, c2));
  c3 = gst_stream_new ("nope", NULL, GST_STREAM_TYPE_VIDEO,
      GST_STREAM_FLAG_NONE);
  fail_if (gst_stream_has_component (stream, c3));
  gst_object_unref (c3);

  /* Cleanup */
  gst_object_unref (stream);
  gst_object_unref (c1);
  gst_object_unref (c2);
}

GST_END_TEST;

GST_START_TEST (test_collection_simple)
{
  GstStreamCollection *collection;
  GstStream *stream1, *stream2, *stream;
  GstCaps *caps;

  /* Simple collection with two end-user streams */

  collection = gst_stream_collection_new ("upstream-id");
  fail_unless (collection != NULL);
  fail_unless_equals_int (gst_stream_collection_get_size (collection), 0);
  fail_unless_equals_string (gst_stream_collection_get_upstream_id (collection),
      "upstream-id");

  /* Create streams and add them to the collection */
  caps = gst_caps_from_string ("video/x-stream1");
  stream1 =
      gst_stream_new ("stream1", caps, GST_STREAM_TYPE_VIDEO,
      GST_STREAM_FLAG_NONE);
  gst_caps_unref (caps);
  fail_unless (gst_stream_collection_add_stream (collection, stream1));
  fail_unless_equals_int (gst_stream_collection_get_size (collection), 1);

  caps = gst_caps_from_string ("video/x-stream2");
  stream2 =
      gst_stream_new ("stream2", caps, GST_STREAM_TYPE_AUDIO,
      GST_STREAM_FLAG_NONE);
  gst_caps_unref (caps);
  fail_unless (gst_stream_collection_add_stream (collection, stream2));
  fail_unless_equals_int (gst_stream_collection_get_size (collection), 2);

  /* Collections are ordered */
  stream = gst_stream_collection_get_stream (collection, 0);
  fail_unless (stream == stream1);

  stream = gst_stream_collection_get_stream (collection, 1);
  fail_unless (stream == stream2);

  /* cleanup */
  gst_object_unref (collection);
}

GST_END_TEST;

GST_START_TEST (test_collection_variants)
{
  GstStreamCollection *collection;
  GstStream *astream1, *astream2, *vstream, *vstream1, *vstream2;

  /* Collection with:
   * * two end-user audio streams,
   * * one end-user video stream with 2 variants
   */

  collection = gst_stream_collection_new ("upstream-id");
  fail_unless (collection != NULL);
  fail_unless_equals_int (gst_stream_collection_get_size (collection), 0);
  fail_unless_equals_string (gst_stream_collection_get_upstream_id (collection),
      "upstream-id");

  /* Create audio streams and add them to the collection */
  astream1 =
      new_stream ("astream1", "audio/x-stream1", GST_STREAM_TYPE_AUDIO,
      GST_STREAM_FLAG_NONE);
  fail_unless (gst_stream_collection_add_stream (collection, astream1));
  fail_unless_equals_int (gst_stream_collection_get_size (collection), 1);

  astream2 =
      new_stream ("astream2", "audio/x-stream2", GST_STREAM_TYPE_AUDIO,
      GST_STREAM_FLAG_NONE);
  fail_unless (gst_stream_collection_add_stream (collection, astream2));
  fail_unless_equals_int (gst_stream_collection_get_size (collection), 2);

  /* Create video stream (which has no caps) */
  vstream =
      gst_stream_new ("vstream", NULL, GST_STREAM_TYPE_VIDEO,
      GST_STREAM_FLAG_NONE);
  fail_unless (gst_stream_collection_add_stream (collection, vstream));
  fail_unless_equals_int (gst_stream_collection_get_size (collection), 3);

  /* Create variant video streams and add them */
  vstream1 =
      new_stream ("vstream1", "video/x-stream1", GST_STREAM_TYPE_VIDEO,
      GST_STREAM_FLAG_NONE);
  fail_unless (gst_stream_collection_add_variant (collection, "vstream",
          vstream1));
  /* Number of end-user streams hasn't changed */
  fail_unless_equals_int (gst_stream_collection_get_size (collection), 3);

  vstream2 =
      new_stream ("vstream2", "video/x-stream2", GST_STREAM_TYPE_VIDEO,
      GST_STREAM_FLAG_NONE);
  fail_unless (gst_stream_collection_add_variant (collection, "vstream",
          vstream2));
  /* Number of end-user streams hasn't changed */
  fail_unless_equals_int (gst_stream_collection_get_size (collection), 3);

  /* Adding a variant for a stream that doesn't exist should fail */
  fail_if (gst_stream_collection_add_variant (collection, "doesn'texist",
          vstream1));

  /* Check all end-user streams are present and correct */
  fail_unless (gst_stream_collection_get_stream (collection, 0) == astream1);
  fail_unless (gst_stream_collection_get_stream (collection, 1) == astream2);
  fail_unless (gst_stream_collection_get_stream (collection, 2) == vstream);

  /* Check presence and validity of variants */
  fail_unless (gst_stream_collection_is_variant_for (collection, vstream1,
          "vstream"));
  fail_unless (gst_stream_collection_is_variant_for (collection, vstream2,
          "vstream"));
  fail_if (gst_stream_collection_is_variant_for (collection, astream1,
          "vstream"));

  fail_unless_equals_string (gst_stream_collection_get_variant_of (collection,
          "vstream1"), "vstream");

  /* cleanup */
  gst_object_unref (collection);
}

GST_END_TEST;

struct NotifyStats
{
  guint collection_notify;
  guint collection_notify_caps;
  guint collection_notify_tags;
  guint collection_notify_type;
  guint collection_notify_flags;

  guint stream_notify;
  guint stream_notify_caps;
  guint stream_notify_tags;
  guint stream_notify_type;
  guint stream_notify_flags;

  guint stream2_notify;
  guint stream2_notify_caps;
  guint stream2_notify_tags;
  guint stream2_notify_type;
  guint stream2_notify_flags;
};

static void
stream_notify_cb (GstStreamCollection * collection, GstStream * stream,
    GParamSpec * pspec, guint * val)
{
  GST_LOG ("Got stream-notify from %" GST_PTR_FORMAT " for %s from %"
      GST_PTR_FORMAT, stream, pspec->name, collection);
  (*val)++;
}

static void
notify_cb (GstStream * stream, GParamSpec * pspec, guint * val)
{
  GST_LOG ("Got notify from %" GST_PTR_FORMAT " for %s", stream, pspec->name);
  (*val)++;
}

GST_START_TEST (test_notifies)
{
  GstStreamCollection *collection;
  GstStream *stream, *stream2 = NULL;
  GstCaps *caps;
  struct NotifyStats stats = { 0, };
  GstTagList *tags;

  collection = gst_stream_collection_new ("check-collection");
  g_signal_connect (collection, "stream-notify", (GCallback) stream_notify_cb,
      &stats.collection_notify);
  g_signal_connect (collection, "stream-notify::stream-type",
      (GCallback) stream_notify_cb, &stats.collection_notify_type);
  g_signal_connect (collection, "stream-notify::stream-flags",
      (GCallback) stream_notify_cb, &stats.collection_notify_flags);
  g_signal_connect (collection, "stream-notify::caps",
      (GCallback) stream_notify_cb, &stats.collection_notify_caps);
  g_signal_connect (collection, "stream-notify::tags",
      (GCallback) stream_notify_cb, &stats.collection_notify_tags);

  caps = gst_caps_from_string ("some/audio-caps");
  stream = gst_stream_new ("here/we/go", caps, GST_STREAM_TYPE_AUDIO, 0);
  gst_caps_unref (caps);
  g_signal_connect (stream, "notify", (GCallback) notify_cb,
      &stats.stream_notify);
  g_signal_connect (stream, "notify::stream-type", (GCallback) notify_cb,
      &stats.stream_notify_type);
  g_signal_connect (stream, "notify::stream-flags", (GCallback) notify_cb,
      &stats.stream_notify_flags);
  g_signal_connect (stream, "notify::caps", (GCallback) notify_cb,
      &stats.stream_notify_caps);
  g_signal_connect (stream, "notify::tags", (GCallback) notify_cb,
      &stats.stream_notify_tags);
  gst_stream_collection_add_stream (collection, stream);

  caps = gst_caps_from_string ("some/video-caps");
  stream2 = gst_stream_new ("here/we/go/again", caps, GST_STREAM_TYPE_VIDEO, 0);
  gst_caps_unref (caps);
  g_signal_connect (stream2, "notify", (GCallback) notify_cb,
      &stats.stream2_notify);
  g_signal_connect (stream2, "notify::stream-type", (GCallback) notify_cb,
      &stats.stream2_notify_type);
  g_signal_connect (stream2, "notify::stream-flags", (GCallback) notify_cb,
      &stats.stream2_notify_flags);
  g_signal_connect (stream2, "notify::caps", (GCallback) notify_cb,
      &stats.stream2_notify_caps);
  g_signal_connect (stream2, "notify::tags", (GCallback) notify_cb,
      &stats.stream2_notify_tags);
  gst_stream_collection_add_stream (collection, stream2);

  caps = gst_caps_from_string ("some/new-video-caps");
  gst_stream_set_caps (stream2, caps);
  gst_caps_unref (caps);

  fail_unless (stats.collection_notify == 1);
  fail_unless (stats.collection_notify_caps == 1);
  fail_unless (stats.stream_notify == 0);
  fail_unless (stats.stream_notify_caps == 0);
  fail_unless (stats.stream_notify_tags == 0);
  fail_unless (stats.stream2_notify == 1);
  fail_unless (stats.stream2_notify_caps == 1);
  fail_unless (stats.stream2_notify_tags == 0);

  tags = gst_tag_list_new (GST_TAG_ALBUM, "test-album", NULL);
  gst_stream_set_tags (stream, tags);
  gst_tag_list_unref (tags);

  fail_unless (stats.collection_notify == 2);
  fail_unless (stats.collection_notify_caps == 1);
  fail_unless (stats.collection_notify_tags == 1);
  fail_unless (stats.stream_notify == 1);
  fail_unless (stats.stream_notify_caps == 0);
  fail_unless (stats.stream_notify_tags == 1);
  fail_unless (stats.stream2_notify == 1);
  fail_unless (stats.stream2_notify_caps == 1);
  fail_unless (stats.stream2_notify_tags == 0);

  gst_object_unref (collection);
}

GST_END_TEST;

static Suite *
gst_streams_suite (void)
{
  Suite *s = suite_create ("GstStream");
  TCase *tc_chain = tcase_create ("general");

  suite_add_tcase (s, tc_chain);
  tcase_add_test (tc_chain, test_stream_creation);
  tcase_add_test (tc_chain, test_stream_event);
  tcase_add_test (tc_chain, test_stream_components);
  tcase_add_test (tc_chain, test_collection_simple);
  tcase_add_test (tc_chain, test_collection_variants);
  tcase_add_test (tc_chain, test_notifies);
  return s;
}

GST_CHECK_MAIN (gst_streams);
