# ⚡ unix

<div align="center">

[![CI](https://github.com/GabrielFrigo4/unix/actions/workflows/ci.yml/badge.svg)](https://github.com/GabrielFrigo4/unix/actions/workflows/ci.yml)
[![License: MIT](https://img.shields.io/badge/License-MIT-blue.svg)](LICENSE)
[![Standards: POSIX.1-2008](https://img.shields.io/badge/Standards-POSIX.1--2008-success.svg)](https://pubs.opengroup.org/onlinepubs/9699919799/)
[![C Standard: C99](https://img.shields.io/badge/C_Standard-C99-informational.svg)](https://en.cppreference.com/w/c/99)

*Suíte minimalista, segura e auditável de ferramentas e utilitários Unix em C puro.*

</div>

---

## 📖 Visão Geral

O projeto **unix** reúne utilitários de sistema projetados sob a filosofia clássica do Unix: ferramentas focadas, com base de código reduzida, livres de dependências externas inchadas, fáceis de auditar e prontas para rodar em ambientes Linux e FreeBSD.

O utilitário inaugural da suíte é o **`msud`** (*Minimal SUID Doas/Sudo*), uma alternativa ultra-leve e segura aos tradicionais executores de privilégios.

---

## 🧩 Utilitários Disponíveis

| Utilitário | Descrição | Linhas de Código | Plataformas |
| :--- | :--- | :--- | :--- |
| [**`msud`**](msud/) | Executor de comandos com privilégio elevado (alternativa a `sudo`/`doas`) | ~180 LOC | Linux, FreeBSD |

---

## 🏛️ Arquitetura do `msud`

```mermaid
sequenceDiagram
    autonumber
    actor User as Usuário
    participant TTY as /dev/tty
    participant Msud as msud (SUID Root)
    participant Shadow as /etc/shadow
    participant Kernel as Linux Kernel

    User->>Msud: Executa: msud <comando> [args...]
    Msud->>Shadow: Valida identidade real e obtém hash
    Msud->>TTY: Desabilita ECHO e solicita senha
    User->>TTY: Digita senha
    TTY-->>Msud: Retorna senha
    Msud->>Msud: crypt() e explicit_bzero(senha)
    Msud->>Msud: Sanitiza variáveis de ambiente (LD_*)
    Msud->>Kernel: initgroups("root", 0) + setuid(0) + setgid(0)
    Msud->>Kernel: execvp(comando, args)
```

---

## 🚀 Instalação e Compilação

### Pré-requisitos
- Compilador C (`gcc` ou `clang`)
- `make` compatível com POSIX
- Biblioteca de criptografia (`libcrypt-dev` em Linux; nativa na `libc` em FreeBSD)

### Compilação Padrão
```bash
# Compilar todos os utilitários da suíte
make all

# Compilar com sanitizers (AddressSanitizer + UndefinedBehaviorSanitizer)
CC=clang make debug
```

### Instalação no Sistema
Para instalar os binários com bit SUID root (`chmod 4755`) e as manpages correspondentes em `/usr/local/bin`:
```bash
sudo make install
```

---

## 🧪 Quality Gates & Ganchos Git (.githooks)

Para habilitar a validação de formatação (`clang-format`), integridade de whitespace e compilação antes de cada commit:

```bash
chmod 0755 .githooks/pre-commit
git config core.hooksPath .githooks
```

Ou simplesmente execute o instalador automatizado:
```bash
./.githooks/install.sh
```

Para rodar a verificação manual de qualidade:
```bash
make check
```

---

## 📚 Documentação Técnica

- 🏛️ [**ARCHITECTURE.md**](docs/ARCHITECTURE.md): Análise de fluxo de privilégios e isolamento de descritores.
- 🔐 [**SECURITY.md**](docs/SECURITY.md): Modelo de ameaças, vetores de injeção e mitigação de LPE.
- 🤖 [**AGENTS.md**](AGENTS.md): Diretrizes para agentes autônomos de IA trabalhando neste repositório.
- 🌐 [**ENVIRONMENT.md**](ENVIRONMENT.md): Requisitos de SO e matriz de compatibilidade.

---

## 📄 Licença

Distribuído sob a licença **MIT**. Veja [LICENSE](LICENSE) para mais detalhes.
