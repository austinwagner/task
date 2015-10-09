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
#include <unistd.h>
#include <File.h>
#include <text.h>
#include <util.h>
#include <i18n.h>
#include <limits>

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
  if (_path.empty() || !_file.valid()) {
    return false;
  }

  _file.reset(CreateFileW(
    _path.wstring().c_str(),
    GENERIC_READ | GENERIC_WRITE,
    FILE_SHARE_READ | FILE_SHARE_WRITE,
    nullptr,
    OPEN_ALWAYS,
    FILE_ATTRIBUTE_NORMAL,
    nullptr
  ));

  return _file.valid();
}

////////////////////////////////////////////////////////////////////////////////
bool File::openAndLock ()
{
  return open () && lock ();
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
  _locked = false;
  if (_file.valid())
  {
    _file.reset(ReOpenFile(_file.get(), GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ, 0));
    _locked = _file.valid();
  }

  return _locked;
}

////////////////////////////////////////////////////////////////////////////////
void File::unlock ()
{
  if (_locked)
  {
    _file.reset(ReOpenFile(_file.get(), GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, 0));
  }
}

////////////////////////////////////////////////////////////////////////////////
// Opens if necessary.
void File::read (std::string& contents) {
  contents = "";
  contents.reserve (size ());

  if (!_file.valid()) {
    open();
  }

  char buffer[512];
  DWORD read = 1;

  while (read > 0) {
    if (ReadFile(_file.get(), buffer, sizeof(buffer), &read, nullptr) != TRUE) {
      throw getErrorString(GetLastError());
    }

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
    if (ReadFile(_file.get(), buffer, sizeof(buffer), &read, nullptr) != TRUE) {
      throw getErrorString(GetLastError());
    }

    for (DWORD i = 0; i < read; i++) {
      char c = buffer[i];

      // Naive line ending conversion
      if (c == '\r') {
        continue;
      }
      else if (c == '\n') {
        contents.push_back(line);
        line = std::string();
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
    if (line.size() > std::numeric_limits<DWORD>::max()) {
      throw "Line too long.";
    }

    if (WriteFile(_file.get(), line.c_str(), (DWORD)line.size(), nullptr, nullptr) != TRUE) {
      throw getErrorString(GetLastError());
    }

    if (WriteFile(_file.get(), "\r\n", 2, nullptr, nullptr) != TRUE) {
      throw getErrorString(GetLastError());
    }
  }
}

////////////////////////////////////////////////////////////////////////////////
// Opens if necessary.
void File::write (const std::vector <std::string>& lines)
{
  if (!_file.valid()) {
    open();
  }

  if (_file.valid()) {
    for (auto& line : lines) {
      if (line.size() > std::numeric_limits<DWORD>::max()) {
        throw "Line too long.";
      }

      if (WriteFile(_file.get(), line.c_str(), (DWORD) line.size(), nullptr, nullptr) != TRUE) {
        throw getErrorString(GetLastError());
      }

      if (WriteFile(_file.get(), "\r\n", 2, nullptr, nullptr) != TRUE) {
        throw getErrorString(GetLastError());
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
    if (line.size() > std::numeric_limits<DWORD>::max()) {
      throw "Line too long.";
    }

    if (SetFilePointer(_file.get(), 0, nullptr, FILE_END) != TRUE) {
      throw getErrorString(GetLastError());
    }

    if (WriteFile(_file.get(), line.c_str(), (DWORD) line.size(), nullptr, nullptr) != TRUE) {
      throw getErrorString(GetLastError());
    }

    if (WriteFile(_file.get(), "\r\n", 2, nullptr, nullptr) != TRUE) {
      throw getErrorString(GetLastError());
    }
  }
}

////////////////////////////////////////////////////////////////////////////////
// Opens if necessary.
void File::append (const std::vector <std::string>& lines) {
  if (!_file.valid()) {
    open();
  }

  if (_file.valid()) {
    for (auto& line : lines) {
      if (line.size() > std::numeric_limits<DWORD>::max()) {
        throw "Line too long.";
      }

      if (SetFilePointer(_file.get(), 0, nullptr, FILE_END) != TRUE) {
        throw getErrorString(GetLastError());
      }

      if (WriteFile(_file.get(), line.c_str(), (DWORD) line.size(), nullptr, nullptr) != TRUE) {
        throw getErrorString(GetLastError());
      }

      if (WriteFile(_file.get(), "\r\n", 2, nullptr, nullptr) != TRUE) {
        throw getErrorString(GetLastError());
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
    if (SetFilePointer(_file.get(), 0, nullptr, FILE_END) != TRUE) {
      throw getErrorString(GetLastError());
    }
    if (SetEndOfFile(_file.get()) != TRUE) {
      throw getErrorString(GetLastError());
    }
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
  if (GetFileSizeEx(const_cast<File*>(this)->_file.get(), &size) != TRUE) {
    throw getErrorString(GetLastError());
  }

  if (size.QuadPart < 0) {
    throw "Invalid file size.";

  }
  if ((unsigned)size.QuadPart > std::numeric_limits<size_t>::max()) {
    throw "File too large.";
  }

  return (size_t)size.QuadPart;
}

////////////////////////////////////////////////////////////////////////////////
time_t File::mtime () const
{
  BY_HANDLE_FILE_INFORMATION fileInfo;
  if (GetFileInformationByHandle(const_cast<File*>(this)->_file.get(), &fileInfo) != TRUE) {
    throw getErrorString(GetLastError());
  }

  return filetime_to_timet(fileInfo.ftLastWriteTime);
}

////////////////////////////////////////////////////////////////////////////////
time_t File::ctime () const
{
  BY_HANDLE_FILE_INFORMATION fileInfo;
  if (GetFileInformationByHandle(const_cast<File*>(this)->_file.get(), &fileInfo) != TRUE) {
    throw getErrorString(GetLastError());
  }

  return filetime_to_timet(fileInfo.ftLastAccessTime);
}

////////////////////////////////////////////////////////////////////////////////
time_t File::btime () const
{
  BY_HANDLE_FILE_INFORMATION fileInfo;
  if (GetFileInformationByHandle(const_cast<File*>(this)->_file.get(), &fileInfo) != TRUE) {
    throw getErrorString(GetLastError());
  }

  return filetime_to_timet(fileInfo.ftCreationTime);
}

////////////////////////////////////////////////////////////////////////////////
bool File::create (const std::string& name, int mode /* = 0640 */)
{
  std::string full_name = expand (name);
  std::ofstream out (full_name.c_str ());
  if (out.good ())
  {
    out.close ();
    return true;
  }

  return false;
}

////////////////////////////////////////////////////////////////////////////////
std::string File::read (const std::string& name)
{
  std::string contents = "";

  std::ifstream in (name.c_str ());
  if (in.good ())
  {
    std::string line;
    line.reserve (1024);
    while (getline (in, line))
      contents += line + "\n";

    in.close ();
  }

  return contents;
}

////////////////////////////////////////////////////////////////////////////////
bool File::read (const std::string& name, std::string& contents)
{
  contents = "";

  std::ifstream in (name.c_str ());
  if (in.good ())
  {
    std::string line;
    line.reserve (1024);
    while (getline (in, line))
      contents += line + "\n";

    in.close ();
    return true;
  }

  return false;
}

////////////////////////////////////////////////////////////////////////////////
bool File::read (const std::string& name, std::vector <std::string>& contents)
{
  contents.clear ();

  std::ifstream in (name.c_str ());
  if (in.good ())
  {
    std::string line;
    line.reserve (1024);
    while (getline (in, line))
      contents.push_back (line);

    in.close ();
    return true;
  }

  return false;
}

////////////////////////////////////////////////////////////////////////////////
bool File::write (const std::string& name, const std::string& contents)
{
  std::ofstream out (name.c_str (),
                     std::ios_base::out | std::ios_base::trunc);
  if (out.good ())
  {
    out << contents;
    out.close ();
    return true;
  }

  return false;
}

////////////////////////////////////////////////////////////////////////////////
bool File::write (
  const std::string& name,
  const std::vector <std::string>& lines,
  bool addNewlines /* = true */)
{
  std::ofstream out (expand (name).c_str (),
                     std::ios_base::out | std::ios_base::trunc);
  if (out.good ())
  {
    std::vector <std::string>::const_iterator it;
    for (it = lines.begin (); it != lines.end (); ++it)
    {
      out << *it;

      if (addNewlines)
        out << "\n";
    }

    out.close ();
    return true;
  }

  return false;
}

////////////////////////////////////////////////////////////////////////////////
bool File::append (const std::string& name, const std::string& contents)
{
  std::ofstream out (expand (name).c_str (),
                     std::ios_base::out | std::ios_base::app);
  if (out.good ())
  {
    out << contents;
    out.close ();
    return true;
  }

  return false;
}

////////////////////////////////////////////////////////////////////////////////
bool File::append (
  const std::string& name,
  const std::vector <std::string>& lines,
  bool addNewlines /* = true */)
{
  std::ofstream out (expand (name).c_str (),
                     std::ios_base::out | std::ios_base::app);
  if (out.good ())
  {
    std::vector <std::string>::const_iterator it;
    for (it = lines.begin (); it != lines.end (); ++it)
    {
      out << *it;

      if (addNewlines)
        out << "\n";
    }

    out.close ();
    return true;
  }

  return false;
}

////////////////////////////////////////////////////////////////////////////////
bool File::remove (const std::string& name)
{
  return boost::filesystem::remove(boost::filesystem::path(name));
}

////////////////////////////////////////////////////////////////////////////////

