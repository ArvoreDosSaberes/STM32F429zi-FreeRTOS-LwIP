# Tutorial: Configuração do Sistema de Clocks no STM32F429ZI

Este tutorial explica em detalhes como configurar o sistema de clocks do STM32F429ZI para operar a 82 MHz usando o PLL (Phase-Locked Loop) com cristal externo HSE de 8 MHz.

## Índice

1. [Arquitetura de Clocks do STM32F4](#arquitetura-de-clocks-do-stm32f4)
2. [Fontes de Clock Disponíveis](#fontes-de-clock-disponíveis)
3. [Configuração do PLL](#configuração-do-pll)
4. [Cálculo das Frequências](#cálculo-das-frequências)
5. [Implementação Passo a Passo](#implementação-passo-a-passo)
6. [Configuração da Flash](#configuração-da-flash)
7. [Código Completo](#código-completo)
8. [Verificação](#verificação)

---

## Arquitetura de Clocks do STM32F4

O STM32F429ZI possui um sistema de clocks complexo e flexível. Os principais barramentos são:

```
                    ┌─────────────┐
     HSE (8 MHz) ──►│             │
                    │     PLL     │──► SYSCLK (82 MHz)
     HSI (16 MHz)──►│             │        │
                    └─────────────┘        │
                                           ▼
                                    ┌─────────────┐
                                    │  AHB Bus    │──► HCLK (82 MHz)
                                    │  Prescaler  │
                                    └─────────────┘
                                           │
                          ┌────────────────┼────────────────┐
                          │                │                │
                          ▼                ▼                ▼
                    ┌──────────┐    ┌──────────┐    ┌──────────┐
                    │  APB1    │    │  APB2    │    │ Cortex   │
                    │ Prescaler│    │ Prescaler│    │ System   │
                    └──────────┘    └──────────┘    │  Timer   │
                          │                │        └──────────┘
                          ▼                ▼
                    PCLK1 (41 MHz)   PCLK2 (82 MHz)
```

### Limites de Frequência

| Barramento | Máximo | Nossa Config |
|------------|--------|--------------|
| SYSCLK | 180 MHz | 82 MHz |
| AHB (HCLK) | 180 MHz | 82 MHz |
| APB1 (PCLK1) | 45 MHz | 41 MHz |
| APB2 (PCLK2) | 90 MHz | 82 MHz |

---

## Fontes de Clock Disponíveis

O STM32F429 oferece várias fontes de clock:

| Fonte | Frequência | Precisão | Características |
|-------|------------|----------|-----------------|
| **HSI** | 16 MHz | ±1% | Oscilador RC interno, sempre disponível |
| **HSE** | 4-26 MHz | Alta | Cristal externo, maior precisão |
| **LSI** | 32 kHz | ±5% | Oscilador RC interno, baixo consumo |
| **LSE** | 32.768 kHz | Alta | Cristal externo para RTC |
| **PLL** | Até 180 MHz | Depende da fonte | Multiplicador de frequência |

### Por que usar HSE?

- **Precisão**: Cristal de quartzo tem precisão de ppm (partes por milhão)
- **Estabilidade**: Menos variação com temperatura
- **Requisito para USB/Ethernet**: Precisão necessária para protocolos de comunicação

Na placa NUCLEO-F429ZI, há um cristal HSE de **8 MHz**.

---

## Configuração do PLL

O PLL do STM32F4 possui os seguintes parâmetros configuráveis:

### Diagrama do PLL

```
                   PLLM        PLLN          PLLP
HSE (8 MHz) ──►[ ÷ M ]──►[ × N ]──►[ ÷ P ]──► PLLCLK (SYSCLK)
                   │           │
                   │           └────►[ ÷ Q ]──► PLL48CK (USB)
                   │
                   ▼
            VCO_input (1-2 MHz)
                   │
                   │ × N
                   ▼
            VCO_output (100-432 MHz)
```

### Parâmetros

| Parâmetro | Faixa | Nossa Config | Descrição |
|-----------|-------|--------------|-----------|
| **PLLM** | 2-63 | 8 | Divisor de entrada |
| **PLLN** | 50-432 | 328 | Multiplicador VCO |
| **PLLP** | 2,4,6,8 | 4 | Divisor de saída para SYSCLK |
| **PLLQ** | 2-15 | 7 | Divisor para USB (48 MHz) |

### Regras Importantes

1. **VCO_input** = HSE / PLLM deve estar entre **1-2 MHz**
2. **VCO_output** = VCO_input × PLLN deve estar entre **100-432 MHz**
3. **PLLCLK** = VCO_output / PLLP é o clock do sistema

---

## Cálculo das Frequências

### Passo 1: Definir a frequência desejada

- **Objetivo**: SYSCLK = 82 MHz

### Passo 2: Calcular PLLM

Para maximizar a precisão, queremos VCO_input = 1 MHz:

```
VCO_input = HSE / PLLM
1 MHz = 8 MHz / PLLM
PLLM = 8
```

### Passo 3: Calcular PLLN

Com VCO_input = 1 MHz e PLLP = 4:

```
SYSCLK = (HSE × PLLN) / (PLLM × PLLP)
82 MHz = (8 MHz × PLLN) / (8 × 4)
82 MHz = PLLN / 4
PLLN = 328
```

Verificação do VCO_output:
```
VCO_output = VCO_input × PLLN = 1 MHz × 328 = 328 MHz ✓ (dentro de 100-432 MHz)
```

### Passo 4: Verificar PLLQ (opcional, para USB)

```
PLL48CK = VCO_output / PLLQ = 328 MHz / 7 ≈ 46.86 MHz
```
(Não é exatamente 48 MHz, então USB não funcionaria bem, mas não usamos USB neste projeto)

### Resumo Final

| Parâmetro | Valor | Resultado |
|-----------|-------|-----------|
| HSE | 8 MHz | Entrada |
| PLLM | 8 | VCO_input = 1 MHz |
| PLLN | 328 | VCO_output = 328 MHz |
| PLLP | 4 | SYSCLK = 82 MHz |
| AHB Prescaler | 1 | HCLK = 82 MHz |
| APB1 Prescaler | 2 | PCLK1 = 41 MHz |
| APB2 Prescaler | 1 | PCLK2 = 82 MHz |

---

## Implementação Passo a Passo

### Passo 1: Habilitar o HSE

```c
/* Endereço do registrador RCC_CR */
#define RCC_CR  (*(volatile uint32_t *)(0x40023800UL + 0x00))

/* Habilitar HSE */
RCC_CR |= (1U << 16);  /* HSEON bit */

/* Aguardar HSE estabilizar */
while ((RCC_CR & (1U << 17)) == 0)  /* HSERDY bit */
{
    /* Aguardar */
}
```

**Explicação:**
- Bit 16 (HSEON): Liga o oscilador HSE
- Bit 17 (HSERDY): Indica quando HSE está estável (leva alguns ms)

### Passo 2: Configurar a Flash

A Flash precisa de wait states quando o clock aumenta:

| HCLK | Wait States | VOS |
|------|-------------|-----|
| ≤30 MHz | 0 WS | - |
| ≤60 MHz | 1 WS | - |
| ≤90 MHz | 2 WS | Scale 2 |
| ≤120 MHz | 3 WS | Scale 2 |
| ≤150 MHz | 4 WS | Scale 1 |
| ≤180 MHz | 5 WS | Scale 1 |

Para 82 MHz, precisamos de **2 wait states**:

```c
#define FLASH_ACR  (*(volatile uint32_t *)(0x40023C00UL + 0x00))

/* Configurar Flash: 2 WS + Prefetch + Caches */
FLASH_ACR = (2U << 0) |   /* LATENCY = 2 wait states */
            (1U << 8) |   /* PRFTEN - Prefetch enable */
            (1U << 9) |   /* ICEN - Instruction cache enable */
            (1U << 10);   /* DCEN - Data cache enable */
```

### Passo 3: Desabilitar o PLL

O PLL não pode ser configurado enquanto está ligado:

```c
/* Desabilitar PLL */
RCC_CR &= ~(1U << 24);  /* PLLON bit */

/* Aguardar PLL desligar */
while ((RCC_CR & (1U << 25)) != 0)  /* PLLRDY bit */
{
    /* Aguardar */
}
```

### Passo 4: Configurar o PLL

```c
#define RCC_PLLCFGR  (*(volatile uint32_t *)(0x40023800UL + 0x04))

/* Configurar PLLCFGR:
 * Bits 0-5:   PLLM = 8
 * Bits 6-14:  PLLN = 328
 * Bits 16-17: PLLP = 4 (valor 01)
 * Bit 22:     PLLSRC = HSE
 * Bits 24-27: PLLQ = 7
 */
RCC_PLLCFGR = (8U << 0) |      /* PLLM = 8 */
              (328U << 6) |    /* PLLN = 328 */
              (1U << 16) |     /* PLLP = 4 (valor 01: 00=2, 01=4, 10=6, 11=8) */
              (1U << 22) |     /* PLLSRC = HSE */
              (7U << 24);      /* PLLQ = 7 */
```

**Codificação do PLLP:**
| Valor | Código |
|-------|--------|
| 2 | 00 |
| 4 | 01 |
| 6 | 10 |
| 8 | 11 |

### Passo 5: Habilitar o PLL

```c
/* Habilitar PLL */
RCC_CR |= (1U << 24);  /* PLLON bit */

/* Aguardar PLL estabilizar */
while ((RCC_CR & (1U << 25)) == 0)  /* PLLRDY bit */
{
    /* Aguardar */
}
```

### Passo 6: Configurar os Prescalers

```c
#define RCC_CFGR  (*(volatile uint32_t *)(0x40023800UL + 0x08))

/* Configurar prescalers:
 * Bits 4-7:   HPRE  = 0 (AHB não dividido, /1)
 * Bits 10-12: PPRE1 = 4 (APB1 dividido por 2)
 * Bits 13-15: PPRE2 = 0 (APB2 não dividido, /1)
 */
RCC_CFGR = (0U << 4) |   /* HPRE = /1 */
           (4U << 10) |  /* PPRE1 = /2 (código 100) */
           (0U << 13);   /* PPRE2 = /1 */
```

**Codificação dos Prescalers:**

| Divisor | HPRE | PPRE1/PPRE2 |
|---------|------|-------------|
| /1 | 0xxx | 0xx |
| /2 | 1000 | 100 |
| /4 | 1001 | 101 |
| /8 | 1010 | 110 |
| /16 | 1011 | 111 |

### Passo 7: Selecionar PLL como fonte de SYSCLK

```c
/* Selecionar PLL como fonte de clock */
RCC_CFGR |= (2U << 0);  /* SW = PLL */

/* Aguardar PLL ser selecionado */
while ((RCC_CFGR & (3U << 2)) != (2U << 2))  /* SWS bits */
{
    /* Aguardar */
}
```

**Codificação de SW/SWS:**
| Fonte | Código |
|-------|--------|
| HSI | 00 |
| HSE | 01 |
| PLL | 10 |

---

## Configuração da Flash

A Flash possui recursos adicionais para melhorar a performance:

### Wait States

A CPU é mais rápida que a Flash. Wait states fazem a CPU esperar:

```
CPU Clock: 82 MHz → Período: 12.2 ns
Flash Access: ~30 ns → Precisa de 2-3 ciclos de CPU
```

### Prefetch

O prefetch buffer antecipa a leitura de instruções:

```
Sem Prefetch:     [Read][Wait][Wait][Execute][Read][Wait][Wait][Execute]...
Com Prefetch:     [Read][Execute][Read][Execute][Read][Execute]...
                        ↑ Overlap - enquanto executa, já lê a próxima
```

### Caches

- **Instruction Cache (I-Cache)**: Armazena instruções recentes
- **Data Cache (D-Cache)**: Armazena dados recentes

```c
FLASH_ACR = (2U << 0) |   /* 2 wait states para 82 MHz */
            (1U << 8) |   /* Prefetch enable */
            (1U << 9) |   /* I-Cache enable */
            (1U << 10);   /* D-Cache enable */
```

---

## Código Completo

```c
/**
 * @brief Configura o sistema de clocks para 82 MHz.
 */
void systemClockConfig(void)
{
    /* Definições de registradores */
    #define RCC_CR      (*(volatile uint32_t *)(0x40023800UL + 0x00))
    #define RCC_PLLCFGR (*(volatile uint32_t *)(0x40023800UL + 0x04))
    #define RCC_CFGR    (*(volatile uint32_t *)(0x40023800UL + 0x08))
    #define FLASH_ACR   (*(volatile uint32_t *)(0x40023C00UL + 0x00))

    /* 1. Habilitar HSE */
    RCC_CR |= (1U << 16);
    while ((RCC_CR & (1U << 17)) == 0);

    /* 2. Configurar Flash (2 wait states + caches) */
    FLASH_ACR = (2U << 0) | (1U << 8) | (1U << 9) | (1U << 10);

    /* 3. Desabilitar PLL para configurar */
    RCC_CR &= ~(1U << 24);
    while ((RCC_CR & (1U << 25)) != 0);

    /* 4. Configurar PLL: M=8, N=328, P=4, Q=7, SRC=HSE */
    RCC_PLLCFGR = (8U << 0) | (328U << 6) | (1U << 16) | 
                  (1U << 22) | (7U << 24);

    /* 5. Habilitar PLL */
    RCC_CR |= (1U << 24);
    while ((RCC_CR & (1U << 25)) == 0);

    /* 6. Configurar prescalers: AHB/1, APB1/2, APB2/1 */
    RCC_CFGR = (0U << 4) | (4U << 10) | (0U << 13);

    /* 7. Selecionar PLL como fonte */
    RCC_CFGR |= (2U << 0);
    while ((RCC_CFGR & (3U << 2)) != (2U << 2));
}
```

---

## Verificação

### Usando Debugger

Com GDB, você pode verificar os registradores:

```gdb
# Ler RCC_CR
x/1wx 0x40023800

# Ler RCC_CFGR
x/1wx 0x40023808

# Verificar SWS (bits 3:2) - deve ser 10 (PLL)
```

### Usando GPIO Toggle

Uma forma prática de verificar o clock é fazer um LED piscar com delay conhecido:

```c
/* Se SYSCLK = 82 MHz, este loop leva ~1 segundo */
for (volatile uint32_t i = 0; i < 10250000; i++);
```

### Verificando com Osciloscópio

Configure MCO1 (PA8) ou MCO2 (PC9) para saída de clock:

```c
/* MCO1 = SYSCLK/4 = 20.5 MHz em PA8 */
RCC_CFGR |= (3U << 21) | (6U << 24);  /* MCO1 = SYSCLK, PRE = /4 */
```

---

## Resumo

| Passo | Ação | Registrador |
|-------|------|-------------|
| 1 | Ligar HSE | RCC_CR bit 16 |
| 2 | Aguardar HSE | RCC_CR bit 17 |
| 3 | Configurar Flash | FLASH_ACR |
| 4 | Desligar PLL | RCC_CR bit 24 |
| 5 | Configurar PLL | RCC_PLLCFGR |
| 6 | Ligar PLL | RCC_CR bit 24 |
| 7 | Aguardar PLL | RCC_CR bit 25 |
| 8 | Configurar prescalers | RCC_CFGR bits 4-15 |
| 9 | Selecionar PLL | RCC_CFGR bits 0-1 |
| 10 | Aguardar seleção | RCC_CFGR bits 2-3 |

---

## Referências

- **Reference Manual**: RM0090 (STM32F405/415/407/417/427/437/429/439)
- **Datasheet**: DS8626 (STM32F427xx e STM32F429xx)
- Seções relevantes:
  - Capítulo 6: Reset and Clock Control (RCC)
  - Capítulo 3: Embedded Flash memory interface

---

*Desenvolvido por Arvore dos Saberes - https://mcu.tec.br*
