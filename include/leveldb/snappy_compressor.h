// Copyright (c) 2011 The LevelDB Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file. See the AUTHORS file for names of contributors.

#ifndef STORAGE_LEVELDB_INCLUDE_SNAPPY_COMPRESSOR_H_
#define STORAGE_LEVELDB_INCLUDE_SNAPPY_COMPRESSOR_H_

#include "leveldb/compressor.h"

namespace leveldb {

    class DLLX SnappyCompressor : public Compressor {
    public:
        static const char SERIALIZE_ID = 1;

        virtual ~SnappyCompressor() {}

        SnappyCompressor() : Compressor(SERIALIZE_ID) {}

        bool compressImpl(const char* input, size_t length, ::std::string& output) const override;

        bool decompress(const char* input, size_t length, ::std::string& output) const override;
    };

}

#endif  // STORAGE_LEVELDB_INCLUDE_SNAPPY_COMPRESSOR_H_
