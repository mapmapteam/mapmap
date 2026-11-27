# Release notes for MapMap

## 1.0.0-beta.2 - unreleased

New:

- Convert the colors of camera sources on the graphics card when the camera gives YUV images, like videos.
- Measure the time of each step of the image path with the option `--frame-stats`: one line per second in the log. `--frame-stats-sync` waits for the graphics card to give the real cost of each step, and `--force-cpu-conversion` converts the colors of videos on the processor, to compare.
- Give each project an output resolution, chosen in View > Output Resolution among common projector resolutions, or set by hand. The output window scales it to fill the output screen. New projects take the resolution set in the preferences, which can follow the output screen; older projects take the size of the output screen.
- Rotate and scale the selected layer with a single handle. Press R to only rotate, S to only scale, T for both, or click the handle to switch; Shift for steps of 15° or 10 %, Ctrl (Cmd on macOS) for fine motion. The handle shows which way the image is set in the layer, and replaces the thick first edge.
- Control every setting of a source or a layer by OSC at an address that follows its path: `/mapmap/layer/<id>/<path>`, for example `/mapmap/layer/3/output/vertices/5`. The new MCP tool `list_settings` gives the settings of a source or a layer with their type, bounds and options, and `set_property` takes a path.

Improvements:

- Check the values sent by OSC or MCP against the bounds and the type of the setting: an opacity above 1 stops at 1, and an unknown property name, an invalid color, or a text other than true, false, 1 or 0 for a boolean is refused with a warning. A boolean takes any finite number other than 0 as true; a number that is not finite is refused.
- Keep the unknown keys of a project file when the project is saved again.
- In the settings panel of a layer, "Mesh Subdivisions" is now Columns and Rows.
- At 100% zoom, the input editor shows the whole image of the selected source and the output editor the whole output frame. "Actual pixels (1:1)" shows one image pixel per screen pixel. New shapes are placed inside the output frame.
- Projects converted with the option `--output-size` of `scripts/convert-xml-project.py` take that size as their output resolution.
- Only show the mapping in the fullscreen output when the mouse has not moved over it for three seconds: the mouse cursor, the crosshair, and the outlines and vertices of the shapes are hidden. They come back when the mouse moves.

Changes:

- Clicking again on the selected layer no longer switches to scaling or rotating, and the M key does nothing. Use the handle of the layer instead: R, S and T choose what it does.
- The two meshes of a layer always keep the same grid.
- The names of settings are written in lowercase with underscores, in OSC addresses, in MCP and in project files: `n_columns`, `n_rows`, `source_id`. Projects saved with the former names (`nColumns`, `nRows`, `sourceId`) still open; they are saved with the new names, which MapMap 1.0.0-beta.1 does not read.
- The settings panel shows the four corners of a mesh only. Its other points are moved on the canvas, or by OSC and MCP at their path.
- A mesh has at most 32 columns and 32 rows, by OSC, MCP or the settings panel. A larger mesh of an existing project is reduced to 32 columns or rows when the project is opened: evenly spaced columns and rows are kept, its outline included.
- `get_source` and `get_layer` of the MCP server no longer return `objectName`, which was always empty.

Fix:

- Fix a crash when drawing a layer whose two meshes have different grids.
- Fix the text of drop-down lists, such as the zoom of the editors, which was invisible with dark system themes.
- Fix a crash on macOS when importing a media file or adding a color.
- Fix a video racing through its frames at a speed of zero or below: it now stops. A negative speed is kept in the project, for when videos can play backwards again.

## 1.0.0-beta.1 - 2026-11-23

First pre-release of MapMap 1.0. MapMap is developed again, with the support of Art Plus Code.

Not compatible with MapMap 0.6:

- Projects are now saved as JSON files. To open a project of MapMap 0.4 to 0.6, saved as XML, convert it first with the Python script `scripts/convert-xml-project.py`. Projects of MapMap 0.3 and earlier cannot be converted.
- The point (0, 0) is now the top left corner of the output screen. It was near its middle, so the shapes of older projects are not at the same place anymore. To move them, convert the project with the option `--output-size` of `scripts/convert-xml-project.py`, followed by the size of the screen: `--output-size 1920x1080`.
- OSC messages now say "source" and "layer", like the interface. Replace `/mapmap/paint/...` with `/mapmap/source/...`; `/mapmap/mapping/...` still works.
- OSC messages are only accepted from the same computer. To control MapMap from another one, enable "Accept OSC from the network" in the preferences.
- The OSC `/mapmap/quit` message is ignored unless it is allowed in the preferences.
- On macOS, the preferences of MapMap 0.6 are not kept.

New:

