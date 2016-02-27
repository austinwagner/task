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

#ifdef WINDOWS
#include <winsock2.h>
#include <ws2tcpip.h>
#define SHUT_RDWR SD_BOTH
#else
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#endif

#if (defined OPENBSD || defined SOLARIS || defined NETBSD)
#include <errno.h>
#elseif !defined(WINDOWS)
#include <sys/errno.h>
#endif

#include <sys/types.h>
#include <TLSClient.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <openssl/x509.h>
#include <openssl/bio.h>
#include <text.h>
#include <i18n.h>
#include <nowide/iostream.hpp>
#include <nowide/convert.hpp>

#define MAX_BUF 16384

////////////////////////////////////////////////////////////////////////////////
TLSClient::TLSClient ()
: _ca ("")
, _cert ("")
, _key ("")
, _host ("")
, _port ("")
, _context (nullptr)
, _conn(nullptr)
, _limit (0)
, _debug (false)
, _trust(strict)
{
}

////////////////////////////////////////////////////////////////////////////////
TLSClient::~TLSClient ()
{
  if (_conn) BIO_free_all(_conn);
  if (_context) SSL_CTX_free(_context);
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

  SSL_load_error_strings();
  ERR_load_BIO_strings();
  OpenSSL_add_all_algorithms();

  _context = SSL_CTX_new(TLSv1_2_client_method());
  // TODO: Use trust level

  SSL_CTX_use_PrivateKey_file(_context, _key.c_str(), SSL_FILETYPE_PEM);
  SSL_CTX_use_certificate_file(_context, _cert.c_str(), SSL_FILETYPE_PEM);
  SSL_CTX_load_verify_locations(_context, _ca.c_str(), nullptr);
}

////////////////////////////////////////////////////////////////////////////////
void TLSClient::connect (const std::string& host, const std::string& port)
{
  _conn = BIO_new_ssl_connect(_context);
  SSL * ssl;
  BIO_get_ssl(_conn, &ssl);
  SSL_set_mode(ssl, SSL_MODE_AUTO_RETRY);
  BIO_set_conn_hostname(_conn, (host + ":" + port).c_str());

  if (BIO_do_connect(_conn) <= 0)
  {
    // TODO: Improve error message
    throw std::runtime_error("Connection failed");
  }

  if (SSL_get_verify_result(ssl) != X509_V_OK)
  {
    // TODO: Improve error message
    throw std::runtime_error("Certificate validation failed");
  }
}

////////////////////////////////////////////////////////////////////////////////
void TLSClient::bye ()
{
  BIO_free_all(_conn);
  _conn = nullptr;
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

  while (true)
  {
    if (BIO_write(_conn, static_cast<const void*>(packet.c_str()), packet.length()))
    {
      break;
    }

    if (!BIO_should_retry(_conn))
    {
      // TODO: Improve error message
      throw std::runtime_error("Failed to send data");
    }
  }
}

////////////////////////////////////////////////////////////////////////////////
void TLSClient::recv (std::string& data)
{
  data = "";          // No appending of data.
  int received = 0;

  // Get the encoded length.
  unsigned char header[4] = {0};
  do
  {
    received = BIO_read(_conn, header, 4);
  }
  while (received > 0 && BIO_should_retry(_conn));

  int total = received;

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
      received = BIO_read(_conn, buffer, MAX_BUF - 1);
    }
    while (received > 0 && BIO_should_retry(_conn));

    // TODO: Improve errors
    // Other end closed the connection.
    if (received == 0)
    {
      if (_debug)
        nowide::cout << "c: INFO Peer has closed the TLS connection\n";
      break;
    }

    // Something happened.
    if (received < 0)
    {
      throw std::runtime_error("Failed to read from host");
    }

    buffer [received] = '\0';
    data += buffer;
    total += received;

    // Stop at defined limit.
    if (_limit && total > _limit)
      break;
  }
  while (received > 0 && total < (int) expected);

  if (_debug)
    nowide::cout << "c: INFO Receiving 'XXXX"
              << data.c_str ()
              << "' (" << total << " bytes)"
              << std::endl;
}

////////////////////////////////////////////////////////////////////////////////
#ifdef WINDOWS
namespace {
  struct initialize {
      initialize() {
        WSADATA wsa_data;
        if (WSAStartup(MAKEWORD(2, 0), &wsa_data) != 0) {
          // Purposely avoiding nowide here
          fprintf(stderr, "FATAL: WSAStartup failed with code %d\n", WSAGetLastError());
          exit(100);
        }
      }
  } inst;
}
#endif
