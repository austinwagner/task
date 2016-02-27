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

#include <cmake.h>
#include <fstream>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <fcntl.h>
#include <File.h>
#include <text.h>
#include <util.h>
#include <i18n.h>
#include <limits>
#include <boost/numeric/conversion/cast.hpp>

////////////////////////////////////////////////////////////////////////////////
File::File ()
: Path::Path ()
, _locked (false)
{
}

////////////////////////////////////////////////////////////////////////////////
File::File (const Path& other)
: Path::Path (other)
, _locked (false)
{
}

////////////////////////////////////////////////////////////////////////////////
File::File (const File& other)
: Path::Path (other)
, _locked (false)
{
}

////////////////////////////////////////////////////////////////////////////////
File::File (const std::string& in)
: Path::Path (in)
, _locked (false)
{
}

////////////////////////////////////////////////////////////////////////////////
File::~File ()
{
}

////////////////////////////////////////////////////////////////////////////////
File& File::operator= (const File& other)
{
  if (this != &other)
    Path::operator= (other);

  _locked = false;
  return *this;
}

////////////////////////////////////////////////////////////////////////////////
bool File::create (int mode /* = 0640 */)
{
  if (open ())
  {
    // Ignore mode on Windows
    close ();
    return true;
  }

  return false;
}

////////////////////////////////////////////////////////////////////////////////
bool File::remove () const
{
  return boost::filesystem::remove(_path);
}

////////////////////////////////////////////////////////////////////////////////
bool File::open ()
{
  return open(false);
}

////////////////////////////////////////////////////////////////////////////////
bool File::open (bool lock)
{
  if (_path.empty() || _file.valid()) {
    return false;
  }

  _file.reset(CreateFileW(
    _path.wstring().c_str(),
    GENERIC_READ | GENERIC_WRITE,
    FILE_SHARE_READ | (lock ? 0u : FILE_SHARE_WRITE),
    nullptr,
    OPEN_ALWAYS,
    FILE_ATTRIBUTE_NORMAL,
    nullptr
  ));

  if (_file.valid()) {
    _locked = lock;
  }

  return _file.valid();
}

////////////////////////////////////////////////////////////////////////////////
bool File::openAndLock ()
{
  return open (true);
}

////////////////////////////////////////////////////////////////////////////////
void File::close ()
{
  if (_file.valid())
  {
    // Windows will unlock the file when the handle is closed
    _locked = false;
    _file.reset();
  }
}

////////////////////////////////////////////////////////////////////////////////
bool File::lock ()
{
  if (_locked) {
    return true;
  }

  close();
  return open(true);
}

////////////////////////////////////////////////////////////////////////////////
void File::unlock ()
{
  if (!_locked) {
    return;
  }

  close();
  open(false);
}

////////////////////////////////////////////////////////////////////////////////
// Opens if necessary.
void File::read (std::string& contents) {
  contents = "";
  if (!_file.valid()) {
    open();
  }

  contents.reserve (size ());

  char buffer[512];
  DWORD read = 1;

  while (read > 0) {
    WIN_TRY(ReadFile(_file.get(), buffer, sizeof(buffer), &read, nullptr));

    for (DWORD i = 0; i < read; i++) {
      char c = buffer[i];

      // Naive line ending conversion
      if (c != '\r') {
        contents.push_back(c);
      }
    }
  }
}

////////////////////////////////////////////////////////////////////////////////
// Opens if necessary.
void File::read (std::vector <std::string>& contents)
{
  contents.clear ();

  if (!_file.valid()) {
    open();
  }

  char buffer[512];
  DWORD read = 1;
  std::string line;

  while (read > 0) {
    WIN_TRY(ReadFile(_file.get(), buffer, sizeof(buffer), &read, nullptr));

    for (DWORD i = 0; i < read; i++) {
      char c = buffer[i];

      // Naive line ending conversion
      if (c == '\r') {
        continue;
      }
      else if (c == '\n') {
        if (!line.empty()) {
          contents.push_back(line);
          line = std::string();
        }
      }
      else {
        line.push_back(c);
      }
    }
  }

  if (!line.empty()) {
    contents.push_back(line);
  }
}

////////////////////////////////////////////////////////////////////////////////
// Opens if necessary.
void File::write (const std::string& line) {
  if (!_file.valid()) {
    open();
  }

  if (_file.valid()) {
    DWORD line_size = boost::numeric_cast<DWORD>(line.size());

    WIN_TRY(WriteFile(_file.get(), line.c_str(), line_size, nullptr, nullptr));
    WIN_TRY(WriteFile(_file.get(), "\n", 1, nullptr, nullptr));
  }
}

////////////////////////////////////////////////////////////////////////////////
// Opens if necessary.
void File::write (const std::vector <std::string>& lines)
{
  write(lines, true);
}

////////////////////////////////////////////////////////////////////////////////
// Opens if necessary.
void File::write (const std::vector <std::string>& lines, bool add_newlines)
{
  if (!_file.valid()) {
    open();
  }

  if (_file.valid()) {
    for (auto& line : lines) {
      DWORD line_size = boost::numeric_cast<DWORD>(line.size());

      WIN_TRY(WriteFile(_file.get(), line.c_str(), line_size, nullptr, nullptr));
      if (add_newlines) {
        WIN_TRY(WriteFile(_file.get(), "\n", 1, nullptr, nullptr));
      }
    }
  }
}

