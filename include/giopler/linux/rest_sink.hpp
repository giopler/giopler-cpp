// Copyright (c) 2023 Giopler
// Creative Commons Attribution No Derivatives 4.0 International license
// https://creativecommons.org/licenses/by-nd/4.0
// SPDX-License-Identifier: CC-BY-ND-4.0
//
// Share         — Copy and redistribute the material in any medium or format for any purpose, even commercially.
// NoDerivatives — If you remix, transform, or build upon the material, you may not distribute the modified material.
// Attribution   — You must give appropriate credit, provide a link to the license, and indicate if changes were made.
//                 You may do so in any reasonable manner, but not in any way that suggests the licensor endorses you or your use.

#pragma once
#ifndef GIOPLER_LINUX_REST_SINK_HPP
#define GIOPLER_LINUX_REST_SINK_HPP

#if __cplusplus < 202002L
#error Support for C++20 or newer is required to use this library.
#endif

#include <cstdlib>
#include <mutex>
#include <regex>
#include <string>
#include <string_view>
#include <vector>

#include <openssl/err.h>
#include <openssl/bio.h>
#include <openssl/ssl.h>
using namespace std::literals;

#define ZLIB_CONST
#include "zlib.h"

// -----------------------------------------------------------------------------
// https://stackoverflow.com/questions/22077802/simple-c-example-of-doing-an-http-post-and-consuming-the-response
#include <stdio.h>      /* printf, sprintf */
#include <stdlib.h>     /* exit */
#include <unistd.h>     /* read, write, close */
#include <string.h>     /* memcpy, memset */
#include <sys/socket.h> /* socket, connect */
#include <netinet/in.h> /* struct sockaddr_in, struct sockaddr */
#include <netdb.h>      /* struct hostent, gethostbyname */

// -----------------------------------------------------------------------------
namespace giopler::sink {

// -----------------------------------------------------------------------------
// https://quuxplusone.github.io/blog/2020/01/28/openssl-part-5/
// https://wiki.openssl.org/index.php/SSL/TLS_Client
// https://www.sslproxies.org/
// https://www.ipify.org/
// https://wiki.mozilla.org/Security/Server_Side_TLS
// https://ssl-config.mozilla.org/

// -----------------------------------------------------------------------------
// OpenSSL 1.1.1 removed the need for external locking of its internal data structures
// https://www.openssl.org/blog/blog/2017/02/21/threads/
// https://github.com/openssl/openssl/issues/2165
// This means we are safe to make API calls as long as the data objects are not
// shared between user threads. This matches the general design of this library.

// -----------------------------------------------------------------------------
// there is only one of these objects created per host/path
// we use a mutex to serialize the HTTP writes
class Rest
{
 public:
  /// open a persistent connection to the server
  // example host: "www.domain.com"
  // port is either a string representation of the port number
  // or one of: http, telnet, socks, https, ssl, ftp, or gopher
  explicit Rest()
  {
    const char* proxy_host  = std::getenv("GIOPLER_PROXY_HOST");
    _proxy_host             = proxy_host ? proxy_host : "";
    _is_proxy               = (!_proxy_host.empty());
    const char* proxy_port  = std::getenv("GIOPLER_PROXY_PORT");
    _proxy_port             = proxy_port ? proxy_port : "443";

    const char* local_host  = std::getenv("GIOPLER_LOCAL");
    _is_localhost           = local_host;   // convert pointer to boolean
    _server_host            = _is_localhost ? "127.0.0.1" : "www.giopler.com";
    _server_port            = _is_localhost ? "3000" : "443";

    _json_web_token         = std::getenv("GIOPLER_TOKEN");

    //printf("Giopler Server: %s%s:%s\n", (_is_localhost ? "http://" : "https://"), _server_host.c_str(), _server_port.c_str());

    //if (!_is_localhost)   open_connection();
  }

  ~Rest() {
    close_connection();
  }

  /// post the records to the server
  void write_records(std::string_view json_body) {
    if (_is_localhost) {
      http_post(_json_web_token, _server_host, _server_port, json_body);
    } else {
      https_post(json_body);
    }
  }

 private:
  const std::regex _http_first_line_regex{"^HTTP/1\\.[01] ([[:digit:]]+) ", std::regex::optimize};
  const std::regex _http_chunked_regex{"transfer-encoding: chunked", std::regex::optimize|std::regex::icase};
  const std::regex _http_end_chunk_regex{"0\r\n\r\n", std::regex::optimize};
  std::string _proxy_host;
  std::string _proxy_port;
  std::string _server_host;
  std::string _server_port;
  std::string _json_web_token;
  bool _is_proxy = false;
  bool _is_localhost = false;

