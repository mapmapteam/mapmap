Contribute to MapMap
====================

Build software, doc and the translations
----------------------------------------
See INSTALL.

You will need to install markdown to build some of the documentation that comes with the software.

Coding style
------------
* indent with 2 spaces
* opening curly braces on a new line
* function and method names camelCase, with lowercase first letter
* class names CamelCase
* private data members with an underscore as a prefix: _likeThis
* file names all lowercase
* always add spaces between operators such as +, -, /, * casts, etc. (except pointers and references)

Version numbers
---------------
We use Semantic Versioning 2.0.0. Given a version number MAJOR.MINOR.PATCH, increment the:
* MAJOR version when you make incompatible API changes,
* MINOR version when you add functionality in a backwards-compatible manner, and
* PATCH version when you make backwards-compatible bug fixes.
See http://semver.org/

We want the 0 series to be backward-compatible. That means that a project created with MapMap 0.1 should still work with MapMap 0.99, if we ever get to such a version number. When we will break this backward-compatibilty, we will start the 1 series and provide a migration script. This script might be ran from the command-line or on the Web. The other thing we need to be backward-compatible is the OSC interface. The rest of the software can change. This includes the names of the menu items and the appearance of the user interface. We will try to keep these consistent, though.

Files
-----
* HACKING: What to know to contribute to the project.
* INSTALL: Instructions to build and install the software.
* NEWS: Release notes for each tag.
* README: The short documentation at the root of the project
* TODO: You can put things to do there, or use https://github.com/mapmapteam/mapmap, or https://www.pivotaltracker.com/s/projects/954570/
* images/: Images part of the GUI.
* prototypes/ : Contains prototypes. Please ask the project maintainer before removing some of these.
* docs/informations/*.md Markdown files to generate the About dialog

Code management with Git
------------------------
* We develop in the develop branch. The master branch is only for the latest tag.
* Create a branch for each new features. Merge it to develop.
* Create release-x.y.z branch for releases.
* If possible, avoid merges, and use git rebase instead. (like in the GNOME projects)
* Create a x.y branch for each x.y major.minor version series.
* For a new release, merge develop to master (or your branch to the major.minor branch if it's a bugfix release) and then tag it there.
* In doubt, ask the release manager.

Make a release
--------------
* If it's a new feature, increment minor. If it's bugfix, increment micro. If it's not backward-compatible, increment major.
* Create a release-x.y.z branch
* Verify the version number:
  * vim -o VERSION.txt DMGVERSION.txt mapmap.pro docs/Doxyfile NEWS src/core/MM.cpp
  * VERSION.txt
  * VERSION in MM.cpp
  * VERSION in mapmap.pro
  * PROJECT_NUMBER in Doxyfile
* Edit NEWS - update with the news for the release you are about to make
* Run ./scripts/update-changelog.sh and commit the changes
* Maybe update the docs/informations/osc.md file and run scripts/update-osc.sh and commit the changes
* Maybe update the docs/informations/CONTRIBUTORS.md file and run scripts/update-contributors.sh and commit the changes
* run make
* run ./sh_build_doc.sh
* merge to master, or to the major.minor branch
* run ./sh_make_tarball.sh
* then untar it, cd to that dir and build the whole thing to see if it works
* git tag x.y.z
* go back to your release branch
* increment micro (or minor) version number in the files above.
* merge into develop.
* keep developing.

XML file version number
-----------------------
* update its minor number when you introduce new features.
* update its major number when it's not backward-comptatible anymore with its previous versions
* generally, we should follow the MapMap version, when new changes are introduced. (no need to increment it otherwise)
* we will need to implement some fancy XML file version number checking in the future.

Qt resources system
-------------------
* The mapmap.pro file is where the packaging is done
* The mapmap.qrc file is where we specify which resources are packaged with the app.
* Images are set there. They are then available as a path-like alias such as ":/fullscreen"
* See http://doc.qt.io/qt-5/resources.html

