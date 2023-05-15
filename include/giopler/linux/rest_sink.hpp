// Copyright (c) 2023 Giopler
// Creative Commons Attribution No Derivatives 4.0 International license
// https://creativecommons.org/licenses/by-nd/4.0
// SPDX-License-Identifier: CC-BY-ND-4.0
//
// Share         — copy and redistribute the material in any medium or format for any purpose, even commercially.
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

#include <brotli/encode.h>

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
class Rest : public Sink
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

    const char* server_host = std::getenv("GIOPLER_HOST");
    _server_host            = server_host ? server_host : "www.giopler.com";
    _is_localhost           = (_server_host == "localhost");
    const char* server_port = std::getenv("GIOPLER_PORT");
    _server_port            = server_port ? server_port : (_is_localhost ? "80" : "443");

    _json_web_token         = std::getenv("GIOPLER_TOKEN");

    printf("Host: %s:%s\nToken: %s\n\n", _server_host.c_str(), _server_port.c_str(), _json_web_token.c_str());

    if (!_is_localhost)   open_connection();
  }

  ~Rest() override {
    close_connection();
  }

  /// add a new JSON format data record sink
  static void add_sink()
  {
    g_sink_manager.add_sink(std::make_unique<Rest>());
  }

 protected:
  /// post the record to the server
  // a thread gets created for each record we are trying to write
  // there is only one Rest sink object created
  // use a mutex to serialize the writes
  // this maximizes the chances that the HTTP connection will stay open
  bool write_record(std::shared_ptr<Record> record) override {
    const std::string json_body{record_to_json(record)};   // don't need the lock for this
    const std::lock_guard<std::mutex> lock{_mutex};

    if (_is_localhost) {
      http_post(_json_web_token, _server_host, _server_port, json_body);
    } else {
      post(json_body);
    }

    return true;   // record was not filtered and it was written out (vestigial)
  }

  void flush() override { }

 private:
  std::string _proxy_host;
  std::string _proxy_port;
  std::string _server_host;
  std::string _server_port;
  std::string _json_web_token;
  bool _is_proxy;
  bool _is_localhost;

  std::mutex _mutex;
  constexpr static std::size_t RESULT_BUFFER_SIZE = 2048;
  SSL_CTX* _ssl_ctx = nullptr;
  const SSL_METHOD* _ssl_method;
  BIO* _bio = nullptr;   // OpenSSL I/O stream abstraction (similar to FILE*)
  SSL* _ssl = nullptr;
  int _response_status;
  const char* _result_contents;
  char result_buffer[RESULT_BUFFER_SIZE];

  /// open a secure and persistent connection to the server
  void open_connection()
  {
      _ssl_method = TLS_client_method();
      assert(_ssl_method);

      _ssl_ctx = SSL_CTX_new(_ssl_method);
      assert(_ssl_ctx);

      const int verify_paths_status = SSL_CTX_set_default_verify_paths(_ssl_ctx);
      assert(verify_paths_status == 1);

      SSL_CTX_set_verify_depth(_ssl_ctx, 4);

      const int min_proto_status = SSL_CTX_set_min_proto_version(_ssl_ctx, TLS1_VERSION);
      assert(min_proto_status);

      SSL_CTX_set_options(_ssl_ctx, SSL_OP_NO_COMPRESSION);

      _bio = BIO_new_ssl_connect(_ssl_ctx);
      assert(_bio);

      //BIO_set_callback(_bio, BIO_debug_callback);   // ********************************

      const long bio_get_ssl_status = BIO_get_ssl(_bio, &_ssl);
      assert(bio_get_ssl_status);

      const int tlsext_host_status = SSL_set_tlsext_host_name(_ssl, _server_host.c_str());
      assert(tlsext_host_status);

      SSL_set_mode(_ssl, SSL_MODE_AUTO_RETRY);

      const long set_conn_hostname_status = BIO_set_conn_hostname(_bio, _server_host.c_str());
      assert(set_conn_hostname_status == 1);

      const long set_conn_port_status = BIO_set_conn_port(_bio, _server_port.c_str());
      assert(set_conn_port_status == 1);

      const int bio_conn_status = BIO_do_connect(_bio);
      ERR_print_errors(_bio);
      assert(bio_conn_status == 1);
  }

  /// perform a HTTP POST of a JSON payload
  // example path: "/software/htp/cics/index.html"
  // https://reqbin.com/Article/HttpPost
  // https://developer.mozilla.org/en-US/docs/Web/HTTP/Methods/POST
  // returns the JSON content if HTTP result status code >= 200 and < 300
  // https://wiki.openssl.org/index.php/Library_Initialization
  // https://wiki.openssl.org/index.php/SSL/TLS_Client
  std::string_view post(std::string_view json_content) {
    if (SSL_get_shutdown(_ssl) == SSL_RECEIVED_SHUTDOWN) {
        close_connection();
        open_connection();
    }

    send_request(json_content);
    read_response();
    parse_response_status();
    assert(_response_status == 200);
    parse_response_contents();
    return _result_contents;
  }

  void send_request(std::string_view json_content) {
      const std::string brotli_body = compress_json(json_content);

      const int bytes_written = BIO_printf(_bio,
          "POST /api/v1/post_event HTTP/1.1\r\n"
          "Host: %s:%s\r\n"
          "Connection: keep-alive\r\n"
          "User-Agent: Giopler/1.0\r\n"
          "Authorization: Bearer %s\r\n"
          "Accept: application/json\r\n"
          "Content-Encoding: br\r\n"
          "Content-Type: application/json\r\n"
          "Content-Length: %lu\r\n\r\n",
          _server_host.c_str(), _server_port.c_str(), _json_web_token.c_str(), brotli_body.size());
      assert(bytes_written > 0);

      const int write_status = BIO_write(_bio, brotli_body.data(), static_cast<int>(brotli_body.size()));
      assert(write_status > 0);
  }

  void read_response() {
      std::size_t bytes_total = 0;
      std::size_t bytes_read;
      int read_status;
      do {
          bytes_read = 0;
          read_status = BIO_read_ex(_bio, &result_buffer[bytes_total], RESULT_BUFFER_SIZE-bytes_total, &bytes_read);
          bytes_total += bytes_read;
      } while (read_status == 1 || BIO_should_retry(_bio));

      result_buffer[bytes_total] = '\0';
  }

  // HTTP method names are case sensitive
  // HTTP header names are case insensitive
  void parse_response_status() {
      static std::regex http_first_line_regex{"^HTTP/1\\.1 ([[:digit:]]+) ", std::regex::optimize};

      std::cmatch match;
      std::regex_search(result_buffer, match, http_first_line_regex);
      assert(match.ready());
      if (match.size() == 2) {
          _response_status = std::stoi(match[1]);
      }
  }

  void parse_response_contents() {
      std::string_view result{result_buffer};
      const std::size_t position = result.find("\r\n\r\n");
      assert(position != std::string_view::npos);
      _result_contents = &result_buffer[position+4];
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

  // -----------------------------------------------------------------------------
  // https://www.lucidchart.com/techblog/2019/12/06/json-compression-alternative-binary-formats-and-compression-methods/
  // Summary: sending plain JSON data compressed with Brotli(10) works really well
  // Brotli works well for compressing JSON (textual) data and is supported by web browsers
  // default quality is 11
  static std::string compress_json(std::string_view json) {
    std::string output;
    std::size_t output_size = BrotliEncoderMaxCompressedSize(json.size());
    assert(output_size);
    output.reserve(output_size);
    const BROTLI_BOOL status = BrotliEncoderCompress(BROTLI_DEFAULT_QUALITY, BROTLI_DEFAULT_WINDOW, BROTLI_MODE_TEXT,
                                                     json.size(), reinterpret_cast<const uint8_t *>(json.data()),
                                                     &output_size, reinterpret_cast<uint8_t *>(output.data()));
    assert(status == BROTLI_TRUE);
    output.resize(output_size);
    return output;
  }

  // -----------------------------------------------------------------------------
  static void http_error(const char *msg) { perror(msg); exit(0); }

  // ---------------------------------------------------------------------------
  /// this is a stand-alone function to test sending events to an http connection (typically localhost)
  // uses the Linux socket API
  // closes the connection after each POST message
  // does not bother to compress the data
  // https://stackoverflow.com/questions/22077802/simple-c-example-of-doing-an-http-post-and-consuming-the-response
  static void http_post(std::string token, std::string host, std::string port_str, std::string_view json_body)
  {
      uint16_t port = std::atoi(port_str.c_str());
      struct hostent *server;
      struct sockaddr_in serv_addr;
      int sockfd, bytes, sent, received, total;
      char headers[4096], response[4096];

      sprintf(headers,
        "POST /api/v1/post_event HTTP/1.1\r\n"
        "Host: %s:%hd\r\n"
        "Connection: close\r\n"
        "User-Agent: Giopler/1.0\r\n"
        "Authorization: Bearer %s\r\n"
        "Accept: application/json\r\n"
        "Content-Type: application/json\r\n"
        "Content-Length: %lu\r\n\r\n",
        host.c_str(), port, token.c_str(), json_body.length());

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
          sent += bytes;
      } while (sent < total);

      // send the body
      total = (int)json_body.length();
      sent = 0;
      do {
          bytes = (int)write(sockfd,json_body.data()+sent,total-sent);
          if (bytes < 0)
              http_error("ERROR writing body to socket");
          if (bytes == 0)
              break;
          sent += bytes;
      } while (sent < total);

      // receive the response
      //memset(response,0,sizeof(response));   // no need to clear the input buffer
      total = sizeof(response)-1;
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
