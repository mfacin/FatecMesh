#!/bin/bash

mkdir -p keys
cd keys
rm -f *.crt *.key *.csr *.srl

# certificado e chaves da CA
echo && echo Certificado e chaves da CA
echo ---------------------------
openssl req -new -x509 -days 365 -extensions v3_ca -keyout ca.key -out ca.crt -subj "/C=BR/ST=Sao Paulo/L=Sao Caetano do Sul/O=Fatec SCS CA/OU=IoT Mesh Security/CN=fatecmesh.tech/emailAddress=" -addext "subjectAltName=IP:150.230.81.30"

# chave do servidor
echo && echo Chave do servidor
echo ---------------------------
openssl genrsa -out server.key 2048

# requisicao de assinatura do servidor
echo && echo Requisicao de assinatura do servidor
echo ---------------------------
openssl req -new -out server.csr -extensions v3_ca -key server.key -subj "/C=BR/ST=Sao Paulo/L=Sao Caetano do Sul/O=Fatec SCS/OU=IoT Mesh Security/CN=fatecmesh.tech/emailAddress=" -addext "subjectAltName=IP:150.230.81.30"

# assinatura da requisicao do servidor utilizando o certificado da CA
echo && echo Assinatura da requisicao do servidor
echo ---------------------------
openssl x509 -req -in server.csr -CA ca.crt -CAkey ca.key -CAcreateserial -out server.crt -days 90 -extfile <(printf "subjectAltName=IP:150.230.81.30")

# chave do cliente
echo && echo Chave do cliente
echo ---------------------------
openssl genrsa -out client.key 2048

# requisicao de assinatura do cliente
echo && echo Requisicao de assinatura do cliente
echo ---------------------------
openssl req -new -out client.csr -key client.key -subj "/C=BR/ST=Sao Paulo/L=Sao Caetano do Sul/O=Fatec SCS/OU=IoT Mesh Security/CN=station/emailAddress="

# assinatura da requisicao do cliente utilizando o certificado da CA
echo && echo Assinatura da requisicao do cliente
echo ---------------------------
openssl x509 -req -in client.csr -CA ca.crt -CAkey ca.key -CAcreateserial -out client.crt -days 90

# removendo as requisicoes
rm server.csr
rm client.csr
