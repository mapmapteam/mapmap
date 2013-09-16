#include <mapmap/version.h>
#include <clutter-gst/clutter-gst.h>

typedef struct _Data
{
  ClutterActor *stage;
  ClutterActor *actor;
  GstElement *pipeline0;
  GstElement *videotestsrc0;
  GstElement *capsfilter0;
  GstElement *videoconvert0;
  GstElement *cluttersink0;
  ClutterTimeline *timeline;
} Data;

static const ClutterColor STAGE_COLOR = { 0, 0, 0, 255 };
static const gint DEFAULT_WIDTH = 1280;
static const gint DEFAULT_HEIGHT = 720;

static gboolean
change_videotest_pattern (gpointer user_data)
{
  Data *data = (Data *) user_data;
  gint current_pattern;
  gint new_pattern;

  /* Let's disable this for now */
  return FALSE;

  g_object_get (data->videotestsrc0,
    "pattern", &current_pattern,
    NULL);

  new_pattern = (current_pattern + 1) % 2;

  g_message ("Set video test pattern to %d", new_pattern);

  g_object_set (data->videotestsrc0,
    "pattern", new_pattern,
    NULL);
  return TRUE;
}

static void
mm_create_stage (Data *data)
{
  gchar * caps_str;
  GstCaps *size_caps;

  g_assert (data->stage == NULL);

  data->stage = clutter_stage_new ();
  clutter_actor_set_background_color (data->stage, &STAGE_COLOR);
  clutter_actor_set_size (data->stage, DEFAULT_WIDTH, DEFAULT_HEIGHT);
  clutter_stage_set_minimum_size (CLUTTER_STAGE (data->stage), DEFAULT_WIDTH, DEFAULT_HEIGHT);

  data->pipeline0 = gst_pipeline_new ("pipeline0");
  data->videotestsrc0 = gst_element_factory_make ("videotestsrc", "videotestsrc0");
  data->capsfilter0 = gst_element_factory_make ("capsfilter", "capsfilter0");
  data->videoconvert0 = gst_element_factory_make ("videoconvert", "videoconvert0");
  data->cluttersink0 = gst_element_factory_make ("cluttersink", "cluttersink0");
  gst_bin_add_many (GST_BIN (data->pipeline0),
    data->videotestsrc0,
    data->capsfilter0,
    data->videoconvert0,
    data->cluttersink0,
    NULL);

  g_assert (data->videotestsrc0);
  g_assert (data->capsfilter0);
  g_assert (data->videoconvert0);
  g_assert (data->cluttersink0);

  g_assert (gst_element_link_many (
    data->videotestsrc0,
    data->capsfilter0,
    data->videoconvert0,
    data->cluttersink0,
    NULL));

  /* set size caps */
  caps_str = g_strdup_printf (
    "video/x-raw,width=%d,height=%d,pixel-aspect-ratio=1/1",
    DEFAULT_WIDTH, DEFAULT_HEIGHT);
  size_caps = gst_caps_from_string (caps_str);
  g_free (caps_str);
  g_object_set (data->capsfilter0,
    "caps", size_caps,
    NULL);
  gst_caps_unref (size_caps);

  /* create clutter texture */
  data->actor = CLUTTER_ACTOR (clutter_texture_new ());
  clutter_actor_set_size (data->actor, DEFAULT_WIDTH, DEFAULT_HEIGHT);
  g_object_set (G_OBJECT (data->cluttersink0),
    "texture", data->actor,
    NULL);
  clutter_actor_add_child (CLUTTER_ACTOR (data->stage), data->actor);

  g_signal_connect (data->stage,
    "destroy",
    G_CALLBACK (clutter_main_quit),
    NULL);

  data->timeline = clutter_timeline_new (1000);
  g_object_set (data->timeline,
    "loop", TRUE,
    NULL);
  clutter_timeline_start (data->timeline);

  clutter_actor_show (data->stage);
  gst_element_set_state (data->pipeline0, GST_STATE_PLAYING);

  g_timeout_add (1000, change_videotest_pattern, data);
}

static void
print_version ()
{
  gchar *version;

  version = mapmap_get_version ();
  g_message ("MapMap version: %s", version);

  g_free (version);
}

gint
main (gint argc, gchar ** argv)
{
  ClutterInitError status;
  Data *data;

  print_version ();

  status = clutter_gst_init (&argc, &argv);
  if (status != CLUTTER_INIT_SUCCESS)
  {
    g_critical ("Could not initiate Clutter GStreamer.");
    return 1;
  }

  data = g_new0 (Data, 1);
  mm_create_stage (data);

  clutter_main ();

  g_free (data);

  return 0;
}
