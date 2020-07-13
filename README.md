Following tutorial at:

https://realpython.com/build-python-c-extension-module/


To build and install
====================

Navigate to the directory containing setup.py and run the following command:

$ python3 setup.py install

This command will compile and install your Python C extension module in the current directory. If there are any errors or warnings, then your program will throw them now. Make sure you fix these before you try to import your module.

By default, the Python interpreter uses clang for compiling the C code. If you want to use gcc or any other C compiler for the job, then you need to set the CC environment variable accordingly, either inside the setup script or directly on the command line. For instance, you can tell the Python interpreter to use gcc to compile and build your module this way:

$ CC=gcc python3 setup.py install

However, the Python interpreter will automatically fall back to gcc if clang is not available.





To test:
====================

$ pytest -q


