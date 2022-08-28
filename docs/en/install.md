## Installation

On Windows, there is no difference between the installation of Windows and UNIX environments, you only need to install and configure `mingw`, `git bash` and `make` first.

Please select the following settings when installing [MingW-W64-builds](https://www.mingw-w64.org/downloads/#mingw-builds):

- `Version`: `8.1.0`

- `Architecture`: `i686`

- `Threads`: `posix`

- `Exception`: `dwarf`

- `Build revision`: `0`


Execute the following command to install Melon:

```bash
$ git clone https://github.com/Water-Melon/Melon.git
$ ./configure
$ make
$ sudo make install
```

Melon generates both dynamic and static libraries at the same time. For Linux systems, when using Melon's dynamic library, the path to the library needs to be added to the system configuration:

```bash
$ sudo echo "/usr/local/melon/lib/" >> /etc/ld.so.conf
$ sudo ldconfig
```

By default, Melon is installed in `/usr/local/melon` on UNIX and `$HOME/libmelon` on Windows.

