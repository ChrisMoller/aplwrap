Selected key bindings for aplwrap
=================================

Navigation
----------

```
PageUp, PageDown            backward/forward by page height
Ctrl-PageUp, Ctrl-PageDown  backward/forward by page width
Home, End                   beginning/end of line
Ctrl-Home, Ctrl-End         beginning/end of buffer
Up, Down, Left, Right       move by line/character
Ctrl-Up, Ctrl-Down          beginning/end of previous/next line
Ctrl-Left, Ctrl-Right       backward/forward word
Alt-PageUp, Alt-PageDown    backward/forward by APL prompt
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
Tab                         completion (see COMPLETION.md)
Esc                         move cursor to end of completion
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

Menu Accelerators, main window
------------------------------

```
Ctrl-N                      create a new APL object
Ctrl-O                      open an APL object for editing
Ctrl-I                      import a text file for editing
Ctrl-K                      display the APL keyboard map
```

Menu Accelerators, edit window
------------------------------

```
Ctrl-N                      create a new APL object
Ctrl-O                      open an APL object for editing
Ctrl-I                      import a text file for editing
Ctrl-S                      save an edit window
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

Invalid Input
-------------

APL expects to receive its input one line at a time. If the APL input
area contains multiple lines (which can only happen if you paste text
containing newline characters), this is treated as invalid. When you
press the Return or Enter key, aplwrap will delete the invalid input
and beep.

Nabla Editor
------------

GNU APL, like APLs of old, offers a way to edit functions inside APL.
The "nabla" editor starts using the ∇ character for which the editor
is named. (Actually, the nabla editor offered the *only* means of
editing APL functions until Unicode offered APL character support.)

The nabla editor, which always displays the number of the current
line, accepts commands as shown below. Exit the editor using the ∇
character to save your edits or the ⍫ command to save your edits and
lock the function. (In GNU APL, the lock affects only execution
properties, not visibility.)

All edits take effect immediately. There is no way for the nabla
editor to abandon an edit and revert the function to its unedited
state.

Note that you may specify fractional line numbers while editing; this
is how you insert lines. Deleted lines leave gaps in the line numbers
while editing. The next time you edit the function, the line numbers
will be consecutive integers.

Commands may be stacked on the same line. For example,

```
  [∆18] [15⎕20] [16.1] x←0
```

deletes line 18, shows lines 15 through 20 and inserts (or changes)
line 16.1 with the text `x←0`.

The nabla editor is primarily useful if you keep your code in an APL
workspace file (an XML file in GNU APL).

```
Invocation
  ∇FUN                                   (open FUN to edit)
  ∇FUN[⎕]                                (list FUN and open to edit)
  ∇FUN[⎕]∇                               (list FUN)

Commands
  [⎕] [n⎕] [⎕m] [n⎕m] [⎕n-m]             (show: all, from, to, range)
  [n∆] [∆m] [n∆m] [∆n-m] [∆n1 n2 ...]    (delete: one, range, set)
  [→]                                    (wipe all lines except 0)
  [n]                                    (set current line number)
  text                                   (replace text on line)
  ∇                                      (save function)
  ⍫                                      (save and lock function)
```
