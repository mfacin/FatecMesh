# MQTT - TLS Mútuo

Essa é a implementação da terceira medida de segurança tomada: autenticação dos clientes via certificado RSA no MQTTS, a fim de evitar a utilização de credenciais roubadas para autenticação do MQTT.  

## Funcionalidade

Esse programa implementa a autenticação dos clientes por certiciados RSA na comunicação com o broker MQTT, via TLS mútuo.  

Para configurar o TLS mútuo, é necessário possuir os certificados e chaves do servidor e da Autoridade Certificadora (CA), caso os certificados sejam auto-assinados e criar os certificados dos clientes.  

### Geranção das chaves e certificados

O primeiro passo é criar o certificado e chave da CA, utilizando o `openssl`. Esse comando apenas deve ser executado se os certificados forem auto-assinados, como é o caso desse cenário.

Com o certificado do CA criado na versão anterior, é necessário criar uma chave para o cliente e, em seguida, a solicitação de assinatura a ser enviada para o CA.  

``` bash
$ openssl genrsa -out client.key 2048

$ openssl req -new -out client.csr -key client.key -subj "/C=BR/ST=Sao Paulo/L=Sao Caetano do Sul/O=Fatec SCS/OU=IoT Mesh Security/CN=station/emailAddress="
```

> O atributo CN nesse caso identificará o usuário conectado no broker.

Com a solicitação de assinatura criada (`client.csr`), ela deve ser enviada ao CA para assinatura, ou, no caso desse cenário, ser auto-assinada:  

``` bash
$ openssl x509 -req -in client.csr -CA ca.crt -CAkey ca.key -CAcreateserial -out client.crt -days 90
```

### Configuração do servidor

O uso do certificado deve ser requerido pela configuração `require_certificate true`, e o nome de usuário será inferido pelo CN, como dito anteriormente, pela configuração `use_identity_as_username true`. Nesse caso, o arquivo de senhas não será mais utilizado, pois assume-se que o cliente com o certificado já é autorizado a se autenticar no broker.

``` txt
persistence true
persistence_location /mosquitto/data/
log_dest file /mosquitto/log/mosquitto.log
allow_anonymous false
port 8883
cafile /mosquitto/ca_certificates/ca.crt
keyfile /mosquitto/certs/server.key
certfile /mosquitto/certs/server.crt
tls_version tlsv1.2
require_certificate true
use_identity_as_username true
```

### Configuração do client

No dispositivo cliente, o certificado e chave do cliente devem ser adicionados ao projeto e indicado para compilação no final do arquivo `CMakeList.txt`:  

``` make
target_add_binary_data(${CMAKE_PROJECT_NAME}.elf "main/client.crt" TEXT)
target_add_binary_data(${CMAKE_PROJECT_NAME}.elf "main/client.key" TEXT)
```

Para o carregamento da chave no código fonte, as seguintes funções devem ser chamadas no início do programa:  

``` C
extern const uint8_t mqtt_client_crt_start[]  asm("_binary_mqtt_client_crt_start");
extern const uint8_t mqtt_client_crt_end[]    asm("_binary_mqtt_client_crt_end");
extern const uint8_t mqtt_client_key_start[]  asm("_binary_mqtt_client_key_start");
extern const uint8_t mqtt_client_key_end[]    asm("_binary_mqtt_client_key_end");
```

Em seguida, o certificado deve ser informado no objeto de configurações do client MQTT:  

``` C
// configuring mqtt credentials
esp_mqtt_client_config_t mqtt_cfg = {
    // certificado e chave do cliente
    .credentials.authentication.key = (const char*) client_key_pem_start,
    .credentials.authentication.certificate = (const char*) client_crt_pem_start,

    // uri da conexão
    .broker.address.uri = MQTT_URI,

    // pointer para o certificado, com a função definida no início
    .broker.verification.certificate = (const char*) mqtt_server_crt_start,
    
    // para ignorar a verificação no CN no certificado:
    // .broker.verification.skip_cert_common_name_check = true
};
```


## Organização do código

`main/mesh_main.c` - Contém as inicialização do WiFi, mesh e MQTT, as tasks de envio das mensagens MQTT, e a retransmissão de mensagens para a rede externa pelo root.  
`main/mesh_event_handler.c` - Contém o handler dos eventos do esp_mesh.  
`main/mqtt_event_handler.c` - Contém o handler dos eventos do mqtt_client.  
`main/sensor_simulator.c` - Sintetiza os dados dos sensores a serem enviados pelo MQTT.  
`main/mqtt_server.crt` - Certificado do CA para configuração do TLS.
`main/client.crt` - Certificado do cliente para configuração do TLS mútuo.
`main/client.key` - Chave do cliente para configuração do TLS mútuo.
  
`main/include/` - Contém os headers dos arquivos criados, além do header contendo a definição de `mesh_config_t`.

## Arquivos de configuração

`.vscode/` - Configurações do ambiente para o VS Code.  
`CMakeLists.txt` - Informações para o compilador.  
`partitions.csv` - Configuração das partições do ESP32.  
`sdkconfig` - Configurações das variáveis de ambiente.  
  
`main/CMakeLists.txt` - Informações para o compilador.  
`Kconfig.projbuild` - Configurações das variáveis de ambiente.  

## Instalação

1. Seguir as orientações para instalar a [extenção ESP-IDF](https://github.com/espressif/vscode-esp-idf-extension/blob/master/docs/tutorial/install.md) para o VS Code;  
2. Conectar o dispositivo no computador via USB;  
3. Selecionar a porta de comunicação com o dispositivo (`1`);  
4. Selecionar o tipo de placa (`2`);  
5. Compilar o projeto e gerar a pasta `/build` contendo os binários (`3`);  
6. Enviar os binários e as configurações do bootloader para o dispositivo (`4`);  
7. Monitorar o dispositivo e as mensagens de log geradas (`5`);  
`*` - Essa opção é um atalho para os itens `3`, `4` e `5`;  

![](../images/vscode.png)