- Move to Qt 6. There is no need to install GStreamer anymore.
- Add Syphon sources on macOS, to show the output of another application.
- Add camera sources on macOS.
- Flip and rotate layers, or only the image inside a layer, with the menus, the keyboard or OSC.
- Show the frame of the output screen, with its size, in the output editor. All of it is visible when MapMap starts.
- Raise and lower layers with the menus or the keyboard.
- Set the frame rate in the preferences (#249).
- Control more with OSC: play and pause a single source, move a layer or one of its vertices. See the `OSC` file.
- Add an MCP (Model Context Protocol) server, off by default, so an AI assistant can build and edit a project.
- Add a welcome dialog, and a keyboard shortcuts window in the Help menu.
- Translate the interface to German, Portuguese and Spanish, and complete the French and Chinese translations.

Improvements:

- Convert the colors of videos on the graphics card. On a Raspberry Pi 4, 720p videos now play without dropping frames.
- Fix MapMap keeping a processor core busy when nothing plays (#555).
- Import videos without blocking the interface.

Fix:

- Fix projects with a camera that could not be reopened (#542).
- Fix the frame rate, which stayed below the requested value, and its display, which always showed 0.00.
- Fix new shapes, which could be outside of the output screen: they are now in the middle of it, and not larger.
- Fix the position of the mouse in the status bar, which is now in pixels of the output screen (#264).
- Fix zooming and moving the view, which could jump to another place.
- Fix the crosshair of the fullscreen output, which was below the point that a click reaches on a Mac with a notch.
- Fix the center of ellipses, which was not correct in perspective (#177).
- Fix the path of a missing media file being lost when locating it is cancelled.
- Fix zooming with Ctrl + scroll on macOS, and with small trackpad scrolls.
- A damaged project file does not crash MapMap: what cannot be read is skipped.

## 0.6.2 - 2018-07-24

- Remove an assert that might crash MapMap
- Fix parsing of MapMap version in the project file (critical bug fixed)
- Add qt5opengl in deps in INSTALL.md
- Also create a zip archive in scripts/build.sh

## 0.6.1 - 2018-04-09

- Add .url link file to GStreamer on macOS and packaged it in the DMG.
- Fix: crash because undoStack was uninitialized (closes #413).
- Windows environnement variable fix

## 0.6.0 - 2018-04-08

- Add macOS Application Icon
- Fix video rate bug (Closes #369)
- Change the default test card
- Add preference to show controls only on mouse over
- Add simplified and traditional chinese translations
- Add Spanish translation
- Rename "Mesh subdivisions" to "Subdivisions"
- Rename width/height to horizontal/vertical for mesh subdivision naming
- Rename GUI elements: paint, mapping, source, destination, dimension #376
- Fix: Cursor no longer displays in fullscreen without controls (closes #387)
- Allow to change source of the current layer in context menu. Request #377
- Implement scale/rotate icons and minor fix
- Replace CTRL-click for free-hand transforms with re-click
- Prevent adding camera action in OSX and WINDOWS where it is not created (closes #366).
- Fixed linker problem on Ubuntu 17.10 (closes #382)
- Transition to the Mardown syntax for the README file
- Prevent changing paint for mapping if not compatible.
- Allow the change of paint id on mapping using OSC (closes #302).
- Added paintId as property + preserved backwards compatibility for files.
- Allows possibility of changing paint of a mapping from the interface (related to #302).
- Remove infinite recursion in VideoImpl (closes #351).
- Add item ID as part of properties.
- Simplified property browser interface by removing useless top-item property.
- Added tooltips specifying the ID of each element.
- Bugfix: let mappings names adjust size (closes #365).
- Fix problem with OSC messages (unset names in mappings were left empty).
- Integrate transform function
- Improve some icons, zoom, solo mode, prefs, test signal
- Fix the play/pause button when adding media or color paints
- Enabled paint context menu
- Implemented basic rotation/scaling of polygons.
- Fix error in the Mesh::toPolygon() (wrong number of vertices).
- Fit all shapes to view canvas instead current shape
- Add a .travis.yml file for automated builds on travis-ci.org
- Reduce the size of the zoom tool bars
- Add drag and drop feature for media file and project
- Reorganize source code files
- Mouse wheel: press control to zoom.
- Always show scroll bars in mapping canvas
- Boundary checks - fix #319 crash when receiving osc messages
- Fix crash when DEL is pressed while no mappings to delete - fixes #315
- Proper management of mappings related to a paint in undo/redo (relates to #315).
- Only show camera input button on GNU/Linux, since it doesn't work on the other platforms

## 0.5.0 - 2017-01-01

- Fix bug: it was impossible to get out of test signal mode by pressing ESC.
- Reorganize _updateToPreferredScreen().
- Rewrite constraining of screen no in setPreferredScreen(int).
- Bug fix: force fullscreen on output window if test signal is activated (closes #304).
- Added a _setFullScreen() method + renamed _is_fullscreen to _isFullScreen (camelCase).
- Put some order in the view menu.
- Remove toolbars sub-menu.
- Fix all french language files for 0.5 (closes #219).
- Thread-safe access OSC server to prevent crashes (not fully tested) (closes #307).
- Prevent property browsers from disappearing by making splitters' children non-collapsible.
- Fix #305 merge menus
- Fix #274: Document OSC within the software
- Comment-out unused elements in preferences.
- Remove playback menu and reorganize items in View menu.
- Fix #310: Document how to generate doc with markdown + delete CHANGELOG.md that was a duplicate
- Minor reorganization of sub-menu declarations.
- Rename outputsMenu for outputScreenMenu (more appropriate).
- Fix bug: outputs menu was empty when only one screen.
- Integrate outputs menu into view menu.
- Fix #303: Update list of contributors by hand
- Fix icons for test signal.
- Change vertices background to make them visible on white layers
- Stylesheet improvements
- Changed play/pause key sequence from spacebar to CTRL-SHIFT-P (closes #292)
- update translation files (closes #278).
- Fix potential bug: translators were declared out of scope.
- Fix bug: duplicating a mesh resulted in crash (closes #298).
- Implement support for animated GIFs (closes #291, closes #189).
- Update install instructions on ArchLinux (closes #269) and Ubuntu
- Add support for webm file type.
- Increase pull-away parameter cause it was still possible to bring vertex outside of opposite corners in quads.
- Update INSTALL information to allow detection of cameras on Ubuntu
- Fix bug: it was possible to cross through an opposing vertex in a quad. - Used constants instead of magic numbers to control the vertex constraining procedure.
- Fix bug: moving quad vertices around glued vertices in impractical ways to the edges.
- Fix bug: duplicating a color mapping resulted in segfault.
- Bug fix: audio was not stopping when opening dialog (eg. save as, open, import media, etc.)
- Fixed bug: in some cases the video was blank because of bad audio connection.
- Fix problems with video files without audio codec not working properly (closes #280) - Cleanup and refactoring
- Test cards pattern improvements
- Change crosshair style for more visibility on light mapping
- Add toolbar title for  source and destination canvas
- Change "perspective" word to "layout" and their shortcut to avoid conflicts with some OS
- Cosmetic changes
- Trying to fix translation but still not working
- Fixed style problem: on Ubuntu, text was white-on-white in property browser
- Fixed typo in translations
- Added mention of 15.04 in INSTALL
- Comment checkbox and radio (for later still not working)
- Look n feel improvements
- Add librairies to about dialog
- About Dialog Improvements
- Add about projection mapping resource
- Make release notes accesible for users
- Make a script to generate contributors from the git logs
- Add Gui class forwarding file
- Change mapmap resource file name and add new documentation resource file
- Remove old useless code from ancient OSC-support system.
- Fix bug: Hiding a solo-ed mapping was stopping playback.
- Reintegrate audio support (works on Linux).
- Fix bug: New paints were automatically started (play()) when added, thus invalidating the feature that non-visible paints do not play.
- Add "Send feedback" to help menu
- Update "Undo stack" to "Undo history"
- Improve vertices movement shortcuts
- Always align mapping layers buttons on the right side
- Cosmetic changes in duplicate mapping function
- Bug Fix: Clicking the zoom toolbars button of the output panel while there is no mesh present will crash the software.
- Bug Fix: Crash when try to duplicate color mapping
- Fix some bugs about Windows release
- Improve preferences dialog
- Changes for Windows packaging
- Implement camera on user interface
- Bug fix: when loading project solo/visible/locked were not activated in the mapping list widgets.
- Bug fix: when moving layers around video would pause.
- Cleanup in code related to display of paint icons in list widgets.
- Adjustments in paint icons (make sure they are always square with the right dimensions).
- Fixed bug: main pause button was not working anymore.
- New feature: Paints that are currently not visible are shown in interface with a red bar on them.
- Small fix: Switch between vertex with Shift+Space keys
- Fix bug: OSC messages that change solo, visible, locked status of mappings were not changed in the GUI.
- Small fix: path is valid for regexp if at least one element was valid
- Allow OSC message addressing of paints and mappings by names through regular expressions.
- encode4mmp: more doc + fix shebang syntax
- Add support for OSC message: rewind individual paint.
- Fix problem with OSC port being overriden by default settings even after changing it in preferences.
- Integrate true FPS in the status bar.
- Fix bug: vertices were not sticking correctly on source.
- Cleanup in main.cpp options.
- Some cleanup related to OSC.
- Frame rate is now an option that can be specified on the commandline.
- Pause paints that are not visible anyway.
- Create meshes instead of quads for color mappings (closes #150).
- Write log to file
- Fixed bug introduced by paint control option (segfaulted with color paints).
- Increased the sticky radius a bit.
- Fixed bug introduced by paint control option (segfaulted with color paints).
- Refactor videoimpl
- Enable display of controls of mappings related to current paint (closes #142).
- Improved look of locked color.
- Added script to automatically convert videos to PhotoJPEG (a good format for MapMap).
- Disable right click on context menu actions
- more decklink prototypes
- Don't need screen actions on single screen mode
- Merge MapperGLCanvas.cpp
- Allow to finely choose on which display to output
- improving the menu and adding some useful link
- Make the duplication of mapping undoable and redoable.

## 0.4.0 - 2016-04-19

- Add zoom toolbar
- Add console window
- Fix #179: Paints and mappings renamings are now saved in file
- Fix #162: Zooming in the destination canvas changes the size of controls in output window.
- Fix #154: Problem with some video files: shape is size of single point
- Can rename paints and mappings via OSC
- Can rename paints and mappings with double click
- Fix #156: White rectangle around the fullscreen window
- Fix #152: The software frozen when we load a project and the video files are not found
- Abled to locate the video files if is not found when load a project
- Fix #149: Deleting a mapping actually deletes a paint when the paint tab is chosen
- Improve Test signal
- Enhancement #117: Ellipse conical projection
- Performance improvements
- OSC general bug fixes
- OSC support on OSX
- Done #174: Be able to rename a Paint
- Done #72: Be able to name paints and mappings - for OSC controls
- Center the test signal #97 (must use all the space available)
- Fix #159: Program just freezes when importing corrupted video file
- Done #145: The user doesn't need to see the undo stack
- Done #40: Ctrl-Q should quit the application
- Done #183: Display logging output in a console and be able to turn it on and off
- Done #203: The toolbar can be shown or hidden at will by the user
- Done #147: Be able to delete a mapping with the delete key #147 (tested on Ubuntu)
- Done #184: Make sure names support UTF-8 characters
- Done #201 UX Design document: implement main toolbar improvements
- Add solo/lock/mute/delete/duplicate mapping icons
- Hide cursor when canvas controls are not shown
- Can undo or redo Add/Remove paint
- Implement #240 Allow easy switch between destination+source, destination-only and source-only views
- Automatically move output screen to secondary screen
- Fix bug: ESC key would not work properly to exit full screen.
- Remove implicit shortcuts for menus (they were conflicting with other ALT+ based shortcuts).
- Fix: It should be possible to have an output window when using a single screen (closes #209
- Add LICENSE
- Can reorder mapping by drag & drop
- Fix: Flickering video images when loading from the commandline (closes #218).
- Introduction of namespace
- Add OSC support for play, pause, and rewind.
- Fix: On Ubuntu 15.10 : window geometry not preserved after quit (closes #227).
- Add option on the commandline to force language.
- Fix error in mapmap.pro related to translations.
- Fix blocking bug on OSX: program was freezing when move was loaded
- Implemented new feature: holding the shift key while moving a vertex temporarily inverts stick

## 0.3.1 - 2015-10-30

- Fix #138: splash screen does not show when installed
- Fix #139: Stylesheet is not applied when the app is installed on Linux
- Fix #135: Video playback is broken
- Fix #103: OSC not working on OS X

## 0.3.0 - 2015-07-17

- Add zoom and scroll/drag in editor windows
- Add transparency support for both paint and mapping
- Use native color dialog
- Undo/redo move and add actions
- Fix perspective-correct textures in quads

## 0.2.0 - 2014-12-28

- Add --file option.
- Add --osc-port option.
- Add --reset-settings option.
- Add crosshair cursor in fullscreen mode.
- Add logo title in about dialog.
- Can change OSC port via the new preference dialog.
- Control mapping visibility from OSC (ticket #43)
- File chooser now remembers last used directory.
- Port to Qt5.
- Recent files and videos menu items.
- Select a shape with a click.
- Select individual vertex and move it with the arrow keys.
- Speed rate for playback.
- Test signal image.
- Videos plays by default at startup.
- Fix: Add missing MM.h
- Fix: Check for a valid OSC port number.
- Fix: Fix fullscreen on GNOME
- Fix: Fix main splitter proportions
- Fix: Fix memory leak with the GStreamer pipeline manager.
- Fix: Hide cursor in output window.
- Fix: Press escape to toggle fullscreen.
- Fix: Save fullscreen setting.
- Fix: Save visibility of mappings.

## 0.1.1 - 2014-07-07

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

## 0.1.0 - 2014-04-30

- Initial release.
- Qt user interface.
- Video mapping with GStreamer.
- Quad, meshes, triangles, circles.
- Color paint.
- Fullscreen output window.
- MMP project XML file format version 0.1.