////////////////////////////////////////////////////////////////////////////////
// Opens if necessary.
void File::append (const std::string& line) {
  if (!_file.valid()) {
    open();
  }

  if (_file.valid()) {
    DWORD line_size = boost::numeric_cast<DWORD>(line.size());

    LARGE_INTEGER offset;
    offset.QuadPart = 0;

    WIN_TRY(SetFilePointerEx(_file.get(), offset, nullptr, FILE_END));

    WIN_TRY(WriteFile(_file.get(), line.c_str(), line_size, nullptr, nullptr));
    WIN_TRY(WriteFile(_file.get(), "\n", 1, nullptr, nullptr))
  }
}

////////////////////////////////////////////////////////////////////////////////
// Opens if necessary.
void File::append (const std::vector <std::string>& lines) {
  append(lines, true);
}

////////////////////////////////////////////////////////////////////////////////
// Opens if necessary.
void File::append (const std::vector <std::string>& lines, bool add_newlines) {
  if (!_file.valid()) {
    open();
  }

  if (_file.valid()) {
    for (auto& line : lines) {
      DWORD line_size = boost::numeric_cast<DWORD>(line.size());

      WIN_TRY(SetFilePointer(_file.get(), 0, nullptr, FILE_END));

      WIN_TRY(WriteFile(_file.get(), line.c_str(), line_size, nullptr, nullptr))

      if (add_newlines) {
        WIN_TRY(WriteFile(_file.get(), "\n", 1, nullptr, nullptr))
      }
    }
  }
}

////////////////////////////////////////////////////////////////////////////////
void File::truncate ()
{
  if (!_file.valid()) {
    open();
  }

  if (_file.valid()) {
    WIN_TRY(SetFilePointer(_file.get(), 0, nullptr, FILE_END));
    WIN_TRY(SetEndOfFile(_file.get()));
  }
}

////////////////////////////////////////////////////////////////////////////////
//  S_IFMT          0170000  type of file
//         S_IFIFO  0010000  named pipe (fifo)
//         S_IFCHR  0020000  character special
//         S_IFDIR  0040000  directory
//         S_IFBLK  0060000  block special
//         S_IFREG  0100000  regular
//         S_IFLNK  0120000  symbolic link
//         S_IFSOCK 0140000  socket
//         S_IFWHT  0160000  whiteout
//  S_ISUID         0004000  set user id on execution
//  S_ISGID         0002000  set group id on execution
//  S_ISVTX         0001000  save swapped text even after use
//  S_IRUSR         0000400  read permission, owner
//  S_IWUSR         0000200  write permission, owner
//  S_IXUSR         0000100  execute/search permission, owner
mode_t File::mode ()
{
  return 0;
}

////////////////////////////////////////////////////////////////////////////////
size_t File::size () const
{
  LARGE_INTEGER size;
  WIN_TRY(GetFileSizeEx(const_cast<File*>(this)->_file.get(), &size))

  return boost::numeric_cast<size_t>(size.QuadPart);
}

////////////////////////////////////////////////////////////////////////////////
time_t File::mtime () const
{
  BY_HANDLE_FILE_INFORMATION fileInfo;
  WIN_TRY(GetFileInformationByHandle(const_cast<File*>(this)->_file.get(), &fileInfo));

  return filetime_to_timet(fileInfo.ftLastWriteTime);
}

////////////////////////////////////////////////////////////////////////////////
time_t File::ctime () const
{
  BY_HANDLE_FILE_INFORMATION fileInfo;
  WIN_TRY(GetFileInformationByHandle(const_cast<File*>(this)->_file.get(), &fileInfo));

  return filetime_to_timet(fileInfo.ftLastAccessTime);
}

////////////////////////////////////////////////////////////////////////////////
time_t File::btime () const
{
  BY_HANDLE_FILE_INFORMATION fileInfo;
  WIN_TRY(GetFileInformationByHandle(const_cast<File*>(this)->_file.get(), &fileInfo));

  return filetime_to_timet(fileInfo.ftCreationTime);
}

////////////////////////////////////////////////////////////////////////////////
bool File::create (const std::string& name, int mode /* = 0640 */)
{
  return File(name).create(mode);
}

////////////////////////////////////////////////////////////////////////////////
std::string File::read (const std::string& name)
{
  std::string contents = "";
  File(name).read(contents);

  return contents;
}

////////////////////////////////////////////////////////////////////////////////
bool File::read (const std::string& name, std::string& contents)
{
  contents = "";
  File(name).read(contents);
  return true;
}

////////////////////////////////////////////////////////////////////////////////
bool File::read (const std::string& name, std::vector <std::string>& contents)
{
  contents.clear ();
  File(name).read(contents);
  return true;
}

////////////////////////////////////////////////////////////////////////////////
bool File::write (const std::string& name, const std::string& contents)
{
  File(name).write(contents);
  return true;
}

////////////////////////////////////////////////////////////////////////////////
bool File::write (
  const std::string& name,
  const std::vector <std::string>& lines,
  bool addNewlines /* = true */)
{
  File(name).write(lines, addNewlines);
  return true;
}

////////////////////////////////////////////////////////////////////////////////
bool File::append (const std::string& name, const std::string& contents)
{
  File(name).append(contents);
  return true;
}

////////////////////////////////////////////////////////////////////////////////
bool File::append (
  const std::string& name,
  const std::vector <std::string>& lines,
  bool addNewlines /* = true */)
{
  File(name).append(lines, addNewlines);
  return true;
}

////////////////////////////////////////////////////////////////////////////////
bool File::remove (const std::string& name)
{
  return boost::filesystem::remove(boost::filesystem::path(name));
}

////////////////////////////////////////////////////////////////////////////////

