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
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include <text.h>
#include <util.h>
#include <i18n.h>
#include <FS.h>
#include <nowide/convert.hpp>
#include <Shlwapi.h>
#include <ShlObj.h>
#include <src/numeric_cast.hpp>

using namespace FSHelper;

////////////////////////////////////////////////////////////////////////////////
std::ostream& operator<< (std::ostream& out, const Path& path)
{
  out << path._data;
  return out;
}

////////////////////////////////////////////////////////////////////////////////
Path::Path ()
{
}

////////////////////////////////////////////////////////////////////////////////
Path::Path (const Path& other)
{
  if (this != &other)
  {
    _original = other._original;
    _data     = other._data;
  }
}

////////////////////////////////////////////////////////////////////////////////
Path::Path (const std::string& in)
{
  _original = in;
  _data = expand (in);
}

////////////////////////////////////////////////////////////////////////////////
Path::~Path ()
{
}

////////////////////////////////////////////////////////////////////////////////
Path& Path::operator= (const Path& other)
{
  if (this != &other)
  {
    this->_original = other._original;
    this->_data     = other._data;
  }

  return *this;
}

////////////////////////////////////////////////////////////////////////////////
bool Path::operator== (const Path& other)
{
  return _data == other._data;
}

////////////////////////////////////////////////////////////////////////////////
Path& Path::operator+= (const std::string& dir)
{
  wchar_t result[MAX_PATH];
  if (PathCombineW(result, nowide::widen(_data).c_str(), nowide::widen(dir).c_str()) == NULL) {
    THROW_WIN_ERROR_FMT("Failed to combine \"{1}\" with \"{2}\".", _data, dir);
  }

  _data = nowide::narrow(result);
  return *this;
}

////////////////////////////////////////////////////////////////////////////////
Path::operator std::string () const
{
  return _data;
}

////////////////////////////////////////////////////////////////////////////////
std::string Path::name () const
{
  auto wdata = nowide::widen(_data);
  const wchar_t * name = PathFindFileNameW(wdata.c_str());
  if (name == wdata.c_str()) {
    return "";
  }

  return nowide::narrow(name);
}

////////////////////////////////////////////////////////////////////////////////
std::string Path::parent () const
{
  auto parent = nowide::widen(_data);
  wchar_t *parent_buf = const_cast<wchar_t*>(parent.c_str());
  if (!PathRemoveFileSpecW(parent_buf)) {
    return "";
  }

  return nowide::narrow(parent_buf);
}

////////////////////////////////////////////////////////////////////////////////
std::string Path::extension () const
{
  const wchar_t *extension = PathFindExtensionW(nowide::widen(_data).c_str());
  if (*extension == 0) {
    return "";
  }

  extension += sizeof(wchar_t);
  return nowide::narrow(extension);
}

////////////////////////////////////////////////////////////////////////////////
bool Path::exists () const
{
  return PathFileExistsW(nowide::widen(_data).c_str()) == TRUE;
}

////////////////////////////////////////////////////////////////////////////////
bool Path::is_directory () const
{
  return PathIsDirectoryW(nowide::widen(_data).c_str()) == TRUE;
}

////////////////////////////////////////////////////////////////////////////////
bool Path::is_absolute () const
{
  return PathIsRelativeW(nowide::widen(_data).c_str()) == FALSE;
}

////////////////////////////////////////////////////////////////////////////////
bool Path::is_link () const
{
  DWORD attribs = GetFileAttributesW(nowide::widen(_data).c_str());
  return (attribs & FILE_ATTRIBUTE_REPARSE_POINT) == FILE_ATTRIBUTE_REPARSE_POINT;
}

////////////////////////////////////////////////////////////////////////////////
bool Path::readable () const
{
  return true;
}

////////////////////////////////////////////////////////////////////////////////
bool Path::writable () const
{
  return true;
}

////////////////////////////////////////////////////////////////////////////////
bool Path::executable () const
{
  return true;
}

////////////////////////////////////////////////////////////////////////////////
bool Path::rename (const std::string& new_name)
{
  std::wstring expanded = nowide::widen(expand(new_name));
  std::wstring wdata = nowide::widen(_data);

  if (wdata != expanded)
  {
    if (!MoveFileW(wdata.c_str (), expanded.c_str ()))
    {
      return false;
    }
  }

  wdata = expanded;
  return true;
}

