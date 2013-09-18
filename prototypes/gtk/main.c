/*
 * pixmap.c:
 * Simple off-screen OpenGL rendering example.
 *
 * written by Naofumi Yasufuku  <naofumi@users.sourceforge.net>
 */

#include <stdlib.h>

#include <gtk/gtk.h>

#include <gtk/gtkgl.h>

#ifdef G_OS_WIN32
#define WIN32_LEAN_AND_MEAN 1
#include <windows.h>
#endif

#ifdef GDK_WINDOWING_QUARTZ
#include <OpenGL/glu.h>
#else
#include <GL/gl.h>
#include <GL/glu.h>
#endif

static GdkGLConfig *glconfig = NULL;
static GdkGLContext *glcontext = NULL;
static GdkPixmap *pixmap = NULL;

static void
init (void)
{
  GLUquadricObj *qobj;
  static GLfloat light_diffuse[] = {1.0, 0.0, 0.0, 1.0};
  static GLfloat light_position[] = {1.0, 1.0, 1.0, 0.0};

  qobj = gluNewQuadric ();
  gluQuadricDrawStyle (qobj, GLU_FILL);
  glNewList (1, GL_COMPILE);
  gluSphere (qobj, 1.0, 20, 20);
  glEndList ();

  glLightfv (GL_LIGHT0, GL_DIFFUSE, light_diffuse);
  glLightfv (GL_LIGHT0, GL_POSITION, light_position);
  glEnable (GL_LIGHTING);
  glEnable (GL_LIGHT0);
  glEnable (GL_DEPTH_TEST);

  glClearColor (1.0, 1.0, 1.0, 1.0);
  glClearDepth (1.0);

  glMatrixMode (GL_PROJECTION);
  glLoadIdentity ();
  gluPerspective (40.0, 1.0, 1.0, 10.0);

  glMatrixMode (GL_MODELVIEW);
  glLoadIdentity ();
  gluLookAt (0.0, 0.0, 3.0,
             0.0, 0.0, 0.0,
             0.0, 1.0, 0.0);
  glTranslatef (0.0, 0.0, -3.0);
}

