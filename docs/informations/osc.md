# Currently supported commands

General rules:

 - Every address starts with `/mapmap`.
 - The first argument selects the source or the layer: its id (an integer), or a pattern matched against names (a string).
 - On/off properties such as *solo*, *visible* and *locked* are set using 1 (on) or 0 (off).
 - *opacity* and *volume* are real values in the [0, 1] range.
 - "mapping" is still accepted in place of "layer". The former "paint" addresses are now "source" addresses.

## Sources

Rename: `/mapmap/source/name ,is <id> <name>`  
Adjust opacity: `/mapmap/source/opacity ,if <id> <opacity>`  
Play: `/mapmap/source/play ,i <id>`  
Pause: `/mapmap/source/pause ,i <id>`  
Rewind: `/mapmap/source/rewind ,i <id>`

### Color source

Adjust color (eg. "#ff0000"): `/mapmap/source/color ,is <id> <color>`

### Image/Video source

Change file (eg. "/path/to/clip.mov"): `/mapmap/source/uri ,is <id> <uri>`  
Change rate (speed) (*): `/mapmap/source/rate ,if <id> <rate>`  
Adjust audio volume: `/mapmap/source/volume ,if <id> <volume>`

(*) 1 = same speed, 0.5 = half speed, 2 = double speed

## Layers

Rename: `/mapmap/layer/name ,is <id> <name>`  
Adjust opacity: `/mapmap/layer/opacity ,if <id> <opacity>`  
Set visibility status: `/mapmap/layer/visible ,ii <id> <visible>`  
Set solo status: `/mapmap/layer/solo ,ii <id> <solo>`  
Set lock status: `/mapmap/layer/locked ,ii <id> <locked>`  
Adjust depth (layer order): `/mapmap/layer/depth ,ii <id> <depth>`  
Move so the centre of the layer is at (x, y): `/mapmap/layer/move/xy ,iff <id> <x> <y>`  
Move by (dx, dy): `/mapmap/layer/translate/xy ,iff <id> <dx> <dy>`  
Move a vertex of the output shape: `/mapmap/layer/vertex/xy ,iiff <id> <index> <x> <y>`  
Move a vertex of the input shape: `/mapmap/layer/vertex/source/xy ,iiff <id> <index> <x> <y>`  
Flip the shape: `/mapmap/layer/flip/horizontal ,i <id>` (or `/vertical`)  
Rotate the shape by a quarter or half turn: `/mapmap/layer/rotate/cw ,i <id>` (or `/ccw`, `/180`)  
Flip the image inside the shape, without moving it: `/mapmap/layer/flip/source/horizontal ,i <id>` (or `/vertical`)  
Rotate the image inside the shape, without moving it: `/mapmap/layer/rotate/source/cw ,i <id>` (or `/ccw`, `/180`)

## Settings by path

Every setting of a source or a layer can also be written at an address that follows its path: `/mapmap/source/<id>/<path>` or `/mapmap/layer/<id>/<path>`. The source or the layer is given by its id, in the address; every argument is part of the value. A value outside the range of the setting is clamped to it: an opacity of 1.5 becomes 1. A value of the wrong type, such as an invalid color, or a text other than true, false, 1 or 0 for a boolean, is refused with a warning. A boolean takes any number: 0 is false, any other number true, like the 127 that controllers send for "on".

Adjust the opacity of layer 3: `/mapmap/layer/3/opacity ,f 0.5`  
Adjust the color of source 2: `/mapmap/source/2/color ,s "#ff0000"`  
Move point 5 of the output shape of layer 3 to (10, 20): `/mapmap/layer/3/output/vertices/5 ,ff 10 20`

The shapes of a layer are under `output` and `input`; their points are numbered from 0.

## Name patterns

Ids are hard to remember and manipulate. Alternatively, one can give a string pattern matched against the names of sources or layers. The pattern follows a simple "file globbing" / wildcard syntax (`*`, `?`, `[0-9]`). It is case-sensitive.

### Examples

Change opacity of the source named "movie.mov" to 0.1 (10%):  
`/mapmap/source/opacity ,sf "movie.mov" 0.1`

Change opacity of all sources whose name begins by "movie-" to 0.5 (50%):  
`/mapmap/source/opacity ,sf "movie-*" 0.5`

Rewind all .mov sources:  
`/mapmap/source/rewind ,s "*.mov"`

Set all layers that begin with "mesh-" followed by a single digit to solo mode:  
`/mapmap/layer/solo ,si "mesh-[0-9]" 1`

## Playback

Pause: `/mapmap/pause`  
Play: `/mapmap/play`  
Rewind/reset: `/mapmap/rewind`  
Quit: `/mapmap/quit`

## Security

OSC has no authentication. By default MapMap listens on the loopback
interface only (`127.0.0.1`), so OSC is reachable solely from the same
computer. To control MapMap from another device, enable **Accept OSC from
the network** in *Preferences > OSC Setup* — only on a trusted show network.

The `/mapmap/quit` command is ignored unless **Allow OSC to quit the
application** is enabled in the same panel, so a remote sender cannot close
the app during a show.
