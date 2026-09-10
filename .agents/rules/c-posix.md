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
