<p align="center"><img width="210" src="http://melonc.io/static/img/logo.44ac06b.png" alt="Melon logo"></p>
<p align="center"><img src="https://img.shields.io/github/license/Water-Melon/Melang" /></p>



Melon是一个面向C语言的跨平台的通用基础库。本库包含了诸多算法、数据结构、功能组件、脚本语言以及实用框架，可便于开发人员依此快速开发应用功能，避免了重复造轮子的窘境。




### 功能

Melon当前提供了如下功能：

- 组件
  - 初始化
  - 配置
  - 日志
  - 返回值管理
  - 内存池
  - 线程池
  - I/O线程
  - TCP连接及网络I/O
  - 事件
  - 文件集合
  - HTTP
  - 脚本任务
  - 词法分析器
  - 语法解析器生成器
  - Websocket
  - 字符串
  - 正则表达式
  - 大数计算
  - FEC
  - JSON
  - 矩阵运算
  - 里德所罗门编码
  - Cron格式解析器
  - 自旋锁
  - 素数生成器
  - 资源开销
  - 表达式
- 数据结构
  - 双向链表
  - 斐波那契堆
  - 哈希表
  - 队列
  - 红黑树
  - 栈
  - 数组
- 算法
  - AES
  - DES/3DES
  - RC4
  - RSA
  - MD5
  - SHA
  - Base64
- 模板
  - 函数模板
  - 类模板
- 脚本语言开发
- 框架
  - 多进程模型
  - 多线程模型
  - 动态跟踪模式
  - IPC



### 平台支持

- Linux

- MacOSX

- Windows
  - `msys2` (完全支持)
  - `msvc` (部分支持，详情参考下一小节的安装文档)


### 快速安装
```
git clone https://github.com/Water-Melon/Melon.git
cd Melon
./configure [--prefix=LIB_INSTALL_PATH | ...]
make
make install
```

关于如何在其他平台安装以及configure的更多选项，请参考[安装](http://doc.melonc.io/cn/install.html)章节内容。



### Docker

在Docker Hub上可找到Melon的开发和运行环境的容器镜像，方便用户构建云原生环境。

```
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



### 开源贡献
我们非常期待您能够提交Pull Request（PR），我们也尽可能简化PR的流程来使开发者可以比较容易地提交PR。
在开发Melon库之前，希望您能花费一些宝贵的时间浏览一下如下开发者相关的文档：

- 开源贡献指南 ([CONTRIBUTING.md](https://github.com/Water-Melon/Melon/blob/master/CONTRIBUTING.md))：获悉如何向Melon提交您的代码贡献。



### 联系方式

Twitter: [@MelonTechnology](https://twitter.com/MelonTechnology)

QQ群: 756582294
