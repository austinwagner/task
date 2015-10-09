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
#include <fstream>
#include <sstream>
#include <algorithm>
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <Context.h>
#include <Directory.h>
#include <File.h>
#include <Eval.h>
#include <Variant.h>
#include <text.h>
#include <util.h>
#include <main.h>
#include <i18n.h>
#include <nowide/iostream.hpp>

#ifdef HAVE_COMMIT
#include <commit.h>
#endif

// Supported modifiers, synonyms on the same line.
static const char* modifierNames[] =
{
  "before",     "under",    "below",
  "after",      "over",     "above",
  "none",
  "any",
  "is",         "equals",
  "isnt",       "not",
  "has",        "contains",
  "hasnt",
  "startswith", "left",
  "endswith",   "right",
  "word",
  "noword"
};

#define NUM_MODIFIER_NAMES       (sizeof (modifierNames) / sizeof (modifierNames[0]))

////////////////////////////////////////////////////////////////////////////////
Context::Context ()
: rc_file ("~/.taskrc")
, data_dir ("~/.task")
, config ()
, tdb2 ()
, dom ()
, determine_color_use (true)
, use_color (true)
, run_gc (true)
, verbosity_legacy (false)
, terminal_width (0)
, terminal_height (0)
{
}

////////////////////////////////////////////////////////////////////////////////
Context::~Context ()
{
  std::map<std::string, Command*>::iterator com;
  for (com = commands.begin (); com != commands.end (); ++com)
    delete com->second;

  std::map<std::string, Column*>::iterator col;
  for (col = columns.begin (); col != columns.end (); ++col)
    delete col->second;
}

