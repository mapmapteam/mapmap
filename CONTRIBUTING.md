# Contribute to MapMap

Thank you for contributing! MapMap is a community-built, maintained and operated project that welcomes contributions from everyone. Please read our [code of conduct](CODE_OF_CONDUCT.md)

This document outlines the procedures and what to expect when contributing a bug report, a feature request, or new code via a pull request.
If you want to contribute documentation have a look at the [Wiki](https://github.com/mapmapteam/mapmap/wiki). 
To contribute tutorials or help documents, get in touch with the _Documentation leader_, [Alexandre Quessy](https://www.artpluscode.com/) (aalex).

**Please read** and **follow these guidelines** before submitting a bug report, feature request or pull request.
It really helps us efficiently process your contribution!

**Table Of Contents**  
[Report a bug](#bug-reports)   
[Request a feature](#feature-requests)   
[Contribute code](#contributing-code)  
[Coding style](#coding-style)  
[Building](#building)  
[Version numbers](#version-numbers)  
[Files overview](#files-overview)  
[Code management with Git](#code-management-with-git)  
[Make a release](#make-a-release)  
[Project file version number](#project-file-version-number)  
[Qt resources system](#qt-resources-system)  

 ## <a id='bug-reports'></a>Report a bug

We love hearing about bugs! It's how we get them fixed. 

- If you notice something odd happening, try to make it happen again (reproduce it). 
- If you can reproduce it, try to figure out if it's caused by the video format that you use or by MapMap. 
- If it's caused by MapMap, make sure it **still occurs in the current development version** (where we continually fix problems) by checking out with git to the current `dev` branch of MapMap.
- If it looks like it's caused by MapMap (or if you're not sure) and still occurs on the current `dev` branch, submit a bug report to the [issue tracker](https://github.com/mapmapteam/mapmap/issues).  
  - **Search the issue tracker** to make sure your problem has not been reported yet. If you find a relevant bug, comment there, even if it's an old or closed one! 
  - Only if you don't find a relevant issue, open a new issue. A new issue does not generate more exposure/visibility than commenting on an existing one.
  - Make sure you give it a good title! A good title explains the core of the problem in about 5-10 words. (It's sometimes easier to write the title after you've written the description.)
  - In the description, include the following details:  
    1. **relevant system information** such as which MapMap version, operating system, Qt version and IDE you are using,
    2. what you were doing when you noticed the bug,
    3. what you expected to happen,
    4. what actually happened, and how to make it happen again (ie how to reproduce the bug).
  - If you can demonstrate the bug in a few lines of code, include this code in the description. 
  But please __don't__ copy and paste your entire source file.
- Someone will be along to review your bug report and assign tags to it.
You might be asked some questions about it, or you might not. 
If you don't hear back for a while don't despair, it's not being ignored!
We're simply a busy group of people, but you will hear back eventually.


## <a id='feature-requests'></a>Request a feature

We already have a formal [roadmap](https://github.com/mapmapteam/mapmap/wiki/RoadMap) for the project but nevertheless we are a community of people who each contributes to sections that we feel are important for the project.
Feature requests are therefore mostly a way of us discussing/feeling out together where we'd like the project to go. 
This can sometimes involve a lot of discussion, as everyone uses MapMap differently. You can join the [MapMap Slack team](https://mapmap.slack.com/) and use the `#feature` channel channel for general questions or discussion about new features

Feature requests are created as Github Issues, just like bugs.
Feature requests are also where code that you or anyone else would like to include in a future pull request is **discussed before being implemented!**
If you're writing code to add a new feature that you think would be awesome to have in the core, that's great! 
But please make sure it's been discussed as a feature request _before_ you submit your pull request, as that increases the chances that your pull request will be accepted. 

Before opening a feature request, please search the issue tracker and confirm an existing request touching the same topic doesn't already exist.

- We are generally a friendly and open collection of people, but communication over the internet can be difficult, so please don't treat a lot of discussion as a negative point against your feature request. 
Usually a lot of discussion just means that we haven't thought about what you're requesting before, which is a good thing!
- On the other hand, we are also busy people, often professionally involved in making large scale projects ourselves, using MapMap. 
If you feel like no-one is paying attention to your feature request, just be patient, it will be considered eventually.
- And finally, a small reality check: please don't expect that a general agreement in the discussion that a feature request is a good idea means it will get made immediately! 
As with most open source projects, the fastest way to get a feature made is to make it yourself, or if you are not a programmer, make friends with someone who is, introduce them to MapMap, and ask them to make it for you.

## <a id='contributing-code'></a>Contribute code

We are more likely to accept your code if we feel like it has been discussed already. 
If you are submitting a new feature, it's best if the feature has been discussed beforehand, either as a [feature request](#feature-requests) or on the [Slack](https://mapmap.slack.com/).

- Please read the [coding style](#coding-style) section below and make sure your code conforms to it.
If in doubt, try and match the style and practices you find in the code you are working with.
- Please write _descriptive commit messages_ for each of the commits that you make.
They don't have to be in-depth, just a brief summary of what the commit contains. A page describing how well-written commit messages look like can be found [here](http://tbaggery.com/2008/04/19/a-note-about-git-commit-messages.html).

#### Organising your code


- Submit from a dedicated branch on your own repository **branched off from current `dev`**. Your branch should be only about a single topic or area of MapMap. 
If you have multiple things to submit, make separate branches for each topic and submit multiple pull requests. 
(This makes it easier to review different parts of your code separately, and get it into the core faster.)
- The branch name should start with either __feat-__ for features or __fix-__ for bug fixes, followed by the number of the GitHub issue when there is one.
  - For example, if your patch adds code to draw ellipses, your branch should be called something like __feat-draw-ellipses__, or __fix-542-reopen-camera-project__ for a fix to issue 542.
  - Remember, _commit early, commit often_ - use commits to isolate small subsets of code. 
This granularity makes the code easier to deal with in cases where some things have to be modified/isolated/removed from the pull request.
- When you commit your files and you find you can't do that without using `git add -f/--force`, this is because of the existing gitignore patterns. _Think about if those files really should be in the repo in the first place_. Then, instead of force-adding files which really should be in the repo (i.e. incorrectly match a gitignore pattern), correct the gitignore pattern (ask for help if necessary) and commit normally.
- Don't mix code style/formatting changes with bugfixes or features. They should be separate commits or better still, separate branches and separate pull requests.
- If you are able to do so, test your code on different platforms before submitting it, but at least test it on your platform to make sure it doesn't break anything.

#### Submitting the pull request

- Submit your pull request to the __`dev`__ branch of MapMap (which you branched off from), _not_ the `master` branch.
- All pull requests that contain changes that need to be in the changelog **must include relevant additions to `CHANGELOG.md`**. Use previous entries as a guide for style/indentation/etc.
- In the comments field on your new pull request, enter a description of everything that the code in the pull request does. 
  - This description is the first contact most of the core team will have with your code, so you should use it to explain why your pull request is awesome and we should accept it. 
  - Reference any issues or bugs in the [MapMap issue tracker](https://github.com/mapmapteam/mapmap/issues) that are relevant to your pull request using `#issue number` notation, eg to reference issue __1234__ write `#1234`.
  - Mention if the code has been tested and on what platform.

## <a id='coding-style'></a>Coding style

* Indent with 2 spaces
* Opening curly braces on a new line
* Function and method names camelCase, with lowercase first letter
* Class names CamelCase
* Private data members with an underscore as a prefix: `_likeThis`
* File names all lowercase
* Always add spaces between operators such as `+`, `-`, `/`, `*`, casts, etc. (except pointers and references)

## <a id='building'></a>Building

See [INSTALL.md](INSTALL.md).

You will need to install markdown to build some of the documentation that comes with the software.

## <a id='version-numbers'></a>Version numbers

We use Semantic Versioning 2.0.0. Given a version number MAJOR.MINOR.PATCH, increment the:
* MAJOR version when you make incompatible API changes,
* MINOR version when you add functionality in a backwards-compatible manner, and
* PATCH version when you make backwards-compatible bug fixes.
See http://semver.org/

Version 1.0 is not backward-compatible with the 0 series: projects are now saved as JSON files, and the XML projects of MapMap 0.6 and earlier cannot be opened. `scripts/convert-xml-project.py` converts those of MapMap 0.4 to 0.6. Pre-releases are named like `1.0.0-beta.1`.

## <a id='files-overview'></a>Files overview

* AGENTS.md: How to build, test and work in this repository; written for coding agents, useful to everyone.
* CHANGELOG.md: Release notes for each version. It is shown in the About dialog.
* CONTRIBUTING.md: What to know to contribute to the project.
* INSTALL.md: Instructions to build the software.
* OSC: The OSC messages that control MapMap.
* README.md: The short documentation at the root of the project.
* resources/: Images, icons, translations and style sheet of the user interface.
* docs/informations/: Texts shown inside the software (OSC commands, keyboard shortcuts, contributors).

## <a id='code-management-with-git'></a>Code management with Git

The rules are in [AGENTS.md](AGENTS.md), section "Git rules". In short:

* We develop in the `dev` branch. The `master` branch only receives released versions.
* Create a branch for each feature or fix, from an up-to-date `dev`, and open a pull request to `dev`.
* Rebase, never merge. Pull requests are integrated with "Rebase and merge".
* Every commit builds and passes the tests.
* In doubt, ask the release manager.

## <a id='make-a-release'></a>Make a release

* If it's a new feature, increment minor. If it's bugfix, increment micro. If it's not backward-compatible, increment major.
* Set the version number in `mapmap.pro`: `VERSION` (numbers only) and `MAPMAP_VERSION` (the name shown to users, such as `1.0.0-beta.1`).
* Add the section of the release at the top of `CHANGELOG.md`.
* Maybe update `docs/informations/osc.md`, `docs/informations/keyboard-shortcuts.html` and `docs/informations/CONTRIBUTORS.md`.
* Open a pull request to `dev` and wait for the automated builds to pass on macOS, Ubuntu and Windows.
* Once it is integrated, tag that commit of `dev` with the version number.
* Publish the files built for that tag on the [releases page](https://github.com/mapmapteam/mapmap/releases).

## <a id='project-file-version-number'></a>Project file version number

* Update its minor number when you introduce new features.
* Update its major number when it's not backward-compatible anymore with its previous versions.
* Generally, we should follow the MapMap version, when new changes are introduced. (no need to increment it otherwise)

## <a id='qt-resources-system'></a>Qt resources system

* The mapmap.pro file is where the packaging is done
* The `.qrc` files (`main.qrc`, `resources/interface.qrc`, `docs/documentation.qrc`) are where we specify which resources are packaged with the app.
* Images are set there. They are then available as a path-like alias such as ":/fullscreen"
* See https://doc.qt.io/qt-6/resources.html
