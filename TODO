To do in MapMap
===============

TODO before 0.2.0
-----------------
* change xy position to integers?
* "avec le soutien de l'Organisation internationale de la Francophonie", with the logo.
* integration des icones de mike
* shmsrc???? non... on a plutot autre chose.. c'est a dire, plus de formes.
* traduction complete. multilinguisme fonctionnel.
* video upside-down fixed
* manuel de base, bilingue. (en HTML dans le Contents?)
* save media URI when it's changed via OSC
* remove name? (and re-add it soon)
* allow the user to change the OSC port number user a menu item and a dialog.
* be able to change the media source
* is it really the closest vertex that is dragged?
* how come paint quads don't have a dimension?
* create more number boxes for points when we change the dimension of a quad texture mapping.
* Be able to cancel a change when moving around a mapping.
* Be able to change the color of a color paint.
* Be able to undo deleting a mapping, and a paint.
* implement name : dans l'inspecteur

Code
----

* MapperGLCanvas has a strange name considering we also have a Mapper class; actually I'm thinking we should rename Mapper to MappingView or something (since a Mapper is actually the View of the Mapping class)
* DONE Override includesPoint() in Mesh
* video gstreamer

Packaging
---------
* delete old branches
* update doc
* does translation work?

Fonctionnalités importantes manquantes
--------------------------------------
En plus du support de la lecture de fichiers vidéo, (et de la mémoire partagée) sur la quelle je travaille, voici les éléments à faire que j'ai notés. Certains d'entre eux sont critiques.

* FIXME: Lorsque les images sont très grandes, il est impossible de saisir les coins de sa source, ou de sa destination. Pour cela:
* TODO: Clic-centre pour se déplacer dans la scène au complet.
* TODO: Clic-drag gauche pour déplacer un objet. (et non clic-drag droit)
* TODO: Clic-gauche pour sélectionner un objet.
* TODO: Clic-droit devrait faire apparaître un menu contextuel. (effacer, masquer, etc.)
* TODO: Il faudrait qu'on puisse ne voir aucun contour bleu autour des mappings.
* FIXME: La fenêtre de sortie devrait pouvoir être plein-écran.
* TODO: La roulette de la souris devrait permettre de zommer/dézoomer dans la scène.
* TODO: On devrait pouvoir changer une couleur. (double-clic clic-droit sur la couleur)
* TODO: Afficher davantage d'aide contextuelle quand on survole un élément de l'interface.

Bogues majeurs
--------------
* BUG: Le focus devrait revenir sur la fenêtre principale quand on ferme le dialogue de sauvegarde.
* BUG: Coin de quads croisés quand on load.
* BUG: Plante que redimensionne une source cercle d'une image. (mais pas quand on roule sous GDB?)
* BUG: L'inspecteur, ou un des panneaux, peut être caché si l'usager en a rangé le tiroir.

Trucs à faire plus tard
-----------------------
* LATER: Si on déplace les coins d'une source au-délà de la texture, mettre du noir plutôt que clamper la texture.
* LATER: Soft edge sur les polygones.
* LATER: Supporter Syphon sur Mac OS X.
* BUG: Les textures des rectangles ont une apparence étrange. (deux zones triangles)

Fonctionnalités cool à faire plus tard
--------------------------------------
* lignes des quads assignable on/off, voire animales, en midi
* animation de stroke par groupe
* OSC pour controler les positions et l'alpha des quads

OSC INTERFACE ADDITIONAL CALLBACKS
----------------------------------
Instead of using boolean values (true or false) we use numbers, where 0 means false, and any positive number means true.

You should make sure to block all incoming messages on that port if you don't want to be hacked during a show.

/mapmap/mapping/move/xy ,iff <mapping identifier> <x> <y>
/mapmap/mapping/vertex/source/xy ,iiff <mapping identifier> <vertex index> <x> <y>
/mapmap/mapping/vertex/destination/xy ,iiff <mapping identifier> <vertex index> <x> <y>
/mapmap/mapping/visible ,ii <mapping identifier> <enable>
/mapmap/mapping/highlight ,ii <mapping identifier> <enable>
/mapmap/mapping/vertex/highlight ,iii <mapping identifier> <vertex identifier> <enable>
/mapmap/project/load ,s <file>
/mapmap/project/save ,s <file>
/mapmap/paint/color/rgba ,iffff <paint identifier> <red> <green> <blue> <alpha> (each channel within [0,1])
/mapmap/paint/media/load ,is <paint identifier> <file>
/mapmap/paint/media/speed ,if <paint identifier> <speed ratio> (1.0 means 100% speed)
/mapmap/paint/media/seek ,il <paint identifier> <time position> (in milliseconds)
/mapmap/output/fullscreen ,i <enable>
/mapmap/output/size ,ii <width> <height>
/mapmap/output/position ,ii <x> <y>


Roadmap (to do)
---------------
* Change default OSC port. (currently 12345)
* Allow to change the default OSC port via the GUI.
* Reorder layers.
* MIDI support.
* Art-Net support.
* Make output window fullscreen.
* Port to Microsoft Windows 8.
* Port to iOS.
* Port to Android.
* Support multiple output windows.
* Chroma-key effect.
* Luma-key effect.
* More control via OSC.
* Live video capture. (camera)
* Syphon support on Apple OS X.
* Shared memory support on all operating systems.
* Transition between video clips.
* Anti-aliasing.
* Multiple cues for video files playback in a project.
* Multiple layouts in a project.
* Reorder mappings.
* Sample OSC clients: Python, bash, Pure Data, Processing.
* Allow to start the software with no GUI.
* Support OSC via TCP.
* Implement a JSON REST interface.
* Bézier curves.
* Pause and resume media files.
* Support RTSP URI.
* Support audio.






Fiche technique
===========
















