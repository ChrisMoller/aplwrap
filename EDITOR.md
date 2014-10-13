Editor
======

Functions
---------

The aplwrap editor may be used to create or edit APL functions.

To create a function, use the `File > New` command to create an empty
edit window. Create an APL function with the header on the first line.
When you are done, use the `File > Save` command to define the
function in the APL session.

To edit an existing function, use the `File > Open Object` command to
select from a list of functions and operators in the workspace.

The Open Object dialog may be searched; use the Ctrl-F key to initiate
a search. The active search may be repeated in a forward direction
using the Ctrl-G key or in a reverse direction using Shift-Ctrl-G.

Files
-----

The aplwrap editor may be used to edit existing text files, including
plain-text APL files. The `File > Open File` command opens an
existing. When you are done, save the file using the `File > Save`
command.

Exports
-------

You can export the content of an object editor window to a text file.
Use the `File > Export` command. This does not turn the editor into a
file editor; the saved file may be imported as described above.

Clones
------

An editor window may be cloned using the `File > Clone` command. A
clone shares title, content, cursor, selection and (in the case of a
file editor) path. Each clone's scroller is independent; you may take
advantage of this to open multiple views into a file or a long
function.

Revert
------

Revert an editor window using the `File > Revert` command. This
restores the editor content to the last saved version.

Search
------

An editor window may be searched. Use the Ctrl-F key to toggle the
search entry field. Ctrl-G and Shift-Ctrl-G repeat the search in a
forward or reverse direction, respectively.
