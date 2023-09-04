# Configurações

Esse diretório contém os arquivos necessários para configurar o Docker e o broker MQTT.  

## Cenários

[`v1_base_code/`](./v1_base_code/) - Configuração básica do Docker e Mosquitto Broker  
[`v2_mqtt_login/`](./v2_mqtt_login/) - Configurações de login e senha no Mosquitto, com as variáveis de ambiente necessárias no Docker, e o arquivo `passwordfile`  
[`v3_mqtt_tls/`](./v3_mqtt_tls/) - Configurações do TLS no Mosquitto, com as variáveis de ambiente necessárias no Docker, e as chaves e certificados do cervidor e CA  

## Postman

O arquivo [`FatecMesh.postman_collection.json`](./FatecMesh.postman_collection.json) contém as requisições de configuração e verificação do IoT-Agent e do Orion.  

> Toda  aconfiguração pode ser realizada automaticamente pelo arquivo [`/create_mqtt_env.py`](/create_mqtt_env.py)

### Variáveis de ambiente

Para editar as variáveis: Clique na workspace `FatecMesh` > `Variables`.  

| Variável | Descrição | Valor |
| -------- | --------- | ----- |
| url  | URL do servidor  | fatecmesh.tech