////////////////////////////////////////////////////////////////////////////////
// Only expand tilde on Windows
std::string Path::expand (const std::string& in)
{
  std::wstring result_str;

  if (in.size() >= 2 && in[0] == '~' && (in[1] == '\\' || in[1] == '/')) {
    wchar_t home[MAX_PATH];
    if (SHGetFolderPathW(nullptr, CSIDL_PROFILE, nullptr, SHGFP_TYPE_CURRENT, home) != S_OK) {
      throw "Failed to get home path.";
    }

    result_str = home;
    result_str.append(nowide::widen(in.substr(1)));
  }
  else {
    result_str = nowide::widen(in);
  }

  for (auto& c : result_str) {
    if (c == L'/') {
      c = '\\';
    }
  }

  wchar_t result[MAX_PATH];
  PathCanonicalizeW(result, result_str.c_str());

  return nowide::narrow(result);
}

////////////////////////////////////////////////////////////////////////////////
std::vector <std::string> Path::glob (const std::string& pattern)
{
  std::vector <std::string> results;

  WIN32_FIND_DATAW findData;
  FindHandle handle(FindFirstFileW(nowide::widen(pattern).c_str(), &findData));
  if (!handle.valid())
  {
    return results;
  }

  do
  {
    if (Path::isDots(findData.cFileName))
    {
      continue;
    }

    results.push_back(nowide::narrow(findData.cFileName));
  } while (FindNextFileW(handle.get(), &findData) == TRUE);

  return results;
}

////////////////////////////////////////////////////////////////////////////////
bool Path::isDots(const wchar_t *name) {
  return name[0] == L'.' && (name[1] == L'\0' || (name[1] == L'.' && name[2] == L'\0'));
}

////////////////////////////////////////////////////////////////////////////////
File::File ()
: Path ()
, _locked (false)
{
}

////////////////////////////////////////////////////////////////////////////////
File::File (const Path& other)
: Path (other)
, _locked (false)
{
}

////////////////////////////////////////////////////////////////////////////////
File::File (const File& other)
: Path (other)
, _locked (false)
{
}

////////////////////////////////////////////////////////////////////////////////
File::File (const std::string& in)
: Path (in)
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
    // Ignore mode on windows
    close ();
    return true;
  }

  return false;
}

////////////////////////////////////////////////////////////////////////////////
bool File::remove () const
{
  return DeleteFileW (nowide::widen(_data).c_str ()) == TRUE;
}

////////////////////////////////////////////////////////////////////////////////
bool File::open ()
{
  return open (FileOpenFlags::Create);
}

