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
#include <Directory.h>
#include <sstream>
#include <Path.h>
#include <util.h>
#include <Shlobj.h>
#include <nowide/convert.hpp>

// Fixes build with musl libc.
#ifndef GLOB_TILDE
#define GLOB_TILDE 0
#endif

#ifndef GLOB_BRACE
#define GLOB_BRACE 0
#endif

////////////////////////////////////////////////////////////////////////////////
std::ostream& operator<< (std::ostream& out, const Path& path)
{
  out << path._path;
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
    _path = other._path;
  }
}

////////////////////////////////////////////////////////////////////////////////
Path::Path (const std::string& in)
{
  _path = boost::filesystem::path(expand(in));
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
    this->_path = other._path;
  }

  return *this;
}

////////////////////////////////////////////////////////////////////////////////
bool Path::operator== (const Path& other)
{
  return _path == other._path;
}

////////////////////////////////////////////////////////////////////////////////
Path& Path::operator+= (const std::string& dir)
{
  _path = _path / dir;
  return *this;
}

////////////////////////////////////////////////////////////////////////////////
Path::operator std::string () const
{
  return _path.string();
}

////////////////////////////////////////////////////////////////////////////////
std::string Path::name () const
{
  return _path.filename().string();
}

////////////////////////////////////////////////////////////////////////////////
std::string Path::parent () const
{
  return _path.parent_path().string();
}

////////////////////////////////////////////////////////////////////////////////
std::string Path::extension () const
{
  return _path.string();
}

////////////////////////////////////////////////////////////////////////////////
bool Path::exists () const
{
  return boost::filesystem::exists(_path);
}

////////////////////////////////////////////////////////////////////////////////
bool Path::is_directory () const
{
  return boost::filesystem::is_directory(_path);
}

////////////////////////////////////////////////////////////////////////////////
bool Path::is_absolute () const
{
  return _path.is_absolute();
}

////////////////////////////////////////////////////////////////////////////////
bool Path::is_link () const
{
  return boost::filesystem::is_symlink(_path);
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
  boost::filesystem::path new_path(new_name);
  boost::filesystem::rename(_path, new_path);
  _path = new_path;
  return true;
}

////////////////////////////////////////////////////////////////////////////////
// Only expand tilde on Windows
std::string Path::expand (const std::string& in)
{
  if (in.size() >= 2 && in[0] == '~' && (in[1] == '\\' || in[1] == '/')) {
    wchar_t home[MAX_PATH];
    if (SHGetFolderPathW(nullptr, CSIDL_PROFILE, nullptr, SHGFP_TYPE_CURRENT, home) != S_OK) {
      throw "Could not get home path.";
    }

    std::string home_str = nowide::narrow(home);
    return home_str + in.substr(1, std::string::npos);
  }

  return in;
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
    if (findData.cFileName[0] == L'.' && (findData.cFileName[1] == L'\0' ||
      (findData.cFileName[1] == L'.' && findData.cFileName[2] == L'\0')))
    {
      continue;
    }

    results.push_back(nowide::narrow(findData.cFileName));
  } while (FindNextFileW(handle.get(), &findData) == TRUE);

  return results;
}

////////////////////////////////////////////////////////////////////////////////
std::string Path::to_string() const {
  return _path.string();
}

std::string Path::original() const {
  return _path.string();
}