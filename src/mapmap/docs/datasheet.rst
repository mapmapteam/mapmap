Datasheet
===========

User Interface
--------------
MapMap allows its users to:

* Create and destroy an unlimited amount of texture sources paints. Textures can be video or image files.
* Create and destroy an unlimited amount of mappings. Each mapping is a shape on which a source paint is drawn.
* Color paints can be used as masks.
* Move layers.
* Save the project to a human-readable XML file.
* Load the project from a human-readable XML file.
* Turn any quad into a mesh.
* Inspect properties.

MapMap doesn't currently play sound from video files.

Supported codecs and file formats
---------------------------------
Image file formats
~~~~~~~~~~~~~~~~~~
* PNG.
* SVG.
* JPEG.

Video containers
~~~~~~~~~~~~~~~~
* OGG.
* Quicktime. (MOV)
* AVI.
* Matroska. (MKV)
* etc.

Recommended video codecs
~~~~~~~~~~~~~~~~~~~~~~~~
* Motion-JPEG.
* OGG Theora.
* MPEG4.
* H.264.

Supported Operating Systems
---------------------------
* Apple OS X 10.9. (it doesn't currently work on OS X 10.10)
* Ubuntu GNU/Linux 12.04 and up.
* Fedora GNU/Linux 19 and up.

Recommended hardware configuration
----------------------------------
* Dual-core 1.0 GHz processor AMD64, or better.
* At least 1 Gb of RAM.
* Video card with 3D acceleration, OpenGL 2.0 support and at least 512 Mb of VRAM.

Using the OSC interface
-----------------------
You can control MapMap via a set of OSC messages. OpenSoundControl allows users to send messages to control audio and video software. These messages are transmitted via UDP/IP. This allows one to control multi-projector shows with little efforts.

MapMap listens for incoming OSC messages on the default port 12345.

There is a Processing bridge to MapMap as well as a Toonloop to MapMap.

