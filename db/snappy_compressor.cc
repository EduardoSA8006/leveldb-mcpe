// Copyright (c) 2011 The LevelDB Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file. See the AUTHORS file for names of contributors.

#ifdef SNAPPY

#include "leveldb/snappy_compressor.h"

#include <snappy.h>

namespace leveldb {

bool SnappyCompressor::compressImpl(const char* input,
                                    size_t length,
                                    std::string& output) const {
    output.resize(snappy::MaxCompressedLength(length));
    size_t outlen = 0;
    snappy::RawCompress(input, length, &output[0], &outlen);
    output.resize(outlen);
    return true;
}

bool SnappyCompressor::decompress(const char* input,
                                  size_t length,
                                  std::string& output) const {
    size_t ulength = 0;
    if (!snappy::GetUncompressedLength(input, length, &ulength)) {
        return false;
    }
    output.resize(ulength);
    return snappy::RawUncompress(input, length, &output[0]);
}

}  // namespace leveldb

#endif  // SNAPPY
