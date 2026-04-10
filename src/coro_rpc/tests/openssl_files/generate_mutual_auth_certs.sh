#!/bin/bash
# Generate test certificates for mutual SSL authentication
# All generated files use "mutual_" prefix to avoid overwriting original files
# The original upstream certificate/key files are NOT modified

echo "========================================"
echo "Generating mutual authentication certificates"
echo "========================================"

echo ""
echo "[1/5] Generating CA private key..."
openssl genrsa -out mutual_ca.key 2048

echo ""
echo "[2/5] Generating CA certificate..."
openssl req -new -x509 -days 3650 -key mutual_ca.key -out mutual_ca.crt \
    -subj "/C=CN/ST=Beijing/L=Beijing/O=MutualTest/OU=Test/CN=MutualTestCA"

echo ""
echo "[3/5] Generating server certificate (CA-signed)..."
openssl genrsa -out mutual_server.key 2048
openssl req -new -key mutual_server.key -out mutual_server.csr \
    -subj "/C=CN/ST=Beijing/L=Beijing/O=MutualTest/OU=Test/CN=127.0.0.1"
openssl x509 -req -days 3650 -in mutual_server.csr -CA mutual_ca.crt -CAkey mutual_ca.key \
    -CAcreateserial -out mutual_server.crt

echo ""
echo "[4/5] Generating client certificate (CA-signed)..."
openssl genrsa -out mutual_client.key 2048
openssl req -new -key mutual_client.key -out mutual_client.csr \
    -subj "/C=CN/ST=Beijing/L=Beijing/O=MutualTest/OU=Test/CN=MutualTestClient"
openssl x509 -req -days 3650 -in mutual_client.csr -CA mutual_ca.crt -CAkey mutual_ca.key \
    -CAcreateserial -out mutual_client.crt

echo ""
echo "[5/5] Generating fake client certificate (self-signed, NOT by CA)..."
openssl genrsa -out mutual_fake.key 2048
openssl req -new -x509 -days 3650 -key mutual_fake.key -out mutual_fake.crt \
    -subj "/C=CN/ST=Beijing/L=Beijing/O=Fake/OU=Fake/CN=FakeClient"

echo ""
echo "Cleaning up temporary files..."
rm -f mutual_ca.key *.csr *.srl

echo ""
echo "========================================"
echo "Mutual auth certificates generated!"
echo "========================================"
echo ""
echo "Generated files (mutual_ prefix):"
echo "  mutual_ca.crt       - CA certificate"
echo "  mutual_server.crt   - Server certificate (CA-signed)"
echo "  mutual_server.key   - Server private key"
echo "  mutual_client.crt   - Client certificate (CA-signed)"
echo "  mutual_client.key   - Client private key"
echo "  mutual_fake.crt     - Fake client certificate (self-signed)"
echo "  mutual_fake.key     - Fake client private key"
echo ""
echo "Original upstream files NOT modified:"
echo "  ca.crt, server.crt, server.key, client.crt, client.key"
echo "  fake.crt, fake.key, dh512.pem, dhparam.pem, generate.txt"
