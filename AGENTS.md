# ⚡ Core POSIX — AI Agent Briefing

> Suíte de utilitários Unix minimalistas, seguros e estritamente aderentes ao padrão POSIX em linguagem C.

---

## 🧭 Identidade e Papel

O repositório **core-posix** reúne implementações limpas de ferramentas de sistema, concebidas sob a filosofia Unix: ferramentas pequenas, focadas, sem dependências externas desnecessárias, com código auditável e segurança em primeiro plano.

A suíte disponibiliza dois executores de privilégios SUID:

- **`rtdo`** (_Root Do_): Executor com verificação de senha interativa via `/dev/tty`.
- **`rtgo`** (_Root Go_): Executor imediato sem senha para membros autorizados do grupo `wheel`.

---

## 📁 Estrutura do Repositório

```text
Sysutils/
├── .agents/
│   └── rules/
│       ├── c-posix.md             # Padrões C99/POSIX, Clean Code e convenção em inglês
│       ├── security.md            # Diretrizes de segurança SUID e restrição a wheel
│       └── standards.md           # Padrões de engenharia e governança
├── .github/
│   └── workflows/
│       └── ci.yml                 # CI com GCC, Clang, clang-format e sanitizers
├── .githooks/
│   └── pre-commit                 # Quality gate local
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
├── .clang-format                  # Especificação de estilo de código
├── .gitignore                     # Filtros de build
├── ENVIRONMENT.md                 # Especificação do ecossistema
├── LICENSE                        # Licença MIT
├── Makefile                       # Orquestrador raiz
├── PRINCIPLES.md                  # 22 Princípios de Engenharia adaptados
├── README.md                      # Documentação de entrada
└── TODO.md                        # Backlog e status operacional
```

---

## ⚠️ Regras Críticas para Agentes de IA

1. **Conformidade POSIX e Padrão C99:** Todo código C deve compilar sem avisos com `cc -Wall -Wextra -Werror -pedantic -std=c99`.
2. **Restrição Mandatória a `wheel` (4750):** Utilitários de elevação de privilégio DEVEM ser restritos exclusivamente ao grupo `wheel` (ou `root`), tanto em permissões de arquivo (`chmod 4750 root:wheel`) quanto por validação em runtime C (`is_authorized_group()`).
3. **Higiene de Memória & Segredos:** Qualquer dado sensível (senhas lidas de TTY em `rtdo`, buffers intermediários) DEVE ser sobrescrito com `explicit_bzero` antes do término do escopo.
4. **Segurança de Binários SUID:**
    - Limpar variáveis de ambiente perigosas (`LD_PRELOAD`, `LD_LIBRARY_PATH`, `IFS`) antes de `execvp`.
    - Resetar grupos secundários com `initgroups("root", 0)` antes de alterar o UID/GID efetivo.
    - Restaurar atributos de terminal (`termios`) em manipuladores de sinal (`SIGINT`, `SIGQUIT`, `SIGTERM`) se a entrada foi capturada com echo desabilitado.
5. **Portabilidade Linux & BSD:** Garantir que o código compile e execute tanto em distribuições Linux quanto em FreeBSD (`pwd->pw_passwd` vs `getspnam`, detecção de `-lcrypt`).
6. **Zero Alocações Dinâmicas Desnecessárias:** Preferir buffers estáticos com checagem rígida de limites (`sizeof`) em vez de `malloc`/`free` para ferramentas essenciais de sistema.
7. **Formatação Padronizada:** Todo código C deve passar na validação do `.clang-format` (`make format-check`).
8. **Clean Code & Idioma Inglês:** Código autoexplicativo (zero comentários redundantes), funções pequenas de responsabilidade única (SRP), fail-fast e nível único de abstração na `main`. Todos os identificadores e mensagens de terminal DEVEM ser estritamente em inglês.

9. **Taxonomia Tripartite de Comentários:** O código adota estritamente três categorias: 1. Comentários de Sistema = PERMITIDOS (shebangs, pragmas, linters); 2. Comentários Estruturais de 64/32 colunas = PERMITIDOS (cabeçalhos de 64 `-`, seções de 32 colunas `### ===` e `### ---` com títulos em MAIÚSCULO, réguas de 64 `=` em `.gitignore` com títulos em MAIÚSCULO, sem vazamento); 3. Comentários Explicativos/Narrativos = PROIBIDOS em código, configs E blocos de código dentro de Markdowns.

---

## 🛡️ Regra da Proatividade e Correção Contínua (Boy Scout Rule)

O agente de IA atua de forma proativa na manutenção e aplicação dos padrões canônicos deste repositório.

Ao identificar linhas ou artefatos fora dos padrões estabelecidos:

