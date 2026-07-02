<div align="center">
  <h1>🧠 SmartELFLoader</h1>
  <p><b>A custom, lazily-evaluated ELF 32-bit executable loader with dynamic page-fault handling</b></p>
</div>

<br/>

## 📖 Overview

**SmartELFLoader** is a systems programming project that acts as an intelligent loader for 32-bit ELF executables. Unlike a standard upfront loader, this project implements **lazy loading**—a technique used by modern operating systems like Linux. It loads program segments into memory *only* when they are strictly needed during execution.

By intercepting invalid memory access errors (Segmentation Faults) and dynamically allocating memory on the fly using `mmap`, this loader demonstrates advanced control over virtual memory, page fault handling, and memory fragmentation tracking.

## ✨ Key Features

### 1. Lazy Loading & Page Fault Interception
Instead of mapping the entire executable into memory at startup, the loader attempts to execute the `_start` method directly. This intentionally triggers a segmentation fault. The loader intercepts this fault via custom signal handlers, treating it as a legitimate **Page Fault**.

### 2. On-Demand Dynamic Memory Allocation
When a page fault is caught, the loader dynamically calculates the exact segment required. It allocates memory strictly in multiples of the standard page size (`4KB`) using `mmap`, copies the required block of data from the ELF file into memory, and seamlessly resumes program execution.

### 3. Memory Profiling & Statistics
Upon successful execution of the loaded ELF file, the loader profiles its own memory efficiency by outputting:
- **Total Page Faults:** How many times the program tried to access unmapped memory.
- **Total Page Allocations:** How many 4KB pages were dynamically mapped.
- **Internal Fragmentation:** The exact amount of wasted memory (in KB) due to the 4KB page boundary padding.

## 🛠️ Tech Stack & Architecture

- **Language:** C
- **Concepts:** Virtual Memory, Page Faults (`SIGSEGV`), ELF Binary Parsing, `mmap`, Signal Handling.
- **Environment:** Unix/Linux, GNU Make, GCC 

### 📂 Files Coded for this Assignment
- `loader/loader.c` - Contains the core logic for the custom signal handler, dynamic page allocation, and ELF segment parsing.
- `launcher/launch.c` - Bootstraps the execution environment.

## 🚀 Quick Start

### Prerequisites
You need a Unix-based environment (Linux or WSL) to run this, as it relies heavily on native Unix system calls (`mmap`, `signal`).

### Building and Running
1. Clone the repository and compile the loader and test files:
   ```bash
   make clean
   make
   ```
2. Run the loader with a compiled 32-bit ELF executable (e.g., the provided Fibonacci or Sum test files):
   ```bash
   ./loader test/fib
   ```
3. The loader will execute the binary, handle the page faults seamlessly, and print out the final memory fragmentation statistics!

---
*This project was completed as part of the Operating Systems course (Monsoon 2025). Built to explore the internals of process loading and virtual memory management.*
