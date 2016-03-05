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
#ifndef INCLUDED_TLSCLIENT
#define INCLUDED_TLSCLIENT

#include <string>

typedef ptrdiff_t ssize_t;
#include <tls.h>

class TLSConfig
{
public:
  TLSConfig();
  ~TLSConfig();
  void setKeyFile(const std::string& file);
  void setCaFile(const std::string& file);
  void setCertFile(const std::string& file);
  void setCiphers(const std::string& ciphers);
  void configure(tls* context);
  void verify();
  void noVerifyCert();
  void noVerifyHost();

private:
  tls_config * _conf;
};

class TLSClient
{
public:
  enum trust_level { strict, ignore_hostname, allow_all };

  TLSClient ();
  ~TLSClient ();
  void limit (int);
  void debug (int);
  void trust (const enum trust_level);
  void ciphers (const std::string&);
  void init (const std::string&, const std::string&, const std::string&);
  void connect (const std::string&, const std::string&);
  void bye ();

  void send (const std::string&);
  void recv (std::string&);

private:
  std::string                      _ca;
  std::string                      _cert;
  std::string                      _key;
  std::string                      _ciphers;
  std::string                      _host;
  std::string                      _port;
  tls *                            _context;
  int                              _limit;
  bool                             _debug;
  enum trust_level                 _trust;
  TLSConfig                        _conf;
};

class TLSException : public std::exception
{
public:
  explicit TLSException(tls*);

  const char * what() const;

  std::string _what;
};

#endif

////////////////////////////////////////////////////////////////////////////////

