Datasheet
===========

User Interface
--------------
MapMap allows its users to:

* Create and destroy an unlimited amount of sources. A source is a video file, an image file, a camera, a color, or, on macOS, a Syphon server.
* Create and destroy an unlimited amount of layers. Each layer is a shape on which a source is drawn: a triangle, a mesh (a quad with subdivisions) or an ellipse.
* Use color sources as masks.
* Reorder, hide, lock and solo layers.
* Save the project to a human-readable JSON file.
* Load the project from a human-readable JSON file.
* Turn any quad into a mesh.
* Inspect and edit the properties of sources and layers.
* Play the sound of video files, with a volume for each source.

Projects saved by MapMap 0.6 and earlier (XML files) cannot be opened. Those of
MapMap 0.4 to 0.6 can be converted with ``scripts/convert-xml-project.py``.

Supported codecs and file formats
---------------------------------
MapMap reads media files with Qt Multimedia, which uses the decoders of the
operating system. What plays therefore depends on the system.

Image file formats
~~~~~~~~~~~~~~~~~~
* PNG.
* JPEG.
* GIF, including animated ones.
* BMP and TIFF.

Video containers
~~~~~~~~~~~~~~~~
* MPEG-4. (MP4)
* Quicktime. (MOV)
* AVI.
* WebM.
* OGG.

Supported Operating Systems
---------------------------
MapMap is built and tested for every change on:

* macOS, on the current GitHub runner image.
* Ubuntu GNU/Linux 24.04.
* Windows, on the current GitHub runner image.

Recommended hardware configuration
----------------------------------
* Dual-core processor, or better.
* A video card with 3D acceleration and OpenGL 2.0 support.

Remote control
--------------
You can control MapMap with OSC messages. OpenSoundControl allows users to
send messages to control audio and video software, from another program or
another computer. MapMap listens for incoming OSC messages on port 12345, on
the local machine only unless the user allows the network. See the ``OSC``
file.

MapMap also includes an MCP (Model Context Protocol) server, off by default,
which lets an AI assistant build and edit a project.
