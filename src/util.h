////////////////////////////////////////////////////////////////////////////////
//
// Copyright 2006 - 2015, Paul Beckingham, Federico Hernandez.
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included
// in all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
// OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
// THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.
//
// http://www.opensource.org/licenses/mit-license.php
//
////////////////////////////////////////////////////////////////////////////////

#ifndef INCLUDED_UTIL
#define INCLUDED_UTIL

#include <cmake.h>
#include <string>
#include <vector>
#include <map>
#include <sys/types.h>
#ifdef FREEBSD
#include <uuid.h>
#elif defined(WINDOWS)
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <memory>
#include <sstream>
#else
#include <uuid/uuid.h>
#endif
#include <Task.h>

// util.cpp
bool confirm (const std::string&);
int confirm4 (const std::string&);
std::string formatBytes (size_t);
int autoComplete (const std::string&, const std::vector<std::string>&, std::vector<std::string>&, int minimum = 1);

#if !defined(HAVE_UUID_UNPARSE_LOWER) && !defined(WINDOWS)
void uuid_unparse_lower (uuid_t uu, char *out);
#endif
const std::string uuid ();

int execute (const std::string&, const std::vector <std::string>&, const std::string&, std::string&);

const std::string indentProject (
  const std::string&,
  const std::string& whitespace = "  ",
  char delimiter = '.');

const std::vector <std::string> extractParents (
  const std::string&,
  const char& delimiter = '.');

#ifndef HAVE_TIMEGM
  time_t timegm (struct tm *tm);
#endif

#ifdef WINDOWS
std::string getErrorString(DWORD errCode);

int setenv(const char *name, const char* value, int overwrite);
int unsetenv(const char *name);

struct CloseHandleCloser {
    void operator()(HANDLE handle) { CloseHandle(handle); }
};

struct FindCloseCloser {
    void operator()(HANDLE handle) { FindClose(handle); }
};

template <LONG_PTR NullValue, typename Closer>
class SafeHandleBase
{
private:
    HANDLE _handle;
    Closer closer;

public:
    SafeHandleBase() : _handle((HANDLE)NullValue) { }
    explicit SafeHandleBase(HANDLE handle) : _handle(handle) { }

    void reset() {
      reset((HANDLE)NullValue);
    }

    void reset(HANDLE handle) {
      if (_handle != (HANDLE)NullValue) {
        closer(_handle);
      }

      _handle = handle;
    }
    ~SafeHandleBase() { closer(_handle); }
    HANDLE get() { return _handle; }
    HANDLE* ptr() { return &_handle; }
    bool valid() { return _handle != (HANDLE)NullValue; }
};

typedef SafeHandleBase<(LONG_PTR)INVALID_HANDLE_VALUE, CloseHandleCloser> SafeHandle;
typedef SafeHandleBase<(LONG_PTR)NULL, CloseHandleCloser> SafeHandle2;
typedef SafeHandleBase<(LONG_PTR)INVALID_HANDLE_VALUE, FindCloseCloser> FindHandle;

time_t filetime_to_timet(const FILETIME& ft);

bool supports_ansi_codes();

#define WIN_TRY(f) if (!(f)) { \
  std::ostringstream oss; \
  oss << getErrorString(GetLastError()) << " (" << __FILE__ << ":" << __LINE__ << ")"; \
  throw oss.str(); \
}

#endif

#endif
////////////////////////////////////////////////////////////////////////////////
