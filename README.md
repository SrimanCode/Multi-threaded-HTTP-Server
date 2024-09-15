# Multi-threaded HTTP Server

## Project Overview

This project is an implementation of a multi-threaded HTTP server in C using **pthreads**. The server is capable of handling multiple concurrent client connections to process HTTP GET and PUT requests efficiently. This was developed as part of an assignment for the CSE 130 course at San Jose State University, Winter 2023.

## Goals of the Project

The primary objectives of this project were:
1. **Implement Multi-threading**: Develop a thread pool using pthreads to handle concurrent client requests in a scalable manner.
2. **Concurrency Control**: Ensure thread-safe operations using synchronization primitives like mutexes and condition variables.
3. **Efficient Resource Management**: Use a producer-consumer model to manage client connections and optimize CPU utilization across multiple threads.
4. **File Handling**: Implement HTTP GET and PUT request handling with proper file I/O operations, ensuring concurrent access is handled safely with file locks.
5. **Debugging Multi-threaded Applications**: Identify and resolve common concurrency issues such as race conditions and deadlocks.

## Features

- **Thread Pool**: The server uses a configurable number of worker threads to handle incoming client connections concurrently.
- **Producer-Consumer Queue**: A thread-safe queue is used to manage client requests, following a producer-consumer model where the main thread adds tasks and worker threads process them.
- **File I/O with Thread Safety**: Supports handling GET and PUT requests with appropriate file locking mechanisms to prevent data corruption during concurrent access.
- **Concurrency Debugging**: Thoroughly tested and debugged for race conditions and deadlocks using **gdb** and other debugging tools.

## Usage

### Prerequisites

- Linux-based system (or any POSIX-compliant system)
- GCC compiler
- pthreads library

### Compilation

To compile the project, use the provided `Makefile`. Simply run the following command in the terminal:
- ./httpserver [-t threads] <port>

### Project Structure
├── asgn2_helper_funcs.h     # Helper functions
├── connection.h             # Handles client-server connection
├── debug.h                  # Debugging utilities
├── httpserver.c             # Main HTTP server implementation
├── Makefile                 # Build configuration
├── queue.h                  # Task queue implementation
├── request.h                # HTTP request handling
├── response.h               # HTTP response handling
└── README.md                # Project documentation



