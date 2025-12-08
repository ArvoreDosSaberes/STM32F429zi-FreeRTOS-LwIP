# Servidor OPC-UA para STM32F429ZI

## Visão Geral

Este documento descreve a implementação do servidor OPC-UA embarcado para o STM32F429ZI, que expõe a temperatura do microcontrolador e permite controle remoto do ventilador via protocolo OPC-UA.

## Características

- **Protocolo**: OPC-UA Binary (UA TCP)
- **Segurança**: None (sem criptografia - adequado para redes internas)
- **Porta**: 4840 (padrão OPC-UA)
- **Conexões simultâneas**: 2
- **Transporte**: TCP/IP via LwIP

## Arquitetura

```
┌─────────────────────────────────────────────────────────────────┐
│                    Cliente OPC-UA (Python/C#/Java)              │
│                         Dashboard Web                            │
└───────────────────────────────┬─────────────────────────────────┘
                                │ TCP/IP (porta 4840)
                                │ OPC-UA Binary
┌───────────────────────────────▼─────────────────────────────────┐
│                        STM32F429ZI                               │
│  ┌─────────────────────────────────────────────────────────┐    │
│  │                   Servidor OPC-UA                        │    │
│  │  ┌─────────────┐  ┌──────────────┐  ┌───────────────┐   │    │
│  │  │ Temperature │  │  Fan State   │  │  Fan Command  │   │    │
│  │  │  (leitura)  │  │  (leitura)   │  │   (escrita)   │   │    │
│  │  └──────┬──────┘  └──────┬───────┘  └───────┬───────┘   │    │
│  │         │                │                  │            │    │
│  │  ┌──────▼──────┐  ┌──────▼───────┐  ┌───────▼───────┐   │    │
│  │  │ Temp Sensor │  │ Fan Control  │  │ Fan Control   │   │    │
│  │  │    ADC      │  │    GPIO      │  │    GPIO       │   │    │
│  │  └─────────────┘  └──────────────┘  └───────────────┘   │    │
│  └─────────────────────────────────────────────────────────┘    │
│                            LwIP                                  │
│                           Ethernet                               │
└─────────────────────────────────────────────────────────────────┘
```

## Nós OPC-UA (Address Space)

### Namespace Index

- **Namespace 0**: Nós padrão OPC-UA
- **Namespace 1**: Nós do dispositivo (customizados)

### Nós Disponíveis

| NodeId | Nome | Tipo | Acesso | Descrição |
|--------|------|------|--------|-----------|
| `ns=1;i=1001` | Temperature | Float | Leitura | Temperatura do MCU em °C |
| `ns=1;i=1002` | FanState | Boolean | Leitura | Estado atual do ventilador |
| `ns=1;i=1003` | FanCommand | Boolean | Escrita | Comando para ligar/desligar ventilador |
| `ns=1;i=1004` | TempMax | Float | Leitura/Escrita | Limite máximo de temperatura |
| `ns=1;i=1005` | TempMin | Float | Leitura/Escrita | Limite mínimo de temperatura |

## Serviços OPC-UA Suportados

- **GetEndpoints**: Retorna informações do endpoint
- **CreateSession**: Cria sessão de cliente
- **ActivateSession**: Ativa sessão criada
- **Read**: Lê valores dos nós
- **Write**: Escreve valores nos nós
- **Browse**: Lista nós disponíveis

## Compilação

O módulo OPC-UA é compilado automaticamente com o projeto. Certifique-se de que o arquivo está incluído no `CMakeLists.txt`:

```cmake
set (PROJECT_SOURCES
    # ...
    Sources/opcua_server.c
)
```

## Uso

### No Firmware

A tarefa OPC-UA é iniciada automaticamente no `main.c`:

```c
#include "opcua_server.h"

// Na tarefa de inicialização
opcuaServerInit();
opcuaServerSetFanCallback(fanCommandCallback);
opcuaServerStart();
```

### Dashboard Python

```bash
# Instalar dependências
pip install -r scripts/requirements.txt

# Executar dashboard
python scripts/opcua_dashboard.py --server opc.tcp://192.168.0.228:4840
```

O dashboard estará disponível em `http://localhost:5000`.

## Configuração

### Definições no Firmware

Edite `opcua_server.h` para alterar:

```c
#define OPCUA_SERVER_PORT           4840    // Porta do servidor
#define OPCUA_MAX_CONNECTIONS       2       // Conexões simultâneas
#define OPCUA_UPDATE_INTERVAL_MS    1000    // Intervalo de atualização
```

### Argumentos do Dashboard

```
--server, -s    URL do servidor OPC-UA (padrão: opc.tcp://192.168.0.228:4840)
--port, -p      Porta do servidor web (padrão: 5000)
--temp-max      Temperatura máxima inicial (padrão: 45°C)
--temp-min      Temperatura mínima inicial (padrão: 20°C)
--debug         Habilita modo debug
```

## Controle Automático do Ventilador

O dashboard implementa controle automático do ventilador baseado em histerese:

1. **Temperatura > Temp Máxima**: Liga o ventilador
2. **Temperatura < Temp Mínima**: Desliga o ventilador
3. **Entre os limites**: Mantém o estado atual

Isso evita oscilações frequentes do ventilador.

## Limitações

- **Segurança**: Apenas SecurityMode=None implementado
- **Subscriptions**: Não suportado (polling apenas)
- **Métodos**: Não suportado (apenas variáveis)
- **Histórico**: Não suportado

## Troubleshooting

### Conexão Recusada

1. Verifique se o STM32 está conectado à rede
2. Confirme o IP usando a saída serial
3. Verifique se a porta 4840 está acessível

### Timeout na Leitura

1. Verifique a conectividade de rede
2. Aumente o timeout no cliente
3. Verifique se o servidor está rodando (log serial)

### Valores Incorretos

1. Verifique a calibração do sensor de temperatura
2. Confirme os NodeIds no cliente

## Referências

- [OPC UA Specification](https://opcfoundation.org/developer-tools/specifications-unified-architecture)
- [LwIP Documentation](https://www.nongnu.org/lwip/)
- [asyncua Python Library](https://github.com/FreeOpcUa/opcua-asyncio)
