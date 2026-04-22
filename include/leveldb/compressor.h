// Copyright (c) 2011 The LevelDB Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file. See the AUTHORS file for names of contributors.

#ifndef STORAGE_LEVELDB_INCLUDE_COMPRESSOR_H_
#define STORAGE_LEVELDB_INCLUDE_COMPRESSOR_H_

#include <atomic>
#include <cstdint>
#include <string>

namespace leveldb {
    class DLLX Slice;

    // Abstract base for pluggable block compressors.
    //
    // Ownership: a Compressor installed in Options::compressors[] is NOT
    // owned by the DB. The caller is responsible for keeping the object
    // alive for the entire lifetime of any DB opened with those options
    // and for deleting it afterwards.
    //
    // Threading: compressImpl() and decompress() are invoked concurrently
    // from multiple background threads (compaction, flush) on the same
    // Compressor instance and therefore must be thread-safe. The built-in
    // statistic counters use std::atomic so the base class is safe; any
    // mutable state added by a subclass is the subclass's responsibility.
    class DLLX Compressor {
    public:
        // An ID that must be unique across the whole DB. It is written into
        // the trailer of every compressed block so that the reader knows
        // which compressor to invoke for decompression. 0 is reserved for
        // "uncompressed" and must not be used by a subclass.
        const char uniqueCompressionID;

        virtual ~Compressor() {}

        explicit Compressor(char uniqueCompressionID)
            : inputBytes_(0),
              compressedBytes_(0),
              uniqueCompressionID(uniqueCompressionID) {
        }

        double getAverageCompression() const {
            const uint64_t in = inputBytes_.load(std::memory_order_relaxed);
            const uint64_t out = compressedBytes_.load(std::memory_order_relaxed);
            return in ? (static_cast<double>(out) / static_cast<double>(in)) : 0.0;
        }

        void resetAverageCompressionStats() {
            inputBytes_.store(0, std::memory_order_relaxed);
            compressedBytes_.store(0, std::memory_order_relaxed);
        }

        uint64_t inputBytes() const {
            return inputBytes_.load(std::memory_order_relaxed);
        }

        uint64_t compressedBytes() const {
            return compressedBytes_.load(std::memory_order_relaxed);
        }

        // Compress `length` bytes from `input` into `output`. Returns true on
        // success. On failure `output` is cleared and the caller must treat
        // the data as incompressible.
        bool compress(const char* input, size_t length, ::std::string& output) {
            output.clear();
            if (!compressImpl(input, length, output)) {
                output.clear();
                return false;
            }
            inputBytes_.fetch_add(length, std::memory_order_relaxed);
            compressedBytes_.fetch_add(output.length(), std::memory_order_relaxed);
            return true;
        }

        bool compress(const std::string& in, std::string& out) {
            return compress(in.data(), in.length(), out);
        }

        // Subclasses implement the actual algorithm. Must return true on
        // success and false for any failure (out-of-memory, library error,
        // oversized input, etc.). On failure the contents of `output` are
        // undefined; the base compress() wrapper clears it.
        virtual bool compressImpl(const char* input, size_t length, ::std::string& output) const = 0;

        // Decompress `length` bytes from `input` into `output`. Returns true
        // on success. On failure `output` is undefined and the caller must
        // treat the block as corrupt.
        virtual bool decompress(const char* input, size_t length, ::std::string& output) const = 0;

        bool decompress(const std::string& input, ::std::string& output) const {
            return decompress(input.data(), input.length(), output);
        }

    private:
        std::atomic<uint64_t> inputBytes_;
        std::atomic<uint64_t> compressedBytes_;

        // Atomics are neither copyable nor movable; forbid both explicitly
        // so misuse of a Compressor instance becomes a compile error.
        Compressor(const Compressor&) = delete;
        Compressor& operator=(const Compressor&) = delete;
    };
}

#endif  // STORAGE_LEVELDB_INCLUDE_COMPRESSOR_H_
