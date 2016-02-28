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
#include <iostream>
#include <iomanip>
#include <sstream>
#include <Timer.h>
#include <Context.h>

extern Context context;

QpcFreq QpcFreq::_inst;

#ifdef WINDOWS
QpcFreq::QpcFreq()
{
  LARGE_INTEGER freq;
  QueryPerformanceFrequency(&freq);
  _freq = static_cast<double>(freq.QuadPart);
}

double QpcFreq::value()
{
  return _inst._freq;
}
#endif

////////////////////////////////////////////////////////////////////////////////
// Timer starts when the object is constructed.
Timer::Timer ()
: _description ("-")
, _running (false)
, _total (0)
{
}

////////////////////////////////////////////////////////////////////////////////
// Timer starts when the object is constructed with a description.
Timer::Timer (const std::string& description)
: _description (description)
, _running (false)
, _total (0)
{
  start ();
}

////////////////////////////////////////////////////////////////////////////////
// Timer stops when the object is destructed.
Timer::~Timer ()
{
  stop ();

  std::stringstream s;
  s << "Timer " // No i18n
    << _description
    << " "
    << std::setprecision (6)
#ifndef HAIKU
    // Haiku fails on this - don't know why.
    << std::fixed
#endif
#ifdef WINDOWS
    << _total / QpcFreq::value()
#else
    << _total / 1000000.0
#endif
    << " sec";

  context.debug (s.str ());
}

////////////////////////////////////////////////////////////////////////////////
void Timer::start ()
{
  if (!_running)
  {
#ifdef WINDOWS
    QueryPerformanceCounter(&_start);
#else
    gettimeofday (&_start, NULL);
#endif
    _running = true;
  }
}

////////////////////////////////////////////////////////////////////////////////
void Timer::stop ()
{
  if (_running)
  {
#ifdef WINDOWS
    LARGE_INTEGER end;
    QueryPerformanceCounter(&end);
    _total += end.QuadPart - _start.QuadPart;
#else
    struct timeval end;
    gettimeofday (&end, NULL);
    _running = false;
    _total += (end.tv_sec - _start.tv_sec) * 1000000
            + (end.tv_usec - _start.tv_usec);
#endif
  }
}

////////////////////////////////////////////////////////////////////////////////
unsigned long Timer::total () const
{
  return _total;
}

////////////////////////////////////////////////////////////////////////////////
void Timer::subtract (unsigned long value)
{
  if (value > _total)
    _total = 0;
  else
    _total -= value;
}

////////////////////////////////////////////////////////////////////////////////
unsigned long Timer::now ()
{
  struct timeval now;
  gettimeofday (&now, NULL);
  return now.tv_sec * 1000000 + now.tv_usec;
}

////////////////////////////////////////////////////////////////////////////////
