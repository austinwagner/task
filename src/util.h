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
  std::string getErrorString (DWORD errCode);

  int setenv (const char *name, const char *value, int overwrite);
  int unsetenv (const char *name);

  struct CloseHandleCloser {
    void operator() (HANDLE handle) {
      CloseHandle (handle);
    }
  };

  struct FindCloseCloser {
    void operator() (HANDLE handle) {
      FindClose (handle);
    }
  };

  template <typename Closer> class SafeHandleBase {
  private:
    HANDLE _handle;
    Closer _closer;
    HANDLE _nullValue;

  public:
    SafeHandleBase (HANDLE nullValue) : _handle (nullValue), _nullValue (nullValue) {}
    SafeHandleBase (HANDLE nullValue, HANDLE handle) : _handle (handle), _nullValue (nullValue) {}

    void reset () {
      reset (_nullValue);
    }

    void reset (HANDLE handle) {
      if (_handle != _nullValue) {
        _closer (_handle);
      }

      _handle = handle;
    }
    virtual ~SafeHandleBase () {
      _closer (_handle);
    }
    HANDLE get () {
      return _handle;
    }
    HANDLE *ptr () {
      return &_handle;
    }
    bool valid () {
      return _handle != _nullValue;
    }
  };

  class SafeHandle : public SafeHandleBase<CloseHandleCloser> {
  public:
    SafeHandle () : SafeHandleBase (INVALID_HANDLE_VALUE) {}
    explicit SafeHandle (HANDLE handle) : SafeHandleBase (INVALID_HANDLE_VALUE, handle) {}
  };

  class SafeHandle2 : public SafeHandleBase<CloseHandleCloser> {
  public:
    SafeHandle2 () : SafeHandleBase (NULL) {}
    explicit SafeHandle2 (HANDLE handle) : SafeHandleBase (NULL, handle) {}
  };

  class FindHandle : public SafeHandleBase<FindCloseCloser> {
  public:
    FindHandle () : SafeHandleBase (INVALID_HANDLE_VALUE) {}
    explicit FindHandle (HANDLE handle) : SafeHandleBase (INVALID_HANDLE_VALUE, handle) {}
  };

  time_t filetime_to_timet (const FILETIME &ft);

  bool supports_ansi_codes ();

  class WindowsError : public std::runtime_error {
  public:
    explicit WindowsError (const std::string &msg) : std::runtime_error (msg) {}
  };

#define WIN_TRY(f) do {                                                                                                \
    if (!(f)) {                                                                                                        \
      std::ostringstream oss;                                                                                          \
      oss << getErrorString (GetLastError ()) << " (" << __FILE__ << ":" << __LINE__ << ")";                           \
      throw WindowsError (oss.str ());                                                                                 \
    }                                                                                                                  \
  } while (0)

#define THROW_WIN_ERROR(s) do {                                                                                        \
    std::ostringstream oss;                                                                                            \
    oss << (s) << " " << getErrorString (GetLastError ());                                                             \
    throw WindowsError (oss.str ());                                                                                   \
  } while (0)

#define THROW_WIN_ERROR_FMT(s, ...) do {                                                                               \
    std::ostringstream oss;                                                                                            \
    oss << format ((s), __VA_ARGS__) << " " << getErrorString (GetLastError ());                                       \
    throw WindowsError (oss.str ());                                                                                   \
  } while (0)

#if _MSC_VER < 1900
  inline int snprintf (char *dest, size_t len, const char *format, ...) {
    va_list args;
    va_start (args, format);
    int result = vsprintf_s (dest, len, format, args);
    va_end (args);
    return result;
  }
#endif

#endif

#endif
  ////////////////////////////////////////////////////////////////////////////////
