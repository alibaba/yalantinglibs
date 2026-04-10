@echo off
REM 生成测试 SSL 证书的批处理脚本
REM 用于 yalantinglibs 的 SSL 示例程序

echo ========================================
echo 生成测试 SSL 证书
echo ========================================

REM 设置证书目录
set CERT_DIR=.\certs

REM 创建证书目录
if not exist "%CERT_DIR%" (
    mkdir "%CERT_DIR%"
)

cd /d "%CERT_DIR%"

echo.
echo [1/8] 创建 OpenSSL 配置文件...
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
echo organizationName = Test
echo organizationalUnitName = Test
echo commonName = localhost
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
) > openssl.cnf

echo.
echo [2/8] 生成 CA 私钥...
openssl genrsa -out root-ca.key 2048

echo.
echo [3/8] 生成 CA 证书...
openssl req -new -x509 -days 3650 -key root-ca.key -out root-cert.crt -subj "/C=CN/ST=Beijing/L=Beijing/O=Test/OU=Test/CN=Test-Root-CA" -config openssl.cnf -extensions v3_ca

echo.
echo [4/8] 生成服务器私钥...
openssl genrsa -out server-rsa-sign.key 2048

echo.
echo [5/8] 生成服务器证书 CSR...
openssl req -new -key server-rsa-sign.key -out server-rsa-sign.csr -subj "/C=CN/ST=Beijing/L=Beijing/O=Test/OU=Test/CN=localhost" -config openssl.cnf

echo.
echo [6/8] 签发服务器证书...
openssl x509 -req -days 3650 -in server-rsa-sign.csr -CA root-cert.crt -CAkey root-ca.key -CAcreateserial -out server-rsa-sign.crt -extfile openssl.cnf -extensions v3_req

echo.
echo [7/8] 生成客户端私钥...
openssl genrsa -out client-rsa-sign.key 2048

echo.
echo [8/8] 生成并签发客户端证书...
openssl req -new -key client-rsa-sign.key -out client-rsa-sign.csr -subj "/C=CN/ST=Beijing/L=Beijing/O=Test/OU=Test/CN=Test-Client" -config openssl.cnf
openssl x509 -req -days 3650 -in client-rsa-sign.csr -CA root-cert.crt -CAkey root-ca.key -CAcreateserial -out client-rsa-sign.crt

echo.
echo 清理临时文件...
del *.csr 2>nul
del *.srl 2>nul
del openssl.cnf 2>nul

echo.
echo ========================================
echo 证书生成完成！
echo ========================================
echo.
echo 生成的文件：
echo   - root-cert.crt         (CA 证书)
echo   - root-ca.key            (CA 私钥)
echo   - server-rsa-sign.crt    (服务器证书)
echo   - server-rsa-sign.key     (服务器私钥)
echo   - client-rsa-sign.crt    (客户端证书)
echo   - client-rsa-sign.key     (客户端私钥)
echo.
echo 证书已保存到: %CERT_DIR%
echo.

cd /d ..
echo.
echo 现在可以运行服务器和客户端程序了！
pause
