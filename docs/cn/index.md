<p align="center"><img width="210" src="https://github.com/Water-Melon/Melon/blob/master/docs/logo.png?raw=true" alt="Melon logo"></p>
<p align="center"><img src="https://img.shields.io/github/license/Water-Melon/Melang" /></p>



欢迎使用Melon C语言库，本库包含了诸多算法、数据结构、功能组件、脚本语言以及实用框架，可便于开发人员依此快速开发应用功能，避免了重复造轮子的窘境。
QQ: 756582294


### 功能

Melon当前提供了如下功能：

- 数据结构
  - 双向链表
  - 斐波那契堆
  - 哈希表
  - 队列
  - 红黑树
  - 栈
- 算法
  - 加密算法：AES 、DES 、3DES 、RC4 、RSA
  - 哈希算法：MD5 、SHA1 、SHA256
  - Base64
  - 大数计算
  - FEC
  - JSON
  - 矩阵运算
  - 里德所罗门编码
  - 正则匹配算法
  - KMP
  - Cron格式解析器
- 组件
  - 错误码
  - 内存池
  - 线程池
  - I/O线程模型
  - 数据链
  - TCP 封装
  - 事件机制
  - 文件缓存
  - HTTP 处理
  - 脚本语言
  - 词法分析器
  - 语法解析器生成器
  - websocket
- 脚本语言
  - 抢占式协程语言——Melang
- 框架
  - 多进程模型
  - 多线程模型



### 平台支持

Melon最初是为UNIX系统编写，因此适用于Linux、MacOS等类UNIX系统，并在针对Intel CPU有少量优化。

目前Melon也已经完成了向Windows的初步移植，因此可以在Windows上进行使用。但由于Windows在创建进程上与UNIX系统差异较大，因此导致上述`框架`部分功能在Windows中暂时不支持。



### 目录

- [安装](https://water-melon.github.io/Melon/cn/install.html)
- [快速入门](https://water-melon.github.io/Melon/cn/quickstart.html)
- [初始化](https://water-melon.github.io/Melon/cn/core_init.html)
- [配置](https://water-melon.github.io/Melon/cn/conf.html)
- [日志](https://water-melon.github.io/Melon/cn/log.html)
- [字符串](https://water-melon.github.io/Melon/cn/string.html)
- [素数生成器](https://water-melon.github.io/Melon/cn/prime.html)
- [哈希表](https://water-melon.github.io/Melon/cn/hash.html)
- [红黑树](https://water-melon.github.io/Melon/cn/rbtree.html)
- [双向链表](https://water-melon.github.io/Melon/cn/double_linked_list.html)
- [栈](https://water-melon.github.io/Melon/cn/stack.html)
- [队列](https://water-melon.github.io/Melon/cn/queue.html)
- [斐波那契堆](https://water-melon.github.io/Melon/cn/fheap.html)
- [事件](https://water-melon.github.io/Melon/cn/event.html)
- [错误码](https://water-melon.github.io/Melon/cn/error.html)
- [内存池](https://water-melon.github.io/Melon/cn/mpool.html)
- [TCP连接及网络I/O链](https://water-melon.github.io/Melon/cn/tcp_io.html)
- [文件集合](https://water-melon.github.io/Melon/cn/file.html)
- [自旋锁](https://water-melon.github.io/Melon/cn/spinlock.html)
- [线程池](https://water-melon.github.io/Melon/cn/threadpool.html)
- [I/O线程模型](https://water-melon.github.io/Melon/cn/iothread.html)
- [Cron格式解析器](https://water-melon.github.io/Melon/cn/cron.html)
- [正则表达式](https://water-melon.github.io/Melon/cn/regex.html)
- [Base64](https://water-melon.github.io/Melon/cn/base64.html)
- [JSON](https://water-melon.github.io/Melon/cn/json.html)
- [AES](https://water-melon.github.io/Melon/cn/aes.html)
- [DES](https://water-melon.github.io/Melon/cn/des.html)
- [RC4](https://water-melon.github.io/Melon/cn/rc4.html)
- [RSA](https://water-melon.github.io/Melon/cn/rsa.html)
- [MD5](https://water-melon.github.io/Melon/cn/md5.html)
- [SHA](https://water-melon.github.io/Melon/cn/sha.html)
- [HTTP](https://water-melon.github.io/Melon/cn/http.html)
- [Websocket](https://water-melon.github.io/Melon/cn/websocket.html)
- [FEC](https://water-melon.github.io/Melon/cn/fec.html)
- [里德所罗门纠错码](https://water-melon.github.io/Melon/cn/reedsolomon.html)
- [矩阵计算](https://water-melon.github.io/Melon/cn/matrix.html)
- [大数计算](https://water-melon.github.io/Melon/cn/bignum.html)
- [词法分析器](https://water-melon.github.io/Melon/cn/lex.html)
- [语法解析器生成器](https://water-melon.github.io/Melon/cn/parser_generator.html)
- [脚本任务](https://water-melon.github.io/Melon/cn/melang.html)
- [脚本开发](https://water-melon.github.io/Melon/cn/melang-dev.html)
- [多线程框架](https://water-melon.github.io/Melon/cn/multithread.html)
- [IPC模块开发](https://water-melon.github.io/Melon/cn/ipc.html)
- [多进程框架](https://water-melon.github.io/Melon/cn/multiprocess.html)
