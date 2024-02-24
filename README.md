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

![Screenshot_20240224_160647](https://github.com/i-rinat/fragview/assets/3265870/a75bbb88-3edb-405d-b5b6-e00730afcc52)

Compiling and installing
------------------------
```
$ sudo apt-get install libboost-dev libgtkmm-3.0-dev libglibmm-2.4-dev libsqlite3-dev
$ mkdir build && cd build
$ cmake -DCMAKE_BUILD_TYPE=Release ..
$ make
$ sudo make install
```

Binaries
--------

There are three binaries you can run:

* `fragview`
* `fileseverity`
* `fragdb`

`fragview` is a GTK+ GUI application which enables you to browse by clicking
and scrolling. `fragdb` is a command-line utility whose purpose is to collect number of
fragments of files in specified directory recursively while storing its result in sqlite3 database.
Then it is possible to make some simple reports like displaying ten most
fragmented files or files having more than 100 fragments. `fileseverity` is
similar to `filefrag` utility from e2fsprogs, but displays severity metric
rather than fragment count. A few words about this metric can be found
in draftpad.md file.

License
-------

fragview is distributed under the terms of the MIT License. See LICENSE.MIT for full text.
