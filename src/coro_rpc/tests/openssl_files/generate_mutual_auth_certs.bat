@echo off
REM Generate test certificates for mutual SSL authentication
REM This script generates CA, server, and client certificates for mutual auth testing only
REM The original fake_server.crt/key and server.crt/key are NOT modified

echo ========================================
echo Generating mutual authentication certificates
echo ========================================

REM Create OpenSSL config file
(
echo [req]
echo default_bits = 2048
echo distinguished_name = req_distinguished_name
echo req_extensions = v3_req
echo x509_extensions = v3_ca
echo.
echo [req_distinguished_name]
echo countryName = CN
echo stateOrProvinceName = Beijing
echo localityName = Beijing
echo organizationName = Alibaba
echo organizationalUnitName = Test
echo commonName = TestCA
echo.
echo [v3_req]
echo keyUsage = keyEncipherment, dataEncipherment
echo extendedKeyUsage = serverAuth
echo subjectAltName = @alt_names
echo.
echo [v3_ca]
echo subjectKeyIdentifier = hash
echo authorityKeyIdentifier = keyid:always,issuer
echo basicConstraints = critical,CA:TRUE
echo keyUsage = critical, cRLSign, keyCertSign
echo.
echo [alt_names]
echo DNS.1 = localhost
echo DNS.2 = 127.0.0.1
echo IP.1 = 127.0.0.1
) > openssl_mutual_auth.cnf

echo.
echo [1/6] Generating CA private key...
openssl genrsa -out ca.key 2048

echo.
echo [2/6] Generating CA certificate...
openssl req -new -x509 -days 3650 -key ca.key -out ca.crt -subj "/C=CN/ST=Beijing/L=Beijing/O=Alibaba/OU=Test/CN=TestCA" -config openssl_mutual_auth.cnf -extensions v3_ca

echo.
echo [3/6] Generating server private key for mutual auth...
openssl genrsa -out server.key 2048

echo.
echo [4/6] Generating and signing server certificate...
openssl req -new -key server.key -out server.csr -subj "/C=CN/ST=Beijing/L=Beijing/O=Alibaba/OU=Test/CN=127.0.0.1" -config openssl_mutual_auth.cnf
openssl x509 -req -days 3650 -in server.csr -CA ca.crt -CAkey ca.key -CAcreateserial -out server.crt -extfile openssl_mutual_auth.cnf -extensions v3_req

echo.
echo [5/6] Generating client private key...
openssl genrsa -out client.key 2048

echo.
echo [6/6] Generating and signing client certificate...
openssl req -new -key client.key -out client.csr -subj "/C=CN/ST=Beijing/L=Beijing/O=Alibaba/OU=Test/CN=TestClient" -config openssl_mutual_auth.cnf
openssl x509 -req -days 3650 -in client.csr -CA ca.crt -CAkey ca.key -CAcreateserial -out client.crt

echo.
echo Generating fake certificates for negative testing...
openssl genrsa -out fake.key 2048
openssl req -new -key fake.key -out fake.csr -subj "/C=CN/ST=Beijing/L=Beijing/O=Fake/OU=Fake/CN=FakeClient" -config openssl_mutual_auth.cnf
openssl x509 -req -days 3650 -in fake.csr -CA ca.crt -CAkey ca.key -CAcreateserial -out fake.crt

echo.
echo Cleaning up temporary files...
del *.csr 2>nul
del *.srl 2>nul
del ca.key 2>nul
del openssl_mutual_auth.cnf 2>nul

echo.
echo ========================================
echo Mutual auth certificates generated!
echo ========================================
echo.
echo Generated files (for mutual auth only):
echo   ca.crt          - CA certificate
echo   server.crt      - Server certificate (CA-signed)
echo   server.key      - Server private key
echo   client.crt      - Client certificate
echo   client.key      - Client private key
echo   fake.crt        - Fake client certificate (for negative testing)
echo   fake.key        - Fake client private key (for negative testing)
echo.
echo Original files preserved:
echo   fake_server.crt - Original server certificate
echo   fake_server.key - Original server private key
echo   dh512.pem       - DH parameters
echo   dhparam.pem     - DH parameters
echo   generate.txt    - Original generation notes
echo.
pause