static gboolean
configure_event (GtkWidget         *widget,
                 GdkEventConfigure *event,
                 gpointer           data)
{
  GtkAllocation allocation;
  GdkGLDrawable *gldrawable;
  static gboolean is_initialized = FALSE;

  gtk_widget_get_allocation (widget, &allocation);

  /*
   * Create an OpenGL off-screen rendering area.
   */

  if (pixmap != NULL)
    g_object_unref (G_OBJECT (pixmap));

  pixmap = gdk_pixmap_new (gtk_widget_get_window (widget),
			   allocation.width,
			   allocation.height,
                           -1);

  /*
   * Set OpenGL-capability to the pixmap
   */

  gldrawable = GDK_GL_DRAWABLE (gdk_pixmap_set_gl_capability (pixmap,
                                                              glconfig,
                                                              NULL));

  /*
   * Create OpenGL rendering context (not direct).
   */

  if (glcontext == NULL)
    {
      glcontext = gdk_gl_context_new (gldrawable,
                                      NULL,
                                      FALSE,
                                      GDK_GL_RGBA_TYPE);
      if (glcontext == NULL)
        {
          g_print ("Connot create the OpenGL rendering context\n");
          if (gtk_main_level () != 0)
            gtk_main_quit ();
          return TRUE;
        }

      g_print ("The OpenGL rendering context is created\n");
    }

  /*** OpenGL BEGIN ***/
  if (!gdk_gl_drawable_gl_begin (gldrawable, glcontext))
    return FALSE;

  if (!is_initialized)
    {
      init ();
      is_initialized = TRUE;
    }

  glViewport (0, 0,
              allocation.width, allocation.height);

  glClear (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  glCallList (1);

  glFlush ();

  gdk_gl_drawable_gl_end (gldrawable);
  /*** OpenGL END ***/

  return TRUE;
}

static gboolean
expose_event (GtkWidget      *widget,
              GdkEventExpose *event,
              gpointer        data)
{
  gdk_draw_drawable (gtk_widget_get_window (widget),
		     gtk_widget_get_style (widget)->fg_gc[gtk_widget_get_state (widget)],
		     pixmap,
		     event->area.x, event->area.y,
		     event->area.x, event->area.y,
		     event->area.width, event->area.height);

  return FALSE;
}

static gboolean
destroy_gl_context (gpointer data)
{
  if (glconfig != NULL)
    g_object_unref (G_OBJECT (glconfig));

  if (glcontext != NULL)
    g_object_unref (G_OBJECT (glcontext));

  return FALSE;
}

static void
print_gl_config_attrib (GdkGLConfig *glconfig,
                        const gchar *attrib_str,
                        int          attrib,
                        gboolean     is_boolean)
{
  int value;

  g_print ("%s = ", attrib_str);
  if (gdk_gl_config_get_attrib (glconfig, attrib, &value))
    {
      if (is_boolean)
        g_print ("%s\n", value == TRUE ? "TRUE" : "FALSE");
      else
        g_print ("%d\n", value);
    }
  else
    g_print ("*** Cannot get %s attribute value\n", attrib_str);
}

static void
examine_gl_config_attrib (GdkGLConfig *glconfig)
{
  g_print ("\nOpenGL visual configurations :\n\n");

  g_print ("gdk_gl_config_is_rgba (glconfig) = %s\n",
           gdk_gl_config_is_rgba (glconfig) ? "TRUE" : "FALSE");
  g_print ("gdk_gl_config_is_double_buffered (glconfig) = %s\n",
           gdk_gl_config_is_double_buffered (glconfig) ? "TRUE" : "FALSE");
  g_print ("gdk_gl_config_is_stereo (glconfig) = %s\n",
           gdk_gl_config_is_stereo (glconfig) ? "TRUE" : "FALSE");
  g_print ("gdk_gl_config_has_alpha (glconfig) = %s\n",
           gdk_gl_config_has_alpha (glconfig) ? "TRUE" : "FALSE");
  g_print ("gdk_gl_config_has_depth_buffer (glconfig) = %s\n",
           gdk_gl_config_has_depth_buffer (glconfig) ? "TRUE" : "FALSE");
  g_print ("gdk_gl_config_has_stencil_buffer (glconfig) = %s\n",
           gdk_gl_config_has_stencil_buffer (glconfig) ? "TRUE" : "FALSE");
  g_print ("gdk_gl_config_has_accum_buffer (glconfig) = %s\n",
           gdk_gl_config_has_accum_buffer (glconfig) ? "TRUE" : "FALSE");

  g_print ("\n");

  print_gl_config_attrib (glconfig, "GDK_GL_USE_GL",           GDK_GL_USE_GL,           TRUE);
  print_gl_config_attrib (glconfig, "GDK_GL_BUFFER_SIZE",      GDK_GL_BUFFER_SIZE,      FALSE);
  print_gl_config_attrib (glconfig, "GDK_GL_LEVEL",            GDK_GL_LEVEL,            FALSE);
  print_gl_config_attrib (glconfig, "GDK_GL_RGBA",             GDK_GL_RGBA,             TRUE);
  print_gl_config_attrib (glconfig, "GDK_GL_DOUBLEBUFFER",     GDK_GL_DOUBLEBUFFER,     TRUE);
  print_gl_config_attrib (glconfig, "GDK_GL_STEREO",           GDK_GL_STEREO,           TRUE);
  print_gl_config_attrib (glconfig, "GDK_GL_AUX_BUFFERS",      GDK_GL_AUX_BUFFERS,      FALSE);
  print_gl_config_attrib (glconfig, "GDK_GL_RED_SIZE",         GDK_GL_RED_SIZE,         FALSE);
  print_gl_config_attrib (glconfig, "GDK_GL_GREEN_SIZE",       GDK_GL_GREEN_SIZE,       FALSE);
  print_gl_config_attrib (glconfig, "GDK_GL_BLUE_SIZE",        GDK_GL_BLUE_SIZE,        FALSE);
  print_gl_config_attrib (glconfig, "GDK_GL_ALPHA_SIZE",       GDK_GL_ALPHA_SIZE,       FALSE);
  print_gl_config_attrib (glconfig, "GDK_GL_DEPTH_SIZE",       GDK_GL_DEPTH_SIZE,       FALSE);
  print_gl_config_attrib (glconfig, "GDK_GL_STENCIL_SIZE",     GDK_GL_STENCIL_SIZE,     FALSE);
  print_gl_config_attrib (glconfig, "GDK_GL_ACCUM_RED_SIZE",   GDK_GL_ACCUM_RED_SIZE,   FALSE);
  print_gl_config_attrib (glconfig, "GDK_GL_ACCUM_GREEN_SIZE", GDK_GL_ACCUM_GREEN_SIZE, FALSE);
  print_gl_config_attrib (glconfig, "GDK_GL_ACCUM_BLUE_SIZE",  GDK_GL_ACCUM_BLUE_SIZE,  FALSE);
  print_gl_config_attrib (glconfig, "GDK_GL_ACCUM_ALPHA_SIZE", GDK_GL_ACCUM_ALPHA_SIZE, FALSE);

  g_print ("\n");
}

int
main (int   argc,
      char *argv[])
{
  gint major, minor;

  GtkWidget *window;
  GtkWidget *vbox;
  GtkWidget *drawing_area;
  GtkWidget *button;

  /*
   * Init GTK.
   */

  gtk_init (&argc, &argv);

  /*
   * Init GtkGLExt.
   */

  gtk_gl_init (&argc, &argv);

  /*
   * Query OpenGL extension version.
   */

  gdk_gl_query_version (&major, &minor);
  g_print ("\nOpenGL extension version - %d.%d\n",
           major, minor);

  /*
   * Configure OpenGL-capable visual.
   */

  /* Try single-buffered visual */
  glconfig = gdk_gl_config_new_by_mode (GDK_GL_MODE_RGB    |
                                        GDK_GL_MODE_DEPTH  |
                                        GDK_GL_MODE_SINGLE);
  if (glconfig == NULL)
    {
      g_print ("*** No appropriate OpenGL-capable visual found.\n");
      exit (1);
    }

  examine_gl_config_attrib (glconfig);

  /*
   * Top-level window.
   */

  window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  gtk_window_set_title (GTK_WINDOW (window), "pixmap");

  /*
   * If window manager doesn't watch the WM_COLORMAP_WINDOWS property on
   * the top-level window, we have to set OpenGL window's colormap to the
   * top-level window.
   */
  gtk_widget_set_colormap (window,
                           gdk_gl_config_get_colormap (glconfig));

  g_signal_connect (G_OBJECT (window), "delete_event",
                    G_CALLBACK (gtk_main_quit), NULL);

  /*
   * VBox.
   */

  vbox = gtk_vbox_new (FALSE, 0);
  gtk_container_add (GTK_CONTAINER (window), vbox);
  gtk_widget_show (vbox);

  /*
   * Drawing area for drawing OpenGL scene.
   */

  drawing_area = gtk_drawing_area_new ();
  gtk_widget_set_size_request (drawing_area, 200, 200);

  /* Set OpenGL-capable colormap. */
  gtk_widget_set_colormap (drawing_area,
                           gdk_gl_config_get_colormap (glconfig));

  g_signal_connect (G_OBJECT (drawing_area), "configure_event",
		    G_CALLBACK (configure_event), NULL);
  g_signal_connect (G_OBJECT (drawing_area), "expose_event",
		    G_CALLBACK (expose_event), NULL);

  gtk_box_pack_start (GTK_BOX (vbox), drawing_area, TRUE, TRUE, 0);

  gtk_widget_show (drawing_area);

  /*
   * Simple quit button.
   */

  button = gtk_button_new_with_label ("Quit");

  g_signal_connect (G_OBJECT (button), "clicked",
                    G_CALLBACK (gtk_main_quit), NULL);

  gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 0);

  gtk_widget_show (button);

  /*
   * Show window.
   */

  gtk_widget_show (window);

  /*
   * Main loop.
   */

  /* Destroy the GLX context explicitly when application is terminated. */
  gtk_quit_add (0, (GtkFunction) destroy_gl_context, NULL);

  gtk_main ();

  return 0;
}
