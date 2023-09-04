# MQTT - Login e Senha

Essa é a implementação da primeira medida de segurança tomada: autenticação por login e senha no MQTT, a fim de evitar que indivíduos não autorizados subscrevam e publiquem nos tópicos das estações.  

O código implementa a lógica por traz do projeto [SmartWeather](https://github.com/senavn/SmartWeather-painlessMesh) no ESP-IDF a fim de disponibilizar o maior controle possível sobre a rede mesh para a pesquisa realizada.

## Funcionalidade

Esse programa implementa autenticação por login e senha no broker MQTT.  

A comunicação utiliza de prefixos, separados do payload por `;`, para indicar ao nó o tipo de comunicação. Tais prefixos podem ser consultados na explicação de [funcionalidade do cenário `v1_base_code`](../v1_base_code/README.md#funcionalidade).  

A configuração das credenciais é realizada no objeto de configuração do client MQTT:

``` C
esp_mqtt_client_config_t mqtt_cfg = {
    .credentials.username = // nome de usuário,
    .credentials.authentication.password = // senha,
    .broker.address.uri = // uri do broker,
};
```

No caso dessa aplicação, tais informações foram definidas pelo pré-processador ou calculadas durante a execução do programa. A implementação ficou assim:

``` C
esp_mqtt_client_config_t mqtt_cfg = {
    // o nome de usuário é definido durante a execução do código
    // é necessário alocar a memória necessária para futura cópia
    .credentials.username = malloc(strlen(config.mqtt_device_id) + 1),

    // a senha e uri são definidos pelo pré-processador
    .credentials.authentication.password = MQTT_PASSWD,
    .broker.address.uri = MQTT_URI,
};

// é necessário realizar a cópia do nome de usuário
strcpy(mqtt_cfg.credentials.username, config.mqtt_device_id);
```

Do lado do servidor, o arquivo `mosquitto.conf` teve de ser alterado para impedir o acesso anônimo e indicar o arquivo de senhas:

```
persistence true
persistence_location /mosquitto/data/
log_dest file /mosquitto/log/mosquitto.log
allow_anonymous false
password_file /mosquitto/passwordfile
```

O arquivo de senhas foi criado indicando cada par de credencial em uma linha, separando o login e senha por `:`, como descrito na [documentação](https://mosquitto.org/man/mosquitto_passwd-1.html) do Mosquitto. Por questões de simplicidade e demonstração, foi utilizada a mesma para todas as estações:

```
Station5BCB18:fatecmesh@
Station5B4A88:fatecmesh@
Station49B7AC:fatecmesh@
Station5AF2F8:fatecmesh@
StationE0C3E4:fatecmesh@
iot_agent:fatecmesh@
mqttx:fatecmesh@
```

Em seguida, o comando para criptografia das senhas foi executado, a fim de gerar o hash das mesmas e proteger as credenciais:

``` bash
$ mosquitto_passwd -U passwordfile
```

O arquivo final ficou da seguinte forma:

```
Station5BCB18:$7$101$ECdkLPPt1AOAGbRC$f/fJYHbvZCKUyc5QZO+0WBxXqXQ256HHqV2FSFW5p3PCD3GaW9Z8yliwe14brHNM1PpR6f9oH3lfkxhd44LtTw==
Station5B4A88:$7$101$1il10N3p9UbLGTvT$jc9X1N5MhBgK1Q4FY91FbHBoOR5l3U3MqvO1jcfqNe7pJoeYe2tsrlbGtdHwPBQYevqX0ihlsoY6JTsOrPQqTw==
Station49B7AC:$7$101$BnFGrW3YekE20/5e$BIxfQE1ImPoHKXCD3u2uMGH9FLtqtXC2Zr7owBBrX0sOINB0SbyxFj2K7SMoYoh/Rf8g5l+ERBPlhQbscmz+lg==
Station5AF2F8:$7$101$1vQRNblHUD+lIgOj$GND4/1y2DU70FplZP7Rt8eBbZoD/+52t7synvoC4umaHI3wT1pJdDkxJWv+2f6l4fXBKmmmiRuQej1/ZCRPhSA==
StationE0C3E4:$7$101$NPGUjZAvktLgMHkd$hmJ+d8BHL+QTIfQ+qw2iHBvC4BVkgagT1a9FtCdglJjuYd5+XcBL0n1Id9LEp/6h492CX1zxBuaacplqIThadg==
iot_agent:$7$101$FcV8Q93NhO2BjyKI$MxxZQ6k+UFuFjo0nhflNEqn9oUDsktAf8Nl6SKZHvq6EdRbQ3S05RCZ4IY1jGycCazawkvT5iAhz6MYQhd2YXA==
mqttx:$7$101$SgFjgTpE4/dtWLvh$7I8SCnBsACF3OdC/0Oapm5y7lQhHZIuy6LjF6J4gwe4kGKn57yfuZvYzrN28vwi+DQqpGYUsgetCQrUGO8Li5A==
```

Importante notar que o serviço do Mosquitto precisou ser reiniciado com cada alteração de configurações.

## Organização do código

`main/mesh_main.c` - Contém as inicialização do WiFi, mesh e MQTT, as tasks de envio das mensagens MQTT, e a retransmissão de mensagens para a rede externa pelo root.  
`main/mesh_event_handler.c` - Contém o handler dos eventos do esp_mesh.  
`main/mqtt_event_handler.c` - Contém o handler dos eventos do mqtt_client.  
`main/sensor_simulator.c` - Sintetiza os dados dos sensores a serem enviados pelo MQTT.
  
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


