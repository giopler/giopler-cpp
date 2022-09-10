// Copyright (c) 2022. Carlos Reyes
// This code is licensed under the permissive MIT License (MIT).
// SPDX-License-Identifier: MIT
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.
//

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

// -----------------------------------------------------------------------------
// https://www.lucidchart.com/techblog/2019/12/06/json-compression-alternative-binary-formats-and-compression-methods/
// Summary: sending plain JSON data compressed with Brotli(10) works really well
// Brotli works well for compressing JSON (textual) data and is supported by web browsers
// default quality is 11
#if defined(GIOPLER_HAVE_BROTLI)
#include <brotli/encode.h>
std::string compress_json(std::string_view json) {
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
#endif

// -----------------------------------------------------------------------------
namespace giopler::sink {

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
  Rest(std::string customer_token, std::string_view host, std::string_view port)
  : _customer_token{std::move(customer_token)},
    _host{host},
    _port{port}
  {
    _fields = get_sorted_record_keys();
    open_connection();
  }

  ~Rest() override {
    close_connection();
  }

  /// add a new JSON format data record sink
  static void add_sink(std::string_view customer_token)
  {
    g_sink_manager.add_sink(
        std::make_unique<Rest>(std::string(customer_token), SERVER_HOST, SERVER_PORT));
  }

 protected:
  /// post the record to the server
  // a thread gets created for each record we are trying to write
  // there is only one Rest sink object created
  // use a mutex to serialize the writes
  // this maximizes the chances that the HTTP connection will stay open
  bool write_record(std::shared_ptr<Record> record) override {
    if (is_record_filtered(*record)) {
      return false;
    }

    const std::lock_guard<std::mutex> lock{_mutex};
    post(SERVER_PATH, record_to_json(_fields, record));
    return true;   // record was not filtered and it was written out
  }

  void flush() override { }

 private:
  constexpr static inline std::string_view SERVER_HOST{"localhost"sv};
  constexpr static inline std::string_view SERVER_PATH{"/api/v1/log_event"sv};
  constexpr static inline std::string_view SERVER_PORT{"8100"sv};
  std::mutex _mutex;
  std::vector<std::string> _fields;
  constexpr static std::size_t RESULT_BUFFER_SIZE = 2048;
  SSL_CTX* _ssl_ctx;
  const SSL_METHOD* _ssl_method;
  BIO* _bio;   // OpenSSL I/O stream abstraction (similar to FILE*)
  SSL* _ssl;
  std::string _customer_token;
  std::string _host;
  std::string _path;
  std::string _port;
  std::string _json_web_token;
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

      BIO_set_callback(_bio, BIO_debug_callback);   // ********************************

      const long bio_get_ssl_status = BIO_get_ssl(_bio, &_ssl);
      assert(bio_get_ssl_status);

      const int tlsext_host_status = SSL_set_tlsext_host_name(_ssl, _host.c_str());
      assert(tlsext_host_status);

      SSL_set_mode(_ssl, SSL_MODE_AUTO_RETRY);

      const long set_conn_port_status = BIO_set_conn_port(_bio, _port.c_str());
      assert(set_conn_port_status == 1);

      const long set_conn_hostname_status = BIO_set_conn_hostname(_bio, _host.c_str());
      assert(set_conn_hostname_status == 1);

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
  std::string_view post(std::string_view path, std::string_view json_content) {
    if (SSL_get_shutdown(_ssl) == SSL_RECEIVED_SHUTDOWN) {
        close_connection();
        open_connection();
    }

    send_request("POST", std::string{path}, json_content);
    read_response();
    parse_response_status();
    assert(_response_status == 200);
    parse_response_contents();
    return _result_contents;
  }

  void send_request(std::string http_method, std::string path, std::string_view json_content) {
      const int bytes_written = BIO_printf(_bio,
          "%s %s HTTP/1.1\r\n"
          "Host: %s\r\n"
          "Connection: keep-alive\r\n"
          "User-Agent: Gioppler/1.0\r\n"
          "Authorization: Bearer %s\r\n"
          "Accept: application/json\r\n"
          "Content-Type: application/json\r\n"
          "Content-Length: %lu\r\n\r\n",
          http_method.c_str(), path.c_str(), _host.c_str(), _json_web_token.c_str(), json_content.size());
      assert(bytes_written > 0);

      const int write_status = BIO_write(_bio, json_content.data(), json_content.size());
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
      BIO_free_all(_bio);
      SSL_CTX_free(_ssl_ctx);
  }

  /// Skip writing this record due to filter conditions for sink?
  bool is_record_filtered(const Record& record) {
    return false;
  }
};

// -----------------------------------------------------------------------------
}   // namespace giopler::sink

// -----------------------------------------------------------------------------
#endif // defined GIOPLER_LINUX_REST_SINK_HPP
