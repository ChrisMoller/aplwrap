APL Status Line
===============

The status line in the main aplwrap window shows the resources
consumed by the last command or expression evaluated by APL.

```
∆u: 1.28 ∆s: 0.03 ∆v: 151,363,584 ∆r: 13,234,176 ∆f: 20,576 ∆F: 0 ∆b: 0.00
```

| Label | Description              | Units   |
| :---: | :----------------------- | :------ |
|  ∆u:  | CPU time in APL          | seconds |
|  ∆s:  | CPU time in system       | seconds |
|  ∆v:  | virtual memory size      | bytes   |
|  ∆r:  | resident set size        | bytes   |
|  ∆f:  | minor page faults        | count   |
|  ∆F:  | major page faults        | count   |
|  ∆b:  | aggregate block I/O wait | seconds |

Note that these statistics include resources consumed by children and
threads of the APL process.

The `virtual memory size` and `resident set size`, unlike the other
resources reported, may be adjusted downward.
