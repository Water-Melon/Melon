<p align="center"><img width="210" src="https://github.com/Water-Melon/Melon/blob/master/docs/logo.png?raw=true" alt="Melon logo"></p>
<p align="center"><img src="https://img.shields.io/github/license/Water-Melon/Melang" /></p>
<h1 align="center">Melon</h1>



**[中文说明](https://water-melon.github.io/Melon/cn/)**

Melon is a C library for simplifying-development. 
It includes many data structures, algorithms, architectures and many other useful components.

- Data structure
  - Doubly linked list
  - Fibonacci heap
  - Hash table
  - queue
  - Red-black tree
  - stack
- Algorithms
  - Encryption algorithm: AES, DES, 3DES, RC4, RSA
  - Hash algorithm: MD5, SHA1, SHA256
  - Base64
  - Large number calculation
  - FEC
  - JSON
  - Matrix Operations
  - Reed Solomon coding
  - Regular matching algorithm
  - KMP
  - Cron format parser
- Components
  - Error code management
  - Memory pool
  - Thread Pool
  - I/O Thread
  - Data link
  - TCP encapsulation
  - Event mechanism
  - File cache
  - HTTP handling
  - Scripting language
  - Lexical analyzer
  - Parser generator
  - Websocket
- Scripting language
  - Preemptive coroutine language - Melang
- Framework
  - Multi-process model
  - Multi-thread model

You can pick some components or framework based on your demand.

On Windows, framework can NOT be activated, but other components still working.



### Installation

On Windows, please install `mingw`, `git bash` and `make` at first. Install [MingW-W64-builds](https://www.mingw-w64.org/downloads/#mingw-builds) with the installation settings:

- `Version`: `8.1.0`

- `Architecture`: `i686`

- `Threads`: `posix`

- `Exception`: `dwarf`

- `Build revision`: `0`

Then execute these shell commands on git bash or terminal (on UNIX).

```
git clone https://github.com/Water-Melon/Melon.git
cd Melon
./configure [--prefix=LIB_INSTALL_PATH]
make
make install
```

### License

[BSD-3-Clause License](https://github.com/Water-Melon/Melang/blob/master/LICENSE)

Copyright (c) 2014-present, Niklaus F. Schen



### Document

Please refer to [Github Pages](https://water-melon.github.io/Melon/) for more details.

中文文档请参考：[中文指南](https://water-melon.github.io/Melon/cn/)

