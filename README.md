Important notes about luci:
- Luci, no matter what version the file is, will return values in the structs that correspond to the latest version instead of quitting the program (if no version is specified). This means that a ``0.1.0`` file will generate a struct that has information that would normally only be accessible from a file of a higher version - however, this data will all be garbage passed on from the next frame. This is why it is so important to pass a version variable to ``map_and_process``, to make sure that luci will quit instead of returning garbage data.
- The structs returned by luci are nearly 60 MB in size. If memory is a concern, free the variables not needed.
- Better documentation is coming soon, I promise.

C build instructions:
====================
To make libraries, simply run ``make libluci.a`` or ``make libluci.so``. These files will be created in ``lib/``. To build the example file, run ``make example``. This binary will be generated in the source directory as ``luci``, and can be run with ``./luci``. The parsing function is ``map_and_process(char *filenamep, int *versionp)``; this function takes a pointer to a filename and a pointer to a 3 integer version vector (no files can be below this version). The returned type is a ``slp_file_t``, defined in ``luci.h``. To enale debug messages, simply ``make debug`` before making the library files.


Python build instructions
====================

``$ CC=gcc python3 setup.py install``

To import, use ``import luci``.

**This currently does not work; functionality coming soon (along with node.js)**

Fuzzing
====================

Use afl-gcc or afl-clang to complile, then use ``afl-fuzz -m 100 -i test/ -o results/ ./luci @@``

