# SmartELFLoader
> A custom, lazily-evaluated ELF 32-bit executable loader with dynamic page-fault handling.

SmartELFLoader is a systems programming project that acts as an intelligent loader for 32-bit ELF executables. Unlike standard upfront loaders, this project implements lazy loading, loading program segments into memory only when they are strictly requested by the CPU during execution.

By intercepting invalid memory access errors and dynamically allocating memory on the fly using `mmap`, this loader demonstrates advanced control over virtual memory, page fault handling, and memory fragmentation tracking.

---

## Architecture & Features

### Lazy Loading & Page Fault Interception
Instead of mapping the entire executable into memory at startup, the loader attempts to execute the `_start` method directly. This intentionally triggers a segmentation fault. The loader intercepts this fault via custom signal handlers, treating it as a legitimate Page Fault.

### On-Demand Dynamic Memory Allocation
Upon catching a page fault, the loader dynamically calculates the exact segment required. It allocates memory strictly in multiples of the standard page size (4KB) using `mmap`, copies the required block of data from the ELF file into memory, and resumes program execution.

### Memory Profiling & Statistics
Upon successful execution of the loaded ELF file, the loader profiles its own memory efficiency by outputting total page faults, total 4KB page allocations, and the exact amount of internal fragmentation (in KB).

---

## Technical Specifications

- **Language**: C (C99 standard)
- **Core Abstractions**: Virtual Memory, Page Faults (`SIGSEGV`), ELF Binary Parsing, `mmap`, Signal Handling.
- **Environment**: Unix/Linux, GCC, GNU Make

---

## Usage Guide

**1. Compilation**
```bash
make clean && make
```

**2. Execution**
Run the loader with a compiled 32-bit ELF executable.
```bash
./loader test/fib
```
*(The loader will execute the binary, handle the page faults seamlessly, and output the final memory fragmentation statistics to standard output).*
