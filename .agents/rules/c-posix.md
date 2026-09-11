# Diretrizes C e POSIX

> Regras de codificação para os utilitários C no repositório unix.

---

## 1. Padrões de Linguagem e Compilador

- Todo código C deve aderir a **C99 / POSIX.1-2008**.
- Sempre declarar `#define _DEFAULT_SOURCE` e `#define _XOPEN_SOURCE 700` no topo dos arquivos C para portabilidade entre glibc, musl e BSD libc.
- Proibido o uso de extensões GNU não-padrão a menos que encapsuladas em diretivas de pré-processador específicas (`#if defined(...)`).
- O código deve compilar sem nenhum aviso sob:
    ```bash
    CFLAGS="-Wall -Wextra -Werror -pedantic -std=c99"
    ```

---

## 2. Gerenciamento de Memória e Buffers

- Evitar alocações dinâmicas na heap (`malloc`, `calloc`) em ferramentas críticas de sistema quando buffers com tamanhos fixos conhecidos na stack forem suficientes.
- Sempre delimitar manipulação de strings com buffers explicitamente limitados (`snprintf`, nunca `sprintf`; verificar limites em loops de leitura).
- Em buffers que recebam senhas, chaves ou tokens, SEMPRE limpar a memória imediatamente após o uso com `explicit_bzero(ptr, len)`.

---

## 3. Portabilidade Multiplataforma

- **Linux vs FreeBSD**:
    - Em Linux, senhas de usuários comuns residem em `/etc/shadow` e necessitam de `<shadow.h>` e `getspnam()`.
    - Em FreeBSD/OpenBSD, senhas residem em `/etc/master.passwd` acessíveis via `getpwuid()` quando executado como root.
    - A biblioteca de criptografia (`-lcrypt`) deve ser condicional: necessária na maioria dos ambientes Linux (glibc/libxcrypt) e inclusa na `libc` base dos BSDs.

---

## 4. Princípios de Clean Code

- **Código Autoexplicativo (Zero Comentários Redundantes):** O código deve expressar sua intenção diretamente através de boas estruturas e nomes claros. Comentários óbvios que apenas parafraseiam a instrução seguinte são expressamente proibidos.
- **Funções Pequenas com Responsabilidade Única (SRP):** Cada função deve realizar apenas uma tarefa lógica bem definida (ex: checar grupo, sanitizar ambiente, configurar sinais, ler terminal).
- **Mesmo Nível de Abstração (SLAP):** A função `main()` deve orquestrar as operações em alto nível, lendo-se como um índice narrativo limpo e sem lógica profunda aninhada.
- **Cláusulas de Guarda (Early Return):** Falhar rápido (_fail-fast_). Evitar cascateamento profundo de `if/else`, priorizando retornos ou encerramentos imediatos em caso de erro.
- **Higiene de Dependências e Código Morto:** Remover `#include` órfãos, variáveis não utilizadas e blocos condicionais vazios.

---

## 5. Idioma e Nomenclatura Estrita em Inglês

- **Identificadores em Inglês:** Todos os nomes de funções, variáveis, constantes, macros, structs e typedefs DEVEM ser estritamente em língua inglesa (ex: `is_authorized_group`, `sanitize_environment`, `assume_root`, `authenticate_user`, `target_gid`).
- **Interfaces e Mensagens de Terminal em Inglês:** Textos de uso (`Usage: ...`), mensagens de erro em `stderr` e prompts de terminal DEVEM ser redigidos em inglês, garantindo interoperabilidade e alinhamento com padrões POSIX e manpages.
