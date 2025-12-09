# STM32F429ZI Template - FreeRTOS + LwIP HTTP Server

![Visitors](https://visitor-badge.laobi.icu/badge?page_id=ArvoreDosSaberes.STM32F429zi-FreeRTOS-LwIP)
[![License: CC BY 4.0](https://img.shields.io/badge/License-CC%20BY%204.0-lightgrey.svg)](https://creativecommons.org/licenses/by/4.0/)
[![FreeRTOS](https://img.shields.io/badge/FreeRTOS-v11.2-blue.svg)](https://www.freertos.org/)
[![LwIP](https://img.shields.io/badge/LwIP-v2.2-blue.svg)](https://savannah.nongnu.org/projects/lwip/)
[![C](https://img.shields.io/badge/Language-C-blue.svg)](https://en.wikipedia.org/wiki/C_(programming_language))
[![STM32F429ZI](https://img.shields.io/badge/STM32F429ZI-Supported-green.svg)](https://www.st.com/en/microcontrollers-microprocessors/stm32f429zi.html)
[![ARM Cortex-M4](https://img.shields.io/badge/ARM-Cortex--M4-green.svg)](https://developer.arm.com/Processors/Cortex-M4)
[![GitHub Stars](https://img.shields.io/github/stars/ArvoreDosSaberes/STM32F429zi-FreeRTOS-LwIP?style=social)](https://github.com/ArvoreDosSaberes/STM32F429zi-FreeRTOS-LwIP)
[![GitHub Issues](https://img.shields.io/github/issues/ArvoreDosSaberes/STM32F429zi-FreeRTOS-LwIP)](https://github.com/ArvoreDosSaberes/STM32F429zi-FreeRTOS-LwIP/issues)

O projeto tem como objetivo **ensinar e facilitar o desenvolvimento** com microcontroladores da família STM32 em especial o STM32F411zi. Oferecemos à comunidade templates prontos para uso, permitindo que desenvolvedores deem os primeiros passos de forma rápida e eficiente.

Oferecemos à comunidade **templates prontos para uso**, permitindo que desenvolvedores deem os primeiros passos com o STM32 de forma rápida e eficiente, sem a necessidade de configurar tudo do zero.

## Observações:

Pelos que estes que eu fiz com o REACT, a melhor forma de usa-lo com um microcontrolador é ativando a compressão dos arquivos par web, e segmentando os arquivos maiores que 30 a 50Kb.

Veja o documento [Tutorial Factiamento React](docs/tutorial-react-fatiamento-js.md)

### Links

- **GitHub**: https://github.com/ArvoreDosSaberes/STM32F429zi-FreeRTOS-LwIP
- **Site**: https://mcu.tec.br
- **YouTube**: https://youtube.com/@mcu_fpga

### Recursos Principais

| Recurso                         | Descrição                                                                |
| ------------------------------- | -------------------------------------------------------------------------- |
| ⚙️**FreeRTOS v11.2**    | Sistema operacional de tempo real para gerenciamento de tarefas e recursos |
| 🌐**LwIP v2.2**           | Pilha TCP/IP leve e eficiente para conectividade Ethernet                  |
| 📡**Servidor HTTP**       | Servidor web integrado para monitoramento e configuração                 |
| 📚**Código Documentado** | Comentários detalhados e tutoriais para facilitar o aprendizado           |

### Especificações Técnicas

- **Microcontrolador**: STM32F429ZI (ARM Cortex-M4 @ 180 MHz)
- **Memória**: 2 MB Flash, 256 KB SRAM
- **Conectividade**: Ethernet 10/100 Mbps (PHY LAN8742A)
- **Interface Serial**: UART3 (115200 baud, 8N1)

---

## Tutorial

Este documento explica, passo a passo, como:

- **Compilar** o firmware do projeto `STM32-F429zi-http` usando CMake + Make.
- **Gerar e gravar** o binário na placa **Nucleo STM32F429ZI** usando `st-flash`.
- **Usar o OpenOCD** como servidor de debug.
- **Usar o `arm-none-eabi-gdb`** para depurar o código no MCU.
- Consultar uma **tabela de comandos úteis do GDB** com explicações.

> Ambiente considerado: Linux, toolchain ARM (`arm-none-eabi-*`) já instalada, OpenOCD e ST-Link tools instalados.

---

### Documentação Adicional

- **[docs/TUTORIAL.md](docs/TUTORIAL.md)**: Tutorial completo com configuração de rede, personalização de páginas e resolução de problemas
- **[docs/tutorial-clock-config.md](docs/tutorial-clock-config.md)**: Guia detalhado de configuração de clocks
- **[docs/tutorial-uart-config.md](docs/tutorial-uart-config.md)**: Guia de configuração da UART3

### Características do Servidor HTTP

- **DHCP com Fallback**: Obtém IP automaticamente via DHCP; após 30s sem resposta, usa IP estático `192.168.1.100`
- **Monitoramento de Link**: Detecta conexão/desconexão do cabo Ethernet
- **Páginas Embarcadas**: Landing page e página 404 customizadas armazenadas em Flash
- **Hostname DHCP**: Identificado como `stm32-http` no servidor DHCP

---

## 1. Compilando o projeto para

O projeto usa **CMake** com **Unix Makefiles** e a toolchain ARM.

### 1.1. Configurar o build (Debug)

Se desejar gerar o projeto para **Release** substitua a palavra ``Debug`` para ``Release``.

No diretório raiz do projeto (`STM32-F429zi-http`):

```bash
cmake -DCMAKE_TOOLCHAIN_FILE=cubeide-gcc.cmake \
      -S . -B Debug \
      -G "Unix Makefiles" \
      -DCMAKE_BUILD_TYPE=Debug
```

Isso irá:

- Detectar o compilador `arm-none-eabi-gcc`;
- Baixar e configurar o FreeRTOS via `FetchContent`;
- Gerar os arquivos de build dentro da pasta `Debug/`.

### 1.2. Compilar

Ainda na raiz do projeto:

```bash
make -C Debug -j"$(nproc)"
```

Resultado esperado:

- Gera o executável `Debug/STM32-F429zi-http.elf`.
- Mostra, ao final, o uso de memória (`text`, `data`, `bss`).

Se quiser recompilar do zero (limpar e gerar novamente):

```bash
rm -rf Debug
cmake -DCMAKE_TOOLCHAIN_FILE=cubeide-gcc.cmake -S . -B Debug -G "Unix Makefiles" -DCMAKE_BUILD_TYPE=Debug
make -C Debug -j"$(nproc)"
```

---

## 2. Gerar e gravar o binário na Nucleo STM32F429ZI

### 2.1. Gerar o arquivo `.bin` a partir do `.elf`

Na pasta `Debug/`:

```bash
cd Debug
arm-none-eabi-objcopy -O binary STM32-F429zi-http.elf STM32-F429zi-http.bin
```

Isso cria o arquivo `STM32-F429zi-http.bin` pronto para ser gravado na Flash.

### 2.2. Verificar se o ST-Link está visível

Na raiz do projeto (ou qualquer pasta):

```bash
st-info --probe
```

Saída esperada (exemplo):

```text
Found 1 stlink programmers
  version:    V2JxxSyy
  flash:      2097152 (pagesize: 16384)
  sram:       262144
  chipid:     0x419
  dev-type:   STM32F42x_F43x
```

### 2.3. Gravar o binário na Flash

Endereço de Flash da linha F4: `0x08000000`.

Na pasta `Debug/`:

```bash
st-flash write STM32-F429zi-http.bin 0x08000000
```

Se tudo der certo, a saída termina com algo como:Flash written and verified! jolly good!

A placa então começa a executar o firmware gravado.

---

## 3. Usando o OpenOCD

O OpenOCD funciona como um **servidor de debug** ao qual o GDB se conecta.

### 3.1. Iniciar o OpenOCD para STM32F4 + ST-Link

Terminal 1, na raiz do projeto (ou qualquer pasta):

```bash
openocd -f interface/stlink.cfg -f target/stm32f4x.cfg
```

Com isso, o OpenOCD:

- Abre uma interface GDB na porta **3333**.
- Permite comandos especiais via `monitor` dentro do GDB.

### 3.2. Portas padrão do OpenOCD

- **3333** – porta GDB (para `target remote :3333`).
- **4444** – porta telnet (para comandos manuais `telnet localhost 4444`).

> Normalmente você só precisa da porta 3333 com o GDB.

---

## 4. Usando o `arm-none-eabi-gdb` para depurar

### 4.1. Abrir o GDB com o ELF do projeto

Terminal 2, na raiz do projeto:

```bash
arm-none-eabi-gdb Debug/STM32-F429zi-http.elf
```

Dentro do prompt do GDB você verá algo como `(gdb)`.

### 4.2. Conectar ao OpenOCD

No GDB:

```gdb
target remote :3333
```

Se tudo estiver correto, o GDB mostrará o endereço atual do PC (program counter) e passará a controlar o MCU.

### 4.3. Resetar e parar imediatamente após o reset

```gdb
monitor reset halt
```

- `reset` – reseta o microcontrolador.
- `halt` – pede para parar a CPU logo depois do reset.

Assim você pode inspecionar registradores, memória, stack, etc., antes de iniciar a aplicação.

### 4.4. Parar sempre no início da `main`

Sequência típica:

```gdb
monitor reset halt    # garante um estado limpo
break main            # breakpoint na função main
continue              # roda até alcançar main
```

A execução para na **primeira linha** da função `main`.

### 4.5. Breakpoints em funções e linhas específicas

- Breakpoint em uma função:

  ```gdb
  break appTask1
  ```
- Breakpoint em uma linha do arquivo:

  ```gdb
  break Sources/main.c:65
  ```
- Listar breakpoints ativos:

  ```gdb
  info breakpoints
  ```
- Remover um breakpoint específico (por número):

  ```gdb
  delete 1
  ```
- Remover todos os breakpoints:

  ```gdb
  delete breakpoints
  ```

### 4.6. Controle de execução

- Continuar a execução até o próximo breakpoint:

  ```gdb
  continue    # ou 'c'
  ```
- Executar a **próxima linha**, entrando em funções:

  ```gdb
  step        # ou 's'
  ```
- Executar a **próxima linha**, sem entrar em funções chamadas:

  ```gdb
  next        # ou 'n'
  ```
- Sair da função atual, continuando até o retorno:

  ```gdb
  finish
  ```
- Rodar novamente desde o início (útil após reset manual):

  ```gdb
  run         # em ambiente bare-metal geralmente é menos usado, prefira 'monitor reset halt' + 'continue'
  ```

### 4.7. Inspeção de variáveis e memória

- Imprimir o valor de uma variável:

  ```gdb
  print variavel
  ```
- Imprimir em formato hexadecimal:

  ```gdb
  print/x variavel
  ```
- Examinar memória (por exemplo, 10 words de 32 bits a partir de um endereço):

  ```gdb
  x/10wx 0x20000000
  ```
- Ver registradores:

  ```gdb
  info registers
  ```
- Ver apenas um registrador:

  ```gdb
  info registers pc
  ```

### 4.8. Stack trace (pilha de chamadas)

Para ver a pilha de chamadas atual (quem chamou quem):

```gdb
backtrace     # ou 'bt'
```

Para mudar de frame (investigar funções mais acima na pilha):

```gdb
frame 0       # frame atual
frame 1       # frame anterior
```

---

## 5. Exemplo de sessão completa (OpenOCD + GDB)

### Passo a passo

1. **Terminal 1 – OpenOCD**

   ```bash
   openocd -f interface/stlink.cfg -f target/stm32f4x.cfg
   ```
2. **Terminal 2 – GDB**

   ```bash
   cd ~/STM32CubeIDE/workspace_2.0.0/STM32-F429zi-http
   arm-none-eabi-gdb Debug/STM32-F429zi-http.elf
   ```

   No prompt do GDB:

   ```gdb
   target remote :3333        # conecta ao OpenOCD
   monitor reset halt         # reseta e para logo após o reset
   break main                 # breakpoint em main
   continue                   # roda até chegar em main
   ```
3. A partir daí, use `next`, `step`, `print`, etc., para depurar.

---

## 6. Tabela de comandos úteis do GDB

| Comando                            | Descrição                                                                                       | Exemplo                               |
| ---------------------------------- | ------------------------------------------------------------------------------------------------- | ------------------------------------- |
| `target remote HOST:PORT`        | Conecta a um servidor GDB remoto (OpenOCD, st-util, etc.).                                        | `target remote :3333`               |
| `monitor <cmd>`                  | Envia um comando direto ao servidor (OpenOCD).                                                    | `monitor reset halt`                |
| `break <func>`                   | Cria breakpoint no início de uma função.                                                       | `break main`                        |
| `break <file>:<linha>`           | Cria breakpoint em uma linha específica de um arquivo.                                           | `break Sources/main.c:65`           |
| `tbreak ...`                     | Breakpoint temporário (é apagado após ser atingido).                                           | `tbreak main`                       |
| `delete <n>`                     | Remove o breakpoint de número `n`.                                                             | `delete 1`                          |
| `delete breakpoints`             | Remove todos os breakpoints.                                                                      | `delete breakpoints`                |
| `disable <n>`                    | Desativa breakpoint sem removê-lo.                                                               | `disable 1`                         |
| `enable <n>`                     | Reativa breakpoint desativado.                                                                    | `enable 1`                          |
| `info breakpoints`               | Lista todos os breakpoints.                                                                       | `info breakpoints`                  |
| `continue` / `c`               | Continua a execução até próximo breakpoint ou término.                                       | `continue`                          |
| `step` / `s`                   | Executa próxima linha,**entrando** em funções chamadas.                                  | `step`                              |
| `next` / `n`                   | Executa próxima linha,**sem entrar** em funções chamadas.                                | `next`                              |
| `finish`                         | Continua a execução até a função atual retornar.                                             | `finish`                            |
| `run`                            | Inicia ou reinicia o programa (mais comum em programas host, menos em bare-metal).                | `run`                               |
| `interrupt` ou `Ctrl+C`        | Interrompe a execução do programa em qualquer ponto.                                            | `Ctrl+C` no GDB                     |
| `print <expr>` / `p <expr>`    | Avalia e imprime uma expressão (variáveis, ponteiros, etc.).                                    | `print x`, `print myStruct.field` |
| `print/x <expr>`                 | Imprime o valor em hexadecimal.                                                                   | `print/x 0x20000000`                |
| `set var <nome> = <valor>`       | Altera o valor de uma variável em tempo de execução.                                           | `set var counter = 0`               |
| `x/<N><formato><tamanho> <addr>` | Examina memória.`<N>` = quantidade, `<formato>` = x, d, u, etc., `<tamanho>` = b, h, w, g. | `x/10wx 0x20000000`                 |
| `info registers`                 | Mostra todos os registradores da CPU.                                                             | `info registers`                    |
| `info registers pc`              | Mostra apenas o registrador `pc` (program counter).                                             | `info registers pc`                 |
| `backtrace` / `bt`             | Mostra a pilha de chamadas (stack trace).                                                         | `backtrace`                         |
| `frame <n>`                      | Seleciona um frame específico da pilha para inspeção.                                          | `frame 1`                           |
| `info locals`                    | Mostra variáveis locais do frame atual.                                                          | `info locals`                       |
| `info args`                      | Mostra argumentos da função atual.                                                              | `info args`                         |
| `display <expr>`                 | Passa a mostrar automaticamente o valor de uma expressão a cada parada.                          | `display myVar`                     |
| `undisplay <n>`                  | Remove um item da lista de display automático.                                                   | `undisplay 1`                       |
| `set logging on`                 | Liga o log de saída do GDB para um arquivo.                                                      | `set logging on`                    |
| `quit` / `q`                   | Encerra o GDB (perguntará se deve desconectar do alvo).                                          | `quit`                              |

---

## 7. Dicas práticas

- Sempre que suspeitar de estado estranho no MCU, faça:

  ```gdb
  monitor reset halt
  ```
- Para depurar FreeRTOS:

  - Coloque breakpoints nas tarefas (`appMainTask`, `appMonitorTask`, `dhcpClientTask`) e em callbacks de erro (`vApplicationMallocFailedHook`, `vApplicationStackOverflowHook`).
  - Use `backtrace` quando cair em um hook para ver quem chamou.
- Para verificar o uso de memória:

  ```gdb
  print xPortGetFreeHeapSize()
  print xPortGetMinimumEverFreeHeapSize()
  ```
- Se tiver problemas de conexão:

  - Verifique se o OpenOCD ainda está rodando no Terminal 1.
  - Verifique cabos USB e jumpers da Nucleo.

---

Este documento pode ser expandido com mais exemplos específicos do projeto (uso de periféricos, middleware, etc.).

---

## 8. Estrutura do Código Fonte

### Arquivos Principais

| Arquivo               | Descrição                                       |
| --------------------- | ------------------------------------------------- |
| `main.c`            | Função principal, criação de tarefas FreeRTOS |
| `httpserver.c/h`    | Inicialização do servidor HTTP e cliente DHCP   |
| `webpages.c/h`      | Páginas HTML embarcadas e funções fs_*_custom  |
| `ethernetif.c/h`    | Driver Ethernet para LwIP                         |
| `eth_hardware.c`    | Inicialização do hardware Ethernet e PHY        |
| `sys_arch.c`        | Interface FreeRTOS-LwIP (semáforos, threads)     |
| `system_config.c/h` | Configuração de clocks (180 MHz) e UART3        |
| `syscalls.c`        | Syscalls para newlib (redirecionamento printf)    |
| `sysmem.c`          | Gerenciamento de heap para newlib                 |

### Tarefas FreeRTOS

| Tarefa           | Prioridade | Stack         | Descrição                      |
| ---------------- | ---------- | ------------- | -------------------------------- |
| `Main`         | 2          | 512 words     | Inicialização do servidor HTTP |
| `Monitor`      | 1          | 256 words     | Monitoramento de heap            |
| `DHCP`         | 2          | 512 words     | Cliente DHCP com fallback        |
| `tcpip_thread` | 3          | Configurável | Thread principal do LwIP         |

### Configuração de Hardware

- **Clock**: SYSCLK = 180 MHz, HCLK = 180 MHz, APB1 = 45 MHz, APB2 = 90 MHz
- **UART3**: PD8 (TX), PD9 (RX), 115200 baud, 8N1
- **Ethernet**: PHY LAN8742A, RMII interface

---

*Desenvolvido com ❤️ por [Carlos Delfino](https://github.com/carlosdelfino)*
