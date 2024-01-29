## Installation


<iframe width="100%" height="480px" src="https://www.youtube.com/embed/d0G-8BwLi30?si=XzbFCEcPADefc6_8" title="YouTube video player" frameborder="0" allow="accelerometer; clipboard-write; encrypted-media; gyroscope; picture-in-picture; web-share" allowfullscreen></iframe>


On Windows, there is no difference between the installation of Windows and UNIX environments, you only need to install and configure `mingw` or `msys2`.

If you install `mingw`, you may want to install `git bash` and `make` either. Install [MingW-W64-builds](https://www.mingw-w64.org/downloads/#mingw-builds) with the installation settings:

- `Version`: `8.1.0`

- `Architecture`: `i686`

- `Threads`: `posix`

- `Exception`: `dwarf`

- `Build revision`: `0`

If you install `msys2`, you can use `pacman` to install any tool softwares you may need (e.g. `make`, `git`).



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
- `--enable-wasm` Enable `webassembly` mode to generate webassembly format library
- `--debug` Enable `debug` mode. If omited the generated library will not contain symbol information and macro `__DEBUG__`
- `--func` Enable `func` mode. When enabled, the functions defined by `MLN_FUNC` and `MLN_FUNC_VOID` will enable entry and exit callbacks when called.
- `--olevel=[O|O1|O2|O3|...]` The level of compilation optimization, the default is `O3`. The optimization is disabled if no content after `=`.
- `--select=[all | module1,module2,...]` Selectively compile some modules. The default is `all` which means compiling all modules. Module names can be given in the document for each module.
- `--disable-macro=[macro1,macro2,...]` disables the system calls or macros supported by the current operating system detected by `configure`. Currently, only the following is supported:
  - `event`: used to control whether to disable detection of event-related system calls supported by a specific operating system platform. If disabled, `select` is used by default.
  - `sendfile`: Controls whether to disable the `sendfile` system call.
  - `writev`: Controls whether the `writev` system call is disabled.
  - `unix98`: Controls whether to disable the `__USE_UNIX98` macro.
  - `mmap`: Controls whether to disable `mmap` and `munmap` system calls.
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
