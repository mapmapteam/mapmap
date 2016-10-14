
## MapMap Versions History - Release Notes

### v0.4.1 - 2016-04-??

#### Added
- Enable display of controls of mappings related to current paint
- Warping (meshes) instead quads for color mappings
- Integrated true FPS in the status bar
- Allow OSC message addressing of paints and mappings by names through regular expressions.
- Paints that are currently not visible are shown in interface with a red bar on them.
- Support live camera input on mappings

#### Changed
- Finely choose on which display to output
- Help menu improvements
- Make the duplication of mapping {Undo, Redo}able
- Pause paints that are not visible anyway
- Frame rate is now an option that can be specified on the command line.
- Support for OSC message: rewind individual paint

#### Fixed
- Disable right click on context menu actions
- when loading project solo/visible/locked were not activated in the mapping list widgets.

### v0.4.0 - 2016-04-19

#### Added
- Zoom toolbar
- Console window
  - Display logging output in a console and be able to turn it on and off
- Abled to locate the video files if is not found when load a project
- Ellipse conical projection
- OSC support on OSX
- Delete a mapping with the `Delete` key
- `Solo`, `Lock`, `Visible`, `Duplicate` and `Delete` buttons on mapping list
- Switch between Destination+Source, Destination-Only and Source-Only views
- Support UTF-8 characters
- Add LICENSE file

#### Changed
- Be able to rename a Paint
- Rename paints and mappings via OSC
- Add OSC support for play, pause, and rewind
- Rename paints and mappings with double click
- Improve Test signal
  - Center the test signal (use all the available space)
- Performance improvements
- Show/Hide the "Undo Stack" tab
- Main toolbar improvements
  - Show/Hide the main toolbar
- More informations on statut bar
- Hide cursor when canvas controls are inactive
- Add/Remove paint are Undo/Redo able
- Automatically move output screen to secondary screen
- Remove implicit shortcuts for menus (they were conflicting with other ALT+ based shortcuts).
- Reorder mappings by drag & drop
- Introduction of namespace
- Add option on the commandline to force language

#### Fixed
- Paints and mappings renamings are now saved in file
- Zooming in the destination canvas changes the size of controls in output window
- Problem with some video files: shape is size of single point
- White rectangle around the fullscreen window
- The software frozen when we load a project and the video files are not found
- Deleting a mapping actually deletes a paint when the paint tab is chosen
- OSC general bug fixes
- Program just freezes when importing corrupted video file
- Ctrl-Q quit the application
- ESC key would not work properly to exit full screen
- Flickering video images when loading from the commandline
- Window geometry not preserved after quit (On Ubuntu 15.10)
- Fix error in mapmap.pro related to translations.
- Fix blocking bug on OSX: program was freezing when move was loaded

### v0.3.1 - 2015-10-30

#### Fixed
- Splash screen does not show when installed
- Stylesheet is not applied when the app is installed on Linux
- Video playback is broken
- OSC not working on OS X

### v0.3.0 - 2015-07-17

#### Added
- Zoom and scroll/drag in editor windows
- Transparency support for both paint and mapping
- Undo/Redo Moving vertex or shape and Add mapping actions

#### Changed
- Use native color dialog

#### Fixed
- Fix perspective-correct textures in quads

### v0.2.0 - 2014-12-28

#### Added
- Add `--file`, `--osc-port` and `--reset-settings` options in command line
- A crosshair cursor in fullscreen mode.
- Logo title in about dialog.
- Change OSC port number via the new preference dialog.
- Speed rate for playback.
- Recent files and videos menu items.
- Test signal image.

#### Changed
- Control mapping visibility from OSC
- Port to Qt5.
- File chooser now remembers last used directory.
- Select a shape with a click.
- Select individual vertex and move it with the arrow keys.
- Videos plays by default at startup

####  Fixed
- Add missing MM.h
- Check for a valid OSC port number.
- Fix fullscreen on GNOME
- Fix main splitter proportions
- Fix memory leak with the GStreamer pipeline manager.
- Hide cursor in output window.
- Press escape to toggle fullscreen.
- Save fullscreen setting.
- Save visibility of mappings.

### v0.1.1 - 2014-07-07

- Fix packaging on OS X 10.9.3.
- Fix: all media share same OpenGL texture
- Fix: Avoid segfault when right-click with no existing output shape.
- Fix: fix error in ordering of mesh vertices.
- Use GStreamer 1.x
- New icons. Nicer looking control points.
- Play/pause/rewind buttons
- Change URI of media with double-click.
- Play movies when we load a project.
- Append file extension if none is provided.

### v0.1.0 - 2014-04-30

#### Features
- Initial release.
- Qt user interface.
- Video mapping with GStreamer.
- Quad, meshes, triangles, circles.
- Color paint.
- Fullscreen output window.
- MMP project XML file format version 0.1.
