# _Wi-Fi Protected Access-3_ WPA3

Essa é a implementação da quinta medida de segurança tomada: Implementação da autenticação por WPA3 no Wi-Fi.  

## Funcionalidade

Esse programa implementa a obrigatoriedade da utilização do WPA3 na conexão Wi-Fi, protegendo a rede de ataques offline de força bruta mediante a captura do _4-way handshake_.  

### Configuração do dispositivo

Tanto dispositivo cliente (STA - _station_) quanto o _access point_ precisam ser compatível com a tecnologia WPA3. Segundo a Espressif, a última versão do ESP-IDF apresenta suporte à esse recurso.  

A sua ativação utiliza de poucas linhas de código, aplicando uma configuração à interface STA do Wi-Fi. Infelizmente o _access point_ utilizado não era compatível com o recurso SAE PUBLIC-KEY (PK), portanto, ela precisou ser desativada no código:  

``` C
wifi_config_t config_sec = {
    .sta.pmf_cfg.required = true,
    .sta.transition_disable = true,
    // .sta.sae_pk_mode = true,
    .sta.sae_pwe_h2e = WPA3_SAE_PWE_BOTH
};

ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &config_sec));
ESP_ERROR_CHECK(esp_wifi_start()); // wifi start
```

Além disso, duas configurações precisam de atenção. Por padrão, ambas estão ativadas, porém é interessante se certificar.  

```
CONFIG_ESP_WIFI_ENABLE_WPA3_SAE=y
CONFIG_ESP_WIFI_ENABLE_SAE_PK=y
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


