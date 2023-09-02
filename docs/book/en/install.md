## Installation

On Windows, there is no difference between the installation of Windows and UNIX environments, you only need to install and configure `mingw`, `git bash` and `make` first.

Please select the following settings when installing [MingW-W64-builds](https://www.mingw-w64.org/downloads/#mingw-builds):

- `Version`: `8.1.0`

- `Architecture`: `i686`

- `Threads`: `posix`

- `Exception`: `dwarf`

- `Build revision`: `0`



To install Melon on UNIX, execute the following commands:

```bash
$ git clone https://github.com/Water-Melon/Melon.git
$ ./configure
$ make
$ sudo make install
```

Shell script `configure` has the following parameters:

- `--prefix` The installation path of Melon library

- `--melang-prefix` The installation path of the Melang script files that Melon used

- `--cc` Set the C compiler that used to compile Melon

- `--enable-wasm` Enable webassembly mode to generate webassembly format library

- `--debug` Enable debug mode. If omited the generated library will not contain symbol information and macro `__DEBUG__`

- `--olevel=[O|O1|O2|O3|...]` The level of compilation optimization, the default is `O3`. The optimization is disabled if no content after `=`.

- `--help` Show help information



Melon generates both dynamic and static libraries at the same time. For Linux systems, when using Melon's dynamic library, the path to the library needs to be added to the system configuration:

```bash
$ sudo echo "/usr/local/melon/lib/" >> /etc/ld.so.conf
$ sudo ldconfig
```

Or use the command given below to solve dynamic library not found problem:

```shell
$ export LD_LIBRARY_PATH=/path/to/melon/libdir:$LD_LIBRARY_PATH
```



By default, Melon is installed in `/usr/local/melon` on UNIX and `$HOME/libmelon` on Windows.



#### Docker

You can pull the built container image to deploy the running environment

```shell
docker pull melonc/melon
```



### Webassembly

You should install `emsdk` at first, make sure `emcc` and `emar` have been installed. Then execute:

```
./configure --enable-wasm
make && make install
```

There is only one static library `libmelon.a` to be created.
