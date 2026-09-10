# 🏛️ Unix Environment & Baseline

Especificação do ambiente operacional, dependências de sistema e matriz de suporte para a suíte de ferramentas **unix**.

---

## 🎯 Plataformas Alvo

| Sistema Operacional | Suporte | Mecanismo de Autenticação | Dependência Cripto |
| :--- | :--- | :--- | :--- |
| **Linux (glibc / musl)** | Tier 1 (Nativo) | `/etc/shadow` via `<shadow.h>` | `-lcrypt` (libcrypt / libxcrypt) |
| **FreeBSD** | Tier 1 (Nativo) | `/etc/master.passwd` via `<pwd.h>` | Integrado na `libc` base |
| **OpenBSD** | Tier 2 | `/etc/master.passwd` via `<pwd.h>` | Integrado na `libc` base |

---

## ⚙️ Ferramental de Desenvolvimento

- **Compilador C:** `gcc` (>= 9) ou `clang` (>= 10)
- **Make:** GNU Make ou BSD Make (Makefiles escritos em conformidade POSIX)
- **Formatador:** `clang-format` (>= 14)
- **CI:** GitHub Actions rodando `ubuntu-latest`
- **Controle de Versão:** Git 2.20+ com suporte a `core.hooksPath`
