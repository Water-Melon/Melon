## 安装



### UNIX



#### make

UNIX环境（Linux和MacOSX）下Melon的安装执行如下命令：

```bash
$ git clone https://github.com/Water-Melon/Melon.git
$ ./configure
$ make
$ sudo make install
```

`configure`脚本支持如下参数：

- `--prefix` 设置Melon的安装路径。
- `--melang-prefix` 设置Melon库中使用到的Melang脚本的安装路径。
- `--cc` 设置Melon组件编译时所使用的C编译器。
- `--enable-wasm` 启用`webassembly`模式，会编译安装webassembly格式的Melon库。
- `--debug` 开启`debug`模式，若不开启，则生成的库不包含符号信息，也不会启用`__DEBUG__`宏。
- `--func` 开启`func`模式，开启后会将`MLN_FUNC`和`MLN_FUNC_VOID`定义的函数在调用时启用入口和出口回调。
- `--olevel=[O|O1|O2|O3|...]` 编译优化的级别，默认是`O3`。如果`=`后不写内容则为不开启优化。
- `--select=[all | module1,module2,...]` 选择性编译部分模块，默认为`all`表示编译全部模块。模块名称可在各模块文档中给出。
- `--disable-macro=[macro1,macro2,...]` 禁用`configure`检测到的当前操作系统支持的系统调用或宏，目前仅支持如下内容：
  - `event`：用于控制是否禁对特定操作系统平台支持的事件相关系统调用的检测。若禁用，则默认使用`select`。
  - `sendfile`：控制是否禁用`sendfile`系统调用。
  - `writev`：控制是否禁用`writev`系统调用。
  - `unix98`：控制是否禁用`__USE_UNIX98`宏。
  - `mmap`：控制是否禁用`mmap`和`munmap`系统调用。

- `--help` 显示`configure`脚本的帮助信息。



Melon会同时生成动态库（`libmelon.so`）与静态库（`libmelon_static.a`）。对于Linux系统，在使用Melon的动态库时，需要将该库的路径加入到系统配置中：

```bash
$ sudo echo "/usr/local/melon/lib/" >> /etc/ld.so.conf
$ sudo ldconfig
```

或者使用环境变量`LD_LIBRARY_PATH`解决运行时找不到动态库的问题：

```shell
$ export LD_LIBRARY_PATH=/path/to/melon/libdir:$LD_LIBRARY_PATH
```



默认情况下，UNIX中Melon会被安装在`/usr/local/melon`下，Windows中会安装于`$HOME/libmelon`中。



#### CMake

Melon支持了使用CMake完全编译安装库，执行如下命令即可：

```bash
mkdir build
cd build
cmake ..
```



#### Bazel

Melon支持了使用Bazel构建Melon库，执行如下命令即可：

```bash
bazel build //:melon
```



### Windows

Windows平台支持两种安装方式：

- `msys2`
- `msvc`



#### msys2

`msys2`完全支持Melon的特性，因此也是Windows上的推荐安装方式。

##### 安装msys2及依赖项

`msys2`可以在 https://www.msys2.org/ 中进行下载和安装。随后选择`MSYS2 MINGW32`、`MSYS2 MSYS`、`MSYS2 CLANG64`、`MSYS2 CLANGARM64`中的任意一个启动msys2命令行环境，并输入如下命令安装所需依赖：

```
pacman -S vim make gcc git
```

##### 安装Melon

这一步与UNIX环境中的安装步骤就完全一样了。安装好后将得到`libmelon.dll`和`libmelon_static.lib`库。



#### msvc

`msvc`目前仅支持部分模块功能，如下是暂不支持的模块：

- `class`
- `iothread`
- `thread_pool`
- `自旋锁`
- `多进程框架`
- `多线程框架`

要使用`msvc`，需要先行安装[Visual Studio C/C++ IDE](https://visualstudio.microsoft.com/vs/features/cplusplus/)。安装好后，可以使用跟随其一同安装好的Powershell启动MSVC命令行环境。

拉取Melon库并切换至Melon代码库后，执行

```
build.bat
```

即可进行编译安装。安装好后，在`lib`子目录中将看到编译生成的静态库`libmelon_static.lib`。MSVC环境暂不支持生成动态库。

额外需要注意的一点是，MSVC编译出来的Melon库，硬编码了`path`模块内的路径，因此安装路径被强制放在`HOME/libmelon`中。

MSVC目前仅支持完全安装，暂不支持模块选择性编译。



### Docker

可以直接使用如下命令拉取已构建好的Melon环境，其中也包含了Melang脚本所使用到的系统库

```shell
docker pull melonc/melon
```



### Webassembly

Melon支持被编译为webassembly格式库。但在此之前，需要先行安装`emsdk`，确保`emcc`和`emar`命令已被安装。

随后便可执行如下命令，将Melon编译为webassembly静态库：

```
./configure --enable-wasm
make && make install
```

安装后可以看到，在安装目录的`lib`子目录下仅会生成`libmelon_static.a` 静态库。
