# Learning: drive a setting with a key

MapMap can link a key of the computer keyboard to a setting of a layer or a source: the key then drives the setting, during a show as well as while preparing it. The links are saved in the project.

## Learn a key

1. Choose Edit > Keyboard Learn (<kbd>Ctrl</kbd> + <kbd>Shift</kbd> + <kbd>K</kbd>). The controls one can link are outlined in blue: the eye, the solo and the lock of each layer in the list, and the rows of the inspector that have a position between two ends (a slider, a switch, a menu).
2. Click a control. The status bar asks for a key.
3. Press a key, alone or with modifiers. The status bar confirms: "M → Facade: Visible". The key is written on the outline.
4. Click another control to link another key, or press <kbd>Escape</kbd> to stop learning.

A right-click on a row of the inspector, then "Learn a Key", starts learning with that row already chosen.

MapMap refuses a key it already uses, and names the command: "H is already the shortcut of Flip Horizontal". The keys of the editors are refused too: R, S and T, the arrows, Space, Escape, Delete, Backspace, Tab and Return.

A key drives one setting: learning it for another setting moves it there. A setting has one key for each action (see below): learning another key for it replaces the key that toggles it.

## What a key does

| Setting | The key |
| --- | --- |
| A switch, such as the eye, the solo or the lock of a layer | Turns it on or off |
| A menu | Chooses the next option, then the first one after the last |
| A slider | Sends the value to one end, then to the other |

A key toggles. In Window > Links, the Action menu of a link makes its key always turn the setting on ("Turn on", "Set to maximum", "Last option") or always turn it off ("Turn off", "Set to minimum", "First option"): during a show, a key that only hides cannot show the layer again by mistake. A setting has one key for each action, so that one key can show a layer and another hide it: learn a second key for the same control, then change the action of each in Window > Links.

A key acts once each time it is pressed, not while it is held. It does nothing while one types text (renaming a layer, typing a value in the inspector) or in a dialog. It acts in the output window, full screen on the projector.

A key keeps working on the same letter whatever the layout of the keyboard (French, AZERTY, QWERTY).

## Turn the learned keys off

To type for something else during a show without changing it, choose Edit > Learned Keys Active (<kbd>Ctrl</kbd> + <kbd>Alt</kbd> + <kbd>K</kbd>): the learned keys do nothing until it is chosen again. When the project has links, the status bar says "Learned keys: on" or "Learned keys: off".

## See and remove the keys

- The tooltip of a row of the inspector, or of the eye, the solo or the lock of a layer, names its key: "Keyboard: M".
- A row of the inspector that has a key shows a keyboard button at its end; a click on it opens the same menu as a right-click.
- Window > Links lists every link of the project, with a button to remove each one.
- While learning, choose a control and press <kbd>Delete</kbd> to remove its key, or choose "Remove the Key" in the menu of an inspector row.

Deleting a layer makes its keys do nothing; undoing the deletion brings them back. Duplicating a layer does not copy its keys.
