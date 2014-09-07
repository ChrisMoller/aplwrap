Tab Completion
==============

Overview
--------

The tab completion facility in aplwrap calls out to APL to get
completions for variables, functions, operators, ⎕-variables and
⎕-functions.

The APL application may optionally provide a customization function to
examine the context of the completion and provide a list of
completions specific to the application; this is useful, for example,
to provide tab completion for application commands.

Usage
-----

Tab completion is available only when the cursor is in the APL input
area, no text selection is present and at least one character of an
APL identifier is to the immediate left of the cursor. Repeatedly
pressing the `Tab` key cycles through defined identifiers having the
prefix to the left of the cursor. The portion of the input line
following the identifier moves as required to accomodate completions
of varying lengths.

A beep is sounded when the end of the completion list is reached; the
next press of the `Tab` key restarts completion at the beginning of
the list.

You can advance the cursor to the end of the current completion by
pressing the `Esc` key.

Implementation
--------------

The tab completion mode keeps track of three facets of the current
input line: the cursor position, the text of the identifier to the
left of the cursor position (i.e. the prefix), and the text preceeding
the prefix (i.e. the context). When any of these have changed since
the prior invocation of tab completion, the completion is reset using
a fresh prefix and context.

The reset is accomplished by evaluating `¯1 ___ <context>`. In
response, `___` builds a sorted table of identifiers to be used for
completion. (Also see *Customization*, below.) The completion table is
cached in `_` until the next time `___` is called with a left argument
of `¯1`.

After the lookup table has been established, and on each subsequent
lookup with the same cursor position, context and prefix, aplwrap
evaluates `___` in APL with a positive lookup index and prefix, e.g.
`0 ___ 'foo'`. The index is 0 to begin the completion listing. Each
call to the `___` function returns a list of the next index and the
i-th completion, printed by APL as a positive integer and a nonblank
string, surrounded and delimited by blanks. When the end of the list
is reached, the next index is 0 with no completion string.

Customization
-------------

When called with a left argument of `¯1`, `___` calls a user-defined
`__` function (if the function exists). The `__` function examines the
context and returns a customized completion table based upon the
context. The completion table may contain any sorted list of
identifiers that are meaningful to the application given the provided
context; the default (in the case that the context is not recognized
by the application) must be `⎕nl 2 3 4 5 6`.