////////////////////////////////////////////////////////////////////////////////
int Context::initialize (int argc, const char** argv)
{
  timer_init.start ();
  int rc = 0;

  try
  {
    ////////////////////////////////////////////////////////////////////////////
    //
    // [1] Load the correct config file.
    //     - Default to ~/.taskrc (ctor).
    //     - Allow command line override rc:<file>
    //     - Allow $TASKRC override.
    //     - Load resultant file.
    //     - Apply command line overrides to the config.
    //
    ////////////////////////////////////////////////////////////////////////////

    CLI::getOverride (argc, argv, home_dir, rc_file);

    char* override = getenv ("TASKRC");
    if (override)
    {
      rc_file = File (override);
      header (format (STRING_CONTEXT_RC_OVERRIDE, rc_file.to_string()));
    }

    config.clear ();
    config.load (rc_file);
    CLI::applyOverrides (argc, argv);

    ////////////////////////////////////////////////////////////////////////////
    //
    // [2] Locate the data directory.
    //     - Default to ~/.task (ctor).
    //     - Allow command line override rc.data.location:<dir>
    //     - Allow $TASKDATA override.
    //     - Inform TDB2 where to find data.
    //     - Create the rc_file and data_dir, if necessary.
    //
    ////////////////////////////////////////////////////////////////////////////

    CLI::getDataLocation (argc, argv, data_dir);

    override = getenv ("TASKDATA");
    if (override)
    {
      data_dir = Directory (override);
      config.set ("data.location", data_dir.to_string());
      header (format (STRING_CONTEXT_DATA_OVERRIDE, data_dir.to_string()));
    }

    tdb2.set_location (data_dir);
    createDefaultConfig ();

    ////////////////////////////////////////////////////////////////////////////
    //
    // [3] Instantiate Command objects and capture entities.
    //
    ////////////////////////////////////////////////////////////////////////////

    Command::factory (commands);
    std::map <std::string, Command*>::iterator cmd;
    for (cmd = commands.begin (); cmd != commands.end (); ++cmd)
    {
      cli.entity ("cmd", cmd->first);
      cli.entity ((cmd->second->read_only () ? "readcmd" : "writecmd"), cmd->first);

      if (cmd->first[0] == '_')
        cli.entity ("helper", cmd->first);
    }

    ////////////////////////////////////////////////////////////////////////////
    //
    // [4] Instantiate Column objects and capture entities.
    //
    ////////////////////////////////////////////////////////////////////////////

    Column::factory (columns);
    std::map <std::string, Column*>::iterator col;
    for (col = columns.begin (); col != columns.end (); ++col)
      cli.entity ("attribute", col->first);

    cli.entity ("pseudo", "limit");

    ////////////////////////////////////////////////////////////////////////////
    //
    // [5] Capture modifier and operator entities.
    //
    ////////////////////////////////////////////////////////////////////////////

    for (unsigned int i = 0; i < NUM_MODIFIER_NAMES; ++i)
      cli.entity ("modifier", modifierNames[i]);

    std::vector <std::string> operators;
    Eval::getOperators (operators);
    std::vector <std::string>::iterator op;
    for (op = operators.begin (); op != operators.end (); ++op)
      cli.entity ("operator", *op);

    Eval::getBinaryOperators (operators);
    for (op = operators.begin (); op != operators.end (); ++op)
      cli.entity ("binary_operator", *op);

    ////////////////////////////////////////////////////////////////////////////
    //
    // [6] Complete the Context initialization.
    //
    ////////////////////////////////////////////////////////////////////////////

    initializeColorRules ();
    staticInitialization ();
    propagateDebug ();
    loadAliases ();

    ////////////////////////////////////////////////////////////////////////////
    //
    // [7] Parse the command line.
    //
    ////////////////////////////////////////////////////////////////////////////

    // Scan command line for 'rc:<file>' only.
    cli.initialize (argc, argv);
    cli.analyze (true, true);

    // Extract a recomposed command line.
    bool foundDefault = false;
    bool foundAssumed = false;
    std::string combined;
    std::vector <A>::const_iterator a;
    for (a = cli._args.begin (); a != cli._args.end (); ++a)
    {
      if (combined.length ())
        combined += ' ';

      if (a->attribute ("canonical") != "")
        combined += a->attribute ("canonical");
      else
        combined += a->attribute ("raw");

      if (a->hasTag ("DEFAULT"))
        foundDefault = true;

      if (a->hasTag ("ASSUMED"))
        foundAssumed = true;
    }

    if (foundDefault)
      header ("[" + combined + "]");

    if (foundAssumed)
      header (STRING_ASSUME_INFO);

    ////////////////////////////////////////////////////////////////////////////
    //
    // [8] Initialize hooks.
    //
    ////////////////////////////////////////////////////////////////////////////

    hooks.initialize ();
  }

  catch (const std::string& message)
  {
    error (message);
    rc = 2;
  }

  catch (int)
  {
    // Hooks can terminate processing by throwing integers.
    rc = 4;
  }

  catch (...)
  {
    error (STRING_UNKNOWN_ERROR);
    rc = 3;
  }

  // On initialization failure...
  if (rc)
  {
    // Dump all debug messages, controlled by rc.debug.
    if (config.getBoolean ("debug"))
    {
      std::vector <std::string>::iterator d;
      for (d = debugMessages.begin (); d != debugMessages.end (); ++d)
        if (color ())
          nowide::cerr << colorizeDebug (*d) << "\n";
        else
          nowide::cerr << *d << "\n";
    }

    // Dump all headers, controlled by 'header' verbosity token.
    if (verbose ("header"))
    {
      std::vector <std::string>::iterator h;
      for (h = headers.begin (); h != headers.end (); ++h)
        if (color ())
          nowide::cerr << colorizeHeader (*h) << "\n";
        else
          nowide::cerr << *h << "\n";
    }

    // Dump all footnotes, controlled by 'footnote' verbosity token.
    if (verbose ("footnote"))
    {
      std::vector <std::string>::iterator f;
      for (f = footnotes.begin (); f != footnotes.end (); ++f)
        if (color ())
          nowide::cerr << colorizeFootnote (*f) << "\n";
        else
          nowide::cerr << *f << "\n";
    }

    // Dump all errors, non-maskable.
    // Colorized as footnotes.
    std::vector <std::string>::iterator e;
    for (e = errors.begin (); e != errors.end (); ++e)
      if (color ())
        nowide::cerr << colorizeFootnote (*e) << "\n";
      else
        nowide::cerr << *e << "\n";
  }

  timer_init.stop ();
  return rc;
}

