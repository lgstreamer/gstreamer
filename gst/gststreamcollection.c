/* GStreamer
 *
 * Copyright (C) 2015 Centricular Ltd
 *  @author: Edward Hervey <edward@centricular.com>
 *  @author: Jan Schmidt <jan@centricular.com>
 *
 * gststreams.c: GstStreamCollection object and methods
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
 *
 * MT safe.
 */

/**
 * SECTION:gststreamcollection
 * @title: GstStreamCollection
 * @short_description: Base class for collection of streams
 *
 * Since: 1.10
 */

#include "gst_private.h"

#include "gstenumtypes.h"
#include "gstevent.h"
#include "gststreamcollection.h"

GST_DEBUG_CATEGORY_STATIC (stream_collection_debug);
#define GST_CAT_DEFAULT stream_collection_debug

typedef struct _CollectionStream
{
  GstStream *stream;

  /* Cached stream id for fast usage */
  const gchar *stream_id;

  /* Variants */
  GList *variants;

  /* Whether the stream has (or had) variant streams */
  gboolean has_variant;
} CollectionStream;

struct _GstStreamCollectionPrivate
{
  /* Maybe switch this to a GArray if performance is
   * ever an issue? */
  GQueue streams;
};

/* stream signals and properties */
enum
{
  SIG_STREAM_NOTIFY,
  LAST_SIGNAL
};

enum
{
  PROP_0,
  PROP_UPSTREAM_ID,
  PROP_LAST
};

static guint gst_stream_collection_signals[LAST_SIGNAL] = { 0 };

static void gst_stream_collection_dispose (GObject * object);

static void gst_stream_collection_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec);
static void gst_stream_collection_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec);

static void
proxy_stream_notify_cb (GstStream * stream, GParamSpec * pspec,
    GstStreamCollection * collection);

#define _do_init				\
{ \
  GST_DEBUG_CATEGORY_INIT (stream_collection_debug, "streamcollection", GST_DEBUG_BOLD, \
      "debugging info for the stream collection objects"); \
  \
}

#define gst_stream_collection_parent_class parent_class
G_DEFINE_TYPE_WITH_CODE (GstStreamCollection, gst_stream_collection,
    GST_TYPE_OBJECT, G_ADD_PRIVATE (GstStreamCollection) _do_init);

static void
gst_stream_collection_class_init (GstStreamCollectionClass * klass)
{
  GObjectClass *gobject_class;

  gobject_class = (GObjectClass *) klass;

  gobject_class->set_property = gst_stream_collection_set_property;
  gobject_class->get_property = gst_stream_collection_get_property;

  /**
   * GstStream:upstream-id:
   *
   * stream-id
   */
  g_object_class_install_property (gobject_class, PROP_UPSTREAM_ID,
      g_param_spec_string ("upstream-id", "Upstream ID",
          "The stream ID of the parent stream",
          NULL,
          G_PARAM_READWRITE | G_PARAM_CONSTRUCT | G_PARAM_STATIC_STRINGS));

  /**
   * GstStream::stream-notify:
   * @collection: a #GstStreamCollection
   * @prop_stream: the #GstStream that originated the signal
   * @prop: the property that changed
   *
   * The stream notify signal is used to be notified of property changes to
   * streams within the collection.
   */
  gst_stream_collection_signals[SIG_STREAM_NOTIFY] =
      g_signal_new ("stream-notify", G_TYPE_FROM_CLASS (klass),
      G_SIGNAL_RUN_FIRST | G_SIGNAL_NO_RECURSE | G_SIGNAL_DETAILED |
      G_SIGNAL_NO_HOOKS, G_STRUCT_OFFSET (GstStreamCollectionClass,
          stream_notify), NULL, NULL, g_cclosure_marshal_generic, G_TYPE_NONE,
      2, GST_TYPE_STREAM, G_TYPE_PARAM);

  gobject_class->dispose = gst_stream_collection_dispose;
}

