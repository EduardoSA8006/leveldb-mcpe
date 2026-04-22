// Copyright (c) 2011 The LevelDB Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file. See the AUTHORS file for names of contributors.

#ifndef STORAGE_LEVELDB_INCLUDE_ZLIB_COMPRESSOR_H_
#define STORAGE_LEVELDB_INCLUDE_ZLIB_COMPRESSOR_H_

#include <cassert>

#include "leveldb/compressor.h"

namespace leveldb {

    class DLLX ZlibCompressor : public Compressor {
    public:
        static const char SERIALIZE_ID = 2;

        const int compressionLevel;

        virtual ~ZlibCompressor() {}

        // compressionLevel is passed to zlib's deflateInit; -1 requests the
        // library default (6 as of zlib 1.2), 0 means "no compression",
        // and 1..9 trade speed against ratio.
        explicit ZlibCompressor(int compressionLevel = -1)
            : Compressor(SERIALIZE_ID),
              compressionLevel(compressionLevel) {
            assert(compressionLevel >= -1 && compressionLevel <= 9);
        }

        bool compressImpl(const char* input, size_t length, ::std::string& output) const override;

        bool decompress(const char* input, size_t length, ::std::string& output) const override;
    };

}

#endif  // STORAGE_LEVELDB_INCLUDE_ZLIB_COMPRESSOR_H_
