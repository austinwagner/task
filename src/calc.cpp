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
#include <string>
#include <stdlib.h>
#include <string.h>
#include <Eval.h>
#include <Dates.h>
#include <Context.h>
#include <text.h>
#include <i18n.h>
#include <nowide/iostream.hpp>
#include <nowide/args.hpp>

Context context;

////////////////////////////////////////////////////////////////////////////////
// Constants.
bool get (const std::string& name, Variant& value)
{
/*
  // An example, although a bad one because this is supported by default.
  if (name == "pi") {value = Variant (3.14159165); return true;}
*/

  return false;
}

////////////////////////////////////////////////////////////////////////////////
int main (int argc, const char** argv)
{
  nowide::args(argc, *const_cast<char***>(&argv));
  int status = 0;

  try
  {
    bool infix     = true;
    bool ambiguous = false;

    // Add a source for constants.
    Eval e;
    e.addSource (namedDates);
    e.addSource (get);

    // Combine all the arguments into one expression string.
    std::string expression;
    for (int i = 1; i < argc; i++)
      if (!strcmp (argv[i], "-h") || ! strcmp (argv[i], "--help"))
      {
        nowide::cout << "\n"
                  << "Usage: " << argv[0] << " [options] '<expression>'\n"
                  << "\n"
                  << "Options:\n"
                  << "  -h|--help         Display this usage\n"
                  << "  -d|--debug        Debug mode\n"
                  << "  -i|--infix        Infix expression (default)\n"
                  << "  -p|--postfix      Postfix expression\n"
                  << "  -a|--ambiguous    Choose dates over numbers when ambiguous\n"
                  << "  -n|--noambiguous  Choose numbers over dates when ambiguous (default)\n"
                  << "\n";
        exit (1);
      }
      else if (!strcmp (argv[i], "-v") || !strcmp (argv[i], "--version"))
      {
        nowide::cout << "\n"
                  << format (STRING_CMD_VERSION_BUILT, "calc", VERSION)
#if defined (DARWIN)
                  << "darwin"
#elif defined (SOLARIS)
                  << "solaris"
#elif defined (CYGWIN)
                  << "cygwin"
#elif defined (HAIKU)
                  << "haiku"
#elif defined (OPENBSD)
                  << "openbsd"
#elif defined (FREEBSD)
                  << "freebsd"
#elif defined (NETBSD)
                  << "netbsd"
#elif defined (LINUX)
                  << "linux"
#elif defined (KFREEBSD)
                  << "gnu-kfreebsd"
#elif defined (GNUHURD)
                  << "gnu-hurd"
#else
                  << STRING_CMD_VERSION_UNKNOWN
#endif
                  << "\n"
                  << STRING_CMD_VERSION_COPY
                  << "\n"
                  << "\n"
                  << STRING_CMD_VERSION_MIT
                  << "\n"
                  << "\n";

        exit (1);
      }
      else if (!strcmp (argv[i], "-d") || !strcmp (argv[i], "--debug"))
        e.debug (true);
      else if (!strcmp (argv[i], "-i") || !strcmp (argv[i], "--infix"))
        infix = true;
      else if (!strcmp (argv[i], "-p") || !strcmp (argv[i], "--postfix"))
        infix = false;
      else if (!strcmp (argv[i], "-a") || !strcmp (argv[i], "--ambiguous"))
        ambiguous = true;
      else if (!strcmp (argv[i], "-n") || !strcmp (argv[i], "--noambiguous"))
        ambiguous = false;
      else
        expression += std::string (argv[i]) + " ";

    e.ambiguity (ambiguous);

    Variant result;
    if (infix)
      e.evaluateInfixExpression (expression, result);
    else
      e.evaluatePostfixExpression (expression, result);

    // Show any debug output.
    std::vector <std::string>::iterator i;
    for (i = context.debugMessages.begin (); i != context.debugMessages.end (); ++i)
      nowide::cout << *i << "\n";

    // Show the result in string form.
    nowide::cout << (std::string) result
              << "\n";
  }

  catch (const std::string& error)
  {
    nowide::cerr << error << "\n";
    status = -1;
  }

  catch (...)
  {
    nowide::cerr << "Unknown error occured.  Oops.\n";
    status = -2;
  }

  return status;
}

////////////////////////////////////////////////////////////////////////////////
