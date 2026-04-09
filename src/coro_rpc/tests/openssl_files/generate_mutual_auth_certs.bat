@echo off
REM Generate test certificates for mutual SSL authentication

REM Generate CA certificate
openssl genrsa -out ca.key 2048
openssl req -new -x509 -days 3650 -key ca.key -out ca.crt -subj /C=CN/ST=Beijing/L=Beijing/O=Alibaba/CN=TestCA

REM Generate server certificate
openssl genrsa -out server.key 2048
openssl req -new -key server.key -out server.csr -subj /C=CN/ST=Beijing/L=Beijing/O=Alibaba/CN=127.0.0.1
openssl x509 -req -days 3650 -in server.csr -CA ca.crt -CAkey ca.key -CAcreateserial -out server.crt

REM Generate client certificate
openssl genrsa -out client.key 2048
openssl req -new -key client.key -out client.csr -subj /C=CN/ST=Beijing/L=Beijing/O=Alibaba/CN=TestClient
openssl x509 -req -days 3650 -in client.csr -CA ca.crt -CAkey ca.key -CAcreateserial -out client.crt

REM Generate DH parameters
openssl dhparam -out dhparam.pem 1024
openssl dhparam -out dh512.pem 512

echo Certificates generated successfully!
