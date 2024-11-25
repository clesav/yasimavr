Compiling yasimavr
==================


Binaries from `yasimavr` are provided by the Python Package Index here : https://pypi.org/project/yasimavr/
Or can be dowloaded directly using PIP.

The compilation of the shared libraries can be tweaked by using some definitions in the compiling options.
The recommendation is to leave these options with the default values but they can be adjusted for specific needs.

The steps to modify the default settings and compile are :
 - Download the source distribution from GitHub (https://github.com/clesav/yasimavr/releases) or from PyPÏ (https://pypi.org/project/yasimavr/)
 - Edit the file setup.py to change the default compilation settions (look for the global variable YASIMAVR_COMPILE_OPTIONS)
 - Compile using python-build (https://pypi.org/project/build/)
 

..Note:
   The binaries for the python libraries are built with the default settings for all the options below.

Compiling options
-----------------

YASIMAVR_NAMESPACE
******************

Setting this define option to a name places all `yasimavr` API symbols within a namespace.
This is useful if you compile yasimavr as part of a bigger C++ project.
By default, no namespace is defined.


YASIMAVR_NO_TRACE
*****************

This define option removes instruction execution tracing from the CPU simulator. This allows code
optimisation and speeds simulations.
When this option is set, the logging level TRACE is still available but doesn't provide any additional
information compared to the level DEBUG.
By default, instruction tracing is included in the code.


YASIMAVR_NO_ACC_CTRL
********************

This define option removes access control checks during accesses to non-volaile memories, so that all
accesses for read/write/executre succeed even when it should fail.
This allows to slightly speed up the execution of memory access instructions, such as (E)LPM, for
firmwares that don't use access control features or self-programming.
By default, access control checks are included in the code.
