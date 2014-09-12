APL Status Line
===============

The status line in the main aplwrap window displays resources consumed
by the last command or expression evaluated by APL.

The status text is ellipsized when too long to fit its display area.

The status line text may be selected and copied. Even when ellipsized,
the full text is copied. Since GNU APL recognizes `#` as a comment
character, you can paste a copied status line into the APL transcript.

| Label | Description              | Units   |
| :---: | :----------------------- | :------ |
|   #   | sequence                 | count   |
|  ∆e:  | elapsed time             | seconds |
|  ∆u:  | CPU time in APL          | seconds |
|  ∆s:  | CPU time in system       | seconds |
|  ∆v:  | virtual memory size      | bytes   |
|  ∆r:  | resident set size        | bytes   |
|  ∆f:  | minor page faults        | count   |
|  ∆F:  | major page faults        | count   |
|  ∆b:  | aggregate block I/O wait | seconds |
| ∆rc:  | read (all)               | bytes   |
| ∆wc:  | write (all)              | bytes   |
| ∆rb:  | read bytes (bio)         | bytes   |
| ∆wb:  | write bytes (bio)        | bytes   |
| ∆ic:  | input syscalls           | count   |
| ∆oc:  | output syscalls          | count   |
| ∆cw:  | cancelled write          | bytes   |

The `sequence` is the ordinal number of the last command or expression
evaluated.

The `elapsed time` is the "wall time" that was taken to evaluate the
last command or expression.

The `time`, `size`, `wait`, `read` and `write` statistics include
resources consumed by the APL process and its children and threads.

The `virtual memory size` and `resident set size`, unlike the other
resources reported, may be adjusted downward.

The `read (all)` and `write (all)` statistics account for all I/O.
This includes I/O to and from character devices, block devices and
the system cache.

The `read (bio)` and `write (bio)` statistics account only for
transfers to and from the storage (bio) layer.

See `man 5 proc` for additional insights regarding statistics read
from `proc/<pid>/stat` and `proc/<pid>/io`.
