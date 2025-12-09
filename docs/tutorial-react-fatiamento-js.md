# Fatiamento de JavaScript no React (Vite) para arquivos entre 30 e 50 KB

## 1. Contexto do projeto

Este projeto usa **React + TypeScript** com **Vite** (veja `vite.config.ts`). O empacotamento é feito pelo **Rollup** (via Vite), que gera um **bundle inicial** e vários **chunks** (arquivos JS menores) carregados sob demanda.

Trecho relevante do `vite.config.ts` atual:

```ts
import { defineConfig } from 'vite';
import react from '@vitejs/plugin-react-swc';

export default defineConfig({
  plugins: [react()],
  root: '.',
  build: {
    outDir: 'dist',
    chunkSizeWarningLimit: 50,
    rollupOptions: {
      output: {
        manualChunks(id) {
          if (id.includes('node_modules')) {
            if (id.includes('react') || id.includes('react-dom')) {
              return 'react-vendor';
            }
            if (id.includes('react-router') || id.includes('react-router-dom')) {
              return 'router-vendor';
            }
            return 'vendor';
          }
          return undefined;
        },
        entryFileNames: 'assets/[name]-[hash].js',
        chunkFileNames: 'assets/[name]-[hash].js',
        assetFileNames: 'assets/[name]-[hash][extname]',
      },
    },
  },
});
```

Esse arquivo já está preparado para separar alguns **vendors** (React, React Router, etc.) em chunks diferentes.

---

## 2. Conceitos básicos de fatiamento (code splitting)

1. **Bundle**: arquivo JS que contém várias partes da aplicação.
2. **Chunk**: pedaço do bundle que pode ser **carregado separadamente**, sob demanda.
3. **Code splitting**: técnica para dividir o código em vários chunks menores, carregados apenas quando necessários.
4. **Lazy loading**: carregamento "preguiçoso" de componentes/rotas apenas quando forem realmente usados.

Objetivo: manter os arquivos JS **individualmente entre ~30 e 50 KB**, para reduzir:

- **Tempo de download inicial**.
- **Tempo de parse/execução de JS**.
- Consumo de memória em dispositivos mais limitados.

Importante: não existe configuração que **garanta** exatamente 30–50 KB por arquivo, mas conseguimos **aproximar** esse tamanho:

- Dividindo bem as responsabilidades.
- Extraindo dependências grandes em chunks separados.
- Usando `React.lazy` / `Suspense` / `dynamic import()`.

---

## 3. Estratégia geral para este projeto

Para o seu projeto com Vite, vamos combinar três camadas de fatiamento:

1. **Fatiamento de vendors (já parcialmente configurado)**
   - React / ReactDOM em `react-vendor`.
   - React Router em `router-vendor`.
   - Demais bibliotecas em `vendor`.

2. **Fatiamento por página/rota** (route-based splitting)
   - Cada página principal vira um chunk carregado sob demanda.

3. **Fatiamento de componentes pesados**
   - Componentes grandes ou que usam bibliotecas pesadas (gráficos, mapas, etc.) são carregados de forma lazy.

---

## 4. Ajustando o Vite para ajudar no fatiamento

### 4.1. `chunkSizeWarningLimit`

Hoje você já tem:

```ts
build: {
  outDir: 'dist',
  chunkSizeWarningLimit: 50,
  // ...
},
```

- Isso **não limita** o tamanho, mas emite **warnings** quando um chunk ultrapassa 50 KB.
- Use esses warnings para identificar arquivos que precisam ser melhor fatiados.

### 4.2. `manualChunks` para vendors

Trecho existente:

```ts
manualChunks(id) {
  if (id.includes('node_modules')) {
    if (id.includes('react') || id.includes('react-dom')) {
      return 'react-vendor';
    }
    if (id.includes('react-router') || id.includes('react-router-dom')) {
      return 'router-vendor';
    }
    return 'vendor';
  }
  return undefined;
},
```

O que isso faz:

- **`react-vendor`**: Tudo que for `react` ou `react-dom` cai nesse chunk.
- **`router-vendor`**: Tudo que for `react-router` / `react-router-dom` cai nesse chunk.
- **`vendor`**: Todas as outras libs de `node_modules`.

Benefícios:

- O bundle inicial da aplicação **não precisa carregar todas as libs**.
- Chunks de vendor são **cacheáveis** pelo navegador entre deploys (apenas o hash muda quando há alteração real).

### 4.3. Ajustando `manualChunks` para mais granularidade

Se precisar dividir ainda mais as dependências, você pode especializar trechos, por exemplo para uma lib de gráficos hipotética:

```ts
manualChunks(id) {
  if (id.includes('node_modules')) {
    if (id.includes('react') || id.includes('react-dom')) {
      return 'react-vendor';
    }
    if (id.includes('react-router') || id.includes('react-router-dom')) {
      return 'router-vendor';
    }
    if (id.includes('alguma-lib-grafico')) {
      return 'charts-vendor';
    }
    return 'vendor';
  }
  return undefined;
}
```

Use essa abordagem apenas quando identificar, nos relatórios de build, que um vendor específico está muito grande.

---

## 5. Fatiamento por página/rota (route-based splitting)

Hoje o `App.tsx` importa as páginas de forma estática:

```ts
import HomePage from './pages/HomePage';
import NotFoundPage from './pages/NotFoundPage';

const App: React.FC = () => {
  return (
    <Routes>
      <Route path="/" element={<HomePage />} />
      <Route path="*" element={<NotFoundPage />} />
    </Routes>
  );
};
```

Isso significa que o bundle principal contém **toda a lógica** das páginas.

