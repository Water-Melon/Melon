<p align="center"><img width="108" src="https://github.com/Water-Melon/Melon/blob/master/docs/logo.png?raw=true" alt="Melon logo"></p>
<p align="center"><img src="https://img.shields.io/github/license/Water-Melon/Melang" /></p>



欢迎使用Melon C语言库，本库包含了诸多算法、数据结构、功能组件、脚本语言以及实用框架，可便于开发人员依此快速开发应用功能，避免了重复造轮子的窘境。


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
- 组件
  - 内存池
  - 线程池
  - 数据链
  - TCP 封装
  - 事件机制
  - 文件缓存
  - HTTP 处理
  - 脚本语言
  - 词法分析器
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

- [安装](https://water-melon.github.io/Melon/install.html)
- [快速入门](https://water-melon.github.io/Melon/quickstart.html)
- [初始化](https://water-melon.github.io/Melon/core_init.html)
- [配置](https://water-melon.github.io/Melon/conf.html)
- [日志](https://water-melon.github.io/Melon/log.html)
- [字符串](https://water-melon.github.io/Melon/string.html)
- [素数生成器](https://water-melon.github.io/Melon/prime.html)
- [哈希表](https://water-melon.github.io/Melon/hash.html)
- [红黑树](https://water-melon.github.io/Melon/rbtree.html)
- [双向链表](https://water-melon.github.io/Melon/double_linked_list.html)
- [栈](https://water-melon.github.io/Melon/stack.html)
- [队列](https://water-melon.github.io/Melon/queue.html)
- [斐波那契堆](https://water-melon.github.io/Melon/fheap.html)
- [事件](https://water-melon.github.io/Melon/event.html)
- [内存池](https://water-melon.github.io/Melon/mpool.html)
- [TCP连接及网络I/O链](https://water-melon.github.io/Melon/tcp_io.html)
- [文件集合](https://water-melon.github.io/Melon/file.html)
- [自旋锁](https://water-melon.github.io/Melon/spinlock.html)
- [线程池](https://water-melon.github.io/Melon/threadpool.html)
- [正则表达式](https://water-melon.github.io/Melon/regex.html)
- [Base64](https://water-melon.github.io/Melon/base64.html)
- [JSON](https://water-melon.github.io/Melon/json.html)
- [AES](https://water-melon.github.io/Melon/aes.html)
- [DES](https://water-melon.github.io/Melon/des.html)
- [RC4](https://water-melon.github.io/Melon/rc4.html)
- [RSA](https://water-melon.github.io/Melon/rsa.html)
- [MD5](https://water-melon.github.io/Melon/md5.html)
- [SHA](https://water-melon.github.io/Melon/sha.html)
- HTTP
- Websocket
- FEC
- 里德所罗门纠错码
- 矩阵计算
- 大数计算
- 词法分析器
- 脚本任务
- 多进程框架
- IPC模块开发
- 多线程框架
- 脚本库函数开发
