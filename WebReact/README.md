# WebReact - Frontend para STM32F429ZI HTTP Server

![Visitors](https://visitor-badge.laobi.icu/badge?page_id=ArvoreDosSaberes.STM32F429zi-FreeRTOS-LwIP.WebReact)
[![Subproject](https://img.shields.io/badge/Subproject-WebReact%20Frontend-orange.svg)](../README.md)
[![License: CC BY 4.0](https://img.shields.io/badge/License-CC%20BY%204.0-lightgrey.svg)](https://creativecommons.org/licenses/by/4.0/)
[![React](https://img.shields.io/badge/React-18-blue.svg)](https://react.dev/)
[![Vite](https://img.shields.io/badge/Vite-5-purple.svg)](https://vitejs.dev/)
[![TypeScript](https://img.shields.io/badge/TypeScript-5-blue.svg)](https://www.typescriptlang.org/)
[![React Router DOM](https://img.shields.io/badge/React%20Router%20DOM-6-blueviolet.svg)](https://reactrouter.com/)
[![Target STM32F429ZI](https://img.shields.io/badge/Target-STM32F429ZI-green.svg)](https://www.st.com/en/microcontrollers-microprocessors/stm32f429zi.html)
[![GitHub Stars](https://img.shields.io/github/stars/ArvoreDosSaberes/STM32F429zi-FreeRTOS-LwIP?style=social)](https://github.com/ArvoreDosSaberes/STM32F429zi-FreeRTOS-LwIP)
[![GitHub Issues](https://img.shields.io/github/issues/ArvoreDosSaberes/STM32F429zi-FreeRTOS-LwIP)](https://github.com/ArvoreDosSaberes/STM32F429zi-FreeRTOS-LwIP/issues)

Este diretrio contém o **frontend WebReact** do projeto
`STM32-F429zi-http - FreeRTOS + LwIP HTTP Server`.

Ele fornece uma **landing page moderna e responsiva**, construída com **React + Vite + TypeScript**, para
apresentar o template STM32F429ZI e facilitar demonstrações, vídeos e materiais didáticos.

> Este subprojeto é **frontend**. O firmware e o servidor HTTP embarcado estão documentados no
> [`README.md` da raiz](../README.md) e em `docs/`.

---

## Visão Geral

- **Objetivo**: oferecer uma interface web amigável para o template STM32F429ZI
  (FreeRTOS + LwIP HTTP Server), com foco em **didática** e **apresentação visual**.
- **Arquitetura**: SPA em React, empacotada com Vite, utilizando React Router DOM para navegação.
- **Uso típico**:
  - Rodar localmente em desenvolvimento enquanto o firmware roda na placa;
  - Gerar um build estático (`dist/`) para servir em outro servidor web ou como base
    para páginas embarcadas no firmware.

---

## Stack Tecnológica

| Camada        | Tecnologia            | Detalhes                                          |
| ------------- | --------------------- | ------------------------------------------------- |
| Build         | Vite 5                | Dev server rápido e empacotamento para produção  |
| UI            | React 18              | Componentização e SPA                             |
| Linguagem     | TypeScript 5          | Tipagem estática para maior segurança             |
| Roteamento    | React Router DOM 6    | Rotas `/` (home) e `*` (404)                      |

Principais arquivos:

- `src/main.tsx`: ponto de entrada, integra React, BrowserRouter e estilos globais.
- `src/App.tsx`: definição das rotas principais.
- `src/pages/HomePage.tsx`: página inicial com descrição do projeto STM32F429ZI.
- `src/pages/NotFoundPage.tsx`: página de erro 404 amigável.
- `src/components/Butterfly.tsx`: animação decorativa em CSS.
- `src/styles/*.css`: estilos da interface (`global`, `home`, `notFound`, `butterfly`, etc.).

---

## Estrutura de Pastas

```text
WebReact/
  .env.example          # Modelo de variáveis de ambiente do frontend
  index.html            # HTML base usado pelo Vite
  package.json          # Metadados e scripts npm
  tsconfig*.json        # Configuração TypeScript
  vite.config.ts        # Configuração do Vite

  src/
    main.tsx           # Ponto de entrada React
    App.tsx            # Definição das rotas

    components/
      STLogo.tsx      # Logo estilizado da ST (componente React)
      Butterfly.tsx   # Animação da borboleta em CSS

    pages/
      HomePage.tsx    # Página inicial (sobre o template STM32)
      NotFoundPage.tsx# Página 404 (rota coringa)

    styles/
      global.css      # Estilos globais
      home.css        # Layout da home
      notFound.css    # Layout da página 404
      butterfly.css   # Animação da borboleta
```

---

## Tutorial do Projeto (Passo a Passo)

### 1. Pré-requisitos

- **Sistema operacional**: Linux (tutorial pensado em ambiente Linux, mas funciona em outros SOs).
- **Node.js**: versão LTS recomendada (>= 18).
- **npm**: normalmente instalado junto com o Node.
- Repositório `STM32F429zi-FreeRTOS-LwIP` já clonado.

> Caso ainda não tenha o repositório:
>
> ```bash
> git clone https://github.com/ArvoreDosSaberes/STM32F429zi-FreeRTOS-LwIP.git
> cd STM32F429zi-FreeRTOS-LwIP/WebReact
> ```

---

### 2. Configurar variáveis de ambiente

O projeto utiliza um arquivo `.env` específico para o frontend. Um modelo é fornecido em
`.env.example`.

1. Na pasta `WebReact/`, copie o modelo:

   ```bash
   cp .env.example .env
   ```

2. Edite o arquivo `.env` conforme necessário:

   - `VITE_APP_NAME`: nome que será usado pela aplicação (ex.: `STM32F429ZI-WebReact`).

> Por padrão, o `.env` **não é versionado** (segue as boas práticas de configuração),
> e o `.env.example` serve como referência de quais chaves devem existir.

---

### 3. Instalar dependências

Ainda na pasta `WebReact/`:

```bash
npm install
```

Isso irá instalar todas as dependências listadas em `package.json`, incluindo React, Vite
e React Router DOM.

---

### 4. Rodar em modo desenvolvimento

Na pasta `WebReact/` execute:

```bash
npm run dev
```

- O Vite iniciará um servidor de desenvolvimento (por padrão em `http://localhost:5173`).
- Abra o navegador nesse endereço para visualizar a interface.
- A cada alteração em arquivos `tsx` ou `css`, a página será atualizada automaticamente
  (hot reload).

Se desejar alterar a porta ou outros parâmetros, consulte a documentação do Vite ou ajuste
o `vite.config.ts` conforme necessário.

---

### 5. Gerar build de produção

Para gerar a versão otimizada para produção:

```bash
npm run build
```

Ou, se preferir garantir uma instalação limpa antes da build:

```bash
npm run build:install
```

Após a conclusão, os arquivos estáticos serão gerados na pasta `dist/`.

Esses arquivos podem ser:

- Servidos por qualquer servidor HTTP (Nginx, Apache, etc.);
- Usados como base para migrar o layout para páginas embarcadas no servidor HTTP do STM32.

---

### 6. Scripts disponíveis (`package.json`)

Na raiz de `WebReact/`:

| Script             | Comando            | Descrição                                               |
| ------------------ | ------------------ | ------------------------------------------------------- |
| `dev`              | `npm run dev`      | Inicia o servidor de desenvolvimento Vite               |
| `build`            | `npm run build`    | Gera o build otimizado em `dist/`                       |
| `preview`          | `npm run preview`  | Sobe um servidor local para pré-visualizar o build      |
| `build:install`    | `npm run build:install` | Roda `npm install` e em seguida `npm run build`   |

O comando `preview` é útil para validar a build de produção localmente, sem precisar subir
outro servidor web.

---

### 7. Personalizando a interface

Alguns pontos de entrada úteis para customização:

- **Textos e conteúdo principal**:
  - `src/pages/HomePage.tsx`: altere títulos, parágrafos e links (GitHub, site, YouTube).
- **Página 404**:
  - `src/pages/NotFoundPage.tsx`: personalize a mensagem de erro e o botão de retorno.
- **Estilos globais**:
  - `src/styles/global.css`: define fonte, gradiente de fundo e layout base.
- **Layout da home**:
  - `src/styles/home.css`: controla cards, botões e organização geral da página inicial.
- **Animação da borboleta**:
  - `src/styles/butterfly.css`: ajusta cores, tamanho e animação do componente `Butterfly`.

Recomenda-se realizar alterações iterativas, sempre testando em modo desenvolvimento
(`npm run dev`) para validar o impacto visual.

---

### 8. Relação com o projeto principal

- Este diretório `WebReact/` é um **subprojeto frontend** dentro do repositório principal.
- O firmware, FreeRTOS, LwIP e o servidor HTTP embarcado continuam sendo desenvolvidos na raiz
  do repositório e documentados em `README.md` e `docs/`.
- O WebReact serve como:
  - Ferramenta de demonstração visual do template STM32;
  - Base para criação de páginas mais ricas que podem inspirar (ou ser adaptadas para)
    versões embarcadas no microcontrolador.

Para detalhes de compilação do firmware, debug com OpenOCD/GDB e características do servidor
HTTP embarcado, consulte o [`README.md` principal](../README.md) e os arquivos em `docs/`.

---

Desenvolvido com ❤ por [Carlos Delfino](https://github.com/carlosdelfino)

