#HTTP SSL Mutual Authentication

##Overview

                coro_http supports SSL /
            TLS encrypted connections using OpenSSL.This document describes how
                to configure both one -
        way(server authentication) and
    mutual(client and server authentication) SSL / TLS authentication.

                                                   ##Enabling SSL Support

                                                   After installing OpenSSL,
    enable SSL support by setting the CMake option :

```cmake option(YLT_ENABLE_SSL "Enable SSL support" ON)
```

    Or manually add the `YLT_ENABLE_SSL` macro and link to OpenSSL.

    ##One
        -
        way SSL Authentication

        One
        -
        way SSL authentication(server authentication only)
            ensures that the client verifies the server
        's certificate but doesn' t require the client to present a certificate.

        ## #Client
        -
        side Configuration

```cpp
#include <ylt/coro_http/coro_http_client.hpp>

        using namespace cinatra;

coro_http_client client;
bool init_ok =
    client.init_ssl(asio::ssl::verify_peer,  // Verify server certificate
                    "./certs/",              // Base path to certificates
                    "ca.crt",    // CA certificate for server verification
                    "127.0.0.1"  // SNI hostname
    );

if (!init_ok) {
  std::cerr << "Failed to initialize SSL" << std::endl;
  return;
}

// Make HTTPS request
auto result = client.get("https://127.0.0.1:9001/");
if (result.status == 200) {
  std::cout << "Response: " << result.resp_body << std::endl;
}
```

    ## #Server -
    side Configuration

```cpp
#include <ylt/coro_http/coro_http_server.hpp>

    using namespace cinatra;

coro_http_server server(1, 9001);

bool init_ok = server.init_ssl("./certs/server.crt",  // Server certificate
                               "./certs/server.key",  // Server private key
                               "",                    // No password
                               "",                    // No CA cert for one-way
                               false                  // Don't verify client
);

if (!init_ok) {
  std::cerr << "Failed to initialize SSL server" << std::endl;
  return;
}

server.set_http_handler<GET>("/", [](coro_http_request& req,
                                     coro_http_response& resp) {
  resp.set_status_and_content(status_type::ok, "Hello SSL!");
});

server.sync_start();
```

## Mutual SSL Authentication

Mutual SSL authentication (mTLS) requires both client and server to present valid certificates, providing strong security for sensitive applications.

### Client-side Configuration

```cpp
#include <ylt/coro_http/coro_http_client.hpp>

using namespace cinatra;

coro_http_client client;
bool init_ok =
    client.init_ssl(asio::ssl::verify_peer,  // Verify server certificate
                    "./certs/",              // Base path to certificates
                    "ca.crt",      // CA certificate for server verification
                    "client.crt",  // Client certificate
                    "client.key",  // Client private key
                    "127.0.0.1"    // SNI hostname
    );

if (!init_ok) {
  std::cerr << "Failed to initialize SSL client" << std::endl;
  return;
}

// Make HTTPS request with mutual authentication
auto result = client.get("https://127.0.0.1:9002/");
if (result.status == 200) {
  std::cout << "Mutual auth successful: " << result.resp_body << std::endl;
}
```

    ## #Server -
    side Configuration

```cpp
#include <ylt/coro_http/coro_http_server.hpp>

    using namespace cinatra;

coro_http_server server(1, 9002);

bool init_ok =
    server.init_ssl("./certs/server.crt",  // Server certificate
                    "./certs/server.key",  // Server private key
                    "",                    // No password
                    "./certs/ca.crt",  // CA certificate for verifying clients
                    true               // Verify client certificate (mandatory)
    );

if (!init_ok) {
  std::cerr << "Failed to initialize SSL server" << std::endl;
  return;
}

server.set_http_handler<GET>("/", [](coro_http_request& req,
                                     coro_http_response& resp) {
  resp.set_status_and_content(status_type::ok, "Hello Mutual Auth!");
});

server.sync_start();
```

    ##Certificate Management

    ## #Generating Test Certificates

        For testing purposes,
    you can generate self -
        signed certificates using OpenSSL :

    ####Generate CA Certificate

```bash
#Generate CA private key
        openssl genrsa -
    out ca.key 2048

#Generate CA certificate
    openssl req -
    new - x509 - days 3650 - key ca.key - out ca.crt -
    subj "/C=CN/ST=Beijing/L=Beijing/O=MyOrg/CN=TestCA"
```

    ####Generate Server Certificate

