# Segurança em redes MESH para IoT

Esse repositório contém o projeto desenvolvido para o Trabalho de Conclusão de Curso em Segurança da Informação dos alunos da Fatec São Caetano do Sul, sob orientação do Prof. Dr. [Fábio Henrique Cabrini](https://github.com/fabiocabrini).  
  
### Alunos:

- Amanda de Caires Ferreira
- Clara Simone dos Santos
- Francidalva de Sousa Moura
- [Matheus Dominguez Facin](https://github.com/TheusFacin)
- [Matheus Santos de Almeida](https://github.com/MaaSantos45)

## Conteúdo

Este repositório está dividido na seguinte estrutura:

[`v0_comm_test/`](./v0_comm_test/) - Contém o código de teste de comunicação entre os nós  
[`v1_base_code/`](./v1_base_code/) - Contém o código base da aplicação que será utilizada nos testes  
[`images/`](./images/) - Imagens de apoio e explicações  
[`postman/`](./postman/) - Arquivos de configuração do postman

Os diretórios `v0_comm_test/`, `v1_base_code/` e `postman/` possuem os seus próprios arquivos `README.md` explicando em detalhes o propósito e utilização do código e API.

## Arquitetura do projeto

![Arquitetura do projeto](images/Arquitetura%20de%20rede_V2.png)

### ESP32 e ESP-IDF

O ESP32 é um microcontrolador desenvolvido pela Espressif Systems e conta com Wi-Fi e bluetooth integrados, além de funcionalidades que permitem ampla integração em ambientes industriais, dispositivos diversos e aplicativos de IoT.  

Para o desenvolvimento na plataforma ESP32, a Espressif disponibiliza o frameword [ESP-IDF](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/), que conta com uma diversa gama de APIs para a utilização das redes sem fio e dos recursos de hardware do controlador.

### FIWARE

O [FIWARE](https://www.fiware.org/) é um framework de código aberto que disponibiliza padrões para possibilitar o desenvolvimento de soluções IoT de maneira rápida, fácil e acessível.  

Ele define padrões abertos para a interoperabilidade na troca de informações entre diferentes aplicações, e é baseado em serviços e componentes para atuações específicas, chamados de Generic Enabler (GE).

### FIWARE Descomplicado

O [FIWARE Descomplicado](https://github.com/fabiocabrini/fiware/) é uma ferramenta para a instanciação dos principais GEs destinados a operação como back-end para aplicações de IoT com persistência de dados.

Essa ferramenta simplifica o processo de instanciação dos principais GEs e facilita o desenvolvimento de PoCs baseadas no processamento de informações de contexto.

### Orion Context Broker

O [Orion Context Broker](https://fiware-orion.readthedocs.io/en/3.10.1/index.html) é um componente da plataforma FIWARE que realiza a gestão do contexto e das informações armazenadas na solução a ser desenvolvida.

Ele utiliza um modelo de publicação/assinatura por meio de uma API RESTful para fornecer atualizações em tempo real sobre mudanças no contexto das entidades.

### MQTT e IoT-Agent

O [MQTT](https://mqtt.org/) é um protocolo de transmissão de mensagens para dispositivos IoT, desenvolvido como uma implementação leve do modelo de publicação/assinatura, utilizando o mínimo de largura de banda de rede.  

O [IoT-Agent](https://github.com/FIWARE/tutorials.IoT-Agent) é um componente FIWARE que facilita a integração de dispositivos IoT utilizando o protocolo MQTT e permite a comunicação bidirecional desses dispositivos com o Orion Context Broker, transformando as mensagens MQTT em atualizações de contexto compreensívies.
