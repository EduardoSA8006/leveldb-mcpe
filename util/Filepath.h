// Copyright (c) 2011 The LevelDB Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file. See the AUTHORS file for names of contributors.

#ifndef STORAGE_LEVELDB_UTIL_FILEPATH_H_
#define STORAGE_LEVELDB_UTIL_FILEPATH_H_

#if defined(_MSC_VER)
#include <windows.h>
#include <cstdio>
#include <string>
#endif

namespace port {

#if defined(_MSC_VER)
    // Windows file APIs operate on wide-character paths. std::string arguments
    // are converted from UTF-8 on the fly by the helpers below.
    typedef std::wstring filepath;
    typedef wchar_t filepath_char;
#define _FILE_STR(str) L ## str
#else
    typedef std::string filepath;
    typedef char filepath_char;
#define _FILE_STR(str) str
#endif

#if defined(_MSC_VER)
    inline filepath toFilePath(const std::string& in) {
        // std::wstring_convert and std::codecvt_utf8 were deprecated in C++17
        // and removed in C++26; use the Win32 API directly instead.
        if (in.empty()) {
            return filepath();
        }
        const int size_needed = ::MultiByteToWideChar(
            CP_UTF8, 0, in.data(), static_cast<int>(in.size()), nullptr, 0);
        if (size_needed <= 0) {
            return filepath();
        }
        filepath result(static_cast<size_t>(size_needed), L'\0');
        ::MultiByteToWideChar(
            CP_UTF8, 0, in.data(), static_cast<int>(in.size()),
            &result[0], size_needed);
        return result;
    }

    inline FILE* fopen_mb(const filepath_char* filename, const filepath_char* mode) {
        FILE* file = nullptr;
        errno_t error = _wfopen_s(&file, filename, mode);
        _set_errno(error);
        return file;
    }

    // Convenience overload that takes a UTF-8 char* path. Allocates an
    // intermediate wide string on every call.
    inline FILE* fopen_mb(const char* filename, const filepath_char* mode) {
        const filepath path = toFilePath(filename);
        return port::fopen_mb(path.c_str(), mode);
    }
#else
    inline filepath toFilePath(const std::string& in) {
        return in;
    }

    inline FILE* fopen_mb(const filepath_char* filename, const filepath_char* mode) {
        return ::fopen(filename, mode);
    }
#endif

}  // namespace port

#endif  // STORAGE_LEVELDB_UTIL_FILEPATH_H_
