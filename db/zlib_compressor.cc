// Copyright (c) 2011 The LevelDB Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file. See the AUTHORS file for names of contributors.

#ifndef NO_ZLIB

#include "leveldb/zlib_compressor.h"

#include <climits>
#include <cmath>
#include <vector>

#include <zlib.h>

namespace leveldb {

namespace {

unsigned long ZlibCompressBound(unsigned long sourceLen) {
    return static_cast<unsigned long>(std::ceil(sourceLen * 1.001)) + 12;
}

// Decompress a raw zlib stream. Returns Z_OK on success, a zlib error code
// otherwise. On any error `output` contents are undefined; the caller must
// treat the block as corrupt.
int ZlibInflate(const char* input, size_t length, std::string& output) {
    if (length > static_cast<size_t>(UINT_MAX)) {
        return Z_DATA_ERROR;
    }

    const int CHUNK = 64 * 1024;
    std::vector<unsigned char> out(CHUNK);

    z_stream strm;
    strm.zalloc = Z_NULL;
    strm.zfree = Z_NULL;
    strm.opaque = Z_NULL;
    strm.avail_in = static_cast<uInt>(length);
    strm.next_in = reinterpret_cast<Bytef*>(const_cast<char*>(input));

    int ret = inflateInit(&strm);
    if (ret != Z_OK) {
        return ret;
    }

    do {
        do {
            strm.avail_out = static_cast<uInt>(CHUNK);
            strm.next_out = out.data();

            ret = inflate(&strm, Z_NO_FLUSH);

            if (ret == Z_NEED_DICT) {
                ret = Z_DATA_ERROR;
            }
            if (ret < 0) {
                inflateEnd(&strm);
                return ret;
            }

            const size_t have = static_cast<size_t>(CHUNK) - strm.avail_out;
            output.append(reinterpret_cast<char*>(out.data()), have);
        } while (strm.avail_out == 0);
    } while (ret != Z_STREAM_END);

    inflateEnd(&strm);
    return ret == Z_STREAM_END ? Z_OK : Z_DATA_ERROR;
}

}  // namespace

bool ZlibCompressor::compressImpl(const char* input,
                                  size_t length,
                                  std::string& buffer) const {
    // zlib's avail_in is a 32-bit unsigned value; chunks larger than that
    // would silently truncate. LevelDB blocks are kilobytes, so this only
    // serves as a defensive guard against misuse.
    if (length > static_cast<size_t>(UINT_MAX)) {
        return false;
    }

    const size_t BUFSIZE = 128 * 1024;
    std::vector<unsigned char> temp_buffer(BUFSIZE);

    buffer.reserve(ZlibCompressBound(static_cast<unsigned long>(length)));

    z_stream strm;
    strm.zalloc = Z_NULL;
    strm.zfree = Z_NULL;
    strm.opaque = Z_NULL;
    strm.next_in = reinterpret_cast<Bytef*>(const_cast<char*>(input));
    strm.avail_in = static_cast<uInt>(length);
    strm.next_out = temp_buffer.data();
    strm.avail_out = static_cast<uInt>(BUFSIZE);

    int init_res = deflateInit(&strm, compressionLevel);
    if (init_res != Z_OK) {
        return false;
    }

    // Feed all input. A Z_OK return with avail_in != 0 means zlib wants a
    // bigger output buffer; avail_out==0 is our flush trigger.
    while (strm.avail_in != 0) {
        const int deflate_res = deflate(&strm, Z_NO_FLUSH);
        if (deflate_res != Z_OK) {
            deflateEnd(&strm);
            return false;
        }
        if (strm.avail_out == 0) {
            buffer.insert(buffer.end(), temp_buffer.begin(), temp_buffer.end());
            strm.next_out = temp_buffer.data();
            strm.avail_out = static_cast<uInt>(BUFSIZE);
        }
    }

    // Flush trailing data until the stream terminates.
    int deflate_res = Z_OK;
    while (deflate_res == Z_OK) {
        if (strm.avail_out == 0) {
            buffer.insert(buffer.end(), temp_buffer.begin(), temp_buffer.end());
            strm.next_out = temp_buffer.data();
            strm.avail_out = static_cast<uInt>(BUFSIZE);
        }
        deflate_res = deflate(&strm, Z_FINISH);
    }

    if (deflate_res != Z_STREAM_END) {
        deflateEnd(&strm);
        return false;
    }
    buffer.insert(buffer.end(),
                  temp_buffer.begin(),
                  temp_buffer.begin() + (BUFSIZE - strm.avail_out));
    deflateEnd(&strm);
    return true;
}

bool ZlibCompressor::decompress(const char* input,
                                size_t length,
                                std::string& output) const {
    output.clear();
    return ZlibInflate(input, length, output) == Z_OK;
}

}  // namespace leveldb

#endif  // NO_ZLIB
