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
#include <stdlib.h>
#include <string.h>
#include <RX.h>
#include <boost/numeric/conversion/cast.hpp>

////////////////////////////////////////////////////////////////////////////////
RX::RX ()
: _compiled (false)
, _pattern ("")
, _case_sensitive (true)
{
}

////////////////////////////////////////////////////////////////////////////////
RX::RX (
  const std::string& pattern,
  bool case_sensitive /* = true */)
: _compiled (false)
, _pattern (pattern)
, _case_sensitive (case_sensitive)
{
  compile ();
}

////////////////////////////////////////////////////////////////////////////////
RX::RX (const RX& other)
: _compiled (false)
, _pattern (other._pattern)
, _case_sensitive (other._case_sensitive)
{
}

////////////////////////////////////////////////////////////////////////////////
RX& RX::operator= (const RX& other)
{
  if (this != &other)
  {
    _compiled       = false;
    _pattern        = other._pattern;
    _case_sensitive = other._case_sensitive;
  }

  return *this;
}

////////////////////////////////////////////////////////////////////////////////
bool RX::operator== (const RX& other) const
{
  return _pattern        == other._pattern &&
         _case_sensitive == other._case_sensitive;
}

////////////////////////////////////////////////////////////////////////////////
RX::~RX ()
{
}

////////////////////////////////////////////////////////////////////////////////
void RX::compile ()
{
  if (!_compiled)
  {
    std::regex_constants::syntax_option_type opts = std::regex_constants::egrep;
    if (!_case_sensitive) {
      opts |= std::regex_constants::icase;
    }

    _regex = std::regex(_pattern, opts);
    _compiled = true;
  }
}

////////////////////////////////////////////////////////////////////////////////
bool RX::match (const std::string& in)
{
  if (!_compiled)
    compile ();

  return std::regex_match(in, _regex);
}

////////////////////////////////////////////////////////////////////////////////
bool RX::match (
  std::vector<std::string>& matches,
  const std::string& in)
{
  if (!_compiled)
    compile ();

  std::string::const_iterator it = in.begin();
  std::smatch raw_matches;
  while (std::regex_search(it, in.end(), raw_matches, _regex)) {
    matches.push_back(raw_matches[0].str());
  }

  return !matches.empty();
}

////////////////////////////////////////////////////////////////////////////////
bool RX::match (
  std::vector <int>& start,
  std::vector <int>& end,
  const std::string& in)
{
  if (!_compiled)
    compile ();

  std::string::const_iterator it = in.begin();
  std::smatch matches;
  while (std::regex_search(it, in.end(), matches, _regex)) {
    start.push_back(boost::numeric_cast<int>(std::distance(matches[0].first, in.begin())));
    end.push_back(boost::numeric_cast<int>(std::distance(matches[0].second, in.begin()) - 1));
    it = matches[0].second;
  }

  return !start.empty();
}

////////////////////////////////////////////////////////////////////////////////