  static constexpr std::size_t RESULT_BUFFER_SIZE = 1024;
  SSL_CTX* _ssl_ctx = nullptr;
  BIO* _bio = nullptr;   // OpenSSL I/O stream abstraction (similar to FILE*)
  SSL* _ssl = nullptr;   // no need to free
  char _result_buffer[RESULT_BUFFER_SIZE] = "";

  /// open a secure and persistent connection to the server
  void open_connection()
  {
    std::this_thread::yield();                   // give rest of app a chance to initialize

    const SSL_METHOD* ssl_method = TLS_client_method();
    ERR_print_errors_fp(stderr);
    assert(ssl_method);

    _ssl_ctx = SSL_CTX_new(ssl_method);
    ERR_print_errors_fp(stderr);
    assert(_ssl_ctx);

    const int verify_paths_status = SSL_CTX_set_default_verify_paths(_ssl_ctx);
    ERR_print_errors_fp(stderr);
    assert(verify_paths_status == 1);

    SSL_CTX_set_verify_depth(_ssl_ctx, 5);
    ERR_print_errors_fp(stderr);

    const int min_proto_status = SSL_CTX_set_min_proto_version(_ssl_ctx, TLS1_VERSION);
    ERR_print_errors_fp(stderr);
    assert(min_proto_status);

    SSL_CTX_set_mode(_ssl_ctx, SSL_MODE_AUTO_RETRY);   // do not bother app with requests to retry
    ERR_print_errors_fp(stderr);

    _bio = BIO_new_ssl_connect(_ssl_ctx);
    ERR_print_errors_fp(stderr);
    assert(_bio);

    //BIO_set_callback_ex(_bio, BIO_debug_callback_ex);   // ********************************

    const long bio_get_ssl_status = BIO_get_ssl(_bio, &_ssl);
    ERR_print_errors_fp(stderr);
    assert(bio_get_ssl_status);

    SSL_clear_options(_ssl, SSL_OP_NO_COMPRESSION);   // enabled by default

    const int tlsext_host_status = SSL_set_tlsext_host_name(_ssl, _server_host.c_str());
    ERR_print_errors_fp(stderr);
    assert(tlsext_host_status);

    SSL_set_mode(_ssl, SSL_MODE_AUTO_RETRY);   // do not bother app with requests to retry

    const long set_conn_hostname_status = BIO_set_conn_hostname(_bio, _server_host.c_str());
    ERR_print_errors_fp(stderr);
    assert(set_conn_hostname_status == 1);

    const long set_conn_port_status = BIO_set_conn_port(_bio, _server_port.c_str());
    ERR_print_errors_fp(stderr);
    assert(set_conn_port_status == 1);

    int bio_conn_status = BIO_do_connect(_bio);
    ERR_print_errors_fp(stderr);
    assert(bio_conn_status == 1);

    const long handshake_status = BIO_do_handshake(_bio);
    ERR_print_errors_fp(stderr);
    assert(handshake_status == 1);

    X509* cert = SSL_get_peer_certificate(_ssl);
    assert(cert);
    if (cert)   X509_free(cert);    // free immediately

    const long verify_status = SSL_get_verify_result(_ssl);
    ERR_print_errors_fp(stderr);
    assert(verify_status == X509_V_OK);
  }

  /// close and clean-up data for a previously open connection
  void close_connection() {
    if (_bio) {
      BIO_free_all(_bio);
      _bio = nullptr;
    }

    if (_ssl_ctx) {
      SSL_CTX_free(_ssl_ctx);
      _ssl_ctx = nullptr;
    }
  }

  bool check_reopen_connection() {
    if (SSL_get_shutdown(_ssl)) {   // SSL_SENT_SHUTDOWN or SSL_RECEIVED_SHUTDOWN
        close_connection();
        open_connection();
        return true;
    }
    return false;
  }

  /// perform a HTTP POST of a JSON payload
  // example path: "/software/htp/cics/index.html"
  // https://reqbin.com/Article/HttpPost
  // https://developer.mozilla.org/en-US/docs/Web/HTTP/Methods/POST
  // returns the JSON content if HTTP result status code >= 200 and < 300
  // https://wiki.openssl.org/index.php/Library_Initialization
  // https://wiki.openssl.org/index.php/SSL/TLS_Client
  void https_post(std::string_view json_content) {
    //check_reopen_connection();
    open_connection();

    send_request(json_content);
    std::this_thread::yield();

    //if (!check_reopen_connection()) {
      read_response();
      const int response_status = parse_response_status();
      assert(response_status == 201);

      if (is_chunked_response()) {
        read_response();          // discard zero length end chunk
      }
    //}
    close_connection();
  }

