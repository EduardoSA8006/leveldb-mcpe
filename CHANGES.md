# Bug Fixes and Code Improvements

This document summarizes the bug fixes and improvements made in the
`fix/code-improvement` branch.

---

## Bug Fixes

### `db/c.cc` — Memory leaks and missing `break`

- **`leveldb_options_destroy` leaked compressors installed via the C API.**
  Compressor pointers stored in `options->rep.compressors[]` were never freed.
  The destructor now iterates over the array and deletes every non-null entry
  before deleting the options struct.

- **`leveldb_options_set_compression` leaked the previous compressor on
  repeated calls.** Each call allocated a new compressor object and
  overwrote the slot without freeing the old one first. The old pointer is
  now deleted before the slot is reassigned.

- **Missing `break` in `switch` for `zlib_compression`.** The absence of
  the `break` statement caused silent fall-through, leaving the switch in
  an undefined state after installing the `ZlibCompressor`.

### `table/format.cc` — Wrong compressor selected on read / crash on unknown ID

- **Inverted condition in the compressor-lookup loop.**
  The original condition `!c || c->uniqueCompressionID == compressionID`
  would match the *first null slot* rather than the correct compressor,
  making decompression use whichever compressor happened to be in that slot
  (or dereferencing null). Changed to `c && c->uniqueCompressionID == compressionID`.

- **`assert` replaced with a proper `Status::Corruption` return.**
  When no compressor matching the block trailer ID is found, the old code
  called `assert(compressor != nullptr)`, which crashes in release builds.
  It now returns `Status::Corruption("block trailer references an
  unregistered compressor id")` and frees the buffer correctly.

### `port/port_winrt.cc` and `port/port_wp8.cc` — `CondVar::SignalAll` only woke one waiter

The original loop decremented `waiting_` inside a `for` that iterated over
`waiting_`, so the loop body executed exactly once regardless of how many
threads were waiting. Fixed by snapshotting the waiter count before the
loop, issuing one `ReleaseSemaphore` per waiter, and then draining the
acknowledgement semaphore in a separate pass.

---

## Code Improvements

### `include/leveldb/compressor.h` — Thread-safety and cleaner interface

- **Statistics counters made thread-safe.** `inputBytes` and
  `compressedBytes` were plain `uint64_t` public member variables mutated
  from background compaction/flush threads without synchronization. They
  are now `std::atomic<uint64_t>` private members exposed through
  read-only accessors.

- **`compressImpl` return type changed from `void` to `bool`.** This
  allows subclasses to report compression failures to the caller. The base
  `compress()` wrapper propagates the failure, clears the output buffer,
  and skips updating the statistics counters on error.

- **Copy and move constructors deleted explicitly.** The atomics make the
  class non-copyable/non-movable anyway; making that a compile-time error
  prevents accidental misuse.

- **Header guard changed from `#pragma once` to the standard
  `#ifndef` / `#define` / `#endif` pattern** used throughout the rest of
  the codebase.

### `db/zlib_compressor.cc` — Proper error handling and safer stack usage

- **`deflateInit` return value was ignored.** A failure there (e.g.
  out-of-memory) would leave the `z_stream` in an uninitialised state and
  produce garbage output. The return value is now checked; `compressImpl`
  returns `false` on failure.

- **`deflate()` errors during compression were silently ignored.** Any
  non-`Z_OK` return from `deflate()` now causes an early exit with
  `deflateEnd` cleanup and a `false` return.

- **Stack-allocated VLA replaced with `std::vector`.** The 128 KB
  `unsigned char temp_buffer[BUFSIZE]` was allocated on the stack. Large
  blocks or deep call stacks could overflow. Replaced with
  `std::vector<unsigned char>`.

- **Overflow guard added before casting `length` to `uInt`.** If `length`
  exceeds `UINT_MAX`, `compressImpl` returns `false` immediately instead
  of silently truncating the value in the cast.

- **Missing newline at end of file added.**

### `db/snappy_compressor.cc` — Consistent interface

- `compressImpl` now returns `bool` to match the updated base class
  interface.
- C-style cast `(char*)output.data()` replaced with `&output[0]`.

### `table/table_builder.cc` — Explicit types and error propagation

- `auto compressor` changed to `Compressor*` for readability.
- The return value of `compressor->compress()` is now checked; if
  compression fails the block is stored uncompressed instead of writing
  whatever partial data ended up in the output buffer.
- Indentation and `if/else` structure normalised.

### `util/Filepath.h` — Removed deprecated C++17/C++26 API

- `std::wstring_convert` / `std::codecvt_utf8` were deprecated in C++17
  and removed in C++26. `toFilePath` now calls `MultiByteToWideChar`
  (Win32) directly, which is available on all supported Windows targets.
- `fopen_mb(const char*, ...)` overload moved inside the correct
  `#if defined(_MSC_VER)` guard.
- Header guard changed from `#pragma once` to the standard `#ifndef`
  pattern.

### `db/compressor_test.cc` — New test file

Unit tests added for both `ZlibCompressor` and `SnappyCompressor`
covering round-trip compression/decompression and error paths.
