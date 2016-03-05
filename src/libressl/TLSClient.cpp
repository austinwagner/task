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
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include <TLSClient.h>
#include <nowide/iostream.hpp>
#include <sstream>

#define MAX_BUF 16384

TLSConfig::TLSConfig() :
  _conf(tls_config_new())
{
}

TLSConfig::~TLSConfig()
{
  tls_config_free(_conf);
}

void TLSConfig::setKeyFile(const std::string& file)
{
  if (tls_config_set_key_file(_conf, file.c_str()) < 0)
  {
    throw std::runtime_error("Failed to set key file.");
  }
}

void TLSConfig::setCaFile(const std::string& file)
{
  if (tls_config_set_ca_file(_conf, file.c_str()) < 0)
  {
    throw std::runtime_error("Failed to set CA file.");
  }
}

void TLSConfig::setCertFile(const std::string& file)
{
  if (tls_config_set_cert_file(_conf, file.c_str()) < 0)
  {
    throw std::runtime_error("Failed to set certificate file.");
  }
}

void TLSConfig::setCiphers(const std::string& ciphers)
{
  if (tls_config_set_ciphers(_conf, ciphers.c_str()) < 0)
  {
    throw std::runtime_error("Failed to set ciphers.");
  }
}

void TLSConfig::configure(tls* context)
{
  if (tls_configure(context, _conf) < 0)
  {
    throw std::runtime_error("Failed to configure TLS context.");
  }
}

void TLSConfig::verify()
{
  tls_config_verify(_conf);
}

void TLSConfig::noVerifyCert()
{
  tls_config_insecure_noverifycert(_conf);
}

void TLSConfig::noVerifyHost()
{
  tls_config_insecure_noverifyname(_conf);
}

TLSException::TLSException(tls* context)
{
  std::ostringstream oss;
  oss << "TLS error: " << tls_error(context);
  _what = oss.str();
}

const char * TLSException::what() const
{
  return _what.c_str();
}


////////////////////////////////////////////////////////////////////////////////
TLSClient::TLSClient ()
: _ca ("")
, _cert ("")
, _key ("")
, _host ("")
, _port ("")
, _context (tls_client())
, _limit (0)
, _debug (false)
, _trust(strict)
{
}

////////////////////////////////////////////////////////////////////////////////
TLSClient::~TLSClient ()
{
  if (_context) tls_free(_context);
}

////////////////////////////////////////////////////////////////////////////////
void TLSClient::limit (int max)
{
  _limit = max;
}

////////////////////////////////////////////////////////////////////////////////
// Calling this method results in all subsequent socket traffic being sent to
// nowide::cout, labelled with 'c: ...'.
void TLSClient::debug (int level)
{
  if (level)
    _debug = true;
}

////////////////////////////////////////////////////////////////////////////////
void TLSClient::trust (const enum trust_level value)
{
  _trust = value;

  if (_trust == allow_all)
  {
    _conf.noVerifyCert();
    _conf.noVerifyCert();
    _conf.configure(_context);
  }
  else if (_trust == ignore_hostname)
  {
    _conf.verify();
    _conf.noVerifyHost();
  }
  else
  {
    _conf.verify();
  }

  _conf.configure(_context);

  if (_debug)
  {
    if (_trust == allow_all)
      nowide::cout << "c: INFO Server certificate will be trusted automatically.\n";
    else if (_trust == ignore_hostname)
      nowide::cout << "c: INFO Server certificate will be verified but hostname ignored.\n";
    else
      nowide::cout << "c: INFO Server certificate will be verified.\n";
  }
}

////////////////////////////////////////////////////////////////////////////////
void TLSClient::ciphers (const std::string& cipher_list)
{
  _ciphers = cipher_list;
}

////////////////////////////////////////////////////////////////////////////////
void TLSClient::init (
  const std::string& ca,
  const std::string& cert,
  const std::string& key)
{
  _ca   = ca;
  _cert = cert;
  _key  = key;

  tls_init();
  _conf.setKeyFile(_key);
  _conf.setCaFile(_ca);
  _conf.setCertFile(_cert);
  
  if (!_ciphers.empty()) 
  {
    _conf.setCiphers(_ciphers);
  }

  _conf.configure(_context);
}

////////////////////////////////////////////////////////////////////////////////
void TLSClient::connect (const std::string& host, const std::string& port)
{
  tls_connect(_context, host.c_str(), port.c_str());
}

////////////////////////////////////////////////////////////////////////////////
void TLSClient::bye ()
{
  tls_close(_context);
}

////////////////////////////////////////////////////////////////////////////////
void TLSClient::send (const std::string& data)
{
  std::string packet = "XXXX" + data;

  // Encode the length.
  unsigned long l = packet.length ();
  packet[0] = l >>24;
  packet[1] = l >>16;
  packet[2] = l >>8;
  packet[3] = l;

  const char * buffer = packet.data();

  while (l > 0) {
    ssize_t ret = tls_write(_context, buffer, l);
    if (ret == TLS_WANT_POLLIN || ret == TLS_WANT_POLLOUT) {
      continue;
    }

    if (ret < 0) {
      throw TLSException(_context);
    }

    buffer += ret;
    l -= ret;
  }
}

////////////////////////////////////////////////////////////////////////////////
void TLSClient::recv (std::string& data)
{
  data = "";          // No appending of data.
  ssize_t received = 0;

  // Get the encoded length.
  unsigned char header[4] = {0};
  do
  {
    received = tls_read(_context, header + received, 4 - received);
    if (received < 0 && received != TLS_WANT_POLLIN && received != TLS_WANT_POLLOUT)
    {
      throw TLSException(_context);
    }
  } while ((header + received) < (header + 4));

  int total = 4;

  // Decode the length.
  unsigned long expected = (header[0]<<24) |
                           (header[1]<<16) |
                           (header[2]<<8) |
                            header[3];
  if (_debug)
    nowide::cout << "c: INFO expecting " << expected << " bytes.\n";

  // TODO This would be a good place to assert 'expected < _limit'.

  // Arbitrary buffer size.
  char buffer[MAX_BUF];

  // Keep reading until no more data.  Concatenate chunks of data if a) the
  // read was interrupted by a signal, and b) if there is more data than
  // fits in the buffer.
  do
  {
    do
    {
      received = tls_read(_context, buffer, MAX_BUF - 1);
      if (received < 0 && received != TLS_WANT_POLLIN && received != TLS_WANT_POLLOUT)
      {
        throw TLSException(_context);
      }
    }
    while (received < 0);

    buffer [received] = '\0';
    data += buffer;
    total += received;

    // Stop at defined limit.
    if (_limit && total > _limit)
      break;
  }
  while (received > 0 && total < static_cast<int>(expected));

  if (_debug)
    nowide::cout << "c: INFO Receiving 'XXXX"
              << data.c_str ()
              << "' (" << total << " bytes)"
              << std::endl;
}

namespace {
  struct initialize {
    initialize() {
      if (tls_init() < 0)
      {
        throw std::runtime_error("Failed to initialize TLS.");
      }
    }
  } inst;
}