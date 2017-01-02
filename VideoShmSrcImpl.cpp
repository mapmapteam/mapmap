/*
 * VideoShmSrcImpl.cpp
 *
 * (c) 2016 Vasilis Liaskovitis -- vliaskov@gmail.com
 * (c) 2013 Sofian Audry -- info(@)sofianaudry(.)com
 * (c) 2013 Alexandre Quessy -- alexandre(@)quessy(.)net
 * (c) 2012 Jean-Sebastien Senecal
 * (c) 2004 Mathieu Guindon, Julien Keable
 *           Based on code from Drone http://github.com/sofian/drone
 *           Based on code from the GStreamer Tutorials http://docs.gstreamer.com/display/GstSDK/Tutorials
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#include "VideoShmSrcImpl.h"
#include <cstring>
#include <iostream>

namespace mmp {

VideoShmSrcImpl::VideoShmSrcImpl() :
_shmsrc0(NULL),
_gdpdepay0(NULL),
_pollSource(NULL),
_attached(false)
{
}

bool VideoShmSrcImpl::getAttached()
{
  return _attached;
}

void VideoShmSrcImpl::setAttached(bool attach)
{
  _attached = attach;
}

gboolean
gstPollShmsrc (void *user_data)
{
  VideoShmSrcImpl *p = (VideoShmSrcImpl*) user_data;
  if (g_file_test(p->getUri().toUtf8().constData(), G_FILE_TEST_EXISTS) &&
    ! p->getAttached())
  {
    if (! p->setPlayState(true))
    {
      qDebug() << "tried to attach, but starting pipeline failed!" << endl;
      return false;
    }
    p->setAttached(true);
  }
  return true;
}

bool VideoShmSrcImpl::loadMovie(const QString& path) {

  VideoImpl::loadMovie(path);

  _shmsrc0 = gst_element_factory_make ("shmsrc", "shmsrc0");
  _gdpdepay0 = gst_element_factory_make ("gdpdepay", "gdpdepay0");
  _pollSource = g_timeout_source_new (500);

  g_source_set_callback (_pollSource,
      gstPollShmsrc,
      this,
      NULL);
  g_source_attach (_pollSource, g_main_context_default());
  g_source_unref (_pollSource);

  if (! _shmsrc0 || ! _gdpdepay0)
  {
    qWarning() << "Not all elements could be created." << endl;
    if (! _shmsrc0) g_printerr("_shmsrc0");
    if (! _gdpdepay0) g_printerr("_gdpdepay0");
    unloadMovie();
    return -1;
  }

  gst_bin_add_many (GST_BIN(_pipeline), _shmsrc0, _gdpdepay0, NULL);
  if (! gst_element_link_many (_shmsrc0, _gdpdepay0, _queue0, NULL))
  {
    qWarning() << "Could not link shmsrc, deserializer and video queue." << endl;
  }

  QByteArray ba = path.toLocal8Bit();
  gchar* uri = (gchar*) path.toUtf8().constData();
  uri =  (gchar*) ba.data();

  //qDebug() << "LIVE mode" << uri;
  g_object_set (_shmsrc0, "socket-path", uri, NULL);
  g_object_set (_shmsrc0, "is-live", TRUE, NULL);
  _videoIsConnected = true;

  return TRUE;
}

VideoShmSrcImpl::~VideoShmSrcImpl()
{
  // Unref the shmsrc poller.
  if (_pollSource)
  {
     g_source_unref(_pollSource);
     _pollSource = NULL;
  }
}

}
