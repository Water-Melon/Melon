<p align="center"><img width="210" src="https://github.com/Water-Melon/Melon/blob/master/docs/logo.png?raw=true" alt="Melon logo"></p>
<p align="center"><img src="https://img.shields.io/github/license/Water-Melon/Melang" /></p>
<h1 align="center">Melon</h1>



**[中文说明](http://doc.melonc.io/cn/)**

Melon is a generic cross-platform C library. It contains many algorithms, data structures, functional components, scripting languages and practical frameworks, which can facilitate developers to quickly develop applications and avoid the dilemma of repeated wheel building.

- Components
  - Library Initialization
  - Configuration
  - Log
  - Error Code Management
  - Memory Pool
  - Thread Pool
  - I/O Thread
  - TCP Encapsulation
  - Event Mechanism
  - File Set
  - HTTP Handling
  - Scripting Language
  - Lexical Analyzer
  - Parser Generator
  - Websocket
  - String
  - Regular Expression
  - Big Number Calculation
  - FEC
  - JSON
  - Matrix Operations
  - Reed Solomon Coding
  - Cron Format Parser
  - Spin Lock
  - Prime Generator
  - Span
  - Expression
- Data Structures
  - Doubly Linked List
  - Fibonacci Heap
  - Hash Table
  - Queue
  - Red-black Tree
  - Stack
  - Array
- Algorithms
  - AES
  - DES/3DES
  - RC4
  - RSA
  - MD5
  - SHA
  - Base64
- Templates
  - Function Template
  - Class Template
- Scripting Language Development
- Frameworks
  - Multi-Process Model
  - Multi-Thread Model
  - Trace Mode
  - IPC



### Platform Support

- Linux

- MacOSX

- Windows
  - `msys2` (Fully supported)
  - `msvc` (Partially supported)




### Quick Installation

```
git clone https://github.com/Water-Melon/Melon.git
cd Melon
./configure [--prefix=LIB_INSTALL_PATH | ...]
make
make install
```

For installation on other operating systems or for more configure options, please refer to [Official Document: Installation](http://doc.melonc.io/en/install.html).



### License

[BSD-3-Clause License](https://github.com/Water-Melon/Melang/blob/master/LICENSE)

Copyright (c) 2014-present, Niklaus F. Schen



### Documentation

Please refer to [Official Documentation Website](http://doc.melonc.io/) for more details.

中文文档请参考：[中文指南](http://doc.melonc.io/cn/)



### Docker

You can pull the built container image to deploy the running environment.

```shell
docker pull melonc/melon
```



### Webassembly

You should install `emsdk` at first, make sure `emcc` and `emar` have been installed. Then execute:

```shell
./configure --enable-wasm
make && make install
```

There is only one static library `libmelon_static.a` to be created.



### Contributing
We ❤️  pull requests, and we’re continually working hard to make it as easy as possible for developers to contribute.
Before beginning development with the Melon, please familiarize yourself with the following developer resources:

- Contributor Guide ([CONTRIBUTING.md](https://github.com/Water-Melon/Melon/blob/master/CONTRIBUTING.md)) to learn about how to contribute to Melon.



### Contact

Twitter: [@MelonTechnology](https://twitter.com/MelonTechnology)

QQ: [756582294](http://qm.qq.com/cgi-bin/qm/qr?_wv=1027&k=4e2GRrKLo6cz7kptaU_cUHhZ3JeHQT5b&authKey=ffV3ztGX3QAZP%2BRCnbdwAUETeT8O3VIxiIeyBch0DkvxAoM3J%2Bs3Ol1sZjcZwuto&noverify=0&group_code=756582294)