  void send_request(std::string_view json_content) {
    char headers[1024];
    const std::vector<std::uint8_t> compressed_body = compress_gzip(json_content);
    std::size_t total, sent, bytes;

    sprintf(headers,
        "POST /api/v1/post_event HTTP/1.1\r\n"
        "Host: %s:%s\r\n"
        "Connection: keep-alive\r\n"
        "User-Agent: Giopler/1.0\r\n"
        "Authorization: Bearer %s\r\n"
        "Connection: keep-alive\r\n"
        "Accept: application/json\r\n"
        "Accept-Encoding: identity\r\n"
        "Content-Type: application/octet-stream\r\n"
        "Content-Length: %lu\r\n\r\n",
        _server_host.c_str(), _server_port.c_str(), _json_web_token.c_str(), compressed_body.size());

    // send the headers
    total = strlen(headers);
    sent = 0;
    do {
        const int write_status = BIO_write_ex(_bio,headers+sent,total-sent, &bytes);
        ERR_print_errors_fp(stderr);
        if (write_status < 0)
            http_error("ERROR writing headers via OpenSSL", _bio);
        else   // if (bytes >= 0)
            sent += bytes;
    } while (sent < total);

    // send the body
    total = compressed_body.size();
    sent = 0;
    do {
        const int write_status = BIO_write_ex(_bio,compressed_body.data()+sent,total-sent, &bytes);
        ERR_print_errors_fp(stderr);
        if (write_status < 0)
            http_error("ERROR writing body via OpenSSL", _bio);
        else    // if (bytes >= 0)
            sent += bytes;
    } while (sent < total);

    const int flush_status = BIO_flush(_bio);
    assert(flush_status == 1);
  }

  // we use HTTP 1.1 streaming for better performance
  // but that complicates our job, since now the end of a response is harder to detect
  // there is nothing in the response we really need to look at
  // so we punt and just read some bytes and then discard them
  void read_response() {
      std::size_t bytes_read = 0;
      const int read_status = BIO_read_ex(_bio, _result_buffer, RESULT_BUFFER_SIZE, &bytes_read);
      ERR_print_errors_fp(stderr);
      assert(read_status > 0);
      _result_buffer[bytes_read] = '\0';
  }

  // HTTP method names are case-sensitive
  // HTTP header names are case-insensitive
  int parse_response_status() {
      std::cmatch match;
      std::regex_search(_result_buffer, match, _http_first_line_regex);
      assert(match.ready());
      const int response_status = (match.size() == 2) ? std::stoi(match[1]) : 0;
      return response_status;
  }

  // https://en.wikipedia.org/wiki/Chunked_transfer_encoding
  bool is_chunked_response() {
      std::cmatch match_chunked;
      std::regex_search(_result_buffer, match_chunked, _http_chunked_regex);
      assert(match_chunked.ready());

      if (!match_chunked.empty()) {
        std::cmatch match_end_chunk;
        std::regex_search(_result_buffer, match_end_chunk, _http_end_chunk_regex);
        assert(match_end_chunk.ready());
        return match_end_chunk.empty();   // chunked data, but have not seen the end chunk
      }

      return false;   // not chunked
  }

  // -----------------------------------------------------------------------------
  // initialize the gzip data structure
  static void init_gzip(std::string_view input, std::uint8_t* output, int max_output_size, z_stream& zstream) {
    zstream.zalloc     = Z_NULL;
    zstream.zfree      = Z_NULL;
    zstream.opaque     = Z_NULL;
    zstream.avail_in   = static_cast<uint32_t>(input.size());
    zstream.next_in    = reinterpret_cast<const uint8_t*>(input.data());
    zstream.avail_out  = static_cast<uint32_t>(max_output_size);
    zstream.next_out   = reinterpret_cast<uint8_t*>(output);

    // Hard to believe they don't have a macro for gzip encoding. "Add 16" is the best thing zlib can do:
    // "Add 16 to windowBits to write a simple gzip header and trailer around the compressed data instead of a zlib wrapper"
    const int status = deflateInit2(&zstream, Z_DEFAULT_COMPRESSION, Z_DEFLATED, 15 | 16, 9, Z_DEFAULT_STRATEGY);
    assert(status == Z_OK);
  }

  // -----------------------------------------------------------------------------
  // return the maximum possible size of the compressed data
  static std::size_t max_compress_size_gzip(std::string_view input) {
    z_stream zstream;
    init_gzip(input, nullptr, 0, zstream);
    return deflateBound(&zstream, input.size());
  }

