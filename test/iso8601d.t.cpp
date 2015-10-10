////////////////////////////////////////////////////////////////////////////////
//
// Copyright 2013 - 2015, Göteborg Bit Factory.
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
#include <time.h>
#include <test.h>
#include <ISO8601.h>
#include <Context.h>

Context context;

#define AMBIGUOUS          // Include ambiguous forms
#undef  AMBIGUOUS          // Exclude ambiguous forms

////////////////////////////////////////////////////////////////////////////////
void testParse (
  UnitTest& t,
  const std::string& input,
  int in_start,
  int in_year,
  int in_month,
  int in_week,
  int in_weekday,
  int in_julian,
  int in_day,
  int in_seconds,
  int in_offset,
  bool in_utc,
  time_t in_value)
{
  std::string label = std::string ("parse (\"") + input + "\") --> ";

  ISO8601d iso;
  std::string::size_type start = 0;

  t.ok (iso.parse (input, start),               label + "true");
  t.is ((int) start,         in_start,          label + "[]");
  t.is (iso._year,           in_year,           label + "_year");
  t.is (iso._month,          in_month,          label + "_month");
  t.is (iso._week,           in_week,           label + "_week");
  t.is (iso._weekday,        in_weekday,        label + "_weekday");
  t.is (iso._julian,         in_julian,         label + "_julian");
  t.is (iso._day,            in_day,            label + "_day");
  t.is (iso._seconds,        in_seconds,        label + "_seconds");
  t.is (iso._offset,         in_offset,         label + "_offset");
  t.is (iso._utc,            in_utc,            label + "_utc");
  t.is ((size_t) iso._value, (size_t) in_value, label + "_value");
}

