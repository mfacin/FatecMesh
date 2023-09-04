# MQTT - TLS

Essa é a implementação da segunda medida de segurança tomada: autenticação por login e senha no MQTT, a fim de evitar que indivíduos não autorizados subscrevam e publiquem nos tópicos das estações.  

O código implementa a lógica por traz do projeto [SmartWeather](https://github.com/senavn/SmartWeather-painlessMesh) no ESP-IDF a fim de disponibilizar o maior controle possível sobre a rede mesh para a pesquisa realizada.

## Funcionalidade

Esse programa implementa criptografia na comunicação MQTT entre o root e o broker, via TLS.  

Para configurar o protocolo TLS, é necessário possuir os certificados e chaves do servidor e da Autoridade Certificadora (CA), caso os certificados sejam auto-assinados.  

### Geranção das chaves e certificados

O primeiro passo é criar o certificado e chave da CA, utilizando o `openssl`. Esse comando apenas deve ser executado se os certificados forem auto-assinados, como é o caso desse cenário.

``` bash
$ openssl req -new -x509 -days 365 -extensions v3_ca -keyout ca.key -out ca.crt -subj "/C=BR/ST=Sao Paulo/L=Sao Caetano do Sul/O=Fatec SCS CA/OU=IoT Mesh Security/CN=fatecmesh.tech/emailAddress=" -addext "subjectAltName=IP:150.230.81.30"
```

> O parâmetro `-subj` nesse comando permite que os dados de país, estado, cidade, organização, unidade organizacional, common-name, e e-mail sejam informados inline. Já o parâmetro `-addext` permite adicionar a extensão `subjectAltName`, para informar o IP do servidor.

Com o certificado do CA, é necessário criar uma chave para o servidor e, em seguida, a solicitação de assinatura a ser enviada para o CA.  

``` bash
$ openssl genrsa -out server.key 2048

$ openssl req -new -out server.csr -extensions v3_ca -key server.key -subj "/C=BR/ST=Sao Paulo/L=Sao Caetano do Sul/O=Fatec SCS/OU=IoT Mesh Security/CN=fatecmesh.tech/emailAddress=" -addext "subjectAltName=IP:150.230.81.30"
```

Com a solicitação de assinatura criada (`server.csr`), ela deve ser enviada ao CA para assinatura, ou, no caso desse cenário, ser auto-assinada:  

``` bash
$ openssl x509 -req -in server.csr -CA ca.crt -CAkey ca.key -CAcreateserial -out server.crt -days 90 -extfile <(printf "subjectAltName=IP:150.230.81.30")
```

### Configuração do servidor

O caminho para o certificado do CA, e certificado e chave do servidor devem ser informados no arquivo `mosquitto.conf` no servidor, junto com a versão do TLS e a nova porta (`8883`):

``` txt
persistence true
persistence_location /mosquitto/data/
log_dest file /mosquitto/log/mosquitto.log
allow_anonymous false
password_file /mosquitto/passwordfile
port 8883
cafile /mosquitto/ca_certificates/ca.crt
keyfile /mosquitto/certs/server.key
certfile /mosquitto/certs/server.crt
tls_version tlsv1.2
```

> A versão do TLS utilizada nesse caso foi a 1.2, devido ao ESP-IDF não possuir suporte ao TLS 1.3.

### Configuração do client

No dispositivo cliente, o certificado do CA deve ser adicionado ao projeto e indicado para compilação no final do arquivo `CMakeList.txt`:  

``` make
target_add_binary_data(${CMAKE_PROJECT_NAME}.elf "main/mqtt_server.crt" TEXT)
```

Para o carregamento da chave no código fonte, as seguintes funções devem ser chamadas no início do programa:  

``` C
extern const uint8_t mqtt_server_crt_start[]  asm("_binary_mqtt_server_crt_start");
extern const uint8_t mqtt_server_crt_end[]    asm("_binary_mqtt_server_crt_end");
```

Em seguida, o certificado deve ser informado no objeto de configurações do client MQTT:  

``` C
// configuring mqtt credentials
esp_mqtt_client_config_t mqtt_cfg = {
    // username, senha e uri configurados de acordo com a versão anterior
    .credentials.username = malloc(strlen(config.mqtt_device_id) + 1),
    .credentials.authentication.password = MQTT_PASSWD,
    .broker.address.uri = MQTT_URI,

    // pointer para o certificado, com a função definida no início
    .broker.verification.certificate = (const char*) mqtt_server_crt_start,
    
    // para ignorar a verificação no CN no certificado:
    // .broker.verification.skip_cert_common_name_check = true
};

strcpy(mqtt_cfg.credentials.username, config.mqtt_device_id);
```


## Organização do código

`main/mesh_main.c` - Contém as inicialização do WiFi, mesh e MQTT, as tasks de envio das mensagens MQTT, e a retransmissão de mensagens para a rede externa pelo root.  
`main/mesh_event_handler.c` - Contém o handler dos eventos do esp_mesh.  
`main/mqtt_event_handler.c` - Contém o handler dos eventos do mqtt_client.  
`main/sensor_simulator.c` - Sintetiza os dados dos sensores a serem enviados pelo MQTT.  
`main/mqtt_server.crt` - Certificado do CA para configuração do TLS.
  
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