### 5.1. Usando `React.lazy` + `Suspense`

Passos:

1. Trocar imports estáticos por imports lazy.
2. Envolver as rotas em um `Suspense` com fallback leve.

Exemplo conceitual (não é necessário copiar e colar, é para entendimento):

```ts
import React, { Suspense, lazy } from 'react';
import { Routes, Route } from 'react-router-dom';

const HomePage = lazy(() => import('./pages/HomePage'));
const NotFoundPage = lazy(() => import('./pages/NotFoundPage'));

const App: React.FC = () => {
  return (
    <Suspense fallback={<div>Carregando...</div>}>
      <Routes>
        <Route path="/" element={<HomePage />} />
        <Route path="*" element={<NotFoundPage />} />
      </Routes>
    </Suspense>
  );
};
```

O que muda no build:

- Cada `import('./pages/AlgumaPagina')` vira **um chunk separado**.
- O bundle inicial fica menor, pois as páginas só são carregadas quando acessadas.

### 5.2. Benefício para a meta de 30–50 KB

- Se cada página tiver apenas o código que realmente precisa (sem dependências gigantes), os chunks gerados tenderão a ficar próximos da faixa desejada.
- O passo seguinte é **medir** o tamanho dos arquivos gerados (ver seção 8).

---

## 6. Fatiamento de componentes pesados

Além das páginas, componentes específicos podem ser grandes, por exemplo:

- Tabelas com muitas funcionalidades.
- Gráficos.
- Editores ricos de texto.
- Visualização de mapas.

### 6.1. Quando aplicar

Use `React.lazy` para componentes que:

- Não aparecem imediatamente no primeiro paint.
- Estão atrás de abas, modais, botões de ação, etc.

Exemplo conceitual:

```ts
import React, { Suspense, lazy } from 'react';

const HeavyChart = lazy(() => import('../components/HeavyChart'));

const Dashboard: React.FC = () => {
  return (
    <div>
      <h1>Dashboard</h1>
      <Suspense fallback={<div>Carregando gráfico...</div>}>
        <HeavyChart />
      </Suspense>
    </div>
  );
};
```

Resultado no build:

- `HeavyChart` vira um **chunk separado**, carregado apenas quando o usuário chega na tela correspondente.

---

## 7. Boas práticas para manter chunks pequenos

1. **Separar responsabilidades por arquivo e pasta**
   - Evitar arquivos enormes com múltiplos componentes grandes.

2. **Evitar imports globais desnecessários**
   - Só importar o que realmente é usado naquele arquivo.

3. **Extrair utilitários compartilhados**
   - Funções reutilizadas podem ir para um módulo comum, que o bundler pode reusar em vários chunks.

4. **Monitorar dependências pesadas**
   - Antes de adicionar uma nova biblioteca, verificar tamanho aproximado.

5. **Preferir componentes funcionais simples na Home**
   - A Home costuma ser a primeira tela carregada; mantê-la leve ajuda bastante o tempo de carregamento.

No seu caso, o `HomePage.tsx` já é relativamente enxuto, focando em layout, logo e telemetria de carregamento.

---

## 8. Como medir o tamanho dos chunks

### 8.1. Build de produção

1. Rodar o build de produção (no diretório `WebReact`):

   ```bash
   npm run build
   ```

2. Verificar a saída do Vite:
   - Ele lista os arquivos gerados em `dist/assets` com o tamanho.
   - Compare esses tamanhos com a meta de **30–50 KB**.

### 8.2. Inspecionar arquivos em `dist`

- Acesse `dist/assets` e veja os arquivos `*.js`.
- Verifique quais arquivos são maiores que 50 KB.
- Use os nomes (e os comentários no início do arquivo) para entender que partes da aplicação estão contribuindo para o tamanho.

### 8.3. Ajustes iterativos

Caso algum arquivo continue muito grande:

1. Identificar a página/componente que gerou o chunk.
2. Revisar se é possível aplicar `React.lazy` em partes internas.
3. Avaliar extração de vendors específicos via `manualChunks`.

---

## 9. Receita resumida para manter chunks entre 30–50 KB

1. **Configuração do Vite (já aplicada, revisar apenas se necessário)**
   - Manter `chunkSizeWarningLimit: 50`.
   - Usar `manualChunks` para separar `react-vendor`, `router-vendor` e `vendor`.

2. **Fatiamento por página/rota**
   - Usar `React.lazy` + `Suspense` para importar páginas (`HomePage`, etc.) de forma dinâmica.

3. **Fatiamento de componentes pesados**
   - Usar `React.lazy` + `Suspense` para componentes que não precisam estar no primeiro paint.

4. **Medir e ajustar**
   - Rodar `npm run build` e monitorar tamanhos em `dist/assets`.
   - Se algum arquivo ultrapassar **50 KB**, dividir mais a página/componente ou isolar vendors específicos.

---

## 10. Próximos passos sugeridos para este projeto

1. **Aplicar `React.lazy` nas páginas**
   - Transformar `HomePage` e `NotFoundPage` em imports dinâmicos.

2. **Mapear componentes candidatos a lazy loading**
   - Identificar futuros componentes pesados (gráficos, dashboards, etc.) e já planejá-los como lazy.

3. **Documentar decisões em `docs/architecture.md`**
   - Registrar que o projeto adota fatiamento agressivo visando chunks de 30–50 KB, por rodar em dispositivos possivelmente limitados (como o contexto de sistemas embarcados).

Seguindo esses passos, você terá um bundle inicial bem enxuto e vários chunks pequenos, respeitando o objetivo de manter os arquivos JS na faixa de 30–50 KB sempre que possível.
