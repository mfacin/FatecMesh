#!/bin/bash

mkdir -p keys
cd keys
rm -f *.crt *.key *.csr *.srl

# CA's certificate and key
echo && echo CA\'s Certificate and Key
echo ---------------------------
openssl req -new -x509 -days 365 -extensions v3_ca -keyout ca.key -out ca.crt -subj "/C=BR/ST=Sao Paulo/L=Sao Caetano do Sul/O=Fatec SCS CA/OU=IoT Mesh Security/CN=fatecmesh.tech/emailAddress=" -addext "subjectAltName=IP:150.230.81.30"



# server's key
echo && echo Server\'s Key
echo ---------------------------
openssl genrsa -out server.key 2048

# server's signing request
echo && echo Server\'s Signing Request
echo ---------------------------
openssl req -new -out server.csr -extensions v3_ca -key server.key -subj "/C=BR/ST=Sao Paulo/L=Sao Caetano do Sul/O=Fatec SCS/OU=IoT Mesh Security/CN=fatecmesh.tech/emailAddress=" -addext "subjectAltName=IP:150.230.81.30"

# signing the server's request with CA's certificate
echo && echo Signing Server\'s Request
echo ---------------------------
openssl x509 -req -in server.csr -CA ca.crt -CAkey ca.key -CAcreateserial -out server.crt -days 90 -extfile <(printf "subjectAltName=IP:150.230.81.30")

# removing signing request
rm server.csr


exit 0

# ===============
# NOT IMPLEMENTED YET
# ===============


# client's key
echo && echo Client\'s Key
echo ---------------------------
openssl genrsa -out client.key 2048

# client's signing request
echo && echo Client\'s Signing Request
echo ---------------------------
openssl req -out client.csr -key client.key -new -subj "/C=BR/ST=Sao Paulo/L=Sao Caetano do Sul/O=Fatec SCS/OU=IoT Mesh Security/CN=/emailAddress="

# signing the client's request with CA's certificate
echo && echo Signing Client\'s Request
echo ---------------------------
openssl x509 -req -in client.csr -CA ca.crt -CAkey ca.key -CAcreateserial -out client.crt -days 90

# removing signing request
rm client.csr
