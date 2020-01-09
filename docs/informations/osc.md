# Currently supported commands

General rules:

 - Paint and mapping ids are numbered starting from 1.
 - On/off properties such as *solo*, *visible* and *lock* are set using 1 (on) or 0 (off)
 - Percentage properties such as *opacity* and *volume* are set using a real-value in the [0, 1] range.

## Paints

Rename: `/mapmap/paint/name ,is <id> <name>`  
Adjust opacity: `/mapmap/paint/opacity ,if <id> <opacity>`

### Color paint

Adjust color (eg. "#ff0000"): `/mapmap/paint/color ,is <id> <color>`

### Image/Video paint

Change URI (eg. "file:///path/to/clip.mov"): `/mapmap/paint/uri ,is <id> <uri>`  
Change rate (speed) (*): `/mapmap/paint/rate ,if <id> <rate>`  
Adjust audio volume: `/mapmap/paint/volume ,if <id> <volume>`  
Rewind: `/mapmap/paint/rewind ,i <id>`
 
(*) 1 = same speed, 0.5 = half speed, 2 = double speed

## Mappings

Rename: `/mapmap/mapping/name ,is <id> <name>`  
Adjust opacity: `/mapmap/mapping/opacity ,if <id> <opacity>`  
Set visibility status: `/mapmap/mapping/visible ,ii <id> <visible>`  
Set solo status: `/mapmap/mapping/solo ,ii <id> <solo>`  
Set lock status: `/mapmap/mapping/locked ,ii <id> <locked>`  
Adjust depth (layer order): `/mapmap/mapping/depth ,ii <id> <depth>`

## Regular expressions

Paint and mapping ids are hard to remember and manipulate. Alternatively, one can use a string pattern describing a regexp over the paint or mapping names. The regular expression follows a [simple "file globbing" / wildcard syntax](http://doc.qt.io/qt-5/qregexp.html#wildcard-matching). It is case-sensitive.

### Examples

Change opacity of paint named "movie.mov" to 0.1 (10%):  
`/mapmap/paint/opacity ,sf "movie.mov" 0.1`

Change opacity of all paints whose name begins by "movie-" to 0.5 (50%):  
`/mapmap/paint/opacity ,sf "movie-*" 0.5`

Rewind all .mov paints:  
`/mapmap/paint/rewind ,s "*.mov"`

Set all mappings that begin with "mesh-" followed by a single digit to solo mode:  
`/mapmap/mapping/solo ,si "mesh-[0-9]" 1`

## Playback

Pause: `/mapmap/pause`  
Play: `/mapmap/play`  
Rewind/reset: `/mapmap/rewind`  
Quit: `/mapmap/quit`