////////////////////////////////////////////////////////////////////////////////
int Context::run ()
{
  int rc;
  std::string output;

  try
  {
    hooks.onLaunch ();
    rc = dispatch (output);
    tdb2.commit ();           // Harmless if called when nothing changed.
    hooks.onExit ();          // No chance to update data.

    std::stringstream s;
    s << "Perf "
      << PACKAGE_STRING
      << " "
#ifdef HAVE_COMMIT
      << COMMIT
#else
      << "-"
#endif
      << " "
      << Date ().toISO ()

      << " init:"   << timer_init.total ()
      << " load:"   << timer_load.total ()
      << " gc:"     << timer_gc.total ()
      << " filter:" << timer_filter.total ()
      << " commit:" << timer_commit.total ()
      << " sort:"   << timer_sort.total ()
      << " render:" << timer_render.total ()
      << " hooks:"  << timer_hooks.total ()
      << " total:"  << (timer_init.total ()   +
                        timer_load.total ()   +
                        timer_gc.total ()     +
                        timer_filter.total () +
                        timer_commit.total () +
                        timer_sort.total ()   +
                        timer_render.total () +
                        timer_hooks.total ())
      << "\n";
    debug (s.str ());
  }

  catch (const std::string& message)
  {
    error (message);
    rc = 2;
  }

  catch (int)
  {
    // Hooks can terminate processing by throwing integers.
    rc = 4;
  }

  catch (...)
  {
    error (STRING_UNKNOWN_ERROR);
    rc = 3;
  }

  // Dump all debug messages, controlled by rc.debug.
  if (config.getBoolean ("debug"))
  {
    std::vector <std::string>::iterator d;
    for (d = debugMessages.begin (); d != debugMessages.end (); ++d)
      if (color ())
        nowide::cerr << colorizeDebug (*d) << "\n";
      else
        nowide::cerr << *d << "\n";
  }

  // Dump all headers, controlled by 'header' verbosity token.
  if (verbose ("header"))
  {
    std::vector <std::string>::iterator h;
    for (h = headers.begin (); h != headers.end (); ++h)
      if (color ())
        nowide::cerr << colorizeHeader (*h) << "\n";
      else
        nowide::cerr << *h << "\n";
  }

  // Dump the report output.
  nowide::cout << output;

  // Dump all footnotes, controlled by 'footnote' verbosity token.
  if (verbose ("footnote"))
  {
    std::vector <std::string>::iterator f;
    for (f = footnotes.begin (); f != footnotes.end (); ++f)
      if (color ())
        nowide::cerr << colorizeFootnote (*f) << "\n";
      else
        nowide::cerr << *f << "\n";
  }

  // Dump all errors, non-maskable.
  // Colorized as footnotes.
  std::vector <std::string>::iterator e;
  for (e = errors.begin (); e != errors.end (); ++e)
    if (color ())
      nowide::cerr << colorizeError (*e) << "\n";
    else
      nowide::cerr << *e << "\n";

  return rc;
}

////////////////////////////////////////////////////////////////////////////////
// Dispatch to the command found by the CLI parser.
int Context::dispatch (std::string &out)
{
  // Autocomplete args against keywords.
  std::string command = cli.getCommand ();
  if (command != "")
  {
    updateXtermTitle ();
    updateVerbosity ();

    Command* c = commands[command];
    assert (c);

    // GC is invoked prior to running any command that displays task IDs, if
    // possible.
    if (c->displays_id () && !tdb2.read_only ())
    {
      run_gc = config.getBoolean ("gc");
      tdb2.gc ();
    }
    else
    {
      run_gc = false;
    }

/*
    // Only read-only commands can be run when TDB2 is read-only.
    // TODO Implement TDB2::read_only
    if (tdb2.read_only () && !c->read_only ())
      throw std::string ("");
*/

    return c->execute (out);
  }

  assert (commands["help"]);
  return commands["help"]->execute (out);
}

