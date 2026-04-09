# SM2 Certificates for NTLS Examples

This directory contains SM2 certificates for NTLS (国密SSL) examples using Tongsuo.

## Certificate Files

- `sm2_sign.crt` - SM2 signing certificate
- `sm2_sign.key` - SM2 signing private key
- `sm2_enc.crt` - SM2 encryption certificate  
- `sm2_enc.key` - SM2 encryption private key
- `ca.crt` - CA certificate (optional)

## Generating SM2 Certificates with Tongsuo

To generate your own SM2 certificates using Tongsuo, follow these steps:

### 1. Generate CA private key and certificate
```bash
# Generate CA private key
tongsuo genpkey -algorithm SM2 -out ca.key

# Generate CA certificate
tongsuo req -new -x509 -key ca.key -out ca.crt -days 365 \
    -subj "/C=CN/ST=Beijing/L=Beijing/O=Test/OU=Test/CN=Test CA"
```

### 2. Generate server signing certificate
```bash
# Generate server signing private key
tongsuo genpkey -algorithm SM2 -out sm2_sign.key

# Generate server signing certificate request
tongsuo req -new -key sm2_sign.key -out sm2_sign.csr \
    -subj "/C=CN/ST=Beijing/L=Beijing/O=Test/OU=Test/CN=localhost"

# Sign the certificate request
tongsuo x509 -req -in sm2_sign.csr -CA ca.crt -CAkey ca.key \
    -CAcreateserial -out sm2_sign.crt -days 365
```

### 3. Generate server encryption certificate
```bash
# Generate server encryption private key
tongsuo genpkey -algorithm SM2 -out sm2_enc.key

# Generate server encryption certificate request
tongsuo req -new -key sm2_enc.key -out sm2_enc.csr \
    -subj "/C=CN/ST=Beijing/L=Beijing/O=Test/OU=Test/CN=localhost"

# Sign the certificate request
tongsuo x509 -req -in sm2_enc.csr -CA ca.crt -CAkey ca.key \
    -CAcreateserial -out sm2_enc.crt -days 365
```

### 4. Clean up temporary files
```bash
rm *.csr *.srl
```

## Usage

These certificates are used by the NTLS examples:
- `ntls_server.cpp` - NTLS RPC server example
- `ntls_client.cpp` - NTLS RPC client example

Make sure Tongsuo is properly installed and configured before running the examples.

## Security Note

The certificates in this directory are for testing purposes only. 
Do not use them in production environments.