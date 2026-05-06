#include <glib.h>
#include <gst/gst.h>

#define DEFAULT_NUM_BUFFERS 300
#define X264_TUNE_ZEROLATENCY 0x00000004
#define X264_SPEED_PRESET_ULTRAFAST 1
#define GST_PARSE_LAUNCH_OPTION


static gboolean
bus_call (GstBus *bus,
          GstMessage *msg,
          gpointer data)
{
  (void) bus;
  GMainLoop *loop = (GMainLoop *) data;

  switch (GST_MESSAGE_TYPE (msg)) {
    case GST_MESSAGE_EOS:
      g_print ("Finished writing video file\n");
      g_main_loop_quit (loop);
      break;

    case GST_MESSAGE_ERROR: {
      gchar *debug;
      GError *error;

      gst_message_parse_error (msg, &error, &debug);
      g_printerr ("Error: %s\n", error->message);
      if (debug != NULL) {
        g_printerr ("Debug details: %s\n", debug);
      }

      g_free (debug);
      g_error_free (error);
      g_main_loop_quit (loop);
      break;
    }

    default:
      break;
  }

  return TRUE;
}

int
main (int argc,
      char *argv[])
{
  GMainLoop *loop;
  GstElement *pipeline;
  GstElement *source = NULL;
  GstElement *encoder = NULL;
  GstBus *bus;
  guint bus_watch_id;

  gst_init (&argc, &argv);
  loop = g_main_loop_new (NULL, FALSE);

  if (argc != 2) {
    g_printerr ("Usage: %s <output.mp4>\n", argv[0]);
    return -1;
  }
#ifdef GST_PARSE_LAUNCH_OPTION
  GError *error = NULL;
  gchar *pipeline_description;

  pipeline_description = g_strdup_printf (
      "videotestsrc name=video-source pattern=0 num-buffers=%d ! "
      "videoconvert ! "
      "x264enc name=h264-encoder tune=%d speed-preset=%d ! "
      "h264parse ! "
      "mp4mux ! "
      "filesink location=\"%s\"",
      DEFAULT_NUM_BUFFERS,
      X264_TUNE_ZEROLATENCY,
      X264_SPEED_PRESET_ULTRAFAST,
      argv[1]);

  pipeline = gst_parse_launch (pipeline_description, &error);
  g_free (pipeline_description);

  if (error != NULL) {
    g_printerr ("Pipeline creation failed: %s\n", error->message);
    g_error_free (error);
    g_main_loop_unref (loop);
    return -1;
  }

  source = gst_bin_get_by_name (GST_BIN (pipeline), "video-source");
  encoder = gst_bin_get_by_name (GST_BIN (pipeline), "h264-encoder");

  if (!source || !encoder) {
    g_printerr ("Failed to retrieve parsed element handles.\n");
    if (source) {
      gst_object_unref (source);
    }
    if (encoder) {
      gst_object_unref (encoder);
    }
    gst_object_unref (pipeline);
    g_main_loop_unref (loop);
    return -1;
  }
#else
  GstElement *convert;
  GstElement *parser;
  GstElement *muxer;
  GstElement *sink;

  pipeline = gst_pipeline_new ("video-encoder");
  source = gst_element_factory_make ("videotestsrc", "video-source");
  convert = gst_element_factory_make ("videoconvert", "video-convert");
  encoder = gst_element_factory_make ("x264enc", "h264-encoder");
  parser = gst_element_factory_make ("h264parse", "h264-parser");
  muxer = gst_element_factory_make ("mp4mux", "mp4-muxer");
  sink = gst_element_factory_make ("filesink", "file-output");

  if (!pipeline || !source || !convert || !encoder || !parser || !muxer || !sink) {
    g_printerr ("One element could not be created. Verify the required GStreamer plugins are installed.\n");
    g_main_loop_unref (loop);
    return -1;
  }

  g_object_set (G_OBJECT (source),
                "pattern", 0,
                "num-buffers", DEFAULT_NUM_BUFFERS,
                NULL);
  g_object_set (G_OBJECT (encoder),
                "tune", X264_TUNE_ZEROLATENCY,
                "speed-preset", X264_SPEED_PRESET_ULTRAFAST,
                NULL);
  g_object_set (G_OBJECT (sink), "location", argv[1], NULL);

  gst_bin_add_many (GST_BIN (pipeline),
                    source, convert, encoder, parser, muxer, sink, NULL);

  if (!gst_element_link_many (source, convert, encoder, parser, muxer, sink, NULL)) {
    g_printerr ("Elements could not be linked.\n");
    gst_object_unref (pipeline);
    g_main_loop_unref (loop);
    return -1;
  }
#endif

  bus = gst_pipeline_get_bus (GST_PIPELINE (pipeline));
  bus_watch_id = gst_bus_add_watch (bus, bus_call, loop);
  gst_object_unref (bus);

  g_print ("Writing H.264 test video to: %s\n", argv[1]);
  gst_element_set_state (pipeline, GST_STATE_PLAYING);

  g_print ("Encoding %d test frames...\n", DEFAULT_NUM_BUFFERS);
  g_main_loop_run (loop);

  gst_element_set_state (pipeline, GST_STATE_NULL);
  if (source) {
    gst_object_unref (source);
  }
  if (encoder) {
    gst_object_unref (encoder);
  }
  gst_object_unref (pipeline);
  g_source_remove (bus_watch_id);
  g_main_loop_unref (loop);

  return 0;
}
