#!/bin/bash
# Generate test certificates for mutual SSL authentication
# This script generates CA, server, and client certificates for mutual auth testing only
# The original fake_server.crt/key and server.crt/key are NOT modified

echo "========================================"
echo "Generating mutual authentication certificates"
echo "========================================"

# Create OpenSSL config file
cat > openssl_mutual_auth.cnf << 'EOF'
[req]
default_bits = 2048
distinguished_name = req_distinguished_name
req_extensions = v3_req
x509_extensions = v3_ca

[req_distinguished_name]
countryName = CN
stateOrProvinceName = Beijing
localityName = Beijing
organizationName = Alibaba
organizationalUnitName = Test
commonName = TestCA

[v3_req]
keyUsage = keyEncipherment, dataEncipherment
extendedKeyUsage = serverAuth
subjectAltName = @alt_names

[v3_ca]
subjectKeyIdentifier = hash
authorityKeyIdentifier = keyid:always,issuer
basicConstraints = critical,CA:TRUE
keyUsage = critical, cRLSign, keyCertSign

[alt_names]
DNS.1 = localhost
DNS.2 = 127.0.0.1
IP.1 = 127.0.0.1
EOF

echo ""
echo "[1/6] Generating CA private key..."
openssl genrsa -out ca.key 2048

echo ""
echo "[2/6] Generating CA certificate..."
openssl req -new -x509 -days 3650 -key ca.key -out ca.crt \
    -subj "/C=CN/ST=Beijing/L=Beijing/O=Alibaba/OU=Test/CN=TestCA" \
    -config openssl_mutual_auth.cnf -extensions v3_ca

echo ""
echo "[3/6] Generating server private key for mutual auth..."
openssl genrsa -out server.key 2048

echo ""
echo "[4/6] Generating and signing server certificate..."
openssl req -new -key server.key -out server.csr \
    -subj "/C=CN/ST=Beijing/L=Beijing/O=Alibaba/OU=Test/CN=127.0.0.1" \
    -config openssl_mutual_auth.cnf
openssl x509 -req -days 3650 -in server.csr -CA ca.crt -CAkey ca.key \
    -CAcreateserial -out server.crt \
    -extfile openssl_mutual_auth.cnf -extensions v3_req

echo ""
echo "[5/6] Generating client private key..."
openssl genrsa -out client.key 2048

echo ""
echo "[6/6] Generating and signing client certificate..."
openssl req -new -key client.key -out client.csr \
    -subj "/C=CN/ST=Beijing/L=Beijing/O=Alibaba/OU=Test/CN=TestClient" \
    -config openssl_mutual_auth.cnf
openssl x509 -req -days 3650 -in client.csr -CA ca.crt -CAkey ca.key \
    -CAcreateserial -out client.crt

echo ""
echo "Generating fake certificates for negative testing..."
openssl genrsa -out fake.key 2048
openssl req -new -key fake.key -out fake.csr \
    -subj "/C=CN/ST=Beijing/L=Beijing/O=Fake/OU=Fake/CN=FakeClient" \
    -config openssl_mutual_auth.cnf
openssl x509 -req -days 3650 -in fake.csr -CA ca.crt -CAkey ca.key \
    -CAcreateserial -out fake.crt

echo ""
echo "Cleaning up temporary files..."
rm -f *.csr *.srl ca.key openssl_mutual_auth.cnf

echo ""
echo "========================================"
echo "Mutual auth certificates generated!"
echo "========================================"
echo ""
echo "Generated files (for mutual auth only):"
echo "  ca.crt          - CA certificate"
echo "  server.crt      - Server certificate (CA-signed)"
echo "  server.key      - Server private key"
echo "  client.crt      - Client certificate"
echo "  client.key      - Client private key"
echo "  fake.crt        - Fake client certificate (for negative testing)"
echo "  fake.key        - Fake client private key (for negative testing)"
echo ""
echo "Original files preserved:"
echo "  fake_server.crt - Original server certificate"
echo "  fake_server.key - Original server private key"
echo "  dh512.pem       - DH parameters"
echo "  dhparam.pem     - DH parameters"
echo "  generate.txt    - Original generation notes"
