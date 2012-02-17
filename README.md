About
-----
Files stored on disk usually divided into small chunks called blocks, which can
reside anywhere on disk partition. When file blocks are not adjacent, one
can say it's fragmented. There is nothing bad in fragmentation, it helps
filesystem to use disk space more efficiently instead. But if file divided in
too many fragments its read speed can decrease dramatically, especially if
fragments located far one from another.

Fragview's main purpose is to visualize disk content by displaying its map. It
displays bunch of small colored squares (called clusters), each representing
a number of blocks. Square's color is white for unoccupied blocks and colored
for occupied ones. If cluster contains part of fragmented file its color
will be red, and for non-fragmented -- blue.

Fragview uses its own metric to determine fragmented file or not. It is based
on estimation of worst read speed. Files whose read speed more than two times
lower than read speed of continuous ones will be considered fragmented.

Compiling and installing
------------------------
You'll need gtkmm-3.0 and sqlite3 headers to compile. Maybe you'll need to
install kernel headers too.

First, create directory, go there and run cmake:

`$ mkdir build` <br>
`$ cd build` <br>
`$ cmake ..` <br>

Then run make:

`$ make`

There is no install procedure yet. Just run binaries.

Binaries
--------

There are three binaries you can run:

* `fragview`
* `fileseveriy`
* `fragdb`

`fragview` is a GTK+ GUI application which enables you to browse by clicking
and scrolling. `fragdb` is a command-line utility whose purpose is to collect number of
fragments of files in specified directory recursively while storing its result in sqlite3 database.
Then it is possible to make some simple reports like displaying ten most
fragmented files or files having more than 100 fragments. `fileseverity` is
similar to `filefrag` utility from e2fsprogs, but displays severity metric
rather than fragment count. A few words about this metric can be found
in draftpad.md file.

Copying
-------
fragview is free software; you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation; either version 2, or (at your option) any later version.

Contact
-------
Author can be reached at email
`ibragimovrinat-at-mail.ru` or at github: https://github.com/i-rinat/fragview
