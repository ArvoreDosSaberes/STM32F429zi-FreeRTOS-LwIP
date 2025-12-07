# Tutorial: STM32F429ZI com FreeRTOS e LwIP

![Visitors](https://visitor-badge.laobi.icu/badge?page_id=ArvoreDosSaberes.STM32F429zi-FreeRTOS-LwIP)
[![License: CC BY 4.0](https://img.shields.io/badge/License-CC%20BY%204.0-lightgrey.svg)](https://creativecommons.org/licenses/by/4.0/)
[![FreeRTOS](https://img.shields.io/badge/FreeRTOS-v11.2-blue.svg)](https://www.freertos.org/)
[![LwIP](https://img.shields.io/badge/LwIP-v2.2-blue.svg)](https://savannah.nongnu.org/projects/lwip/)
[![C](https://img.shields.io/badge/Language-C-blue.svg)](https://en.wikipedia.org/wiki/C_(programming_language))
[![STM32F429ZI](https://img.shields.io/badge/STM32F429ZI-Supported-green.svg)](https://www.st.com/en/microcontrollers-microprocessors/stm32f429zi.html)
[![ARM Cortex-M4](https://img.shields.io/badge/ARM-Cortex--M4-green.svg)](https://developer.arm.com/Processors/Cortex-M4)
[![GitHub Stars](https://img.shields.io/github/stars/ArvoreDosSaberes/STM32F429zi-FreeRTOS-LwIP?style=social)](https://github.com/ArvoreDosSaberes/STM32F429zi-FreeRTOS-LwIP)
[![GitHub Issues](https://img.shields.io/github/issues/ArvoreDosSaberes/STM32F429zi-FreeRTOS-LwIP)](https://github.com/ArvoreDosSaberes/STM32F429zi-FreeRTOS-LwIP/issues)

Este tutorial explica como utilizar o template de servidor HTTP para o STM32F429ZI com FreeRTOS e LwIP.

## Índice

1. [Visão Geral](#visão-geral)
2. [Requisitos](#requisitos)
3. [Estrutura do Projeto](#estrutura-do-projeto)
4. [Configuração do Ambiente](#configuração-do-ambiente)
5. [Compilação](#compilação)
6. [Gravação no Microcontrolador](#gravação-no-microcontrolador)
7. [Testando o Servidor HTTP](#testando-o-servidor-http)
8. [Personalizando as Páginas Web](#personalizando-as-páginas-web)
9. [Configuração de Rede](#configuração-de-rede)
10. [Resolução de Problemas](#resolução-de-problemas)

---

## Visão Geral

Este projeto é um template para desenvolvimento com o microcontrolador STM32F429ZI, integrando:

- **FreeRTOS v11.2**: Sistema operacional de tempo real para gerenciamento de tarefas
- **LwIP v2.2**: Pilha TCP/IP leve para conectividade Ethernet
- **Servidor HTTP**: Servidor web integrado para servir páginas HTML

### Objetivo

O projeto tem como objetivo **ensinar e facilitar o desenvolvimento** com microcontroladores da família STM32. Oferecemos à comunidade templates prontos para uso, permitindo que desenvolvedores deem os primeiros passos de forma rápida e eficiente.

### Links Importantes

- **GitHub**: https://github.com/ArvoreDosSaberes/STM32F429zi-FreeRTOS-LwIP
- **Site**: https://mcu.tec.br
- **YouTube**: https://youtube.com/@mcu_fpga

---

## Requisitos

### Hardware

| Item | Descrição |
|------|-----------|
| **Placa** | STM32F429ZI-NUCLEO ou similar com STM32F429ZI |
| **Cabo USB** | USB Type-A para Mini-B (programação/debug) |
| **Cabo Ethernet** | RJ45 para conexão de rede |
| **Roteador/Switch** | Com servidor DHCP habilitado |

### Software

| Software | Versão | Download |
|----------|--------|----------|
| **ARM GCC Toolchain** | 12.x ou superior | [ARM Developer](https://developer.arm.com/tools-and-software/open-source-software/developer-tools/gnu-toolchain/gnu-rm) |
| **CMake** | 3.20 ou superior | [cmake.org](https://cmake.org/download/) |
| **Make** | 4.x | Incluído na maioria das distribuições Linux |
| **Git** | 2.x | [git-scm.com](https://git-scm.com/) |
| **ST-Link** | Última versão | [STMicroelectronics](https://www.st.com/en/development-tools/stsw-link007.html) |

### Opcional

- **STM32CubeIDE**: Para debug avançado e visualização
- **Wireshark**: Para análise de pacotes de rede

---

## Estrutura do Projeto

```
STM32-F429zi-http/
├── Sources/
│   ├── main.c              # Programa principal
│   ├── FreeRTOSConfig.h    # Configuração do FreeRTOS
│   ├── lwipopts.h          # Configuração do LwIP
│   ├── stm32f4xx_hal_conf.h # Configuração do HAL
│   ├── httpserver.c/.h     # Servidor HTTP + cliente DHCP
│   ├── webpages.c/.h       # Páginas HTML embarcadas + funções fs_*_custom
│   ├── ethernetif.c/.h     # Driver Ethernet para LwIP
│   ├── eth_hardware.c      # Inicialização hardware ETH e PHY
│   ├── sys_arch.c          # Interface FreeRTOS-LwIP (semáforos, mutexes)
│   ├── system_config.c/.h  # Configuração de clocks e UART3
│   ├── syscalls.c          # Syscalls para newlib (printf redirecionado)
│   ├── sysmem.c            # Gerenciamento de heap para newlib
│   └── arch/
│       ├── cc.h            # Definições do compilador para LwIP
│       └── sys_arch.h      # Tipos para LwIP-FreeRTOS
├── Startup/
│   └── startup_stm32f429zitx.s  # Código de inicialização (Reset_Handler)
├── docs/
│   ├── TUTORIAL.md         # Este tutorial detalhado
│   ├── tutorial-clock-config.md  # Tutorial de configuração de clocks
│   └── tutorial-uart-config.md   # Tutorial de configuração da UART
├── CMakeLists.txt          # Configuração do CMake
├── cubeide-gcc.cmake       # Toolchain para cross-compilation ARM
├── README.md               # Documentação principal do projeto
└── STM32F429ZITX_FLASH.ld  # Linker script
```

---

## Configuração do Ambiente

### Linux (Ubuntu/Debian)

```bash
# 1. Instalar dependências
sudo apt update
sudo apt install -y cmake make git gcc-arm-none-eabi stlink-tools

# 2. Clonar o repositório
git clone https://github.com/ArvoreDosSaberes/STM32F429zi-FreeRTOS-LwIP.git
cd STM32F429zi-FreeRTOS-LwIP
```

### Windows

1. Instale o [MSYS2](https://www.msys2.org/)
2. Abra o terminal MSYS2 e execute:

```bash
pacman -Syu
pacman -S mingw-w64-x86_64-cmake mingw-w64-x86_64-make mingw-w64-x86_64-arm-none-eabi-gcc
```

3. Baixe e instale o [ST-Link](https://www.st.com/en/development-tools/stsw-link007.html)

### macOS

```bash
# Usando Homebrew
brew install cmake make git
brew install --cask gcc-arm-embedded
```

---

## Compilação

### Passo 1: Gerar os arquivos de build

```bash
# Criar diretório de build e configurar
cmake -DCMAKE_TOOLCHAIN_FILE=cubeide-gcc.cmake -S ./ -B Debug -G"Unix Makefiles" -DCMAKE_BUILD_TYPE=Debug
```

**Nota**: Na primeira execução, o CMake irá baixar automaticamente:
- FreeRTOS Kernel v11.2 do GitHub
- LwIP v2.2.0 do repositório oficial

### Passo 2: Compilar o projeto

```bash
# Compilar
make -C Debug -j$(nproc)
```

Saída esperada:
```
[100%] Built target STM32-F429zi-http
   text    data     bss     dec     hex filename
  45632    1024   18432   65088   fe40 STM32-F429zi-http.elf
```

### Build de Release

Para uma build otimizada (produção):

```bash
cmake -DCMAKE_TOOLCHAIN_FILE=cubeide-gcc.cmake -S ./ -B Release -G"Unix Makefiles" -DCMAKE_BUILD_TYPE=Release
make -C Release -j$(nproc)
```

---

## Gravação no Microcontrolador

### Usando ST-Link (Linux)

```bash
# Gravar o firmware
st-flash --reset write Debug/STM32-F429zi-http.bin 0x08000000
```

### Usando OpenOCD

```bash
openocd -f interface/stlink.cfg -f target/stm32f4x.cfg \
  -c "program Debug/STM32-F429zi-http.elf verify reset exit"
```

### Usando STM32CubeProgrammer

1. Abra o STM32CubeProgrammer
2. Conecte via ST-Link
3. Selecione o arquivo `Debug/STM32-F429zi-http.elf`
4. Clique em "Download"

---

## Testando o Servidor HTTP

### Passo 1: Conectar à rede

1. Conecte o cabo Ethernet na placa NUCLEO
2. Conecte a outra ponta em um roteador/switch com DHCP
3. Aguarde a placa obter um IP (LED de rede deve piscar)

### Passo 2: Descobrir o IP

Se você tiver um terminal serial conectado à **UART3** (115200 baud, 8N1), verá:

```
=== Sistema inicializado ===
SYSCLK: 180000000 Hz
HCLK:   180000000 Hz
PCLK1:  45000000 Hz
PCLK2:  90000000 Hz

=== STM32F429ZI HTTP Server ===
Inicializando stack de rede...
[HTTP] Inicializando stack TCP/IP...
[HTTP] Stack TCP/IP inicializado
[HTTP] Adicionando interface de rede...
[HTTP] Interface de rede configurada
[HTTP] Link ativo, iniciando DHCP...
[DHCP] Tarefa de monitoramento iniciada
[HTTP] Servidor HTTP inicializado
Servidor HTTP inicializado com sucesso!
Aguardando IP via DHCP...
[DHCP] IP obtido: 192.168.1.105
[DHCP] Gateway: 192.168.1.1
[DHCP] Mascara: 255.255.255.0
Servidor disponivel em: http://192.168.1.105/
```

> **Nota**: A UART3 na placa NUCLEO-F429ZI utiliza os pinos **PD8 (TX)** e **PD9 (RX)**.
> Estes pinos estão disponíveis no conector ST-Link virtual COM port.

Alternativamente, use um scanner de rede:
```bash
# Linux
nmap -sn 192.168.1.0/24

# Windows (PowerShell)
arp -a
```

### Passo 3: Acessar a página

Abra um navegador e acesse: `http://<IP_DA_PLACA>/`

Você verá a landing page com:
- Informações sobre o projeto
- Botões para GitHub, MCU.tec.br e YouTube
- Badges das tecnologias usadas

### Testando a página 404

Acesse qualquer URL inexistente: `http://<IP_DA_PLACA>/pagina-inexistente`

Você verá a página de erro 404 personalizada.

---

## Personalizando as Páginas Web

### Editando o HTML

As páginas estão em `Sources/webpages.c`:

```c
static const char indexHtml[] = "<!DOCTYPE html>...";
static const char error404Html[] = "<!DOCTYPE html>...";
```

### Adicionando novas páginas

1. Adicione o conteúdo HTML como uma string constante:

```c
static const char aboutHtml[] =
"<!DOCTYPE html>\n"
"<html><head><title>Sobre</title></head>\n"
"<body><h1>Sobre o projeto</h1></body></html>\n";
```

2. Registre no array `webFiles`:

```c
static const WebFile_t webFiles[] =
{
    { "/",           indexHtml,    sizeof(indexHtml) - 1,    "text/html" },
    { "/index.html", indexHtml,    sizeof(indexHtml) - 1,    "text/html" },
    { "/about.html", aboutHtml,    sizeof(aboutHtml) - 1,    "text/html" },
    { "/404.html",   error404Html, sizeof(error404Html) - 1, "text/html" },
    { NULL,          NULL,         0,                         NULL       }
};
```

3. Recompile o projeto.

### Sistema de Arquivos Customizado (fs_*_custom)

O projeto implementa funções customizadas para o httpd do LwIP em `Sources/webpages.c`:

```c
/* Abre um arquivo para leitura */
int fs_open_custom(struct fs_file *file, const char *name);

/* Fecha um arquivo */
void fs_close_custom(struct fs_file *file);

/* Lê dados de um arquivo */
int fs_read_custom(struct fs_file *file, char *buffer, int count);
```

Estas funções permitem:
- **Servir páginas diretamente da Flash** (sem cópia para RAM)
- **Fallback automático para 404** quando página não existe
- **Tipo MIME automático** baseado na extensão do arquivo

### URLs Suportadas

| URL | Descrição |
|-----|--------|
| `/` | Landing page (index.html) |
| `/index.html` | Landing page |
| `/index.htm` | Landing page |
| `/404.html` | Página de erro |
| *qualquer outra* | Redireciona para 404.html |

### Cores do tema

O tema usa as seguintes cores:
- **Fundo**: `#FF6B35` (laranja)
- **Detalhes**: `#2E8B57` (verde) e `#8B4513` (marrom)
- **Texto**: `#FFFFFF` (branco)
- **Botões**: Gradiente laranja com borda marrom/verde

---

## Configuração de Rede

### Comportamento Padrão (DHCP com Fallback)

O sistema implementa um cliente DHCP inteligente com as seguintes características:

1. **Inicialização**: Tenta obter IP via DHCP automaticamente
2. **Monitoramento de Link**: Detecta conexão/desconexão do cabo Ethernet
3. **Fallback Automático**: Após 30 segundos sem resposta DHCP, usa IP estático fallback:
   - IP: `192.168.1.100`
   - Netmask: `255.255.255.0`
   - Gateway: `192.168.1.1`
4. **Reconexão**: Ao reconectar o cabo, reinicia o DHCP automaticamente

### IP Estático Manual

Para usar IP estático permanente, modifique a função `dhcpClientTask` em `Sources/httpserver.c`:

```c
/* Na função dhcpClientTask, alterar o timeout para 0 segundos */
/* e modificar os IPs de fallback conforme necessário: */

ip4_addr_t fallbackIp, fallbackNetmask, fallbackGw;
IP4_ADDR(&fallbackIp, 192, 168, 1, 100);      /* Seu IP desejado */
IP4_ADDR(&fallbackNetmask, 255, 255, 255, 0); /* Sua máscara */
IP4_ADDR(&fallbackGw, 192, 168, 1, 1);        /* Seu gateway */

netif_set_addr(&httpNetif, &fallbackIp, &fallbackNetmask, &fallbackGw);
```

Ou desabilite o DHCP completamente removendo a chamada `dhcp_start()` e configurando o IP diretamente na função `httpServerInit()`.

### Hostname da Placa

O hostname é usado para identificação no servidor DHCP. Para alterá-lo, edite `Sources/httpserver.c`:

```c
/* Configurar hostname para identificação no DHCP */
#if LWIP_NETIF_HOSTNAME
    netif_set_hostname(&httpNetif, "meu-dispositivo");
#endif
```

### Endereço MAC

Para alterar o MAC address, edite `Sources/ethernetif.c`:

```c
/* Configurar endereço MAC */
ethIfControl.macAddr[0] = 0x02;  /* Bit 1 = 1 (localmente administrado) */
ethIfControl.macAddr[1] = 0x00;
ethIfControl.macAddr[2] = 0x00;
ethIfControl.macAddr[3] = 0x00;
ethIfControl.macAddr[4] = 0x00;
ethIfControl.macAddr[5] = 0x01;
```

> **Dica**: Endereços MAC com o bit 1 do primeiro byte setado (0x02, 0x06, etc.)
> são considerados "localmente administrados" e não conflitam com MACs de fabricantes.

---

## Resolução de Problemas

### Problema: Compilação falha ao baixar FreeRTOS/LwIP

**Sintoma**: Erro de conexão durante `cmake` ou `make`.

**Solução**:
1. Verifique sua conexão com a internet
2. Se estiver atrás de proxy, configure as variáveis de ambiente:
   ```bash
   export http_proxy=http://proxy:porta
   export https_proxy=http://proxy:porta
   ```
3. Tente baixar manualmente e colocar em `_deps/`

### Problema: Placa não obtém IP via DHCP

**Sintomas**: LED de rede não pisca, sem resposta HTTP.

**Soluções**:
1. Verifique o cabo Ethernet
2. Confirme que o roteador tem DHCP habilitado
3. Verifique se a porta do switch está funcionando
4. Conecte um terminal serial para ver logs de debug

### Problema: Página não carrega no navegador

**Sintomas**: Timeout ou erro de conexão.

**Soluções**:
1. Faça ping no IP da placa: `ping 192.168.1.x`
2. Verifique firewall do computador
3. Tente outro navegador
4. Verifique se o IP está correto

### Problema: Erro de linker sobre símbolos indefinidos

**Sintoma**: `undefined reference to 'xxx'`

**Solução**:
1. Verifique se todos os arquivos `.c` estão listados em `PROJECT_SOURCES` no `CMakeLists.txt`
2. Limpe o build e recompile:
   ```bash
   rm -rf Debug
   cmake -DCMAKE_TOOLCHAIN_FILE=cubeide-gcc.cmake -S ./ -B Debug -G"Unix Makefiles"
   make -C Debug -j
   ```

### Problema: HardFault ao iniciar

**Sintoma**: Placa trava ou reinicia imediatamente.

**Soluções**:
1. Verifique configuração do FPU em `main.c`
2. Aumente `configTOTAL_HEAP_SIZE` em `FreeRTOSConfig.h`
3. Verifique stack sizes das tarefas
4. Use debug para identificar o ponto de falha

---

## Próximos Passos

Após dominar este template, você pode:

1. **Adicionar sensores**: Integrar leitura de sensores via ADC
2. **Criar API REST**: Adicionar endpoints para controle remoto
3. **Implementar WebSocket**: Comunicação em tempo real
4. **Adicionar TLS/HTTPS**: Conexão segura com mbedTLS
5. **Persistir dados**: Salvar configurações em Flash/EEPROM

---

## Suporte

- **Issues**: https://github.com/ArvoreDosSaberes/STM32F429zi-FreeRTOS-LwIP/issues
- **Discussões**: https://github.com/ArvoreDosSaberes/STM32F429zi-FreeRTOS-LwIP/discussions
- **YouTube**: https://youtube.com/@mcu_fpga

---

*Desenvolvido com ❤️ por Arvore dos Saberes*
