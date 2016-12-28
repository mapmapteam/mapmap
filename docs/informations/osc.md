# Currently supported commands

## Paints
Paint ids are numbered from 1 to N.

- /mapmap/paint/name ,is <id> <name>        Change name of paint.
- /mapmap/paint/opacity ,if <id> <opacity>  Change opacity of paint. Range: [0,1]

### Color paint

- /mapmap/paint/color ,is <id> <color>      Change color of paint. (eg. "#ff0000")

### Image/Video paint

- /mapmap/paint/rate ,if <id> <rate>        Change rate (speed) of paint Range: [0,1]
- /mapmap/paint/uri ,is <id> <uri>          Change URI of paint. (eg. file:///home/example/example.mov)
- /mapmap/paint/volume ,if <id> <volume>    Change audio volume of paint. Range: [0,1]
- /mapmap/paint/rewind ,i <id>              Rewind paint

## Mappings
Mappings are numbered from 1 to N.

- /mapmap/mapping/depth ,ii <id> <depth>      Change depth of mapping. (layer order)
- /mapmap/mapping/locked ,ii <id> <locked>    Change lock status of mapping (0 or 1)
- /mapmap/mapping/name ,is <id> <name>        Change name of mapping
- /mapmap/mapping/opacity ,if <id> <opacity>  Change opacity of mapping. Range: [0,1]
- /mapmap/mapping/solo ,ii <id> <solo>        Change solo status of mapping (0 or 1)
- /mapmap/mapping/visible ,ii <id> <visible>  Change visibility status of mapping (0 or 1)

## Regular expressions
Alternatively to ids, one can use a string pattern describing a regexp over the paint or mapping names. The regular expression follows a [simple "file globbing" / wildcard syntax](http://doc.qt.io/qt-5/qregexp.html#wildcard-matching). It is case-sensitive.

Examples:

- /mapmap/paint/opacity ,sf movie.mov 0.1   Change opacity of paint named "movie.mov" to 0.1 (10%)
- /mapmap/paint/opacity ,sf "movie_*" 0.5   Change opacity of all paints whose name begins by "movie_" to 0.5 (50%)
- /mapmap/paint/rewind ,s "*.mov"           Rewind all .mov paints
- /mapmap/mapping/solo ,s "mesh_[0-9]" 1    Sets all mappings that begin with "mesh_" followed by a single digit to solo mode

## Playback

- /mapmap/pause                 Stop playback
- /mapmap/play                  Start playback
- /mapmap/quit                  Quit MapMap
- /mapmap/rewind                Rewind/reset
