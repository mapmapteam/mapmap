# UX charter

The principles that guide every choice in the interface of MapMap: where a control goes, what it looks like, what it is called, what the mouse and the keys do. It is for anyone who adds or changes something a user sees or touches.

This charter says why and where. The exact gestures and keys will be listed in an interaction guide that follows this charter, written once the charter is approved.

Status: proposal of 27 November 2026, not yet approved.

**What this charter does not decide.** It is about what the user sees, names and touches. It does not say how things are built: how pictures are rendered, what the project file holds, how extensions are loaded, how the code is organized, which widget or file format is used. Where a principle seems to force such a choice, the principle is the part to keep, and the choice stays open for the design of each feature.

A charter says what is aimed at, not what is wrong today: the places where MapMap does not follow it are listed and fixed in the slices of the roadmap. Some rules are followed by an example and a counter-example, where the rule alone could be read in two ways.

## Preamble: the thinking behind this charter

This charter is written from the seat of a designer who looks at MapMap as it is, compares it with what its users already know, and proposes rules. It rests on six ideas. They are few on purpose: when two rules of this document seem to disagree, these decide.

**1. People already know how to use MapMap, from somewhere else.** A user spends most of their time in other software, and brings their hands with them (this is known as Jakob's law). So the first question is never "what would be best?" but "what do they expect?". We look, in this order, at the operating system they are on, at the tools of our public (TouchDesigner, Resolume, MadMapper, Millumin, Max, After Effects, Photoshop, Illustrator), then at MapMap itself. We invent only what none of them has, and where we depart from them we say why. For someone who has never done mapping, "somewhere else" is a web browser, a phone, a drawing program: this is why the operating system comes first in that order.

   *Example:* the arrow keys move what is selected by 1 pixel, and by 10 with Shift, as in Photoshop, Figma and Resolume. *Counter-example:* a way to move the view that none of the tools of our public has, however natural it seems to its author.

**2. A target is reached faster when it is closer and when it is bigger** (Fitts's law). The time needed to reach a target grows with its distance and shrinks with its size.

```
   far and small: slow              near and big: fast
   +---------------------------+    +---------------------------+
   |                      [x]  |    |                           |
   |                           |    |        +---------+        |
   |                           |    |     ^  | target  |        |
   |  ^                        |    |        +---------+        |
   +---------------------------+    +---------------------------+
```

What it means in practice:

- **Make important targets larger.** What is used most, or matters most, gets a generous size: it is reached faster and missed less.

- **Place related controls close together**, and near where the user is already working or looking. The closest place of all is under the pointer, which is what a context menu is for; a shortcut needs no travel at all.

- **Use the edges and corners of the screen.** The pointer cannot go past them, so a menu bar or a toolbar against an edge is fast to reach.

- **Leave enough space between targets.** Targets packed tightly lead to the wrong one being pressed, more so on a touchpad, in the dark.

- **Keep a command that deletes away from the commands used most.**

*Example:* the handle that rotates and scales a layer sits next to that layer, joined to it by a line. *Counter-example:* a small delete button on every line of a list, between the buttons that hide and lock.

**3. To recognize is easier than to remember, and a short list is read faster than a long one** (Hick's law). Commands are written out in menus rather than hidden in gestures; menus are short and grouped; what the user sees tells the state without a click.

**4. Hands on the thing, not on a form.** The user grabs what they want to change and sees the result at once on the projection (direct manipulation). Panels of numbers exist for precision, not as the way in.

**5. Mistakes are cheap.** Everything is undone in one step, so nothing needs to be confirmed, and trying is safe. In front of an audience, the cost of a mistake is public: a slip must never do something that cannot be put back at once.

**6. One model on three operating systems, with the manners of each.** MapMap behaves the same on GNU/Linux, macOS and Windows: same gestures, same words, same places. It takes from each operating system only what that one fixes for all its software: where the menu bar is, Cmd in place of Ctrl, the file dialogs, the order of buttons in a dialog, the standard shortcuts. A gesture that one of the three cannot do, or keeps for itself (Alt and drag on some Linux desktops, Ctrl and click on macOS), is not used on any of them.

Two more ideas follow from these and come back often: **simple first, without a ceiling** (what a beginner needs is in sight; the rest is one step away, never in the way, and never missing), and **the same thing looks and works the same everywhere** in the window.

## Who we design for

**The user.** Two kinds of people, and MapMap is for both. Someone who has never done video mapping: a child in a school group, a participant in a workshop, a student in a first class. And someone who does it for a living: a video artist, a VJ, a stage or installation technician, who knows at least one of TouchDesigner, Resolume, MadMapper, Millumin, Max, After Effects, Photoshop or Illustrator. Neither reads a manual before starting.

MapMap follows the model of Processing and Arduino: a tool built with teaching in mind, that is learned in an hour, and that stays a real tool in expert hands, through its extensions and through what it exchanges with other software. The interface must never make one of the two pay for the other: nothing a beginner needs is hidden behind what an expert knows, and nothing an expert needs is removed to look simple.

**The projection surface.** What MapMap projects on is rarely a screen: a wall, but also a sculpture, a stage set, a building, a pile of boxes, a body. This charter calls it the projection surface. It is where the work is, and where the eyes are.

**The situation.** A dark room. A laptop on a flight case or on a knee, often with only its touchpad. The eyes are on the projection surface, not on the screen of the laptop. Time is short: the audience comes in at eight.

**Four ways to use it**, and MapMap is for all of them:

- **An installation**: everything is set up beforehand, then MapMap runs alone, for an evening or for months. What matters is that the setup is fast and exact, and that nothing happens afterwards.

- **A live performance**: someone plays MapMap in front of an audience, changing sources, layers and effects as the show goes. What matters is that every action is immediate, can be done without looking for it, and cannot go wrong in a way the audience sees.

- **A workshop**: a group discovers video mapping, often for the first time, in a few hours or a few days, and often ends with a small show. Ages and backgrounds vary widely. It is cultural mediation: school groups with KidZLab and LibreMapping, projects of Perte de Signal; workshops of several days held in Africa with local participants. What matters is that the first projection on an object comes within minutes, that what is on the screen can be understood without the words of the trade or a good command of the language, that a mistake costs nothing, and that it works on whatever computers the group has.

- **A classroom**: MapMap is taught at university, as a tool that is learned fast and then used for real work. What matters is that what is learned first stays true later, that the same lesson works on the three operating systems of the students, and that the way to go further (extensions, OSC, other software) is in sight.

Every rule below comes from these users and these situations. When a rule does not answer a question, go back to them.

## Principles

1. **The projection comes first.** The output shows the work and nothing else. No menu, dialog, message or pointer appears on the projection surface unless the user asked for it. Whatever helps the setup (controls, test signal) is switched on and off by the user.

2. **Act on the thing itself.** To change a layer, the user grabs the layer. No tool to choose first. A panel of numbers gives the same change with precision; it never is the only way to do a common task.

   A mode is accepted only when all three are true: it is shown on the object it changes; it is switched with one key or one click; it never changes what a click somewhere else does.

   *Example:* the handle of a layer rotates, scales or does both; its picture says which, and a click on it, or three buttons side by side, switch. *Counter-example:* a "Move" tool and an "Edit vertices" tool in a toolbar, one of which must be chosen before touching a layer.

3. **Nothing is lost.** Every change can be undone, and one gesture is one step of the undo history. Because of that, MapMap does not ask "Are you sure?". The only questions are about what undo cannot bring back: closing a project that is not saved, overwriting a file.

   *Example:* Delete removes the layer at once, and Undo brings it back. *Counter-example:* "Delete this layer? Yes / No".

4. **Safe in front of an audience.** Nothing interrupts the output: no dialog that stops the rendering, no update or message that takes the keyboard. An error is told in the window, never on the projection surface. In a live performance the user acts during the show: what they do takes effect at once, and a key pressed by mistake must not ruin the image. A single key never deletes, closes or resets without undo.

5. **One thing, one name, one place, one look.** A layer is called a layer everywhere. An action has one label, one icon and one shortcut, whether it is in a menu, a toolbar or a context menu. An icon means one action only.

   *Counter-example:* "Delete Layer" in a menu and "Delete layer item" in its tooltip; the same picture on three different buttons.

6. **Everything can be found without knowing it.** The menu bar lists every command of the software, with its shortcut. A gesture, a shortcut, a toolbar button and a context menu are faster ways to a command that the menu bar also has.

   *Counter-example:* a command that exists only as a toolbar button, or only as a gesture.

7. **Conventions before inventions.** For any choice, look in this order: what the operating system does (macOS, Windows, GNOME and KDE guidelines); what most of the tools our users know do; what MapMap already does elsewhere. Invent only when none answers, and write down why.

8. **Things stay where they are.** A control does not move, appear or disappear depending on what is selected. What cannot be used is greyed out, in its place. The window opens as the user left it.

9. **Enough with a laptop alone.** Everything can be done with a touchpad and a keyboard without a numeric keypad, on GNU/Linux, macOS and Windows. A mouse, a second screen and a MIDI controller make it faster; none is needed.

10. **Show, then tell.** The state of things is visible without clicking: what is selected, locked, hidden, playing, in error. The pointer changes before the press to say what a drag will do.

   *Example:* the vertex under the pointer lights up before the press. *Counter-example:* finding out which vertex was caught by seeing it move.

11. **The interface and remote control are two doors to the same room.** What can be changed by hand can be changed from outside (OSC, MCP, a controller), and it carries the same name in both.

## Vocabulary

Use these words, and only these, in labels, tooltips, messages, documentation and OSC addresses.

| Word | Meaning | Do not write |
|---|---|---|
| Source | A video, an image, a camera, a color, a Syphon input | media, paint, texture |
| Layer | A shape that shows a part of a source on the output | mapping, item |
| Group | A named set of layers, and of other groups, that is handled as one: hidden, soloed, locked, moved, faded together. See "Groups and effects" | folder, container, composition |
| Effect | Something that changes the picture of a source, a layer or a group: an adjustment, a filter, a shader. Several can be stacked. See "Groups and effects" | filter, FX, shader, plugin |
| Shape | The geometry of a layer: mesh, triangle, ellipse | |
| Vertex | A point of a shape | control point, node, corner |
| Handle | The control that rotates and scales a layer | |
| Input editor | Where the part of the source is chosen | source view, input view |
| Output editor | Where the layer is placed on the output | destination |
| Output window | The projection itself | |
| Project | The file | mapping project |

Names in the code may differ for historical reasons; a text the user reads may not.

## Layers

The layer is the central thing of MapMap: almost everything the user does is done to a layer. This section describes the layer as the user meets it today, so that the interface explains it well; it does not freeze the model. A layer joins three things.

```
       Source                    Layer                  Projection surface
  +---------------+                                    +--------------------+
  |               |    input shape    output shape     |                    |
  |   +------+    |     +------+        ______         |       ______       |
  |   | part |--------> |      | ----> /     /  -------------> /     /      |
  |   +------+    |     +------+      /_____/          |      /_____/       |
  |               |                                    |                    |
  +---------------+   chosen in the   placed in the    +--------------------+
                      input editor    output editor
```

- **A source**: where the picture comes from.

- **An input shape**: which part of that picture is used. It is chosen in the input editor.

- **An output shape**: where that part goes on the projection surface, and how it is bent to fit it. It is placed in the output editor. A vertex of one goes with a vertex of the other.

What follows from it, and what the interface must make plain:

- **A source can feed many layers; a layer has one source.** Changing a source changes every layer that shows it. Deleting a layer never deletes its source.

- **A layer has a kind of shape** (mesh, triangle, ellipse, line), chosen when it is made, and settings of its own (opacity and control points of shapes). A layer of plain color has no input shape: there is no part to choose.

- **A layer has states**: visible or hidden, solo, locked. They are switched often, and each one is always in sight.

- **Layers are stacked.** Where two overlap on the projection surface, one is over the other. The order of the list is the order of the stack, with the top of the list on top.

- **The two editors show the same layer from its two ends.** What is selected in one is selected in the other; the vertex that goes with the selected one is marked.

## Groups and effects

Neither exists in MapMap yet. The charter fixes the two words, and what the user should be able to expect of them. How they are built, where exactly effects can be applied and in what order pictures are composed are decided in the design of each feature.

**Group.** A named set of layers, and possibly of other groups, handled as one: with groups, the layers form a hierarchy. It is the word of Resolume, MadMapper, Photoshop, Illustrator and Figma. "Folder" would say that it only tidies a list, which is not true here: what is done to a group (hiding it, soloing it) is seen on the output.

**Effect.** Something that changes a picture: an adjustment, a filter, a shader that modifies the image. Several can be applied to the same thing, one after the other: they stack. Effects are meant for layers and groups, and possibly for sources.

```
   a hierarchy                        a stack

   Group                              Layer (or group, or source)
     Layer                              effect 1
     Layer                              effect 2
     Group                              effect 3
       Layer
   Layer
```

What the user should be able to expect:

- **A group is treated like a layer wherever that makes sense**: same states, same commands, same look. The user learns layers once.

- **What is done to a group is seen on all it holds**, and the user can tell whether a layer is hidden by itself or by its group.

- **An effect behaves the same wherever it is applied**: same controls, same way to add, order, switch off and remove.

- **The order that is seen is the order that is applied**, for layers in a group and for effects in a stack.

- **"Shader" is how something is made, not a word the user needs.** The user meets effects, sources and shapes. Which of these a given kind of shader becomes (a fragment shader, a vertex shader) is an open question for the design of effects; the charter only asks that it lands under one of the words of the vocabulary, or gets a word of its own.

## The window

The window is made of areas, and each kind of area holds one kind of content. The plan below is an example of what it could look like, not a prescription: areas can be moved, resized, hidden or laid out otherwise. What is fixed is the role of each area and the principles that follow the plan.

```
 +-----------------------------------------------------------------------+
 | Commands: all of them (menu bar), the frequent few (toolbar)          |
 +-------------+-----------------------------------------+---------------+
 |             |                                         |               |
 | Sources     |  Editors                                | Layers        |
 |             |                                         |               |
 | what comes  |    input editor                         | what goes on  |
 | in          |    output editor                        | the surface,  |
 |             |                                         | in order      |
 +-------------+    the picture, and the hands on it     +---------------+
 | Properties  |                                         | Properties    |
 | of the      |                                         | of the        |
 | source      |                                         | layer         |
 +-------------+-----------------------------------------+---------------+
 | Tools: timeline, scenes, prompt, audio, console...                    |
 |        one at a time or side by side                                  |
 +-----------------------------------------------------------------------+
 | Status: what just happened, the numbers of the moment                 |
 +-----------------------------------------------------------------------+

        the signal reads from left to right: source, layer, output

        Configuration (preferences, extensions) is apart from the plan:
        a place opened on demand, see below
```

| Area | What it is for | Does not hold |
|---|---|---|
| Commands | Reaching every command (menu bar) and the few used all the time (toolbar) | A command that is nowhere in the menu bar |
| Sources | Choosing among what comes in: videos, images, cameras, colors | Settings |
| Layers | Choosing among what goes on the projection surface, in the order of the stack, with the state of each | Settings |
| Editors | The picture, and what is grabbed on it. The biggest area | Buttons and fields over the picture |
| Properties | Every setting of what is selected, next to the list it was selected in | Commands that act on something else |
| Tools | What works on the whole project, not on one source or one layer. Some unfold in time or organize the show (a timeline, scenes), some drive it (a prompt, audio), some watch it (the console, levels). A tool can serve a live performance as well as an installation that runs alone | The properties of what is selected |
| Configuration | The settings of the software itself and of its extensions (for example preferences, or the ports of OSC, MIDI and MCP). Opened on demand; not part of the working areas | The settings of a source or a layer; anything changed many times in a session |
| Status | What just happened; the numbers of the moment | The only place where an error is told |

The principles:

- **The editors get the room.** They are the biggest area, and nothing covers them by itself.

- **The window reads like the signal**: what comes in on one side, what goes out on the other, the picture in between. Menus, toolbars and panels follow the same order: source, then layer, then output.

- **One selection for the whole window.** What is selected in one area is selected in the others.

- **A setting is next to what it belongs to**: the properties of a source near the sources, those of a layer near the layers.

- **What a gesture changes is also a number that can be typed**, in the properties.

- **All tools are reached and handled the same way.** A new tool, built in or brought by an extension, is found where the others are and behaves like them: same way to open, to hide and to resize.

- **What is used in front of an audience can be used without touching the setup.** A tool that plays the show does not require opening an editor or a list, and what is only for setup can be hidden.

- **The user can tell what a setting belongs to**: to what is selected, to the project, or to the software on this computer. Each kind has its place, and a setting is in one place only. Which settings travel with a project and which stay on the computer is decided case by case; the interface must make the answer visible.

- **One place for all configuration.** The settings of the software and of its extensions are found together and look alike. Today that place is a window that opens over the main one; its form is open. Whatever its form, it follows the rest of this charter: a change takes effect at once, it never appears on the output, and it does not stop the output while it is open.

- **Every area can be hidden, and comes back as it was.** A beginner sees few areas; an expert opens what they need. The layout is kept from one session to the next.

## Where a command goes

Ask the questions in order. The first "yes" gives the place; the lines below it still apply.

| Question | Place |
|---|---|
| Is it a command at all? | The menu bar, always |
| Does it act on one object (a source, a layer, a vertex…)? | Also the context menu of that object |
| Is it used many times in a session, by most people? | Also the toolbar, and a shortcut |
| Is it a state of one object that is switched often (visible, solo, locked)? | Also a button on its line in the list |
| Is it a value of one object? | The property panel, not a menu |
| Is it a habit of the user, the same for every project (theme, language, wheel)? | Configuration |
| Is it a fact about this installation (output size, screen)? | With the project, in a menu or a panel |

A command that changes or deletes an object is never only a button on a line of a list: one slip of the finger is enough to press it.

Three examples:

- **Solo a layer**: a command on one object, switched often. Menu bar, context menu of the layer, button on its line, shortcut.

- **Opacity of a layer**: a value. Property panel only; not a menu, not a dialog.

- **"The wheel zooms"**: a habit of the user, the same in every project. Configuration, not the project.

## Menu bar

- **Menus by object, in the order of the signal**: after the standard File, Edit and View, one menu for each kind of thing the user handles (for example Source, Layer, Output), then Window and Help. A command is in the menu of the thing it acts on.

- **A menu has at most about twelve items**, in groups of two to five between separators. A group that grows becomes a submenu; a submenu has no submenu.

- **Labels are verbs with their object**: "Duplicate Layer", "Add Triangle Layer". An item that switches something says the state, without a verb, and carries a check mark: "Full Screen", "Test Signal", "Sticky Vertices"; not "Toggle…", "Show…", "Display…" or "Enable…".

- **An ellipsis (…) ends the label when the command needs more input before it acts** (a file to choose, a size to give). It is the character "…", not three dots.

- **Greyed out, not removed.** A command that cannot be used now stays in its place, disabled. A command that does not exist on an operating system (Syphon outside macOS) is absent there.

- **Follow the operating system for the standard items**: on macOS, Preferences, About and Quit are in the application menu.

## Context menus

- **A context menu is about what is under the pointer**, and it is the same menu for the same object wherever it is clicked: a layer in the list, in the output editor or in the input editor.

- **It holds only commands on that object**, taken from the menu bar with the same labels, in the same order. Nothing lives only in a context menu.

- **One right click opens one menu.** Where two objects are under the pointer (a layer and its source), the menu is that of the topmost one, and it gives access to the other through a submenu.

- **The right button does nothing else.** A right click on something not selected selects it first, then opens its menu. Outside any object, the menu is that of the editor (zoom, fit, controls).

- **Short**: at most about ten items. The commands used most come first; deleting comes last, after a separator.

```
        pointer
          \
           +----------------------+
           | Duplicate    Ctrl+D  |   most used first, nearest to the pointer
           | Rename           F2  |
           | Change Source      > |
           |----------------------|
           | Raise                |
           | Lower                |
           |----------------------|
           | Lock                 |
           | Hide                 |
           | Solo                 |
           |----------------------|
           | Delete          Del  |   what destroys: last, apart
           +----------------------+
```

## Toolbar

- **Only commands used many times in a session**: adding sources and layers, playback, full screen, test signal. Ten to twelve buttons at most. Each new button must replace one or prove that it is used more than the least used one.

- **Grouped in the order of the signal**, with separators: sources, layers, output, playback.

- **Two states, one button.** Play and pause are one button that changes; so is full screen.

- **Each button has a tooltip** made of the label of the command and its shortcut, in a few words: "Add Mesh Layer (Ctrl+M)".

## Icons

**When an icon, when a word, when a list.**

| Use | When |
|---|---|
| An icon alone | The picture is known to everyone (play, pause, lock, eye, zoom, full screen), or it draws the result itself (triangle, ellipse, mesh), and the control is pressed often. Always with a tooltip |
| An icon with a word | A command used often whose picture must be learned (add a color source, test signal) |
| A word alone | Everything else, and every command used rarely. A word that is read is better than a picture that is guessed |
| Buttons side by side | Two to five choices that exclude each other and that the user switches often (the mode of the handle) |
| A drop-down list | More than five choices, or choices that change (screens, cameras, sources), or a choice made once |
| A check box | One setting that is on or off, in a panel |
| A slider with a number field | A value in a range that is adjusted by eye (opacity, volume); the field allows typing it |
| A number field alone | A value that is typed or nudged (position, size, number of columns) |

**How icons look.**

- One family: same line weight, same grid, same corner radius, one color that the theme gives. Color is kept for state (on, selected, error), not for decoration.

- Sharp on every screen, whatever its density of pixels.

- An icon is one action. Three commands do not share a picture; a command without a picture of its own has none.

- A state has two readable pictures (eye open, eye closed; lock closed, lock open), not one picture that is only dimmed.

- Draw objects and results, not metaphors that need a culture or a language. No text inside an icon.

- Size: 16 pixels in lists and menus, 24 in toolbars, on a target of at least 24 by 24 pixels in lists and 32 by 32 in toolbars.

## Mouse and touchpad

The exact gestures belong to the interaction guide. The principles they follow:

- **The left button acts on what is under the pointer**: a click selects, a drag moves. A click and a drag are told apart by distance.

- **The right button opens the context menu, and nothing else.** It does not drag.

- **Moving the view never changes the selection or the project**, and there is a way to do it with a touchpad alone and with a mouse alone.

- **The wheel belongs to the view**, never to a value: it does not change a number under the pointer in a panel unless that field has the focus.

- **One meaning per modifier key, everywhere.** Shift constrains (an axis, a step, a ratio) or takes bigger steps, as in most tools of our public. Whatever the other modifiers are given, each keeps its meaning on every control.

  *Counter-example:* Shift that constrains the rotation on one control and switches snapping on another.

- **No gesture needs a key to be held with a touchpad**, except Shift, Ctrl and Alt: laptops switch the touchpad off while a key is typed. No gesture needs a double click, a long press or a third button without another way.

- **A gesture in progress can be cancelled**, which puts things back as they were.

## Keyboard

- **Standard shortcuts keep their standard meaning** on each operating system: new, open, save, undo, redo, copy, paste, quit, zoom in and out, preferences, full screen.

- **Families of keys.** Each new shortcut joins a family; it does not start one.

  | Keys | Belong to |
  |---|---|
  | With Ctrl (Cmd on macOS) | Commands of the software |
  | Keys without a letter: arrows, Tab, Delete, Backspace, Escape, Space, function keys | The software: the meanings they have everywhere (below), and what is done to the selection or to the view in an editor |
  | Letters and digits, alone or with Shift | The user: they are the keys people assign themselves to a setting, to use during a show. The software never takes them |

  This keeps every learned key working in every version: a later release cannot take a key that a project already uses.

  *Counter-example:* H that flips the selected layer, so that a performer cannot assign H to a setting.

- **A key without Ctrl does nothing while typing** in a field or a dialog, whether it belongs to the software or to the user, and the software's own never does something that undo cannot bring back.

- **The keys that have a meaning everywhere keep it**: arrows move what is selected, by a small step and by a bigger one with Shift; Tab goes to the next; Delete and Backspace delete; Escape cancels or leaves. The steps and the exact keys are in the interaction guide.

- **With Ctrl, the letter is the first letter of the English verb** when it is free (Ctrl+D duplicate), and it does not change with the language of the interface.

- **A command used rarely needs no shortcut**: flipping or rotating a layer by 90° happens once per setup, and its place in the menus is enough.

- **Every shortcut is shown** in its menu item, in the tooltip of its button and in Help > Keyboard Shortcuts. A shortcut that is not in a menu is in that window.

- **No shortcut needs a key that laptops lack** (numeric keypad, Insert, Home, End, Page Up, Page Down) without a second shortcut.

## Feedback

- **Selection** is shown the same way in the lists and in the editors, with one color kept for it. Outlines and vertices are drawn so that they can be seen on any video, light or dark: a light line with a dark edge, never a single color.

- **State is never told by color alone**: a locked or hidden layer has an icon and a changed outline, for the people who do not see colors apart and for projections that wash colors out.

- **The pointer tells what a press will do**: an arrow, a hand over what moves, a closed hand while moving the view, the picture of the mode over the handle.

- **The status bar tells what just happened** ("Layer duplicated") and gives the numbers of the moment (zoom, position of the pointer, frames per second). It is never the only place where an error is told.

- **An error says what happened, why, and what to do**, in plain words, without a code and without blaming: "This video cannot be read: the file is missing. Choose another file…" and not "You can not use this feature!". An error about one object is shown on that object (a mark on its line in the list), and does not stop the rest.

- **Something that takes more than a second shows that it is working**; the output keeps running meanwhile.

## Dialogs

- **A dialog only when the software cannot go on without an answer**: a file to choose, a project that is not saved. Settings are changed in panels, live, with the result visible on the output at once; no "Apply" button.

- **Buttons are verbs that say what they do**: "Save", "Don't Save", "Cancel"; not "Yes", "No", "OK". The safe choice is the default one.

- **A dialog never opens on the output screen** and never while the output is in full screen on the only screen, unless the user asked for it.

- **No confirmation for what undo brings back.**

## Text

- **Menus, buttons and titles use title case** ("Add Mesh Layer"); tooltips, messages and descriptions use sentence case, without a final period when they are one sentence fragment.

- **A tooltip adds something to the label**: what the command does to what, and its shortcut. A tooltip that repeats the label is removed or rewritten.

- **Short, plain, present tense**, with the words of the vocabulary above. No exclamation mark, no jargon of the code (item, mapping, ID), no abbreviation that must be learned (CW, CCW).

- **Every text can be translated**, and must survive being 40 % longer in another language: no fixed width cut to the English word.

- **Numbers have their unit** (pixels, degrees, %), and the same setting has the same unit and the same number of decimals everywhere.

## Look

- **A dark theme by default**: a bright window lights the room and the face of the operator. Contrast between text and background of at least 4.5 to 1.

- **The interface is grey; the color belongs to the content.** One accent color for selection and focus, one for errors, one for "live" states (playing, full screen). Nothing else is colored.

- **One scale of spacings and of text sizes** for the whole interface.

- **Readable at arm's length**: no text under 11 points, no target under 24 pixels.

## Before adding or changing something

1. What is the user trying to do, in the situation described at the top?

2. Does something already do it? Improve it instead of adding a second way.

3. What do the operating system and the tools our users know do?

4. Where does it go ("Where a command goes"), what is it called ("Vocabulary", "Text"), does it have an icon ("Icons")?

5. Can it be undone? Can it be reached from the menu bar, with a touchpad alone, by OSC?

6. What happens in front of an audience if it is triggered by mistake?

7. Add it to the interaction guide and to the shortcut files if it is a gesture or a key, and to `CHANGELOG.md`.

8. Try it by hand: mouse, touchpad, and macOS with Cmd.
