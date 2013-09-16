#include "mapmap/version.h"
#include "src/config.h"

gchar *
mapmap_get_version ()
{
  gchar *version = g_strdup (PACKAGE_VERSION);
  return version;
}
