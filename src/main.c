#include <glib.h>
#include <mapmap/version.h>

gint
main (gint argc, gchar ** argv)
{
  gchar *version;

  version = mapmap_get_version ();
  g_message ("MapMap version: %s", version);
  g_free (version);

  return 0;
}
