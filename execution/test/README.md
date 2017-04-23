# Execution Test

This directory is used to test `execution` module for TkDatabase.

## Usage
* Install `Catch` framework: download `catch.hpp` from [https://raw.githubusercontent.com/philsquared/Catch/master/single_include/catch.hpp](https://raw.githubusercontent.com/philsquared/Catch/master/single_include/catch.hpp) and then move `catch.hpp` to `/usr/include`.
* Install `libev`:
  * download source file from [http://software.schmorp.de/pkg/libev.html](http://software.schmorp.de/pkg/libev.html);
  * `configure`, `make` and `make install`.
* compile test program:
```shell
>$ pwd
.../Coordinator/execution/test
>$ cmake .
>$ make
>$ ./etest
```
* All tests will pass (52 assertions) if everything is ok. 