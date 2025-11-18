Projeto da disciplina ACH2157 - Computação Física e Aplicações (2025)

# Pager morse

## Objetivo(s)

Criar um dispositivo portátil que consiga fazer comunicação de longa distância usando o protocolo LoRa
* Conseguir comunicação usando LoRa
* Fazer com que ambos os dispositivos consigam mandar e receber mensagens simultaneamente

## Introdução 

Esse projeto visa entender o processo de comunicação via rádio e gerar um exemplo funcional com os componentes.

Este dispositivo deve conseguir se comunicar em tempo real e mostrar essa comunicação na tela. 

É composto por dois botões usados para mandar as mensagens, e uma tela que mostra todas as informações recebidas.

## Método (de execução do projeto)

O método adotado para o desenvolvimento do projeto foi o modelo cascata (Waterfall). 
Esse método organiza o trabalho em uma sequência linear de etapas, onde cada fase depende da conclusão da fase anterior.
As etapas são:
 - Fazer o envio de mensagens funcionar
 - Fazer o receptor funcionar
 - Juntar os dois em um ping pong
 - Fazer a captação de input to usuário com botões
 - Transformar esse input em alfanumérico usando o padrão morse
 - Mostrar o que foi recebido na tela

 - Se sobrar tempo pensar em uma forma de divir a tela em dois: A mensagem que foi recebida e a que foi mandada. 


## Resultados 

### Como reproduzir este dispositivo 

#### Lista de Materiais

| Nome | Quantidade | link para foto do componente, de fato, utilizado |
| --- | --- | --- |
| Heltec lora esp32 v2 | 1 | Figura 1 |
| Protoboard | 1 | Figura 2 |
| Buzzer | 1 | Figura 3 |
| Botão | 2 | Figura 4 |

Figura 1:

Figura 2:

Figura 3:

Figura 4:


#### Montagem

##### Lista de conexões 

Tabela de conexões: 


#### Carga do programa em um novo dispositivo 



#### Teste de uso


### Manual de usuário 


### Manual de desenvolvedor

## Conclusão e Comentários 
Eu só queria brincar com código morse, e acabei até agora não brincando (perdi muito tempo fazendo o lora funcionar, tanto que comecei a documentação direito só agora)
