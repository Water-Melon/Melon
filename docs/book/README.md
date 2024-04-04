<p align="center"><img width="210" src="http://melonc.io/static/img/logo.44ac06b.png" alt="Melon logo"></p>
<p align="center"><img src="https://img.shields.io/github/license/Water-Melon/Melang" /></p>



Melon is a generic cross-platform C library. It contains many algorithms, data structures, functional components, scripting languages and practical frameworks, which can facilitate developers to quickly develop applications and avoid the dilemma of repeated wheel building.



### Components

Melon currently provides the following components:

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
- Template
  - Function Template
  - Class Template
- Scripting Language Development
- Frameworks
  - Multi-Process Model
  - Multi-Thread Model
  - Trace Mode
  - IPC


### Platform Support

Melon was originally written for UNIX systems, so it is suitable for UNIX-like systems such as Linux and MacOS. And there are a few optimizations for Intel CPUs.

At present, Melon has also completed the preliminary porting to Windows, so it can be used on Windows. However, because Windows differs greatly from the UNIX system in the creation process, some functions of the above `framework` are temporarily not supported in Windows.



### Docker

You can pull the built container image to deploy the running environment.

```
docker pull melonc/melon
```



### Webassembly

You should install `emsdk` at first, make sure `emcc` and `emar` have been installed. Then execute:

```
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

QQ: 756582294
