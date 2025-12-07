# Tutorial: Serviço MQTT para Controle de Temperatura

Este tutorial explica como usar e testar o serviço MQTT implementado no STM32F429ZI
para monitoramento de temperatura e controle de ventilador.

## Índice

1. [Visão Geral](#visão-geral)
2. [Configuração](#configuração)
3. [Compilação e Flash](#compilação-e-flash)
4. [Tópicos MQTT](#tópicos-mqtt)
5. [Testando Manualmente](#testando-manualmente)
6. [Script de Teste Python](#script-de-teste-python)
7. [Solução de Problemas](#solução-de-problemas)

---

## Visão Geral

O sistema implementa um cliente MQTT que:

- **Publica** a temperatura interna do microcontrolador periodicamente
- **Subscreve** a um tópico para receber comandos de controle do ventilador
- **Publica** o status do ventilador quando há mudanças

### Arquitetura

```
┌─────────────────┐          ┌──────────────┐          ┌─────────────────┐
│   STM32F429ZI   │──────────│ Broker MQTT  │──────────│  Script Python  │
│                 │          │              │          │  (Controlador)  │
│ ┌─────────────┐ │  Publica │              │ Subscreve│                 │
│ │ Sensor Temp │─┼─────────►│  Temperatura │─────────►│ Decide ligar/   │
│ └─────────────┘ │          │              │          │ desligar fan    │
│                 │          │              │          │                 │
│ ┌─────────────┐ │ Subscreve│              │ Publica  │                 │
│ │ Ventilador  │◄┼──────────│ Comando Fan  │◄─────────│                 │
│ └─────────────┘ │          │              │          │                 │
└─────────────────┘          └──────────────┘          └─────────────────┘
```

---

## Configuração

### 1. Criar arquivo env.cmake

Copie o arquivo de exemplo e configure:

```bash
cp env.cmake.example env.cmake
```

### 2. Editar env.cmake

Configure as seguintes variáveis obrigatórias:

```cmake
# Porta do broker MQTT
set(ENV{MQTT_PORT} "1883")

# ID único do cliente (deve ser único na rede)
set(ENV{MQTT_CLIENT_ID} "STM32F429ZI_Rack01")

# Credenciais (deixe "" se não usar autenticação)
set(ENV{MQTT_USERNAME} "seu_usuario")
set(ENV{MQTT_PASSWORD} "sua_senha")

# Tópico base e identificador do rack
set(ENV{MQTT_BASE_TOPIC} "racks")
set(ENV{MQTT_RACK_NUMBER} "rack01")

# Intervalo de publicação (ms)
set(ENV{MQTT_PUBLISH_INTERVAL_MS} "5000")

# QoS e Keep-alive
set(ENV{MQTT_QOS} "1")
set(ENV{MQTT_KEEP_ALIVE_S} "60")
```

### 3. Variáveis com Valores Padrão

| Variável | Padrão | Descrição |
|----------|--------|-----------|
| `MQTT_BROKER` | mqtt.rapport.tec.br | Endereço do broker |
| `LOG_LEVEL` | 1 | Nível de log (1=ERROR) |
| `ENABLE_STACK_WATERMARK` | 0 | Monitoramento de stack |

---

## Compilação e Flash

### 1. Gerar arquivos de build

```bash
cmake -DCMAKE_TOOLCHAIN_FILE=cubeide-gcc.cmake \
      -S . -B Debug -G "Unix Makefiles" \
      -DCMAKE_BUILD_TYPE=Debug
```

### 2. Compilar

```bash
make -C Debug -j$(nproc)
```

### 3. Flash via OpenOCD

```bash
openocd -f interface/stlink.cfg \
        -f target/stm32f4x.cfg \
        -c "program Debug/STM32-F429zi-http.elf verify reset exit"
```

### 4. Monitorar via Serial (opcional)

```bash
# Usando minicom (ajuste a porta conforme seu sistema)
minicom -D /dev/ttyACM0 -b 115200

# Ou usando screen
screen /dev/ttyACM0 115200
```

---

## Tópicos MQTT

Com a configuração padrão (`MQTT_BASE_TOPIC=racks`, `MQTT_RACK_NUMBER=rack01`):

| Tópico | Direção | Descrição |
|--------|---------|-----------|
| `racks/rack01/temperature` | Publicado | Temperatura em JSON |
| `racks/rack01/fan/command` | Subscrito | Comandos do ventilador |
| `racks/rack01/fan/status` | Publicado | Status do ventilador |
| `racks/rack01/status` | Publicado | Status geral do dispositivo |

### Formato das Mensagens

**Temperatura (publicada):**
```json
{"temperature": 35.5, "unit": "C"}
```

**Comando do Ventilador (recebido):**
```
on       - Liga o ventilador
off      - Desliga o ventilador
toggle   - Alterna o estado
1        - Liga o ventilador
0        - Desliga o ventilador
liga     - Liga o ventilador
desliga  - Desliga o ventilador
```

**Status do Ventilador (publicado):**
```json
{"fan": "on", "changed_by": "mqtt"}
```
ou
```json
{"fan": "off", "changed_by": "mqtt"}
```

---

## Testando Manualmente

### Usando mosquitto_sub/pub

Instale o cliente Mosquitto:

```bash
# Ubuntu/Debian
sudo apt install mosquitto-clients

# Fedora
sudo dnf install mosquitto
```

### 1. Monitorar temperatura

```bash
mosquitto_sub -h mqtt.rapport.tec.br \
              -t "racks/rack01/temperature" \
              -u rack -P rack4567senha \
              -v
```

### 2. Monitorar status do ventilador

```bash
mosquitto_sub -h mqtt.rapport.tec.br \
              -t "racks/rack01/fan/status" \
              -u rack -P rack4567senha \
              -v
```

### 3. Enviar comando para ligar ventilador

```bash
mosquitto_pub -h mqtt.rapport.tec.br \
              -t "racks/rack01/fan/command" \
              -u rack -P rack4567senha \
              -m "on"
```

### 4. Enviar comando para desligar ventilador

```bash
mosquitto_pub -h mqtt.rapport.tec.br \
              -t "racks/rack01/fan/command" \
              -u rack -P rack4567senha \
              -m "off"
```

---

## Script de Teste Python

Um script Python completo está disponível em `scripts/mqtt_fan_controller.py`.

### Instalação de Dependências

```bash
pip install paho-mqtt
```

### Execução

```bash
python scripts/mqtt_fan_controller.py
```

### Configuração via Variáveis de Ambiente

```bash
export MQTT_BROKER="mqtt.rapport.tec.br"
export MQTT_PORT="1883"
export MQTT_USERNAME="rack"
export MQTT_PASSWORD="rack4567senha"
export MQTT_BASE_TOPIC="racks"
export MQTT_RACK_NUMBER="rack01"
export FAN_TEMP_ON="40.0"      # Liga ventilador acima desta temp
export FAN_TEMP_OFF="35.0"     # Desliga abaixo desta temp

python scripts/mqtt_fan_controller.py
```

### Parâmetros de Linha de Comando

```bash
python scripts/mqtt_fan_controller.py --help

# Exemplos:
python scripts/mqtt_fan_controller.py --temp-on 45 --temp-off 38
python scripts/mqtt_fan_controller.py --broker outro.broker.com --port 8883
```

---

## Solução de Problemas

### 1. STM32 não conecta ao MQTT

**Sintomas:** Log mostra "DNS resolution failed" ou "Connection failed"

**Soluções:**
- Verifique se o cabo Ethernet está conectado
- Verifique se o DHCP está funcionando (deve obter IP)
- Teste conectividade: `ping mqtt.rapport.tec.br`
- Verifique firewall na porta 1883

### 2. Autenticação falha

**Sintomas:** Log mostra "Connection refused: bad username or password"

**Soluções:**
- Verifique `MQTT_USERNAME` e `MQTT_PASSWORD` no env.cmake
- Teste com mosquitto_pub para validar credenciais

### 3. Mensagens não chegam

**Sintomas:** Temperatura publicada mas script Python não recebe

**Soluções:**
- Verifique se os tópicos estão corretos (case-sensitive)
- Verifique `MQTT_BASE_TOPIC` e `MQTT_RACK_NUMBER`
- Use `mosquitto_sub -t "#"` para ver todas as mensagens

### 4. Ventilador não responde

**Sintomas:** Comando enviado mas ventilador não liga/desliga

**Soluções:**
- Verifique conexão física do GPIO PB0
- Verifique polaridade (active high/low)
- Monitore o tópico `fan/status` para confirmar recebimento

### 5. Temperatura incorreta

**Sintomas:** Valores de temperatura muito altos ou baixos

**Soluções:**
- O sensor interno do STM32 tem precisão limitada (±3°C)
- Valores entre 25°C e 50°C são normais para o chip em operação
- Para maior precisão, use sensor externo (DS18B20, etc.)

---

## Referências

- [LwIP MQTT Client](https://www.nongnu.org/lwip/2_1_x/group__mqtt.html)
- [STM32F4 Reference Manual](https://www.st.com/resource/en/reference_manual/rm0090-stm32f405415-stm32f407417-stm32f427437-and-stm32f429439-advanced-armbased-32bit-mcus-stmicroelectronics.pdf)
- [FreeRTOS API](https://www.freertos.org/a00106.html)
- [Paho MQTT Python](https://eclipse.dev/paho/files/paho.mqtt.python/html/client.html)
