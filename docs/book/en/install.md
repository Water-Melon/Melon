## Installation



### UNIX



#### make

To install Melon on UNIX environments (Linux and MacOSX), execute the following commands:

```bash
$ git clone https://github.com/Water-Melon/Melon.git
$ ./configure
$ make
$ sudo make install
```

The `configure` script supports the following parameters:

- `--prefix`: Set the installation path for Melon.
- `--melang-prefix`: Set the installation path for Melang scripts used in the Melon library.
- `--cc`: Set the C compiler used for compiling Melon components.
- `--enable-wasm`: Enable `webassembly` mode, which compiles and installs the Melon library in webassembly format.
- `--debug`: Enable `debug` mode. If not enabled, the generated library will not include symbol information, and the `__DEBUG__` macro will not be enabled.
- `--func`: Enable `func` mode, which enables entry and exit callbacks for functions defined using `MLN_FUNC` and `MLN_FUNC_VOID`.
- `--olevel=[O|O1|O2|O3|...]`: Set the optimization level for compilation, default is `O3`. If nothing follows the `=`, optimization is disabled.
- `--select=[all | module1,module2,...]`: Selectively compile specific modules. Default is `all` to compile all modules. Module names can be found in the respective module documentation.
- `--disable-macro=[macro1,macro2,...]`: Disable system calls or macros detected by `configure` for the current operating system. Currently supported:
  - `event`: Disable detection of event-related system calls supported by specific operating system platforms. If disabled, `select` is used by default.
  - `sendfile`: Control whether to disable the `sendfile` system call.
  - `writev`: Control whether to disable the `writev` system call.
  - `unix98`: Control whether to disable the `__USE_UNIX98` macro.
  - `mmap`: Control whether to disable the `mmap` and `munmap` system calls.
- `--help`: Display help information for the `configure` script.



Melon generates both a dynamic library (`libmelon.so`) and a static library (`libmelon_static.a`). For Linux systems, when using the dynamic library of Melon, you need to add the path of this library to the system configuration:

```bash
$ sudo echo "/usr/local/melon/lib/" >> /etc/ld.so.conf
$ sudo ldconfig
```

Alternatively, you can use the `LD_LIBRARY_PATH` environment variable to solve the problem of not finding the dynamic library at runtime:

```shell
$ export LD_LIBRARY_PATH=/path/to/melon/libdir:$LD_LIBRARY_PATH
```



By default, Melon will be installed in `/usr/local/melon` on UNIX systems and in `$HOME/libmelon` on Windows.



#### CMake

Melon supports full compilation and installation of the library using CMake. Simply execute the following command:

```bash
mkdir build
cd build
cmake ..
```



#### Bazel

Melon supports full compilation and installation of the library using Bazel. Just execute the following command:

```bash
bazel build //:melon
```



### Windows

Windows platform supports two installation environments:

- `msys2`
- `msvc`



#### msys2

`msys2` fully supports Melon's features and is therefore the recommended installation method on Windows.

##### Installing msys2 and dependencies

`msys2` can be downloaded and installed from https://www.msys2.org/. Then, choose one of `MSYS2 MINGW32`, `MSYS2 MSYS`, `MSYS2 CLANG64` and `MSYS2 CLANGARM64` to start the msys2 command line environment, and enter the following command to install the required dependencies:

```
pacman -S vim make gcc git
```

##### Installing Melon

This step is exactly the same as the installation steps in the UNIX environment. After installation, you will get the `libmelon.dll` and `libmelon_static.lib` libraries.



#### msvc

`msvc` currently only supports partial module functionality. The following modules are currently not supported:

- `class`
- `iothread`
- `thread_pool`
- `spinlock`
- `multi-process framework`
- `multi-thread framework`

To use `msvc`, you need to first install the [Visual Studio C/C++ IDE](https://visualstudio.microsoft.com/vs/features/cplusplus/). After installation, you can use the Powershell installed along with it to start the MSVC command line environment.

After pulling the Melon library and change directory to the Melon codebase, execute:

```
build.bat
```

to compile and install.

After installation, you will see the compiled static library `libmelon_static.lib` in the `lib` subdirectory. MSVC environment currently does not support generating dynamic libraries.

One additional point to note is that the Melon library compiled with MSVC hard-codes the paths inside the `path` module, so the installation path is `HOME/libmelon`.

MSVC currently only supports full installation and does not support selective module installation.



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

There is only one static library `libmelon_static.a` to be created.