static void
gst_stream_collection_init (GstStreamCollection * collection)
{
  collection->priv = gst_stream_collection_get_instance_private (collection);
  g_queue_init (&collection->priv->streams);
}

static void
release_gst_stream (CollectionStream * cstream,
    GstStreamCollection * collection)
{
  g_signal_handlers_disconnect_by_func (cstream->stream,
      proxy_stream_notify_cb, collection);
  gst_object_unref (cstream->stream);
  g_list_free_full (cstream->variants, (GDestroyNotify) gst_object_unref);

  g_slice_free (CollectionStream, cstream);
}

static void
gst_stream_collection_dispose (GObject * object)
{
  GstStreamCollection *collection = GST_STREAM_COLLECTION_CAST (object);

  if (collection->upstream_id) {
    g_free (collection->upstream_id);
    collection->upstream_id = NULL;
  }

  g_queue_foreach (&collection->priv->streams,
      (GFunc) release_gst_stream, collection);
  g_queue_clear (&collection->priv->streams);

  G_OBJECT_CLASS (parent_class)->dispose (object);
}

/**
 * gst_stream_collection_new:
 * @upstream_id: (allow-none): The stream id of the parent stream
 *
 * Create a new #GstStreamCollection.
 *
 * Returns: (transfer full): The new #GstStreamCollection.
 *
 * Since: 1.10
 */
GstStreamCollection *
gst_stream_collection_new (const gchar * upstream_id)
{
  GstStreamCollection *collection;

  collection =
      g_object_new (GST_TYPE_STREAM_COLLECTION, "upstream-id", upstream_id,
      NULL);

  /* Clear floating flag */
  g_object_ref_sink (collection);

  return collection;
}

static void
gst_stream_collection_set_upstream_id (GstStreamCollection * collection,
    const gchar * upstream_id)
{
  g_return_if_fail (collection->upstream_id == NULL);

  /* Upstream ID should only be set once on construction, but let's
   * not leak in case someone does something silly */
  if (collection->upstream_id)
    g_free (collection->upstream_id);

  if (upstream_id)
    collection->upstream_id = g_strdup (upstream_id);
}

/**
 * gst_stream_collection_get_upstream_id:
 * @collection: a #GstStreamCollection
 *
 * Returns the upstream id of the @collection.
 *
 * Returns: (transfer none): The upstream id
 *
 * Since: 1.10
 */
const gchar *
gst_stream_collection_get_upstream_id (GstStreamCollection * collection)
{
  const gchar *res;

  res = collection->upstream_id;

  return res;
}

