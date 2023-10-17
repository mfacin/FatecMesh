# MQTT - _Protected Management Frames_ (PMF)

Essa é a implementação da quarta medida de segurança tomada: proteção dos _frames_ de gerenciamento Wi-Fi, a fim de evitar ataques _de-auth_.  

## Funcionalidade

Esse programa implementa a proteção de _frames_ de gerenciamento Wi-Fi, impedindo a utilização dos mesmos para ataques de negação de serviço baseados em de-autenticação.  

### Configuração do dispositivo

O dispositivo cliente (STA - _station_) precisa ser compatível com a tecnologia PMF. Segundo a Espressif, a última versão do ESP-IDF apresenta suporte à esse recurso.  

A sua ativação utiliza de poucas linhas de código, aplicando uma configuração à interface STA do Wi-Fi:  

``` C
wifi_config_t config_pmf = {
    .sta.pmf_cfg.required = true
};

ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &config_pmf));
ESP_ERROR_CHECK(esp_wifi_start()); // wifi start
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