```bash
#Generate server private key
        openssl genrsa -
    out server.key 2048

#Generate server certificate signing request
    openssl req -
    new - key server.key - out server.csr -
    subj "/C=CN/ST=Beijing/L=Beijing/O=MyOrg/CN=127.0.0.1"

#Sign server certificate with CA
    openssl x509 -
    req - days 3650 - in server.csr - CA ca.crt - CAkey ca.key -
    CAcreateserial -
    out server.crt
```

    ####Generate Client Certificate

```bash
#Generate client private key
        openssl genrsa -
    out client.key 2048

#Generate client certificate signing request
    openssl req -
    new - key client.key - out client.csr -
    subj "/C=CN/ST=Beijing/L=Beijing/O=MyOrg/CN=TestClient"

#Sign client certificate with CA
    openssl x509 -
    req - days 3650 - in client.csr - CA ca.crt - CAkey ca.key -
    CAcreateserial -
    out client.crt
```

    ## #Certificate Requirements

    -
    **Server Certificate *
         * : Must contain the server's IP address or hostname in the Common Name (CN) or Subject Alternative Name (SAN) -
                 **Client Certificate *
                     * : Must be signed by the CA certificate specified in the
                             server's `ca_cert_file` - **CA Certificate *
                                 * : The root CA certificate that signed both
                                         server and client certificates -
    **Certificate Chain ** : For production use,
    ensure proper certificate chains are configured

    ##Verification Modes

        The `init_ssl` function supports different verification modes :

```cpp const int verify_none = asio::ssl::verify_none;  // No verification
const int verify_peer = asio::ssl::verify_peer;  // Verify peer certificate
const int verify_fail_if_no_peer_cert =
    asio::ssl::verify_fail_if_no_peer_cert;  // Fail if no peer cert
const int verify_client_once =
    asio::ssl::verify_client_once;  // Verify client once
```

**Recommended Settings**:
- **One-way auth**: `asio::ssl::verify_peer`
- **Mutual auth**: `asio::ssl::verify_peer | asio::ssl::verify_fail_if_no_peer_cert` (automatically set when `enable_client_verify=true` on server)

## Best Practices

1. **Use Strong Certificates**: In production, use certificates signed by a trusted CA
2. **Proper Hostname Matching**: Ensure the hostname parameter matches the certificate CN
3. **Secure Private Keys**: Never commit private keys to version control
4. **Certificate Rotation**: Implement procedures for rotating certificates before expiration
5. **Error Handling**: Always check the return value of `init_ssl()` and handle errors appropriately
6. **Testing**: Test both successful and failed authentication scenarios

## Troubleshooting

### Connection Failures

If the client fails to connect:

1. **Check Certificate Validity**: Ensure certificates are not expired
2. **Verify Hostname**: Ensure the SNI hostname matches the certificate CN
3. **Check CA Chain**: Ensure the CA certificate is correct and includes the full chain
4. **Verify File Paths**: Ensure certificate file paths are correct and accessible

### Certificate Verification Errors

Common errors and solutions:

- **"certificate verify failed"**: CA certificate doesn't match or is missing
- **"hostname mismatch"**: SNI hostname doesn't match certificate CN
- **"unable to get local issuer certificate"**: CA certificate not provided
- **"no shared cipher"**: SSL/TLS version or cipher suite mismatch

### Debug Mode

Enable SSL debugging for troubleshooting:

```cpp
#ifdef YLT_ENABLE_SSL
// Set SSL debug mode (environment variable)
// On Linux/Mac:
putenv("OPENSSL_DEBUG=1");

// Or use detailed error messages
SSL_load_error_strings();
ERR_print_errors_fp(stderr);
#endif
```

## Complete Example

See the complete example files:
- HTTP SSL server: [src/coro_http/examples/http_ssl_server.cpp](https://github.com/alibaba/yalantinglibs/blob/main/src/coro_http/examples/http_ssl_server.cpp)
- HTTP SSL client: [src/coro_http/examples/http_ssl_client.cpp](https://github.com/alibaba/yalantinglibs/blob/main/src/coro_http/examples/http_ssl_client.cpp)