static void
gst_stream_collection_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec)
{
  GstStreamCollection *collection;

  collection = GST_STREAM_COLLECTION_CAST (object);

  switch (prop_id) {
    case PROP_UPSTREAM_ID:
      gst_stream_collection_set_upstream_id (collection,
          g_value_get_string (value));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void
gst_stream_collection_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec)
{
  GstStreamCollection *collection;

  collection = GST_STREAM_COLLECTION_CAST (object);

  switch (prop_id) {
    case PROP_UPSTREAM_ID:
      g_value_set_string (value,
          gst_stream_collection_get_upstream_id (collection));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void
proxy_stream_notify_cb (GstStream * stream, GParamSpec * pspec,
    GstStreamCollection * collection)
{
  GST_DEBUG_OBJECT (collection, "Stream %" GST_PTR_FORMAT " updated %s",
      stream, pspec->name);
  g_signal_emit (collection, gst_stream_collection_signals[SIG_STREAM_NOTIFY],
      g_quark_from_string (pspec->name), stream, pspec);
}

/**
 * gst_stream_collection_add_stream:
 * @collection: a #GstStreamCollection
 * @stream: (transfer full): the #GstStream to add
 *
 * Add the given @stream to the @collection.
 *
 * Returns: %TRUE if the @stream was properly added, else %FALSE
 *
 * Since: 1.10
 */
gboolean
gst_stream_collection_add_stream (GstStreamCollection * collection,
    GstStream * stream)
{
  CollectionStream *cstream;

  g_return_val_if_fail (GST_IS_STREAM_COLLECTION (collection), FALSE);
  g_return_val_if_fail (GST_IS_STREAM (stream), FALSE);

  GST_DEBUG_OBJECT (collection, "Adding stream %" GST_PTR_FORMAT, stream);

  cstream = g_slice_new0 (CollectionStream);
  cstream->stream = stream;
  cstream->stream_id = gst_stream_get_stream_id (stream);
  cstream->has_variant = FALSE;

  g_queue_push_tail (&collection->priv->streams, cstream);
  g_signal_connect (stream, "notify", (GCallback) proxy_stream_notify_cb,
      collection);

  return TRUE;
}

/**
 * gst_stream_collection_get_size:
 * @collection: a #GstStreamCollection
 *
 * Get the number of streams this collection contains
 *
 * Returns: The number of streams that @collection contains
 *
 * Since: 1.10
 */
guint
gst_stream_collection_get_size (GstStreamCollection * collection)
{
  g_return_val_if_fail (GST_IS_STREAM_COLLECTION (collection), 0);

  return g_queue_get_length (&collection->priv->streams);
}

/**
 * gst_stream_collection_get_stream:
 * @collection: a #GstStreamCollection
 * @index: Index of the stream to retrieve
 *
 * Retrieve the #GstStream with index @index from the collection.
 *
 * The caller should not modify the returned #GstStream
 *
 * Returns: (transfer none) (nullable): A #GstStream
 *
 * Since: 1.10
 */
GstStream *
gst_stream_collection_get_stream (GstStreamCollection * collection, guint index)
{
  CollectionStream *cstream;

  g_return_val_if_fail (GST_IS_STREAM_COLLECTION (collection), NULL);

  cstream = g_queue_peek_nth (&collection->priv->streams, index);
  if (cstream)
    return cstream->stream;
  return NULL;
}

/* Internal function */
static gint
_compare_cstream (CollectionStream * cs1, const gchar * stream_id)
{
  return g_strcmp0 (cs1->stream_id, stream_id);
}

/**
 * gst_stream_collection_add_variant:
 * @collection: a #GstStreamCollection
 * @stream_id: the stream to which we want to add a variant
 * @variant_stream: (transfer full): the variant #GstStream for @stream_id
 *
 * Adds the given @variant_stream as a variant of the end user
 * stream @stream_id.
 *
 * Returns: %TRUE if the @variant_stream was properly added, else
 * %FALSE.
 *
 * Since: 1.16
 */
gboolean
gst_stream_collection_add_variant (GstStreamCollection * collection,
    const gchar * stream_id, GstStream * variant_stream)
{
  GList *cslist;
  CollectionStream *cstream;

  g_return_val_if_fail (GST_IS_STREAM_COLLECTION (collection), FALSE);
  g_return_val_if_fail (stream_id != NULL, FALSE);
  g_return_val_if_fail (variant_stream != NULL, FALSE);

  /* Check if we have a stream for stream_id */
  cslist =
      g_queue_find_custom (&collection->priv->streams, stream_id,
      (GCompareFunc) _compare_cstream);
  if (!cslist) {
    GST_ERROR ("The collection doesn't contain the stream '%s'", stream_id);
    return FALSE;
  }

  cstream = (CollectionStream *) cslist->data;

  if (gst_stream_get_stream_type (cstream->stream) !=
      gst_stream_get_stream_type (variant_stream)) {
    GST_WARNING ("variant isn't of the same type as the parent");
    return FALSE;
  }

  /* If we do, store variant_stream */
  cstream->variants = g_list_append (cstream->variants, variant_stream);
  cstream->has_variant = TRUE;

  return TRUE;
}

/**
 * gst_stream_collection_is_variant_for:
 * @collection: a #GstStreamCollection
 * @candidate: (transfer none): a candidate variant #GstStream
 * @stream_id: The stream-id of the end-user stream
 *
 * Checks if @candidate is a variant of the end-user stream specified
 * with the given @stream_id:
 *
 * Returns: %TRUE if the the candidate stream is a variant of the
 * stream 'stream_id'
 *
 * Since: 1.16
 */
gboolean
gst_stream_collection_is_variant_for (GstStreamCollection * collection,
    const GstStream * candidate, const gchar * stream_id)
{
  CollectionStream *cstream;
  GList *tmp, *cslist;

  g_return_val_if_fail (GST_IS_STREAM_COLLECTION (collection), FALSE);
  g_return_val_if_fail (candidate != NULL, FALSE);
  g_return_val_if_fail (stream_id != NULL, FALSE);

  /* Check if we have a stream for stream_id */
  cslist =
      g_queue_find_custom (&collection->priv->streams, stream_id,
      (GCompareFunc) _compare_cstream);

  if (!cslist)
    return FALSE;
  cstream = (CollectionStream *) cslist->data;

  if (!cstream->variants)
    return FALSE;

  for (tmp = cstream->variants; tmp; tmp = tmp->next) {
    if (tmp->data == candidate)
      return TRUE;
  }

  return FALSE;
}

/**
 * gst_stream_collection_get_variants:
 * @collection: a #GstStreamCollection
 * @stream_id: The stream_id of the end-user stream
 *
 * Returns the list of variant stream-id for a given @stream_id.
 *
 * Returns: (transfer none) (element-type GstStream): The list of
 * variants, or %NULL if there are no variants.
 *
 * Since: 1.16
 */
GList *
gst_stream_collection_get_variants (GstStreamCollection * collection,
    const gchar * stream_id)
{
  CollectionStream *cstream;
  GList *cslist;

  g_return_val_if_fail (GST_IS_STREAM_COLLECTION (collection), NULL);
  g_return_val_if_fail (stream_id != NULL, NULL);

  /* Check if we have a stream for stream_id */
  cslist =
      g_queue_find_custom (&collection->priv->streams, stream_id,
      (GCompareFunc) _compare_cstream);

  if (!cslist)
    return FALSE;
  cstream = (CollectionStream *) cslist->data;

  return cstream->variants;
}

static gint
_compare_variant_id (CollectionStream * cs, const gchar * stream_id)
{
  GList *tmp;

  /* Go over all variants and check whether the stream_id matches */
  if (!cs->variants)
    return -1;

  for (tmp = cs->variants; tmp; tmp = tmp->next) {
    const gchar *sid = gst_stream_get_stream_id ((GstStream *) tmp->data);
    if (!g_strcmp0 (sid, stream_id))
      return 0;
  }

  return -1;
}

/**
 * gst_stream_collection_get_variant_of:
 * @collection: a #GstStreamCollection
 * @stream_id: the stream_id of a variant #GstStream
 *
 * Returns the stream-id of which this the specific stream is a
 * variant of. If the provided stream-id is not a variant stream,
 * NULL is returned.
 *
 * Returns: The end-user stream-id for which the @stream_id is a variant
 * of, else %NULL.
 *
 * Since: 1.16
 */
const gchar *
gst_stream_collection_get_variant_of (GstStreamCollection * collection,
    const gchar * stream_id)
{
  CollectionStream *cstream;
  GList *cslist;

  g_return_val_if_fail (GST_IS_STREAM_COLLECTION (collection), NULL);
  g_return_val_if_fail (stream_id != NULL, NULL);

  /* Check if we have a stream with a variant matching stream_id */
  cslist =
      g_queue_find_custom (&collection->priv->streams, stream_id,
      (GCompareFunc) _compare_variant_id);

  if (!cslist)
    return FALSE;

  cstream = (CollectionStream *) cslist->data;

  return cstream->stream_id;
}
