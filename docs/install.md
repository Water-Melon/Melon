## 安装

Windows与UNIX环境的安装并无差异，仅需要先行安装并配置`mingw`、`git bash`以及`make`即可。

安装[MingW-W64-builds](https://www.mingw-w64.org/downloads/#mingw-builds)时请选择如下设置：

- `Version`: `8.1.0`

- `Architecture`: `i686`

- `Threads`: `posix`

- `Exception`: `dwarf`

- `Build revision`: `0`


执行如下命令安装Melon：

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

默认情况下，UNIX中Melon会被安装在`/usr/local/melon`下，Windows中会安装于`$HOME/libmelon`中。

