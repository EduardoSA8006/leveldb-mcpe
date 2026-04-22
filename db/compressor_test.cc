// Copyright (c) 2011 The LevelDB Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file. See the AUTHORS file for names of contributors.

#include <string>

#include "leveldb/compressor.h"
#include "leveldb/zlib_compressor.h"
#ifdef SNAPPY
#include "leveldb/snappy_compressor.h"
#endif
#include "util/testharness.h"

namespace leveldb {

namespace {

std::string RepeatedText(const std::string& token, size_t count) {
    std::string s;
    s.reserve(token.size() * count);
    for (size_t i = 0; i < count; ++i) s.append(token);
    return s;
}

std::string PseudoRandomBytes(size_t n, uint32_t seed) {
    std::string s;
    s.resize(n);
    uint32_t state = seed;
    for (size_t i = 0; i < n; ++i) {
        state = state * 1103515245u + 12345u;
        s[i] = static_cast<char>((state >> 16) & 0xff);
    }
    return s;
}

void RoundTrip(Compressor* c, const std::string& in) {
    std::string compressed;
    std::string restored;
    ASSERT_TRUE(c->compress(in.data(), in.size(), compressed));
    ASSERT_TRUE(c->decompress(compressed.data(), compressed.size(), restored));
    ASSERT_EQ(in.size(), restored.size());
    ASSERT_TRUE(in == restored);
}

}  // namespace

class CompressorTest {};

TEST(CompressorTest, ZlibRoundTripEmpty) {
    ZlibCompressor c;
    RoundTrip(&c, std::string());
}

TEST(CompressorTest, ZlibRoundTripShort) {
    ZlibCompressor c;
    RoundTrip(&c, std::string("hello world"));
}

TEST(CompressorTest, ZlibRoundTripHighlyCompressible) {
    ZlibCompressor c;
    const std::string in = RepeatedText("abcd", 4096);
    std::string compressed;
    ASSERT_TRUE(c.compress(in.data(), in.size(), compressed));
    ASSERT_LT(compressed.size(), in.size() / 4);
    std::string restored;
    ASSERT_TRUE(c.decompress(compressed.data(), compressed.size(), restored));
    ASSERT_TRUE(in == restored);
}

TEST(CompressorTest, ZlibRoundTripIncompressible) {
    ZlibCompressor c;
    RoundTrip(&c, PseudoRandomBytes(8192, 0xDEADBEEF));
}

TEST(CompressorTest, ZlibRoundTripAcrossCompressionLevels) {
    for (int level = -1; level <= 9; ++level) {
        ZlibCompressor c(level);
        RoundTrip(&c, RepeatedText("The quick brown fox. ", 512));
    }
}

TEST(CompressorTest, ZlibDecompressRejectsGarbage) {
    ZlibCompressor c;
    const std::string garbage = "this is certainly not a valid zlib stream";
    std::string out;
    ASSERT_TRUE(!c.decompress(garbage.data(), garbage.size(), out));
}

TEST(CompressorTest, ZlibStatsTrackInputAndOutput) {
    ZlibCompressor c;
    ASSERT_EQ(c.inputBytes(), uint64_t(0));
    ASSERT_EQ(c.compressedBytes(), uint64_t(0));

    const std::string in = RepeatedText("xyz", 1000);
    std::string out;
    ASSERT_TRUE(c.compress(in.data(), in.size(), out));

    ASSERT_EQ(c.inputBytes(), static_cast<uint64_t>(in.size()));
    ASSERT_EQ(c.compressedBytes(), static_cast<uint64_t>(out.size()));
    ASSERT_GT(c.getAverageCompression(), 0.0);

    c.resetAverageCompressionStats();
    ASSERT_EQ(c.inputBytes(), uint64_t(0));
    ASSERT_EQ(c.compressedBytes(), uint64_t(0));
}

TEST(CompressorTest, ZlibUniqueIdIsTwo) {
    ZlibCompressor c;
    ASSERT_EQ(static_cast<int>(c.uniqueCompressionID),
              static_cast<int>(ZlibCompressor::SERIALIZE_ID));
    ASSERT_EQ(static_cast<int>(c.uniqueCompressionID), 2);
}

#ifdef SNAPPY
TEST(CompressorTest, SnappyRoundTripShort) {
    SnappyCompressor c;
    RoundTrip(&c, std::string("hello world, this is a snappy round trip"));
}

TEST(CompressorTest, SnappyRoundTripHighlyCompressible) {
    SnappyCompressor c;
    RoundTrip(&c, RepeatedText("repeat ", 1024));
}

TEST(CompressorTest, SnappyDecompressRejectsGarbage) {
    SnappyCompressor c;
    const std::string garbage = "not a snappy stream at all";
    std::string out;
    ASSERT_TRUE(!c.decompress(garbage.data(), garbage.size(), out));
}

TEST(CompressorTest, SnappyUniqueIdIsOne) {
    SnappyCompressor c;
    ASSERT_EQ(static_cast<int>(c.uniqueCompressionID),
              static_cast<int>(SnappyCompressor::SERIALIZE_ID));
    ASSERT_EQ(static_cast<int>(c.uniqueCompressionID), 1);
}
#endif

}  // namespace leveldb

int main(int argc, char** argv) {
    return leveldb::test::RunAllTests();
}
