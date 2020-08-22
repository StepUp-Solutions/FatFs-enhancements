# Test Cases

I use CppUTest to run tests.

I implemented fake FatFs functions that actually call file functions from GNU C Library (open, read, write, close...), the source codes of fakes functions are in [inc](inc) and [src](src) folders.

Source codes for the tests are in [test_io](test_io.cpp) file.