////////////////////////////////////////////////////////////////////////////////
int main (int argc, char** argv)
{
  UnitTest t (1610
#ifdef AMBIGUOUS
              + 48
#endif
             );

  ISO8601d iso;
  std::string::size_type start = 0;
  t.notok (iso.parse ("foo", start), "foo --> false");
  t.is ((int)start, 0,               "foo[0]");

  // Determine local and UTC time.
  time_t now = time (NULL);
  struct tm* local_now = localtime (&now);
  int local_s = (local_now->tm_hour * 3600) +
                (local_now->tm_min  * 60)   +
                local_now->tm_sec;
  local_now->tm_hour  = 0;
  local_now->tm_min   = 0;
  local_now->tm_sec   = 0;
  local_now->tm_isdst = -1;
  time_t local = mktime (local_now);
  std::cout << "# local midnight today " << local << "\n";

  local_now->tm_year  = 2013 - 1900;
  local_now->tm_mon   = 12 - 1;
  local_now->tm_mday  = 6;
  local_now->tm_isdst = 0;
  time_t local6 = mktime (local_now);
  std::cout << "# local midnight 2013-12-06 " << local6 << "\n";

  local_now->tm_year  = 2013 - 1900;
  local_now->tm_mon   = 12 - 1;
  local_now->tm_mday  = 1;
  local_now->tm_isdst = 0;
  time_t local1 = mktime (local_now);
  std::cout << "# local midnight 2013-12-01 " << local1 << "\n";

  struct tm* utc_now = gmtime (&now);
  int utc_s = (utc_now->tm_hour * 3600) +
              (utc_now->tm_min  * 60)   +
              utc_now->tm_sec;
  utc_now->tm_hour  = 0;
  utc_now->tm_min   = 0;
  utc_now->tm_sec   = 0;
  utc_now->tm_isdst = -1;
  time_t utc = timegm (utc_now);
  std::cout << "# utc midnight today " << utc << "\n";

  utc_now->tm_year  = 2013 - 1900;
  utc_now->tm_mon   = 12 - 1;
  utc_now->tm_mday  = 6;
  utc_now->tm_isdst = 0;
  time_t utc6 = timegm (utc_now);
  std::cout << "# utc midnight 2013-12-06 " << utc6 << "\n";

  utc_now->tm_year  = 2013 - 1900;
  utc_now->tm_mon   = 12 - 1;
  utc_now->tm_mday  = 1;
  utc_now->tm_isdst = 0;
  time_t utc1 = timegm (utc_now);
  std::cout << "# utc midnight 2013-12-01 " << utc1 << "\n";

  int hms = (12 * 3600) + (34 * 60) + 56; // The time 12:34:56 in seconds.
  int hm  = (12 * 3600) + (34 * 60);      // The time 12:34:00 in seconds.
  int h   = (12 * 3600);                  // The time 12:00:00 in seconds.
  int z   = 3600;                         // TZ offset.

  int ld = local_s > hms ? 86400 : 0;     // Local extra day if now > hms.
  int ud = utc_s   > hms ? 86400 : 0;     // UTC extra day if now > hms.
  std::cout << "# ld " << ld << "\n";
  std::cout << "# ud " << ud << "\n";

  // Aggregated.
  //            input                         i  Year  Mo  Wk WD  Jul  Da   Secs     TZ    UTC      time_t
  testParse (t, "12:34:56  ",                 8,    0,  0,  0, 0,   0,  0,   hms,     0, false, local+hms+ld );

  // time-ext
  //            input                         i  Year  Mo  Wk WD  Jul  Da   Secs     TZ    UTC      time_t
  testParse (t, "12:34:56Z",                  9,    0,  0,  0, 0,   0,  0,   hms,     0,  true, utc+hms+ud   );
  testParse (t, "12:34Z",                     6,    0,  0,  0, 0,   0,  0,    hm,     0,  true, utc+hm+ud    );
  testParse (t, "12Z",                        3,    0,  0,  0, 0,   0,  0,     h,     0,  true, utc+h+ud     );
  testParse (t, "12:34:56+01:00",            14,    0,  0,  0, 0,   0,  0,   hms,  3600, false, utc+hms-z+ud );
  testParse (t, "12:34:56+01",               11,    0,  0,  0, 0,   0,  0,   hms,  3600, false, utc+hms-z+ud );
  testParse (t, "12:34+01:00",               11,    0,  0,  0, 0,   0,  0,    hm,  3600, false, utc+hm-z+ud  );
  testParse (t, "12:34+01",                   8,    0,  0,  0, 0,   0,  0,    hm,  3600, false, utc+hm-z+ud  );
  testParse (t, "12+01:00",                   8,    0,  0,  0, 0,   0,  0,     h,  3600, false, utc+h-z+ud   );
  testParse (t, "12+01",                      5,    0,  0,  0, 0,   0,  0,     h,  3600, false, utc+h-z+ud   );
  testParse (t, "12:34:56",                   8,    0,  0,  0, 0,   0,  0,   hms,     0, false, local+hms+ld );
  testParse (t, "12:34",                      5,    0,  0,  0, 0,   0,  0,    hm,     0, false, local+hm+ld  );
#ifdef AMBIGUOUS
  testParse (t, "12",                         2,    0,  0,  0, 0,   0,  0,     h,     0, false, local+h+ld   );
#endif

  // time
  //            input                         i  Year  Mo  Wk WD  Jul  Da   Secs     TZ    UTC      time_t
  testParse (t, "123456Z",                    7,    0,  0,  0, 0,   0,  0,   hms,     0,  true, utc+hms+ud   );
  testParse (t, "1234Z",                      5,    0,  0,  0, 0,   0,  0,    hm,     0,  true, utc+hm+ud    );
  testParse (t, "123456+0100",               11,    0,  0,  0, 0,   0,  0,   hms,  3600, false, utc+hms-z+ud );
  testParse (t, "123456+01",                  9,    0,  0,  0, 0,   0,  0,   hms,  3600, false, utc+hms-z+ud );
  testParse (t, "1234+0100",                  9,    0,  0,  0, 0,   0,  0,    hm,  3600, false, utc+hm-z+ud  );
  testParse (t, "1234+01",                    7,    0,  0,  0, 0,   0,  0,    hm,  3600, false, utc+hm-z+ud  );
  testParse (t, "12+0100",                    7,    0,  0,  0, 0,   0,  0,     h,  3600, false, utc+h-z+ud   );
#ifdef AMBIGUOUS
  testParse (t, "123456",                     6,    0,  0,  0, 0,   0,  0,   hms,     0, false, local+hms+ld );
#endif

  // datetime-ext
  //            input                         i  Year  Mo  Wk WD  Jul  Da   Secs     TZ    UTC      time_t
  testParse (t, "2013-12-06",                10, 2013, 12,  0, 0,   0,  6,     0,     0, false, local6    );
  testParse (t, "2013-340",                   8, 2013,  0,  0, 0, 340,  0,     0,     0, false, local6    );
  testParse (t, "2013-W49-5",                10, 2013,  0, 49, 5,   0,  0,     0,     0, false, local6    );
  testParse (t, "2013-W49",                   8, 2013,  0, 49, 0,   0,  0,     0,     0, false, local1    );

  testParse (t, "2013-12-06T12:34:56",       19, 2013, 12,  0, 0,   0,  6,   hms,     0, false, local6+hms);
  testParse (t, "2013-12-06T12:34",          16, 2013, 12,  0, 0,   0,  6,    hm,     0, false, local6+hm );
  testParse (t, "2013-340T12:34:56",         17, 2013,  0,  0, 0, 340,  0,   hms,     0, false, local6+hms);
  testParse (t, "2013-340T12:34",            14, 2013,  0,  0, 0, 340,  0,    hm,     0, false, local6+hm );
  testParse (t, "2013-W49-5T12:34:56",       19, 2013,  0, 49, 5,   0,  0,   hms,     0, false, local6+hms);
  testParse (t, "2013-W49-5T12:34",          16, 2013,  0, 49, 5,   0,  0,    hm,     0, false, local6+hm );
  testParse (t, "2013-W49T12:34:56",         17, 2013,  0, 49, 0,   0,  0,   hms,     0, false, local1+hms);
  testParse (t, "2013-W49T12:34",            14, 2013,  0, 49, 0,   0,  0,    hm,     0, false, local1+hm );

  testParse (t, "2013-12-06T12:34:56Z",      20, 2013, 12,  0, 0,   0,  6,   hms,     0,  true, utc6+hms  );
  testParse (t, "2013-12-06T12:34Z",         17, 2013, 12,  0, 0,   0,  6,    hm,     0,  true, utc6+hm   );
  testParse (t, "2013-340T12:34:56Z",        18, 2013,  0,  0, 0, 340,  0,   hms,     0,  true, utc6+hms  );
  testParse (t, "2013-340T12:34Z",           15, 2013,  0,  0, 0, 340,  0,    hm,     0,  true, utc6+hm   );
  testParse (t, "2013-W49-5T12:34:56Z",      20, 2013,  0, 49, 5,   0,  0,   hms,     0,  true, utc6+hms  );
  testParse (t, "2013-W49-5T12:34Z",         17, 2013,  0, 49, 5,   0,  0,    hm,     0,  true, utc6+hm   );
  testParse (t, "2013-W49T12:34:56Z",        18, 2013,  0, 49, 0,   0,  0,   hms,     0,  true, utc1+hms  );
  testParse (t, "2013-W49T12:34Z",           15, 2013,  0, 49, 0,   0,  0,    hm,     0,  true, utc1+hm   );

  testParse (t, "2013-12-06T12:34:56+01:00", 25, 2013, 12,  0, 0,   0,  6,   hms,  3600, false, utc6+hms-z);
  testParse (t, "2013-12-06T12:34:56+01",    22, 2013, 12,  0, 0,   0,  6,   hms,  3600, false, utc6+hms-z);
  testParse (t, "2013-12-06T12:34:56-01:00", 25, 2013, 12,  0, 0,   0,  6,   hms, -3600, false, utc6+hms+z);
  testParse (t, "2013-12-06T12:34:56-01",    22, 2013, 12,  0, 0,   0,  6,   hms, -3600, false, utc6+hms+z);
  testParse (t, "2013-12-06T12:34+01:00",    22, 2013, 12,  0, 0,   0,  6,    hm,  3600, false, utc6+hm-z );
  testParse (t, "2013-12-06T12:34+01",       19, 2013, 12,  0, 0,   0,  6,    hm,  3600, false, utc6+hm-z );
  testParse (t, "2013-12-06T12:34-01:00",    22, 2013, 12,  0, 0,   0,  6,    hm, -3600, false, utc6+hm+z );
  testParse (t, "2013-12-06T12:34-01",       19, 2013, 12,  0, 0,   0,  6,    hm, -3600, false, utc6+hm+z );
  testParse (t, "2013-340T12:34:56+01:00",   23, 2013,  0,  0, 0, 340,  0,   hms,  3600, false, utc6+hms-z);
  testParse (t, "2013-340T12:34:56+01",      20, 2013,  0,  0, 0, 340,  0,   hms,  3600, false, utc6+hms-z);
  testParse (t, "2013-340T12:34:56-01:00",   23, 2013,  0,  0, 0, 340,  0,   hms, -3600, false, utc6+hms+z);
  testParse (t, "2013-340T12:34:56-01",      20, 2013,  0,  0, 0, 340,  0,   hms, -3600, false, utc6+hms+z);
  testParse (t, "2013-340T12:34+01:00",      20, 2013,  0,  0, 0, 340,  0,    hm,  3600, false, utc6+hm-z );
  testParse (t, "2013-340T12:34+01",         17, 2013,  0,  0, 0, 340,  0,    hm,  3600, false, utc6+hm-z );
  testParse (t, "2013-340T12:34-01:00",      20, 2013,  0,  0, 0, 340,  0,    hm, -3600, false, utc6+hm+z );
  testParse (t, "2013-340T12:34-01",         17, 2013,  0,  0, 0, 340,  0,    hm, -3600, false, utc6+hm+z );
  testParse (t, "2013-W49-5T12:34:56+01:00", 25, 2013,  0, 49, 5,   0,  0,   hms,  3600, false, utc6+hms-z);
  testParse (t, "2013-W49-5T12:34:56+01",    22, 2013,  0, 49, 5,   0,  0,   hms,  3600, false, utc6+hms-z);
  testParse (t, "2013-W49-5T12:34:56-01:00", 25, 2013,  0, 49, 5,   0,  0,   hms, -3600, false, utc6+hms+z);
  testParse (t, "2013-W49-5T12:34:56-01",    22, 2013,  0, 49, 5,   0,  0,   hms, -3600, false, utc6+hms+z);
  testParse (t, "2013-W49-5T12:34+01:00",    22, 2013,  0, 49, 5,   0,  0,    hm,  3600, false, utc6+hm-z );
  testParse (t, "2013-W49-5T12:34+01",       19, 2013,  0, 49, 5,   0,  0,    hm,  3600, false, utc6+hm-z );
  testParse (t, "2013-W49-5T12:34-01:00",    22, 2013,  0, 49, 5,   0,  0,    hm, -3600, false, utc6+hm+z );
  testParse (t, "2013-W49-5T12:34-01",       19, 2013,  0, 49, 5,   0,  0,    hm, -3600, false, utc6+hm+z );
  testParse (t, "2013-W49T12:34:56+01:00",   23, 2013,  0, 49, 0,   0,  0,   hms,  3600, false, utc1+hms-z);
  testParse (t, "2013-W49T12:34:56+01",      20, 2013,  0, 49, 0,   0,  0,   hms,  3600, false, utc1+hms-z);
  testParse (t, "2013-W49T12:34:56-01:00",   23, 2013,  0, 49, 0,   0,  0,   hms, -3600, false, utc1+hms+z);
  testParse (t, "2013-W49T12:34:56-01",      20, 2013,  0, 49, 0,   0,  0,   hms, -3600, false, utc1+hms+z);
  testParse (t, "2013-W49T12:34+01:00",      20, 2013,  0, 49, 0,   0,  0,    hm,  3600, false, utc1+hm-z );
  testParse (t, "2013-W49T12:34+01",         17, 2013,  0, 49, 0,   0,  0,    hm,  3600, false, utc1+hm-z );
  testParse (t, "2013-W49T12:34-01:00",      20, 2013,  0, 49, 0,   0,  0,    hm, -3600, false, utc1+hm+z );
  testParse (t, "2013-W49T12:34-01",         17, 2013,  0, 49, 0,   0,  0,    hm, -3600, false, utc1+hm+z );

  // datetime
#ifdef AMBIGUOUS
  testParse (t, "20131206",                   8, 2013, 12,  0, 0,   0,  6,     0,     0, false, local6    );
#endif
  testParse (t, "2013W495",                   8, 2013,  0, 49, 5,   0,  0,     0,     0, false, local6    );
  testParse (t, "2013W49",                    7, 2013,  0, 49, 0,   0,  0,     0,     0, false, local1    );
#ifdef AMBIGUOUS
  testParse (t, "2013340",                    7, 2013,  0,  0, 0, 340,  0,     0,     0, false, local6    );
#endif
  testParse (t, "2013-12",                    7, 2013, 12,  0, 0,   0,  0,     0,     0, false, local1    );

  testParse (t, "20131206T123456",           15, 2013, 12,  0, 0,   0,  6,   hms,     0, false, local6+hms);
  testParse (t, "20131206T12",               11, 2013, 12,  0, 0,   0,  6,     h,     0, false, local6+h  );
  testParse (t, "2013W495T123456",           15, 2013,  0, 49, 5,   0,  0,   hms,     0, false, local6+hms);
  testParse (t, "2013W495T12",               11, 2013,  0, 49, 5,   0,  0,     h,     0, false, local6+h  );
  testParse (t, "2013W49T123456",            14, 2013,  0, 49, 0,   0,  0,   hms,     0, false, local1+hms);
  testParse (t, "2013W49T12",                10, 2013,  0, 49, 0,   0,  0,     h,     0, false, local1+h  );
  testParse (t, "2013340T123456",            14, 2013,  0,  0, 0, 340,  0,   hms,     0, false, local6+hms);
  testParse (t, "2013340T12",                10, 2013,  0,  0, 0, 340,  0,     h,     0, false, local6+h  );
  testParse (t, "2013-12T1234",              12, 2013, 12,  0, 0,   0,  0,    hm,     0, false, local1+hm );
  testParse (t, "2013-12T12",                10, 2013, 12,  0, 0,   0,  0,     h,     0, false, local1+h  );

  testParse (t, "20131206T123456Z",          16, 2013, 12,  0, 0,   0,  6,   hms,     0,  true, utc6+hms  );
  testParse (t, "20131206T12Z",              12, 2013, 12,  0, 0,   0,  6,     h,     0,  true, utc6+h    );
  testParse (t, "2013W495T123456Z",          16, 2013,  0, 49, 5,   0,  0,   hms,     0,  true, utc6+hms  );
  testParse (t, "2013W495T12Z",              12, 2013,  0, 49, 5,   0,  0,     h,     0,  true, utc6+h    );
  testParse (t, "2013W49T123456Z",           15, 2013,  0, 49, 0,   0,  0,   hms,     0,  true, utc1+hms  );
  testParse (t, "2013W49T12Z",               11, 2013,  0, 49, 0,   0,  0,     h,     0,  true, utc1+h    );
  testParse (t, "2013340T123456Z",           15, 2013,  0,  0, 0, 340,  0,   hms,     0,  true, utc6+hms  );
  testParse (t, "2013340T12Z",               11, 2013,  0,  0, 0, 340,  0,     h,     0,  true, utc6+h    );
  testParse (t, "2013-12T123456Z",           15, 2013, 12,  0, 0,   0,  0,   hms,     0,  true, utc1+hms  );
  testParse (t, "2013-12T12Z",               11, 2013, 12,  0, 0,   0,  0,     h,     0,  true, utc1+h    );

  testParse (t, "20131206T123456+0100",      20, 2013, 12,  0, 0,   0,  6,   hms,  3600, false, utc6+hms-z);
  testParse (t, "20131206T123456+01",        18, 2013, 12,  0, 0,   0,  6,   hms,  3600, false, utc6+hms-z);
  testParse (t, "20131206T123456-0100",      20, 2013, 12,  0, 0,   0,  6,   hms, -3600, false, utc6+hms+z);
  testParse (t, "20131206T123456-01",        18, 2013, 12,  0, 0,   0,  6,   hms, -3600, false, utc6+hms+z);
  testParse (t, "20131206T12+0100",          16, 2013, 12,  0, 0,   0,  6,     h,  3600, false, utc6+h-z  );
  testParse (t, "20131206T12+01",            14, 2013, 12,  0, 0,   0,  6,     h,  3600, false, utc6+h-z  );
  testParse (t, "20131206T12-0100",          16, 2013, 12,  0, 0,   0,  6,     h, -3600, false, utc6+h+z  );
  testParse (t, "20131206T12-01",            14, 2013, 12,  0, 0,   0,  6,     h, -3600, false, utc6+h+z  );
  testParse (t, "2013W495T123456+0100",      20, 2013,  0, 49, 5,   0,  0,   hms,  3600, false, utc6+hms-z);
  testParse (t, "2013W495T123456+01",        18, 2013,  0, 49, 5,   0,  0,   hms,  3600, false, utc6+hms-z);
  testParse (t, "2013W495T123456-0100",      20, 2013,  0, 49, 5,   0,  0,   hms, -3600, false, utc6+hms+z);
  testParse (t, "2013W495T123456-01",        18, 2013,  0, 49, 5,   0,  0,   hms, -3600, false, utc6+hms+z);
  testParse (t, "2013W495T12+0100",          16, 2013,  0, 49, 5,   0,  0,     h,  3600, false, utc6+h-z  );
  testParse (t, "2013W495T12+01",            14, 2013,  0, 49, 5,   0,  0,     h,  3600, false, utc6+h-z  );
  testParse (t, "2013W495T12-0100",          16, 2013,  0, 49, 5,   0,  0,     h, -3600, false, utc6+h+z  );
  testParse (t, "2013W495T12-01",            14, 2013,  0, 49, 5,   0,  0,     h, -3600, false, utc6+h+z  );
  testParse (t, "2013W49T123456+0100",       19, 2013,  0, 49, 0,   0,  0,   hms,  3600, false, utc1+hms-z);
  testParse (t, "2013W49T123456+01",         17, 2013,  0, 49, 0,   0,  0,   hms,  3600, false, utc1+hms-z);
  testParse (t, "2013W49T123456-0100",       19, 2013,  0, 49, 0,   0,  0,   hms, -3600, false, utc1+hms+z);
  testParse (t, "2013W49T123456-01",         17, 2013,  0, 49, 0,   0,  0,   hms, -3600, false, utc1+hms+z);
  testParse (t, "2013W49T12+0100",           15, 2013,  0, 49, 0,   0,  0,     h,  3600, false, utc1+h-z  );
  testParse (t, "2013W49T12+01",             13, 2013,  0, 49, 0,   0,  0,     h,  3600, false, utc1+h-z  );
  testParse (t, "2013W49T12-0100",           15, 2013,  0, 49, 0,   0,  0,     h, -3600, false, utc1+h+z  );
  testParse (t, "2013W49T12-01",             13, 2013,  0, 49, 0,   0,  0,     h, -3600, false, utc1+h+z  );
  testParse (t, "2013340T123456+0100",       19, 2013,  0,  0, 0, 340,  0,   hms,  3600, false, utc6+hms-z);
  testParse (t, "2013340T123456+01",         17, 2013,  0,  0, 0, 340,  0,   hms,  3600, false, utc6+hms-z);
  testParse (t, "2013340T123456-0100",       19, 2013,  0,  0, 0, 340,  0,   hms, -3600, false, utc6+hms+z);
  testParse (t, "2013340T123456-01",         17, 2013,  0,  0, 0, 340,  0,   hms, -3600, false, utc6+hms+z);
  testParse (t, "2013340T12+0100",           15, 2013,  0,  0, 0, 340,  0,     h,  3600, false, utc6+h-z  );
  testParse (t, "2013340T12+01",             13, 2013,  0,  0, 0, 340,  0,     h,  3600, false, utc6+h-z  );
  testParse (t, "2013340T12-0100",           15, 2013,  0,  0, 0, 340,  0,     h, -3600, false, utc6+h+z  );
  testParse (t, "2013340T12-01",             13, 2013,  0,  0, 0, 340,  0,     h, -3600, false, utc6+h+z  );
  testParse (t, "2013-12T123456+0100",       19, 2013, 12,  0, 0,   0,  0,   hms,  3600, false, utc1+hms-z);
  testParse (t, "2013-12T123456+01",         17, 2013, 12,  0, 0,   0,  0,   hms,  3600, false, utc1+hms-z);
  testParse (t, "2013-12T123456-0100",       19, 2013, 12,  0, 0,   0,  0,   hms, -3600, false, utc1+hms+z);
  testParse (t, "2013-12T123456-01",         17, 2013, 12,  0, 0,   0,  0,   hms, -3600, false, utc1+hms+z);
  testParse (t, "2013-12T12+0100",           15, 2013, 12,  0, 0,   0,  0,     h,  3600, false, utc1+h-z  );
  testParse (t, "2013-12T12+01",             13, 2013, 12,  0, 0,   0,  0,     h,  3600, false, utc1+h-z  );
  testParse (t, "2013-12T12-0100",           15, 2013, 12,  0, 0,   0,  0,     h, -3600, false, utc1+h+z  );
  testParse (t, "2013-12T12-01",             13, 2013, 12,  0, 0,   0,  0,     h, -3600, false, utc1+h+z  );

  // TODO Test validation of individual values.

  return 0;
}

////////////////////////////////////////////////////////////////////////////////