////////////////////////////////////////////////////////////////////////////////
bool Context::color ()
{
#ifdef FEATURE_COLOR
  if (determine_color_use)
  {
    // What the config says.
    use_color = config.getBoolean ("color");

    // Only tty's support color.
    if (! isatty (STDOUT_FILENO))
    {
      // No ioctl.
      config.set ("detection", "off");
      config.set ("color",     "off");

      // Files don't get color.
      use_color = false;
    }

    // Override.
    if (config.getBoolean ("_forcecolor"))
    {
      config.set ("color", "on");
      use_color = true;
    }

    // No need to go through this again.
    determine_color_use = false;
  }

  // Cached result.
  return use_color;
#else
  return false;
#endif
}

////////////////////////////////////////////////////////////////////////////////
// Support verbosity levels:
//
//   rc.verbose=1          Show all feedback.
//   rc.verbose=0          Show regular feedback.
//   rc.verbose=nothing    Show the absolute minimum.
//   rc.verbose=one,two    Show verbosity for 'one' and 'two' only.
//
// TODO This mechanism is clunky, and should slowly evolve into something more
//      logical and consistent.  This should probably mean that 'nothing' should
//      take the place of '0'.
bool Context::verbose (const std::string& token)
{
  if (! verbosity.size ())
  {
    verbosity_legacy = config.getBoolean ("verbose");
    split (verbosity, config.get ("verbose"), ',');

    // Regular feedback means almost everything.
    // This odd test is to see if a Boolean-false value is a real one, which
    // means it is not 1/true/T/yes/on, but also should not be one of the
    // valid tokens either.
    if (!verbosity_legacy               &&
        verbosity.size ()               &&
        verbosity[0]      != "nothing"  &&
        verbosity[0]      != "blank"    &&  // This list must be complete.
        verbosity[0]      != "header"   &&  //
        verbosity[0]      != "footnote" &&  //
        verbosity[0]      != "label"    &&  //
        verbosity[0]      != "new-id"   &&  //
        verbosity[0]      != "new-uuid" &&  //
        verbosity[0]      != "affected" &&  //
        verbosity[0]      != "edit"     &&  //
        verbosity[0]      != "special"  &&  //
        verbosity[0]      != "project"  &&  //
        verbosity[0]      != "sync"     &&  //
        verbosity[0]      != "filter")      //
    {
      verbosity.clear ();

      // This list emulates rc.verbose=off in version 1.9.4.
      verbosity.push_back ("blank");
      verbosity.push_back ("label");
      verbosity.push_back ("new-id");
      verbosity.push_back ("edit");
    }
  }

  // rc.verbose=true|y|yes|1|on overrides all.
  if (verbosity_legacy)
    return true;

  // rc.verbose=nothing overrides all.
  if (verbosity.size () == 1 &&
      verbosity[0] == "nothing")
    return false;

  // Specific token match.
  if (std::find (verbosity.begin (), verbosity.end (), token) != verbosity.end ())
    return true;

  return false;
}

////////////////////////////////////////////////////////////////////////////////
const std::vector <std::string> Context::getColumns () const
{
  std::vector <std::string> output;
  std::map <std::string, Column*>::const_iterator i;
  for (i = columns.begin (); i != columns.end (); ++i)
    output.push_back (i->first);

  return output;
}

////////////////////////////////////////////////////////////////////////////////
const std::vector <std::string> Context::getCommands () const
{
  std::vector <std::string> output;
  std::map <std::string, Command*>::const_iterator i;
  for (i = commands.begin (); i != commands.end (); ++i)
    output.push_back (i->first);

  return output;
}

////////////////////////////////////////////////////////////////////////////////
// A value of zero mean unlimited.
// A value of 'page' means however many screen lines there are.
// A value of a positive integer is a row/task limit.
void Context::getLimits (int& rows, int& lines)
{
  rows = 0;
  lines = 0;

  // This is an integer specified as a filter (limit:10).
  std::string limit = cli.getLimit ();
  if (limit != "")
  {
    if (limit == "page")
    {
      rows = 0;
      lines = getHeight ();
    }
    else
    {
      rows = (int) strtol (limit.c_str (), NULL, 10);
      lines = 0;
    }
  }
}

