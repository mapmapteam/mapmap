How to use MapMap to create to do video mapping
===============================================

Launching the application
-------------------------
On Apple OS X, open the application by either double-clicking on its icon in the Finder, or by clicking on its icon in the Dock.

On Debian or Ubuntu GNU/Linux, find its icon in the application menu and activate it.

Paints and mappings
-------------------
A paint is some materials to be drawn. Currently supported paint types are color paint and media paint.

A mapping is a shape in the output window on which to display some paint. Currently supported shapes are: quads and circles. When quads have a dimension of two by two (2x2) vertices, they are actuals quads. When they have more than 2x2 vertices, they are called meshes. A mesh is a grid of vertices that allow more flexible mapping.

The elements of the user interface
----------------------------------
The application has a main window and an output window. The main window includes the toolbar, input viewport, the output viewport, the list of paints, the list of mappings and the property inspector.

Make the output window fullscreen before starting to do video mapping on an actual object. This is necessary so that the pixel positions match the actual location of the pixels drawn by the projector. One should also make sure to use the native resolution of the projector, for best results, if any.

Note that there are handles that are drawn on the mappings in the output window, to ease editing. For a show, one will most probably want to disable them. Uncheck the "View > Display canvas controls" menu item to do so.

Create a media paint
--------------------
Choose File > Import media source file... or click on the button in the toolbar. Choose a media file using the dialog.

Create a mapping
----------------
First, select the desired paint for the mapping in the paint list. To create a quad, click on the "Choose Quad/Mesh" button in the toolbar. Move around the vertices of the quad by click and dragging near them. The closest vertex should be dragged. You can also use the number boxes in the property inspector to move the vertices.

To make a mesh from a quad, change its dimension using its width and height mumber boxes.

Creating a triangle is very similar to creating a quad. Move around its vertices with the mouse.

To create a circle do the same as for a quad or a triangle. Changing its size, orientation and position is a bit tricky, though. Play around by drag and dragging the vertices of the blue shape that is in the input and output viewports.

Create a color paint
--------------------
Color paints can be used as black masks and must be created the same way as a media paint, but it's a color dialog instead.

Destroy a mapping
-----------------
Mapping can be destroyed simply by selecting it in the mapping list, and then choosing the "Edit > Delete" menu item.

Destroy a paint
---------------
Paints can be destroyed simply by selecting it in the paint list, and then choosing the "Edit > Delete" menu item. Since all the mappings that use that paint will also be delete, a dialog will ask you for confirmation.

Save the project to a file
--------------------------
To save the current project, choose "File > Save as..." and then choose a file name. The extension file is ".mmp", but the file format is simply XML, a very common one.

Load a project from a file
--------------------------
To load a project from a file, choose "File > Open...".