  // -----------------------------------------------------------------------------
  // compress the input buffer using gzip data compression
  static std::vector<std::uint8_t> compress_gzip(std::string_view input)
  {
    const std::size_t max_output_size = max_compress_size_gzip(input);
    std::vector<std::uint8_t> output(max_output_size);

    z_stream zstream;
    init_gzip(input, output.data(), static_cast<int>(max_output_size), zstream);
    const int deflate_status = deflate(&zstream, Z_FINISH);
    assert(deflate_status == Z_STREAM_END);
    output.resize(zstream.total_out);
    const int deflate_end_status = deflateEnd(&zstream);
    assert(deflate_end_status == Z_OK);
    return output;
  }

  // -----------------------------------------------------------------------------
  static void http_error(const char *msg, BIO* bio = nullptr) {
    ERR_print_errors_fp(stderr);
    perror(msg);
    exit(0);
  }

  // ---------------------------------------------------------------------------
  /// this is a stand-alone function to test sending events to an http connection (typically localhost)
  // uses the Linux socket API
  // closes the connection after each POST message
  // https://stackoverflow.com/questions/22077802/simple-c-example-of-doing-an-http-post-and-consuming-the-response
  static void http_post(std::string token, std::string host, std::string port_str, std::string_view json_body)
  {
      uint16_t port = std::atoi(port_str.c_str());
      struct hostent *server;
      struct sockaddr_in serv_addr;
      int sockfd, bytes, sent, received, total;
      char headers[4096], response[4096];
      const std::vector<std::uint8_t> compressed_body = compress_gzip(json_body);

      sprintf(headers,
        "POST /api/v1/post_event HTTP/1.1\r\n"
        "Host: %s:%hd\r\n"
        "Connection: close\r\n"
        "User-Agent: Giopler/1.0\r\n"
        "Authorization: Bearer %s\r\n"
        "Accept: application/json\r\n"
        "Accept-Encoding: identity\r\n"
        "Content-Type: application/octet-stream\r\n"
        "Content-Length: %lu\r\n\r\n",
        host.c_str(), port, token.c_str(), compressed_body.size());

      // printf("HTTP Request Headers:\n%s\n", headers);
      // printf("HTTP Request Body:\n%s\n", json_body.data());

      // create the socket
      sockfd = socket(AF_INET, SOCK_STREAM, 0);
      if (sockfd < 0)   http_error("ERROR opening socket");

      // lookup the ip address
      server = gethostbyname(host.c_str());
      if (server == nullptr)   http_error("ERROR no such host");

      // fill in the structure
      memset(&serv_addr,0,sizeof(serv_addr));
      serv_addr.sin_family = AF_INET;
      serv_addr.sin_port   = htons(port);
      memcpy(&serv_addr.sin_addr.s_addr,server->h_addr,server->h_length);

      // connect the socket
      if (connect(sockfd,(struct sockaddr *)&serv_addr,sizeof(serv_addr)) < 0)
          http_error("ERROR connecting");

      // send the headers
      total = (int)strlen(headers);
      sent = 0;
      do {
          bytes = (int)write(sockfd,headers+sent,total-sent);
          if (bytes < 0)
              http_error("ERROR writing headers to socket");
          if (bytes == 0)
              break;
          if (bytes > 0)
              sent += bytes;
      } while (sent < total);

      // send the body
      total = (int)compressed_body.size();
      sent = 0;
      do {
          bytes = (int)write(sockfd,compressed_body.data()+sent,total-sent);
          if (bytes < 0)
              http_error("ERROR writing body to socket");
          if (bytes == 0)
              break;
          if (bytes > 0)
              sent += bytes;
      } while (sent < total);

      // receive the response
      //memset(response,0,sizeof(response));   // no need to clear the input buffer
      total = sizeof(response);
      received = 0;
      do {
          bytes = (int)read(sockfd,response+received,total-received);
          if (bytes < 0)
              http_error("ERROR reading response from socket");
          if (bytes == 0)
              break;
          received += bytes;
      } while (received < total);

      // If the number of received bytes is the total size of the
      // array, then we have run out of space to store the response.
      // This means it hasn't all arrived yet - so that's a bad thing.
      if (received == total)
          http_error("ERROR storing complete response from socket");

      if (response[9] != '2')
          http_error(response);

      // close the socket
      close(sockfd);

      // process response
      // printf("HTTP Response:\n%s\n",response);
  }
};

// -----------------------------------------------------------------------------
}   // namespace giopler::sink

// -----------------------------------------------------------------------------
#endif // defined GIOPLER_LINUX_REST_SINK_HPP