////////////////////////////////////////////////////////////////////////////////
// The 'Task' object, among others, is shared between projects.  To make this
// easier, it has been decoupled from Context.
void Context::staticInitialization ()
{
  CLI::minimumMatchLength   = config.getInteger ("abbreviation.minimum");

  Task::defaultProject      = config.get ("default.project");
  Task::defaultDue          = config.get ("default.due");

  Task::searchCaseSensitive = Variant::searchCaseSensitive = config.getBoolean ("search.case.sensitive");
  Task::regex               = Variant::searchUsingRegex    = config.getBoolean ("regex");
  Lexer::dateFormat         = Variant::dateFormat          = config.get ("dateformat");
  Lexer::isoEnabled         = Variant::isoEnabled          = config.getBoolean ("date.iso");

  Config::const_iterator rc;
  for (rc = config.begin (); rc != config.end (); ++rc)
  {
    if (rc->first.substr (0, 4) == "uda." &&
        rc->first.substr (rc->first.length () - 7, 7) == ".values")
    {
      std::string name = rc->first.substr (4, rc->first.length () - 7 - 4);
      std::vector <std::string> values;
      split (values, rc->second, ',');

      for (auto r = values.rbegin(); r != values.rend (); ++r)
        Task::customOrder[name].push_back (*r);
    }
  }

  std::map <std::string, Column*>::iterator i;
  for (i = columns.begin (); i != columns.end (); ++i)
    Task::attributes[i->first] = i->second->type ();

  Task::urgencyProjectCoefficient     = config.getReal ("urgency.project.coefficient");
  Task::urgencyActiveCoefficient      = config.getReal ("urgency.active.coefficient");
  Task::urgencyScheduledCoefficient   = config.getReal ("urgency.scheduled.coefficient");
  Task::urgencyWaitingCoefficient     = config.getReal ("urgency.waiting.coefficient");
  Task::urgencyBlockedCoefficient     = config.getReal ("urgency.blocked.coefficient");
  Task::urgencyInheritCoefficient     = config.getReal ("urgency.inherit.coefficient");
  Task::urgencyAnnotationsCoefficient = config.getReal ("urgency.annotations.coefficient");
  Task::urgencyTagsCoefficient        = config.getReal ("urgency.tags.coefficient");
  Task::urgencyNextCoefficient        = config.getReal ("urgency.next.coefficient");
  Task::urgencyDueCoefficient         = config.getReal ("urgency.due.coefficient");
  Task::urgencyBlockingCoefficient    = config.getReal ("urgency.blocking.coefficient");
  Task::urgencyAgeCoefficient         = config.getReal ("urgency.age.coefficient");
  Task::urgencyAgeMax                 = config.getReal ("urgency.age.max");

  // Tag- and project-specific coefficients.
  std::vector <std::string> all;
  config.all (all);
  std::vector <std::string>::iterator var;
  for (var = all.begin (); var != all.end (); ++var)
    if (var->substr (0, 13) == "urgency.user." ||
        var->substr (0, 12) == "urgency.uda.")
      Task::coefficients[*var] = config.getReal (*var);
}

////////////////////////////////////////////////////////////////////////////////
void Context::createDefaultConfig ()
{
  // Do we need to create a default rc?
  if (! rc_file.exists ())
  {
    if (config.getBoolean ("confirmation") &&
        !confirm (format (STRING_CONTEXT_CREATE_RC, home_dir, rc_file.to_string())))
      throw std::string (STRING_CONTEXT_NEED_RC);

    config.createDefaultRC (rc_file, data_dir.original());
  }

  // Create data location, if necessary.
  config.createDefaultData (data_dir);
}

////////////////////////////////////////////////////////////////////////////////
void Context::decomposeSortField (
  const std::string& field,
  std::string& key,
  bool& ascending,
  bool& breakIndicator)
{
  int length = field.length ();

  int decoration = 1;
  breakIndicator = false;
  if (field[length - decoration] == '/')
  {
    breakIndicator = true;
    ++decoration;
  }

  if (field[length - decoration] == '+')
  {
    ascending = true;
    key = field.substr (0, length - decoration);
  }
  else if (field[length - decoration] == '-')
  {
    ascending = false;
    key = field.substr (0, length - decoration);
  }
  else
  {
    ascending = true;
    key = field;
  }
}

