# Programa Base de Testes

Esse programa será utilizado como base para os testes de segurança do trabalho. Ele não possui nenhuma solução de segurança implementada.  

O código implementa a lógica por traz do projeto [SmartWeather](https://github.com/senavn/SmartWeather-painlessMesh) no ESP-IDF a fim de disponibilizar o maior controle possível sobre a rede mesh para a pesquisa realizada.

## Funcionalidade

Esse programa possibilita os nós enviarem os dados dos seus sensores ao Context Broker e receber comandos para acender o LED embarcado.  

A comunicação utiliza de prefixos, separados do payload por `;`, para indicar ao nó o tipo de comunicação. São eles:  

- `DATA_TO_MQTT`: Enviada dos nós para o root. Indica ao root que ele deve realizar a publicação de uma mensagem MQTT, contendo o payload enviado.  
- `DATA_FROM_MQTT`: Enviada do root para os nós. Contém uma mensagem recebida em um dos tópicos inscritos pela rede mesh.  
- `SUBSCRIBE_MQTT`: Enviada dos nós para o root. Indica ao root que o nó deseja realizar a inscrição no tópico enviado.  
- `SUBSCRIBE_OK`: Enviado do root para o nó que solicitou a inscrição. Indica que a inscrição foi bem sucedida e que o nó receberá os comandos nesse tópico.  
- `SUBSCRIBE_FAIL`: Enviado do root para o nó que solicitou a inscrição. Indica que a inscrição não foi bem sucedida e que o nó deve tentar novamente.  

Os dados são enviados via tópico `/ul/fatecmeshiot/StationXXXXXX/attrs`, e os comandos recebidos via tópico `/fatecmeshiot/StationXXXXXX/cmd`.  

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