////////////////////////////////////////////////////////////////////////////////
bool File::open (int flags)
{
  if (_data.empty() || _file.valid()) {
    return false;
  }

  DWORD creationDisposition;
  if ((flags & FileOpenFlags::Create) == FileOpenFlags::Create) {
    creationDisposition = OPEN_ALWAYS;
  }
  else {
    creationDisposition = OPEN_EXISTING;
  }

  bool lock = (flags & FileOpenFlags::Lock) == FileOpenFlags::Lock;

  _file.reset(CreateFileW(
    nowide::widen(_data).c_str(),
    GENERIC_READ | GENERIC_WRITE,
    FILE_SHARE_READ | (lock ? 0u : FILE_SHARE_WRITE),
    nullptr,
    creationDisposition,
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
  return open (FileOpenFlags::Lock | FileOpenFlags::Create);
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
  return open(FileOpenFlags::Lock);
}

////////////////////////////////////////////////////////////////////////////////
void File::unlock ()
{
  if (!_locked) {
    return;
  }

  close();
  open(FileOpenFlags::None);
}

////////////////////////////////////////////////////////////////////////////////
// Opens if necessary.
void File::read (std::string& contents)
{
  contents = "";
  if (!_file.valid()) {
    open(FileOpenFlags::None);
  }

  contents.reserve(size());

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
  contents.clear();

  if (!_file.valid()) {
    open(FileOpenFlags::None);
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
void File::write (const std::string& line)
{
  if (!_file.valid()) {
    open(FileOpenFlags::Create);
  }

  if (_file.valid()) {
    DWORD line_size = numeric_cast<DWORD>(line.size());

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
void File::write(const std::vector <std::string>& lines, bool add_newlines)
{
  if (!_file.valid()) {
    open(FileOpenFlags::Create);
  }

  if (_file.valid()) {
    for (auto& line : lines) {
      DWORD line_size = numeric_cast<DWORD>(line.size());

      WIN_TRY(WriteFile(_file.get(), line.c_str(), line_size, nullptr, nullptr));
      if (add_newlines) {
        WIN_TRY(WriteFile(_file.get(), "\n", 1, nullptr, nullptr));
      }
    }
  }
}

////////////////////////////////////////////////////////////////////////////////
// Opens if necessary.
void File::append (const std::string& line)
{
  if (!_file.valid()) {
    open(FileOpenFlags::Create);
  }

  if (_file.valid()) {
    DWORD line_size = numeric_cast<DWORD>(line.size());

    LARGE_INTEGER offset;
    offset.QuadPart = 0;

    WIN_TRY(SetFilePointerEx(_file.get(), offset, nullptr, FILE_END));

    WIN_TRY(WriteFile(_file.get(), line.c_str(), line_size, nullptr, nullptr));
    WIN_TRY(WriteFile(_file.get(), "\n", 1, nullptr, nullptr))
  }
}

////////////////////////////////////////////////////////////////////////////////
// Opens if necessary.
void File::append (const std::vector <std::string>& lines)
{
  append(lines, true);
}

////////////////////////////////////////////////////////////////////////////////
// Opens if necessary.
void File::append(const std::vector <std::string>& lines, bool add_newlines) {
  if (!_file.valid()) {
    open(FileOpenFlags::Create);
  }

  if (_file.valid()) {
    for (auto& line : lines) {
      DWORD line_size = numeric_cast<DWORD>(line.size());

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
    open(FileOpenFlags::None);
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
SafeHandle File::open_for_metadata(const char *path) {
  SafeHandle h(
    CreateFileW(
      nowide::widen(path).c_str(),
      0,
      FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
      nullptr,
      OPEN_EXISTING,
      0,
      nullptr));

  if (!h.valid()) {
    THROW_WIN_ERROR_FMT("Error opening \"{1}\" for reading metadata.", path);
  }

  return h;
}

////////////////////////////////////////////////////////////////////////////////
size_t File::size () const
{
  SafeHandle handle = File::open_for_metadata(_data.c_str());
  BY_HANDLE_FILE_INFORMATION fileInfo;
  WIN_TRY(GetFileInformationByHandle(handle.get(), &fileInfo));

  return numeric_cast<size_t>((fileInfo.nFileSizeHigh << sizeof(DWORD)) + fileInfo.nFileSizeLow);
}

////////////////////////////////////////////////////////////////////////////////
time_t File::mtime () const
{
  SafeHandle handle = File::open_for_metadata(_data.c_str());
  BY_HANDLE_FILE_INFORMATION fileInfo;
  WIN_TRY(GetFileInformationByHandle(handle.get(), &fileInfo));

  return filetime_to_timet(fileInfo.ftLastWriteTime);
}

////////////////////////////////////////////////////////////////////////////////
time_t File::ctime () const
{
  SafeHandle handle = File::open_for_metadata(_data.c_str());
  BY_HANDLE_FILE_INFORMATION fileInfo;
  WIN_TRY(GetFileInformationByHandle(handle.get(), &fileInfo));

  return filetime_to_timet(fileInfo.ftLastAccessTime);
}

////////////////////////////////////////////////////////////////////////////////
time_t File::btime () const
{
  SafeHandle handle = File::open_for_metadata(_data.c_str());
  BY_HANDLE_FILE_INFORMATION fileInfo;
  WIN_TRY(GetFileInformationByHandle(handle.get(), &fileInfo));

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
  try {
    File(name).read(contents);
  }
  catch (WindowsError&) {
    return false;
  }
  return true;
}

////////////////////////////////////////////////////////////////////////////////
bool File::read (const std::string& name, std::vector <std::string>& contents)
{
  contents.clear();
  try {
    File(name).read(contents);
  }
  catch (WindowsError&) {
    return false;
  }
  return true;
}

////////////////////////////////////////////////////////////////////////////////
bool File::write (const std::string& name, const std::string& contents)
{
  try {
    File(name).write(contents);
  }
  catch (WindowsError&) {
    return false;
  }
  return true;
}

////////////////////////////////////////////////////////////////////////////////
bool File::write (
  const std::string& name,
  const std::vector <std::string>& lines,
  bool addNewlines /* = true */)
{
  try {
    File(name).write(lines, addNewlines);
  }
  catch (WindowsError&) {
    return false;
  }
  return true;
}

////////////////////////////////////////////////////////////////////////////////
bool File::append (const std::string& name, const std::string& contents)
{
  try {
    File(name).append(contents);
  }
  catch (WindowsError&) {
    return false;
  }
  return true;
}

////////////////////////////////////////////////////////////////////////////////
bool File::append (
  const std::string& name,
  const std::vector <std::string>& lines,
  bool addNewlines /* = true */)
{
  try {
    File(name).write(lines, addNewlines);
  }
  catch (WindowsError&) {
    return false;
  }
  return true;
}

////////////////////////////////////////////////////////////////////////////////
bool File::remove (const std::string& name)
{
  return DeleteFileW(nowide::widen(name).c_str()) == TRUE;
}

////////////////////////////////////////////////////////////////////////////////
Directory::Directory ()
{
}

////////////////////////////////////////////////////////////////////////////////
Directory::Directory (const Directory& other)
: File (other)
{
}

////////////////////////////////////////////////////////////////////////////////
Directory::Directory (const File& other)
: File (other)
{
}

////////////////////////////////////////////////////////////////////////////////
Directory::Directory (const Path& other)
: File (other)
{
}

////////////////////////////////////////////////////////////////////////////////
Directory::Directory (const std::string& in)
: File (in)
{
}

////////////////////////////////////////////////////////////////////////////////
Directory::~Directory ()
{
}

////////////////////////////////////////////////////////////////////////////////
Directory& Directory::operator= (const Directory& other)
{
  if (this != &other)
  {
    File::operator= (other);
  }

  return *this;
}

////////////////////////////////////////////////////////////////////////////////
bool Directory::create (int mode /* = 0755 */)
{
  std::vector<Directory> parents;
  Directory parent_dir(parent());
  while (parent_dir._data != "" && !parent_dir.exists()) {
    parents.push_back(parent_dir);
    parent_dir = Directory(parent_dir.parent());
  }

  std::vector<Directory>::const_reverse_iterator it = parents.rbegin();
  std::vector<Directory>::const_reverse_iterator end = parents.rend();

  for (; it != end; ++it) {
    if (!CreateDirectoryW(nowide::widen(it->_data).c_str(), nullptr)) {
      return false;
    }
  }

  return true;
}

////////////////////////////////////////////////////////////////////////////////
bool Directory::remove () const
{
  return Directory::remove_directory(nowide::widen(_data).c_str());
}

////////////////////////////////////////////////////////////////////////////////
bool Directory::remove_directory (const wchar_t *dir) const
{
  WIN32_FIND_DATAW findData;
  FindHandle findHandle(FindFirstFileW(dir, &findData));
  if (!findHandle.valid()) {
    return false;
  }

  do
  {
    if (Path::isDots(findData.cFileName)) {
      continue;
    }

    if ((findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == FILE_ATTRIBUTE_DIRECTORY) {
      wchar_t next_dir[MAX_PATH];
      if (PathCombineW(next_dir, dir, findData.cFileName) == nullptr) {
        return false;
      }

      if (!Directory::remove_directory(next_dir)) {
        return false;
      }
    }
    else {
      if (!DeleteFileW(findData.cFileName)) {
        return false;
      }
    }
  } while (FindNextFileW(findHandle.get(), &findData));

  return true;
}

////////////////////////////////////////////////////////////////////////////////
std::vector <std::string> Directory::list ()
{
  std::vector <std::string> files;
  Directory::listRecursive(files, nowide::widen(_data).c_str(), L"");
  return files;
}

////////////////////////////////////////////////////////////////////////////////
void Directory::listRecursive(std::vector<std::string>& result, const wchar_t *full_dir, const wchar_t *dir)
{
  std::vector <std::string> files;
  WIN32_FIND_DATAW findData;
  FindHandle findHandle(FindFirstFileW(full_dir, &findData));
  if (!findHandle.valid()) {
    return;
  }

  do
  {
    if (Path::isDots(findData.cFileName)) {
      continue;
    }

    if ((findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == FILE_ATTRIBUTE_DIRECTORY) {
      wchar_t next_full_dir[MAX_PATH];
      wchar_t next_dir[MAX_PATH];
      if (PathCombineW(next_full_dir, full_dir, findData.cFileName) == nullptr ||
        PathCombineW(next_dir, dir, findData.cFileName) == nullptr)
      {
        continue;
      }

      result.push_back(nowide::narrow(next_dir));
      Directory::listRecursive(result, next_full_dir, next_dir);
}
    else {
      wchar_t filename[MAX_PATH];
      if (PathCombineW(filename, dir, findData.cFileName) == nullptr) {
        continue;
      }

      result.push_back(nowide::narrow(filename));
    }
  } while (FindNextFileW(findHandle.get(), &findData));
}

////////////////////////////////////////////////////////////////////////////////
std::string Directory::cwd()
{
  wchar_t result[MAX_PATH];
  if (GetCurrentDirectoryW(MAX_PATH, result) == 0) {
    THROW_WIN_ERROR("Failed to get current directory.");
  }

  return nowide::narrow(result);
}

////////////////////////////////////////////////////////////////////////////////
bool Directory::up()
{
  auto parent_str = nowide::widen(_data);
  wchar_t *parent_buf = const_cast<wchar_t*>(parent_str.c_str());
  if (!PathRemoveFileSpecW(parent_buf)) {
    return false;
  }

  _data = nowide::narrow(parent_buf);
  return true;
}

////////////////////////////////////////////////////////////////////////////////
bool Directory::cd() const
{
  return SetCurrentDirectoryW(nowide::widen(_data).c_str()) == TRUE;
}

////////////////////////////////////////////////////////////////////////////////
