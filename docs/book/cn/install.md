## 安装

Windows与UNIX环境的安装并无差异，仅需要先行安装并配置`mingw`、`git bash`以及`make`即可。

安装[MingW-W64-builds](https://www.mingw-w64.org/downloads/#mingw-builds)时请选择如下设置：

- `Version`: `8.1.0`

- `Architecture`: `i686`

- `Threads`: `posix`

- `Exception`: `dwarf`

- `Build revision`: `0`



安装Melon，可以执行如下命令：

```bash
$ git clone https://github.com/Water-Melon/Melon.git
$ ./configure
$ make
$ sudo make install
```

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

安装后可以看到，在安装目录的`lib`子目录下仅会生成`libmelon.a` 静态库。
