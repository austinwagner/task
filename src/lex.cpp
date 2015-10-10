////////////////////////////////////////////////////////////////////////////////

#include <iostream>
#include <Lexer.h>
#include <Context.h>
#include <nowide/iostream.hpp>
#include <nowide/args.hpp>

Context context;

int main (int argc, const char** argv)
{
  nowide::args(argc, *const_cast<char***>(&argv));

  for (auto i = 1; i < argc; i++)
  {
    nowide::cout << "argument '" << argv[i] << "'\n";

    // Low-level tokens.
    Lexer lexer (argv[i]);
    std::string token;
    Lexer::Type type;
    while (lexer.token (token, type))
      nowide::cout << "  token '" << token << "' " << Lexer::typeToString (type) << "\n";

/*
    // High-level tokens.
    auto all = Lexer::tokens (argv[i]);
    for (auto token : Lexer::tokens (argv[i]))
      nowide::cout << "  token '" << token.first << "' " << Lexer::typeToString (token.second) << "\n";
*/
  }
}

////////////////////////////////////////////////////////////////////////////////
