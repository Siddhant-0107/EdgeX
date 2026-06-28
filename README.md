# EdgeX

> A high-performance HTTP server and networking infrastructure built from scratch in modern C++.

## Overview

EdgeX is a learning-driven systems programming project aimed at understanding how modern web servers and backend infrastructure work internally.

Instead of relying on existing frameworks, the project builds core networking components from the ground up using modern C++ and the POSIX socket API. The objective is not only to create a working HTTP server but also to gain a deep understanding of Computer Networks, Operating Systems, concurrency, and backend engineering.

This repository documents the complete engineering journey—from project initialization to a production-inspired networking system.

---

## Motivation

Most backend developers use web servers every day but rarely understand how they are implemented.

EdgeX is an attempt to bridge that gap by building the underlying infrastructure ourselves, one module at a time.

The project focuses on learning through implementation rather than simply using existing libraries.

---

## Goals

* Build an HTTP server from scratch
* Understand TCP/IP and socket programming
* Learn modern C++ design principles
* Implement multithreaded request handling
* Study HTTP protocol internals
* Practice software architecture and clean code
* Develop production-style engineering habits

---

## Planned Features

### Core

* TCP socket abstraction
* HTTP/1.1 request parser
* HTTP response generation
* Static file serving
* Configuration system
* Logging system

### Concurrency

* Thread pool
* Concurrent request handling
* Graceful shutdown

### Infrastructure

* Reverse proxy
* Load balancer
* Health checks
* Metrics endpoint
* Response caching

### Engineering

* Unit tests
* Benchmarks
* Documentation
* Docker support

---

## Technology Stack

* C++17
* CMake
* Git
* POSIX Sockets
* Visual Studio Code

---

## Project Structure

```text
EdgeX/
├── include/
├── src/
├── tests/
├── docs/
├── benchmarks/
├── config/
├── scripts/
├── CMakeLists.txt
└── README.md
```

---

## Build

```bash
mkdir build
cd build
cmake ..
cmake --build .
```

---

## Project Status

Current Version: **v0.1.0**

Completed:

* Initial repository setup
* Modern CMake configuration
* Project directory structure

Currently Working On:

* Socket abstraction layer

---

## Learning Roadmap

1. Project Setup
2. Socket Programming
3. TCP Server
4. HTTP Parsing
5. Routing
6. Static File Serving
7. Thread Pool
8. Logging
9. Reverse Proxy
10. Load Balancing
11. Performance Optimization

---

## License

This project is released under the MIT License.
