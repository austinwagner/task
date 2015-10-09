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
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <Directory.h>
#include <util.h>

////////////////////////////////////////////////////////////////////////////////
Directory::Directory ()
{
}

////////////////////////////////////////////////////////////////////////////////
Directory::Directory (const Directory& other)
: File::File (other)
{
}

////////////////////////////////////////////////////////////////////////////////
Directory::Directory (const File& other)
: File::File (other)
{
}

////////////////////////////////////////////////////////////////////////////////
Directory::Directory (const Path& other)
: File::File (other)
{
}

////////////////////////////////////////////////////////////////////////////////
Directory::Directory (const std::string& in)
: File::File (in)
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
  return boost::filesystem::create_directories(_path);
}

////////////////////////////////////////////////////////////////////////////////
bool Directory::remove () const
{
  boost::filesystem::remove_all(_path);
  return true;
}

////////////////////////////////////////////////////////////////////////////////
std::vector <std::string> Directory::list ()
{
  std::vector <std::string> files;
  boost::filesystem::directory_iterator end;
  for (boost::filesystem::directory_iterator it(_path); it != end; ++it)
  {
    files.push_back(it->path().string());
  }

  return files;
}

////////////////////////////////////////////////////////////////////////////////
std::vector <std::string> Directory::listRecursive ()
{
  std::vector <std::string> files;
  boost::filesystem::recursive_directory_iterator end;
  for (boost::filesystem::recursive_directory_iterator it(_path); it != end; ++it)
  {
    files.push_back(it->path().string());
  }

  return files;
}

////////////////////////////////////////////////////////////////////////////////
std::string Directory::cwd ()
{
  return boost::filesystem::current_path().string();
}

////////////////////////////////////////////////////////////////////////////////
bool Directory::up ()
{
  if (!_path.has_parent_path())
  {
    return false;
  }

  _path = _path.parent_path();

  return true;
}

////////////////////////////////////////////////////////////////////////////////
bool Directory::cd () const
{
  boost::filesystem::current_path(_path);
  return true;
}
