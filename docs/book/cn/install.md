## 安装


### 视频介绍

<iframe src="//player.bilibili.com/player.html?bvid=BV1Lu411u7Z1&page=1&autoplay=0" scrolling="no" border="0" frameborder="no" framespacing="0" allowfullscreen="true" height="480px" width="100%"> </iframe>



Windows与UNIX环境的安装并无差异，仅需要先行安装并配置`mingw`或`msys2`。

如果你安装`mingw`, 那你也许也会想要安装`git bash`以及`make`，后续步骤与UNIX的完全一致。

安装[MingW-W64-builds](https://www.mingw-w64.org/downloads/#mingw-builds)时请选择如下设置：

- `Version`: `8.1.0`

- `Architecture`: `i686`

- `Threads`: `posix`

- `Exception`: `dwarf`

- `Build revision`: `0`

如果你安装的是`msys2`，那么你可以使用`pacman`命令来安装一些你可能需要的工具，例如`git`，`make`等。



UNIX环境下Melon的安装执行如下命令：

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



Melon会同时生成动态库与静态库。对于Linux系统，在使用Melon的动态库时，需要将该库的路径加入到系统配置中：

```bash
$ sudo echo "/usr/local/melon/lib/" >> /etc/ld.so.conf
$ sudo ldconfig
```

或者使用环境变量`LD_LIBRARY_PATH`解决运行时找不到动态库的问题：

```shell
$ export LD_LIBRARY_PATH=/path/to/melon/libdir:$LD_LIBRARY_PATH
```



默认情况下，UNIX中Melon会被安装在`/usr/local/melon`下，Windows中会安装于`$HOME/libmelon`中。



#### Docker

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