1. **Notificar concisamente** o usuário sobre o ajuste realizado.
2. **Corrigir a inconformidade**, aplicando o padrão correspondente:
    - **Taxonomia Tripartite de Comentários:** Eliminar comentários explicativos óbvios em código, configs e blocos de código em Markdowns, preservando exclusivamente comentários de sistema e delimitadores estruturais (64/32 colunas).
    - **Banners Estruturais:** Ajustar réguas para exatamente 64 hífens no topo ou 32 caracteres com `### ` no corpo (ou 64 `=` em `.gitignore`), com títulos sempre em MAIÚSCULO.
    - **Portabilidade POSIX:** Substituir bashismos (`[[ ]]`, `&>`, arrays, `source`) por sintaxe estrita POSIX `/bin/sh`.
    - **Shebang Universal:** Garantir sempre `#!/usr/bin/env sh` ou `#!/usr/bin/env python3`.
    - **Sequências ANSI & Escapes:** Não use octais (`\033`, `\001`) para caracteres ou escapes. Use `[ -t 1 ] && echo -n $\x27\e...\x27` ou notação hexadecimal (`\x01`, `\x1b`). Octal é exclusivo para permissões POSIX (`chmod 0755`, `chmod 0644`, `umask`).
    - **Redirecionamento Seguro:** Envolver destinos em aspas duplas (ex: `> "/dev/null" 2>&1`).
    - **Makefiles:** Assegurar cabeçalho `.POSIX: .SILENT:`, `MAKEFLAGS += --no-print-directory -s`, alinhamento estético de variáveis e zero `@` redundante.
    - **Permissões Canônicas:** Aplicar 4 dígitos octais (`chmod 0755`, `chmod 0644`, `chmod 0700`, `chmod 0600`).
    - **Invariante Out-of-the-Box:** Garantir modos octais corretos no Git Index e auto-cura em tempo de execução sem requerer intervenção manual pós-clone.
    - **Emissão Semântica de UI:** Substituir `echo` avulsos com emojis pelas rotinas canônicas `_ui_*`.
    - **Antifragilidade & Resiliência:** Resolução ativa de caminhos em cascata, auto-cura de permissões (0600 em chaves/segredos) e validação defensiva de pré-requisitos.
    - **Curadoria Cognitiva:** Capturar decisões estruturais e regras tácitas em skills locais compactas (`.agents/skills/`), mantendo-as atualizadas e expurgando ruído obsoleto.
    - **Refatoração Sem Legado:** Expurgar aliases obsoletos, variáveis mortas e shims de compatibilidade em renomeações.
    - **Elevação Antifrágil & Shift Seguro:** Usar o bloco explícito `if [ "$(id -u)" -ne 0 ]` para resolução de `ELEVATE` (evitando término fatal sob `set -e` como root) e proteger todo comando `shift` com validação prévia de parâmetros (`[ "$#" -ge N ] && shift N`).
    - **Simetria de Documentação:** Mover regras ou invariantes universais documentadas erroneamente em sub-membros específicos para o documento de arquitetura do grupo correspondente, eliminando falsas assimetrias.

---

## 🚀 Comandos de Verificação Rápida (Quality Gates)

| Comando             | Finalidade Operacional                                        |
| :------------------ | :------------------------------------------------------------ |
| `make help`         | Exibe o catálogo completo de tarefas operacionais             |
| `make build`        | Compila todos os utilitários da suíte (`rtdo`, `rtgo`)        |
| `make debug`        | Compila com símbolos de depuração e flags estendidas          |
| `make check`        | Executa testes unitários locais e validação de sintaxe        |
| `make format`       | Aplica clang-format nos fontes C e Prettier nas documentações |
| `make clang-format` | Formata código-fonte C/Header com clang-format                |
| `make prettier`     | Formata documentações e arquivos de configuração com Prettier |
| `make format-check` | Valida conformidade estática de formatação (C e Markdown)     |
| `make clean`        | Remove binários compilados, objetos e diretórios temporários  |
| `make install`      | Instala binários em produção com permissões SUID (`4750`)     |

---

## 📖 Referências Obrigatórias

Antes de qualquer modificação neste ecossistema, consulte:

- [README.md](README.md) — Documentação e visão geral da suíte
- [PRINCIPLES.md](PRINCIPLES.md) — 22 Princípios de Engenharia adaptados
- [ENVIRONMENT.md](ENVIRONMENT.md) — Arquitetura de ambiente
- [TODO.md](TODO.md) — Backlog de tarefas e itens em aberto
- [LICENSE](LICENSE) — Licença MIT do projeto
- [docs/ARCHITECTURE.md](docs/ARCHITECTURE.md) — Modelo arquitetural e fluxo de privilégios
- [docs/SECURITY.md](docs/SECURITY.md) — Modelo de ameaças e auditoria de segurança
- [.agents/rules/c-posix.md](.agents/rules/c-posix.md) — Padrões C99/POSIX e Clean Code
- [.agents/rules/security.md](.agents/rules/security.md) — Diretrizes de segurança SUID
- [.agents/rules/standards.md](.agents/rules/standards.md) — Diretrizes transversais
