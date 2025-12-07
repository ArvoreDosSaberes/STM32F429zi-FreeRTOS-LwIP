# Tutorial: Configuração da UART2 no STM32F429ZI

Este tutorial explica em detalhes como configurar a UART2 do STM32F429ZI para comunicação serial e redirecionamento do `printf`.

## Índice

1. [Introdução à UART](#introdução-à-uart)
2. [Periféricos USART/UART no STM32F429](#periféricos-usartuart-no-stm32f429)
3. [Escolha dos Pinos](#escolha-dos-pinos)
4. [Configuração dos GPIOs](#configuração-dos-gpios)
5. [Configuração da UART](#configuração-da-uart)
6. [Cálculo do Baud Rate](#cálculo-do-baud-rate)
7. [Redirecionamento do printf](#redirecionamento-do-printf)
8. [Código Completo](#código-completo)
9. [Teste e Verificação](#teste-e-verificação)

---

## Introdução à UART

UART (Universal Asynchronous Receiver/Transmitter) é um protocolo de comunicação serial assíncrono. "Assíncrono" significa que não há sinal de clock compartilhado entre transmissor e receptor.

### Características Principais

| Parâmetro | Descrição |
|-----------|-----------|
| **Transmissão** | Serial (bit a bit) |
| **Sincronização** | Assíncrona (sem clock) |
| **Linhas** | TX (transmit), RX (receive) |
| **Velocidade** | Definida pelo baud rate |
| **Formato típico** | 8N1 (8 bits, No parity, 1 stop bit) |

### Estrutura de um Frame

```
     ┌─────┬───┬───┬───┬───┬───┬───┬───┬───┬──────┐
     │Start│ 0 │ 1 │ 2 │ 3 │ 4 │ 5 │ 6 │ 7 │ Stop │
     │ Bit │   │   │   │   │   │   │   │   │ Bit  │
     └─────┴───┴───┴───┴───┴───┴───┴───┴───┴──────┘
       │                                       │
       │◄─────────────── 10 bits ─────────────►│
       
       Idle state = HIGH (1)
       Start bit = LOW (0)
       Data bits = LSB first
       Stop bit = HIGH (1)
```

### Baud Rate

O baud rate define a velocidade de transmissão em bits por segundo (bps):

| Baud Rate | Tempo por bit | Uso típico |
|-----------|---------------|------------|
| 9600 | 104.17 µs | Comunicação lenta |
| 115200 | 8.68 µs | Debug/Console |
| 921600 | 1.09 µs | Alta velocidade |

---

## Periféricos USART/UART no STM32F429

O STM32F429 possui 8 periféricos de comunicação serial:

| Periférico | Barramento | Clock Máx | Características |
|------------|------------|-----------|-----------------|
| USART1 | APB2 | 82 MHz | Síncrono/Assíncrono |
| USART2 | APB1 | 41 MHz | Síncrono/Assíncrono |
| USART3 | APB1 | 41 MHz | Síncrono/Assíncrono |
| UART4 | APB1 | 41 MHz | Apenas Assíncrono |
| UART5 | APB1 | 41 MHz | Apenas Assíncrono |
| USART6 | APB2 | 82 MHz | Síncrono/Assíncrono |
| UART7 | APB1 | 41 MHz | Apenas Assíncrono |
| UART8 | APB1 | 41 MHz | Apenas Assíncrono |

### Por que USART2?

- Está no barramento APB1 (41 MHz) - bom para 115200 baud
- Pinos PA2/PA3 são facilmente acessíveis na NUCLEO
- USART3 é usado pelo ST-Link Virtual COM Port

---

## Escolha dos Pinos

Cada USART pode usar diferentes pinos (função alternativa):

### Opções para USART2

| Função | Pino Opção 1 | Pino Opção 2 | AF |
|--------|--------------|--------------|-----|
| TX | PA2 | PD5 | AF7 |
| RX | PA3 | PD6 | AF7 |
| CK | PA4 | PD7 | AF7 |
| CTS | PA0 | PD3 | AF7 |
| RTS | PA1 | PD4 | AF7 |

### Nossa Escolha: PA2 (TX) e PA3 (RX)

Razões:
- Pinos disponíveis no conector Arduino (CN10)
- Não conflitam com Ethernet ou outros periféricos
- Função alternativa AF7

### Localização na NUCLEO-F429ZI

```
        CN10 (Arduino Header)
    ┌─────────────────────────┐
    │  1  2  3  4  5  6  7  8 │
    │  ○  ○  ○  ○  ○  ○  ○  ○ │
    │                         │
    │  PA3 está no pino D0    │
    │  PA2 está no pino D1    │
    └─────────────────────────┘
```

---

## Configuração dos GPIOs

### Passo 1: Habilitar Clock do GPIOA

```c
#define RCC_AHB1ENR  (*(volatile uint32_t *)(0x40023800UL + 0x30))
#define RCC_AHB1ENR_GPIOAEN  (1U << 0)

/* Habilitar clock do GPIOA */
RCC_AHB1ENR |= RCC_AHB1ENR_GPIOAEN;
```

### Passo 2: Configurar Modo dos Pinos

Os pinos precisam ser configurados como "Função Alternativa" (Alternate Function):

```c
#define GPIOA_MODER  (*(volatile uint32_t *)(0x40020000UL + 0x00))

/* Cada pino usa 2 bits no MODER:
 * 00 = Input
 * 01 = Output
 * 10 = Alternate Function  ← Usamos este
 * 11 = Analog
 */

/* PA2: bits [5:4] = 10 (AF) */
GPIOA_MODER &= ~(3U << (2 * 2));  /* Limpar bits */
GPIOA_MODER |= (2U << (2 * 2));   /* Setar AF */

/* PA3: bits [7:6] = 10 (AF) */
GPIOA_MODER &= ~(3U << (3 * 2));  /* Limpar bits */
GPIOA_MODER |= (2U << (3 * 2));   /* Setar AF */
```

### Passo 3: Configurar Velocidade

Para comunicação serial, velocidade alta é recomendada:

```c
#define GPIOA_OSPEEDR  (*(volatile uint32_t *)(0x40020000UL + 0x08))

/* Cada pino usa 2 bits:
 * 00 = Low speed
 * 01 = Medium speed
 * 10 = Fast speed
 * 11 = High speed  ← Usamos este
 */

/* PA2 e PA3: High speed */
GPIOA_OSPEEDR |= (3U << (2 * 2)) | (3U << (3 * 2));
```

### Passo 4: Configurar Função Alternativa

O STM32F4 tem 16 funções alternativas (AF0-AF15) por pino. USART2 usa **AF7**:

```c
#define GPIOA_AFR0  (*(volatile uint32_t *)(0x40020000UL + 0x20))

/* AFR[0] controla pinos 0-7, cada um com 4 bits:
 * PA2: bits [11:8] = 7 (AF7)
 * PA3: bits [15:12] = 7 (AF7)
 */

/* Limpar e setar AF7 para PA2 e PA3 */
GPIOA_AFR0 &= ~((0xFU << (2 * 4)) | (0xFU << (3 * 4)));
GPIOA_AFR0 |= (7U << (2 * 4)) | (7U << (3 * 4));
```

### Tabela de Funções Alternativas Relevantes

| AF | Função |
|----|--------|
| AF0 | System |
| AF1 | TIM1/TIM2 |
| AF2 | TIM3-5 |
| AF3 | TIM8-11 |
| AF4 | I2C1-3 |
| AF5 | SPI1-6 |
| AF6 | SPI2/3, SAI1 |
| **AF7** | **USART1-3** |
| AF8 | UART4-8, USART6 |
| AF9 | CAN1/2, TIM12-14, LTDC |
| AF10 | OTG, DCMI |
| AF11 | ETH |
| AF12 | FMC, SDIO, OTG |
| AF13 | DCMI |
| AF14 | LTDC |
| AF15 | EVENTOUT |

---

## Configuração da UART

### Passo 1: Habilitar Clock da USART2

```c
#define RCC_APB1ENR  (*(volatile uint32_t *)(0x40023800UL + 0x40))
#define RCC_APB1ENR_USART2EN  (1U << 17)

/* Habilitar clock da USART2 */
RCC_APB1ENR |= RCC_APB1ENR_USART2EN;

/* Pequeno delay para estabilização */
volatile uint32_t dummy = RCC_APB1ENR;
(void)dummy;
```

### Passo 2: Desabilitar USART para Configurar

```c
#define USART2_CR1  (*(volatile uint32_t *)(0x40004400UL + 0x0C))

/* Desabilitar USART (bit UE = 0) */
USART2_CR1 = 0;
```

### Passo 3: Configurar Baud Rate

Ver seção detalhada abaixo.

```c
#define USART2_BRR  (*(volatile uint32_t *)(0x40004400UL + 0x08))

/* BRR = 0x163E para 115200 baud @ 41 MHz APB1 */
USART2_BRR = 0x163E;
```

### Passo 4: Configurar Formato (8N1)

```c
#define USART2_CR2  (*(volatile uint32_t *)(0x40004400UL + 0x10))
#define USART2_CR3  (*(volatile uint32_t *)(0x40004400UL + 0x14))

/* CR2: 1 stop bit (bits 13:12 = 00) */
USART2_CR2 = 0;

/* CR3: Sem controle de fluxo */
USART2_CR3 = 0;
```

### Passo 5: Habilitar TX, RX e USART

```c
#define USART_CR1_UE  (1U << 13)  /* USART Enable */
#define USART_CR1_TE  (1U << 3)   /* Transmitter Enable */
#define USART_CR1_RE  (1U << 2)   /* Receiver Enable */

/* Habilitar transmissor, receptor e USART */
USART2_CR1 = USART_CR1_TE | USART_CR1_RE | USART_CR1_UE;
```

---

## Cálculo do Baud Rate

### Fórmula

O baud rate é configurado pelo registrador BRR (Baud Rate Register):

```
             f_CK
Baud = ─────────────────
       16 × USARTDIV

Onde:
  f_CK = frequência do clock do periférico (APB1 ou APB2)
  USARTDIV = valor programado no BRR
```

Invertendo a fórmula:

```
            f_CK
USARTDIV = ──────────
           16 × Baud
```

### Formato do BRR

O BRR é dividido em duas partes:

```
┌─────────────────────────────────────────────────────┐
│ 31  28 │ 27           16 │ 15         4 │ 3      0 │
├────────┼─────────────────┼──────────────┼──────────┤
│  0000  │    Mantissa     │   Mantissa   │ Fração   │
│        │    (bits altos) │  (bits 11:4) │ (4 bits) │
└─────────────────────────────────────────────────────┘

USARTDIV = Mantissa + (Fração / 16)
```

### Cálculo para 115200 baud @ 41 MHz

```
USARTDIV = 41000000 / (16 × 115200)
USARTDIV = 41000000 / 1843200
USARTDIV = 22.2493...

Parte inteira (Mantissa): 22 = 0x16
Parte fracionária: 0.2493 × 16 = 3.99 ≈ 4 = 0x4

Mas espere! O BRR tem formato especial:
  BRR[15:4] = Mantissa
  BRR[3:0] = Fração
  
Então: BRR = (22 << 4) | 4 = 0x164

Verificação:
USARTDIV = 22 + 4/16 = 22.25
Baud real = 41000000 / (16 × 22.25) = 115168 bps
Erro = (115200 - 115168) / 115200 = 0.028% ✓
```

### Método Alternativo (mais preciso)

```
           f_CK + (Baud / 2)
USARTDIV = ─────────────────
                16 × Baud

USARTDIV = (41000000 + 57600) / 1843200
USARTDIV = 41057600 / 1843200
USARTDIV = 22.28...

BRR = 22.28 × 16 = 356.5 ≈ 357 = 0x165

Ou usando aritmética inteira:
BRR = (41000000 + 921600/2) / 921600 × 16
    = (2 × 41000000 + 115200) / (2 × 115200)
    = 82115200 / 230400
    = 356.42 ≈ 356 = 0x164
```

### Tabela de Valores BRR Comuns

| Baud Rate | APB1 @ 41 MHz | APB2 @ 82 MHz |
|-----------|---------------|---------------|
| 9600 | 0x10A8 | 0x2151 |
| 19200 | 0x0854 | 0x10A8 |
| 38400 | 0x042A | 0x0854 |
| 57600 | 0x02C6 | 0x058D |
| 115200 | 0x0163 | 0x02C6 |
| 230400 | 0x00B2 | 0x0163 |
| 460800 | 0x0059 | 0x00B2 |
| 921600 | 0x002C | 0x0059 |

---

## Redirecionamento do printf

O `printf` da biblioteca C padrão (newlib) chama a função `_write()` para saída de caracteres. Esta, por sua vez, chama `__io_putchar()`.

### Diagrama de Chamadas

```
printf("Hello")
    │
    ▼
_write(fd, buffer, len)   ← Definida em syscalls.c
    │
    ▼
__io_putchar(ch)          ← Implementamos esta
    │
    ▼
uart2SendChar(ch)         ← Envia para UART2
```

### Implementação de __io_putchar

```c
/**
 * @brief Envia um caractere pela UART2.
 * @param ch Caractere a enviar.
 */
void uart2SendChar(char ch)
{
    #define USART2_SR  (*(volatile uint32_t *)(0x40004400UL + 0x00))
    #define USART2_DR  (*(volatile uint32_t *)(0x40004400UL + 0x04))
    #define USART_SR_TXE  (1U << 7)

    /* Aguardar buffer de transmissão estar vazio */
    while ((USART2_SR & USART_SR_TXE) == 0)
    {
        /* Aguardar */
    }

    /* Enviar caractere */
    USART2_DR = (uint32_t)ch;
}

/**
 * @brief Função de saída para printf (newlib).
 * @param ch Caractere a enviar.
 * @return O caractere enviado.
 */
int __io_putchar(int ch)
{
    uart2SendChar((char)ch);
    return ch;
}
```

### Implementação de __io_getchar (para scanf)

```c
/**
 * @brief Recebe um caractere da UART2.
 * @return Caractere recebido.
 */
char uart2ReceiveChar(void)
{
    #define USART_SR_RXNE  (1U << 5)

    /* Aguardar dado disponível */
    while ((USART2_SR & USART_SR_RXNE) == 0)
    {
        /* Aguardar */
    }

    /* Ler caractere */
    return (char)(USART2_DR & 0xFF);
}

/**
 * @brief Função de entrada para scanf (newlib).
 * @return O caractere recebido.
 */
int __io_getchar(void)
{
    return (int)uart2ReceiveChar();
}
```

### Registrador de Status (USART_SR)

| Bit | Nome | Descrição |
|-----|------|-----------|
| 7 | TXE | Transmit Data Register Empty |
| 6 | TC | Transmission Complete |
| 5 | RXNE | Read Data Register Not Empty |
| 4 | IDLE | Idle Line Detected |
| 3 | ORE | Overrun Error |
| 2 | NF | Noise Flag |
| 1 | FE | Framing Error |
| 0 | PE | Parity Error |

---

## Código Completo

```c
/**
 * @brief Inicializa a UART2 para comunicação serial.
 *
 * Configuração: PA2 (TX), PA3 (RX), 115200 baud, 8N1
 */
void uart2Init(void)
{
    /* Registradores */
    #define RCC_AHB1ENR   (*(volatile uint32_t *)0x40023830UL)
    #define RCC_APB1ENR   (*(volatile uint32_t *)0x40023840UL)
    #define GPIOA_MODER   (*(volatile uint32_t *)0x40020000UL)
    #define GPIOA_OSPEEDR (*(volatile uint32_t *)0x40020008UL)
    #define GPIOA_AFR0    (*(volatile uint32_t *)0x40020020UL)
    #define USART2_BRR    (*(volatile uint32_t *)0x40004408UL)
    #define USART2_CR1    (*(volatile uint32_t *)0x4000440CUL)
    #define USART2_CR2    (*(volatile uint32_t *)0x40004410UL)
    #define USART2_CR3    (*(volatile uint32_t *)0x40004414UL)

    /* 1. Habilitar clocks */
    RCC_AHB1ENR |= (1U << 0);   /* GPIOA */
    RCC_APB1ENR |= (1U << 17);  /* USART2 */
    
    /* 2. Configurar PA2 e PA3 como AF */
    GPIOA_MODER &= ~((3U << 4) | (3U << 6));
    GPIOA_MODER |= (2U << 4) | (2U << 6);
    
    /* 3. Velocidade alta */
    GPIOA_OSPEEDR |= (3U << 4) | (3U << 6);
    
    /* 4. AF7 (USART2) */
    GPIOA_AFR0 &= ~((0xFU << 8) | (0xFU << 12));
    GPIOA_AFR0 |= (7U << 8) | (7U << 12);
    
    /* 5. Configurar USART2 */
    USART2_CR1 = 0;              /* Desabilitar para configurar */
    USART2_BRR = 0x0163;         /* 115200 @ 41 MHz */
    USART2_CR2 = 0;              /* 1 stop bit */
    USART2_CR3 = 0;              /* Sem controle de fluxo */
    USART2_CR1 = (1U << 13) |    /* UE - Enable */
                 (1U << 3) |     /* TE - TX Enable */
                 (1U << 2);      /* RE - RX Enable */
}

/**
 * @brief Envia um caractere pela UART2.
 */
void uart2SendChar(char ch)
{
    #define USART2_SR  (*(volatile uint32_t *)0x40004400UL)
    #define USART2_DR  (*(volatile uint32_t *)0x40004404UL)
    
    while ((USART2_SR & (1U << 7)) == 0);  /* Aguardar TXE */
    USART2_DR = (uint32_t)ch;
}

/**
 * @brief Recebe um caractere da UART2.
 */
char uart2ReceiveChar(void)
{
    while ((USART2_SR & (1U << 5)) == 0);  /* Aguardar RXNE */
    return (char)(USART2_DR & 0xFF);
}

/**
 * @brief Função para printf (newlib).
 */
int __io_putchar(int ch)
{
    uart2SendChar((char)ch);
    return ch;
}

/**
 * @brief Função para scanf (newlib).
 */
int __io_getchar(void)
{
    return (int)uart2ReceiveChar();
}
```

---

## Teste e Verificação

### Hardware Necessário

1. **Adaptador USB-Serial** (FTDI, CP2102, CH340, etc.)
2. **Jumpers/Fios** para conexão

### Conexões

```
STM32 NUCLEO          Adaptador USB-Serial
    PA2 (TX) ────────────► RX
    PA3 (RX) ◄──────────── TX
    GND ─────────────────── GND
```

**Importante**: TX do STM32 vai para RX do adaptador e vice-versa!

### Configuração do Terminal

| Parâmetro | Valor |
|-----------|-------|
| Baud rate | 115200 |
| Data bits | 8 |
| Parity | None |
| Stop bits | 1 |
| Flow control | None |

### Softwares de Terminal

**Linux:**
```bash
# Usando screen
screen /dev/ttyUSB0 115200

# Usando minicom
minicom -D /dev/ttyUSB0 -b 115200

# Usando picocom
picocom -b 115200 /dev/ttyUSB0
```

**Windows:**
- PuTTY
- Tera Term
- RealTerm

### Código de Teste

```c
#include <stdio.h>

int main(void)
{
    /* Inicializar hardware */
    systemClockConfig();
    uart2Init();
    
    /* Teste básico */
    printf("=== STM32F429ZI UART Test ===\n");
    printf("SYSCLK: 82 MHz\n");
    printf("APB1: 41 MHz\n");
    printf("Baud: 115200\n\n");
    
    int contador = 0;
    while (1)
    {
        printf("Contador: %d\n", contador++);
        
        /* Delay simples */
        for (volatile int i = 0; i < 1000000; i++);
    }
}
```

### Saída Esperada

```
=== STM32F429ZI UART Test ===
SYSCLK: 82 MHz
APB1: 41 MHz
Baud: 115200

Contador: 0
Contador: 1
Contador: 2
...
```

### Problemas Comuns

| Problema | Causa | Solução |
|----------|-------|---------|
| Caracteres estranhos | Baud rate errado | Verificar cálculo do BRR |
| Nada aparece | TX/RX invertidos | Trocar conexões |
| Funciona às vezes | GND não conectado | Conectar GND |
| Caracteres faltando | Buffer overflow | Usar interrupção ou DMA |

---

## Resumo da Configuração

| Etapa | Registrador | Valor |
|-------|-------------|-------|
| Clock GPIOA | RCC_AHB1ENR | bit 0 |
| Clock USART2 | RCC_APB1ENR | bit 17 |
| PA2 modo AF | GPIOA_MODER | bits [5:4] = 10 |
| PA3 modo AF | GPIOA_MODER | bits [7:6] = 10 |
| PA2 velocidade | GPIOA_OSPEEDR | bits [5:4] = 11 |
| PA3 velocidade | GPIOA_OSPEEDR | bits [7:6] = 11 |
| PA2 AF7 | GPIOA_AFR0 | bits [11:8] = 0111 |
| PA3 AF7 | GPIOA_AFR0 | bits [15:12] = 0111 |
| Baud rate | USART2_BRR | 0x0163 |
| Stop bits | USART2_CR2 | 0 (1 stop) |
| Flow control | USART2_CR3 | 0 (none) |
| Enable | USART2_CR1 | UE + TE + RE |

---

## Referências

- **Reference Manual**: RM0090 - Capítulo 30: Universal synchronous asynchronous receiver transmitter (USART)
- **Datasheet**: DS8626 - Tabela de pinout e funções alternativas
- **Application Note**: AN4655 - USART protocol used in the STM32 bootloader

---

*Desenvolvido por Arvore dos Saberes - https://mcu.tec.br*