////////////////////////////////////////////////////////////////////////////////
// Note: The reason some of these are commented out is because the ::clear
// method is not really "clear" but "clear_some".  Some members do not need to
// be initialized.  That makes this method something of a misnomer.  So be it.
//
// TODO Is this method used anywhere?
void Context::clear ()
{
  tdb2.clear ();

  // Eliminate the command objects.
  std::map <std::string, Command*>::iterator com;
  for (com = commands.begin (); com != commands.end (); ++com)
    delete com->second;

  commands.clear ();

  // Eliminate the column objects.
  std::map <std::string, Column*>::iterator col;
  for (col = columns.begin (); col != columns.end (); ++col)
    delete col->second;

  columns.clear ();
  clearMessages ();
}

////////////////////////////////////////////////////////////////////////////////
// This capability is to answer the question of 'what did I just do to generate
// this output?'.
void Context::updateXtermTitle ()
{
  if (config.getBoolean ("xterm.title") && isatty (STDOUT_FILENO))
  {
    std::string command = cli.getCommand ();
    std::string title;

    std::vector <A>::const_iterator a;
    for (a = cli._args.begin (); a != cli._args.end (); ++a)
    {
      if (a != cli._args.begin ())
        title += ' ';

      title += a->attribute ("raw");
    }

    nowide::cout << "]0;task " << command << " " << title << "";
  }
}

////////////////////////////////////////////////////////////////////////////////
// This function allows a clean output if the command is a helper subcommand.
void Context::updateVerbosity ()
{
  std::string command = cli.getCommand ();
  if (command != "" &&
      command[0] == '_')
  {
    verbosity.clear ();
    verbosity.push_back ("nothing");
  }
}

////////////////////////////////////////////////////////////////////////////////
void Context::loadAliases ()
{
  std::map <std::string, std::string>::iterator i;
  for (i = config.begin (); i != config.end (); ++i)
    if (i->first.substr (0, 6) == "alias.")
      cli.alias (i->first.substr (6), i->second);
}

////////////////////////////////////////////////////////////////////////////////
// Using the general rc.debug setting automaticalls sets debug.tls, debug.hooks
// and debug.parser, unless they already have values, which by default they do
// not.
void Context::propagateDebug ()
{
  if (config.getBoolean ("debug"))
  {
    if (! config.has ("debug.tls"))
      config.set ("debug.tls", 2);

    if (! config.has ("debug.hooks"))
      config.set ("debug.hooks", 1);

    if (! config.has ("debug.parser"))
      config.set ("debug.parser", 1);
  }
  else
  {
    if ((config.has ("debug.hooks")  && config.getInteger ("debug.hooks")) ||
        (config.has ("debug.parser") && config.getInteger ("debug.parser")) )
      config.set ("debug", true);
  }
}

////////////////////////////////////////////////////////////////////////////////
// No duplicates.
void Context::header (const std::string& input)
{
  if (input.length () &&
      std::find (headers.begin (), headers.end (), input) == headers.end ())
    headers.push_back (input);
}

////////////////////////////////////////////////////////////////////////////////
// No duplicates.
void Context::footnote (const std::string& input)
{
  if (input.length () &&
      std::find (footnotes.begin (), footnotes.end (), input) == footnotes.end ())
    footnotes.push_back (input);
}

////////////////////////////////////////////////////////////////////////////////
// No duplicates.
void Context::error (const std::string& input)
{
  if (input.length () &&
      std::find (errors.begin (), errors.end (), input) == errors.end ())
    errors.push_back (input);
}

////////////////////////////////////////////////////////////////////////////////
void Context::debug (const std::string& input)
{
  if (input.length ())
    debugMessages.push_back (input);
}

////////////////////////////////////////////////////////////////////////////////
void Context::clearMessages ()
{
  headers.clear ();
  footnotes.clear ();
  errors.clear ();
  debugMessages.clear ();
}

////////////////////////////////////////////////////////////////////////////////
