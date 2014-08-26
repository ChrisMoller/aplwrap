Selected key bindings for aplwrap
=================================

Navigation
----------

```
PageUp, PageDown            backward/forward by page
Home, End                   beginning/end of line
Ctrl-PageUp, Ctrl-PageDown  beginning/end of line
Ctrl-Home, Ctrl-End         beginning/end of buffer
Up, Down, Left, Right       move by line/character
Ctrl-Up, Ctrl-Down          beginning/end of previous/next line
Ctrl-Left, Ctrl-Right       backward/forward word
```

Selection
---------

```
Shift + navigation key(s)   extend selection
Ctrl-A                      select all
Shift-Ctlr-A                select none
```

Editing
-------

```
Backspace                   delete character to left
Delete                      delete character to right
Ctrl-Backspace              delete word to left
Ctrl-Delete                 delete word to right
Shift-Ctrl-Backspace        delete to end of line
Shift-Ctrl-Delete           delete to beginning of line
Ctrl-Return, Ctrl-Enter     insert a line break
Insert                      toggle insert/overwrite
Tab                         ignored; use space(s)
```

Clipboard
---------

```
Ctrl-C                      copy selection to clipboard
Ctrl-X                      cut selection to clipboard
Ctrl-V                      paste clipboard to selection
```

Command History
---------------

```
Alt-Up                      previous history entry
Alt-Dn                      next history entry
```

APL
---

```
Return, Enter               send current line to APL
Ctrl-Break, Ctrl-Windows    interrupt APL
```

APL Characters
--------------

The APL keyboard follows Dyalog APL conventions with a few changes.
View the keyboard layout using aplwrap's `Help > Keymap` menu.

Copy-Down
---------

When selected (e.g. by mouse or keyboard) text doesn't span a line
boundary, pressing Return or Enter copies the text to the end of the
current APL input area and ensures that the area ends with a blank.
The buffer does not scroll to the input area.

When the cursor is on a previous APL input, pressing Return or Enter
copies the entire input to the current APL input area. The buffer
scrolls to the input area.
