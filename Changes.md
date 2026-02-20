# Changes and Fixes --  Oberon Linux Bootloader Modernization (2026)

This document summarizes all modifications and fixes that were necessary to successfully build and run the bootloader for Oberon system V4 on a modern 64-bit Linux distribution. The update was done with a lot of help by ChatGPT, since I am not really deep in C-programming and all changes here were also summarized by ChatGPT.

------------------------------------------------------------------------

## 1. 32-bit Environment (multilib)

The build required 32-bit support.

``` bash
sudo dpkg --add-architecture i386
sudo apt update
sudo apt install gcc-multilib libc6-dev-i386
```

This allowed compiling with:

```bash
gcc -m32
```

---

## 2. POSIX and GNU Feature Macros

Some functions and system calls were not visible to the compiler.

### Fix

The following macros were added at the very beginning of `obwrapper.c`:

```c
#define _POSIX_C_SOURCE 200809L
#define _GNU_SOURCE
```

This ensured proper declaration of modern POSIX and GNU interfaces.

---

## 3. Missing Headers

Compilation errors and runtime issues were caused by missing system headers.

### Fix

``` c
#include <pthread.h>
#include <stdio.h>
#include <stdint.h>
#include <sys/mman.h>
#include <unistd.h>
```

These fixed:

* memory management visibility
* correct declaration of `_exit`
* runtime stability

---

## 4. Segmentation Fault at Startup

The newly built executable crashed immediately, although older binaries worked.

### Diagnosis

The heap initialization and memory mapping were suspected. Debugging with:

```c
printf("heapAdr = %p, heapSize = %d\n", heapAdr, heapSize);
```

confirmed that the crash occurred after heap setup.

### Root Cause

Incorrect or incomplete memory mapping and missing system call headers caused undefined behavior.

### Fix

Rewriting and cleaning the memory initialization and `mmap` usage in `linux.oberon.c`.

After correcting the implementation, the segmentation fault disappeared.

---

## 5. Wrapper Library Issues

The dynamic wrapper library required:

* correct threading support
* correct 32-bit shared object compilation.

### Build confirmed working:

```bash
gcc -m32 -shared -fPIC -o libobwrapper.so obwrapper.o -lpthread
```

---

## 6. Runtime Script (`sob`)

The startup script was verified and confirmed working. It:

* sets `OBROOT`
* creates symlinks
* configures font path
* exports `OBERON` and `LD_LIBRARY_PATH`
* launches the system.


No modification was required now.

The startup script was modified by me some years ago, since the backticked find statement in line 8 had problems with the sequence of arguments (around 2019, see last section (OberonV4 for Linux) in https://sourceforge.net/p/oberon/wiki/Tools/).

---

## 7. Build Warnings Cleanup

Warnings after `make clean && make all`:

### `_exit` implicit declaration

Fixed by:

```c
#include <unistd.h>
```

### Non-void functions missing return

Functions such as:

* `dl_close`
* `dl_sym`

were updated to return proper values.

### Unused variable

Removed or ignored.

Warnings were confirmed harmless, and the system worked correctly.

---

## 8.  Makefile Adjustments

### 8.1 Enforce 32-bit Build

The original system depends on a 32-bit memory layout.

Added `-m32` consistently to:

- compilation flags
- linker flags
- shared library build

``` make
CFLAGS = -O2 -Wall -m32 -fPIC
LDFLAGS = -m32
```

## 9. Final Result

The system now:

* builds cleanly on modern Linux
* runs without segmentation faults
* supports 32-bit execution
* correctly loads shared modules
* is stable and compatible with existing Oberon binaries.

---

## 9. Verification

Successful test:

```bash
make clean && make all
./sob
```

Output confirmed correct heap initialization and successful startup.

---

**Status:** Stable and functional.
**Date:** 2026-Feb-19
**Author:** Bernhard Treutwein with help from ChatGPT

