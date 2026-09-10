# ⚡ Unix — AI Agent Briefing

> Suíte de utilitários Unix minimalistas, seguros e estritamente aderentes ao padrão POSIX em linguagem C.

---

## 🧭 Identidade e Papel

O repositório **unix** reúne implementações limpas de ferramentas de sistema, concebidas sob a filosofia Unix: ferramentas pequenas, focadas, sem dependências externas desnecessárias, com código auditável e segurança em primeiro plano.

A suíte disponibiliza dois executores de privilégios SUID:
- **`rtdo`** (*Root Do*): Executor com verificação de senha interativa via `/dev/tty`.
- **`rtgo`** (*Root Go*): Executor imediato sem senha para membros autorizados.

---

## ⚠️ Regras Críticas para Agentes de IA

1. **Conformidade POSIX e Padrão C99:** Todo código C deve compilar sem avisos com `cc -Wall -Wextra -Werror -pedantic -std=c99`.
2. **Restrição Mandatória a `wheel` (4750):**
   - Utilitários de elevação de privilégio DEVEM ser restritos exclusivamente ao grupo `wheel` (ou `root`), tanto em permissões de arquivo (`chmod 4750 root:wheel`) quanto por validação em runtime C (`verificar_grupo_autorizado()`).
3. **Higiene de Memória & Segredos:** Qualquer dado sensível (senhas lidas de TTY em `rtdo`, buffers intermediários) DEVE ser sobrescrito com `explicit_bzero` antes do término do escopo.
4. **Segurança de Binários SUID:**
   - Sempre limpar variáveis de ambiente perigosas (`LD_PRELOAD`, `LD_LIBRARY_PATH`, `IFS`) antes de `execvp`.
   - Sempre resetar grupos secundários com `initgroups("root", 0)` antes de alterar o UID/GID efetivo.
   - Sempre restaurar atributos de terminal (`termios`) em manipuladores de sinal (`SIGINT`, `SIGQUIT`, `SIGTERM`) se a entrada foi capturada com echo desabilitado.
5. **Portabilidade Linux & BSD:** Garantir que o código compile e execute tanto em distribuições Linux quanto em FreeBSD (`pwd->pw_passwd` vs `getspnam`, detecção de `-lcrypt`).
6. **Zero Alocações Dinâmicas Desnecessárias:** Preferir buffers estáticos com checagem rígida de limites (`sizeof`) em vez de `malloc`/`free` para ferramentas essenciais de sistema.
7. **Formatação Padronizada:** Todo código C deve passar na validação do `.clang-format` (`make format-check`).

---

## 📁 Estrutura do Repositório

```
unix/
├── .agents/
│   ├── rules/
│   │   ├── c-posix.md             # Padrões C99/POSIX e portabilidade
│   │   └── security.md            # Diretrizes de segurança SUID e restrição a wheel
│   └── skills/
│       └── unix-audit/
│           └── SKILL.md           # Runbook de auditoria estática e testes
├── .github/
│   └── workflows/
│       └── ci.yml                 # CI com GCC, Clang, clang-format e sanitizers
├── .githooks/
│   ├── pre-commit                 # Quality gate local
│   └── install.sh                 # Instalador dos ganchos Git
├── docs/
│   ├── ARCHITECTURE.md            # Modelo arquitetural e fluxo de privilégios
│   └── SECURITY.md                # Modelo de ameaças e auditoria
├── rtdo/
│   ├── Makefile                   # Build rtdo (4750 root:wheel, -lcrypt)
│   ├── rtdo.c                     # Código-fonte auditado com autenticação
│   └── rtdo.1                     # Manpage rtdo
├── rtgo/
│   ├── Makefile                   # Build rtgo (4750 root:wheel, zero crypt)
│   ├── rtgo.c                     # Código-fonte auditado imediato (sem senha)
│   └── rtgo.1                     # Manpage rtgo
├── Makefile                       # Orquestrador raiz
├── .clang-format                  # Especificação de estilo de código
├── .gitignore                     # Filtros de build
└── README.md                      # Documentação de entrada
```

---

## 🛠️ Comandos de Verificação Rápida

| Ação | Comando |
| :--- | :--- |
| **Compilar todos** | `make all` |
| **Compilar com Sanitizers (ASan/UBSan)** | `CC=clang make debug` |
| **Checagem de sintaxe estática** | `make check` |
| **Verificar formatação** | `make format-check` |
| **Formatar código automaticamente** | `make format` |
| **Limpar artefatos de build** | `make clean` |
| **Testar pre-commit hook manualmente** | `./.githooks/pre-commit` |
