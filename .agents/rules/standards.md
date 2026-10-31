# 📐 Padrões de Auditoria, Validação e Conformidade C POSIX

> Runbook e critérios canônicos para auditoria estática, verificação com sanitizers, qualidade de código e conformidade de segurança.

---

## 1. Ciclo de Auditoria e Verificação de Qualidade

1. **Validação de Formatação Estrita (.clang-format):**
    - Verificar conformidade de estilo:
        ```sh
        make format-check
        ```
    - Aplicar formatação automática quando necessário:
        ```sh
        make format
        ```

2. **Compilação Estrita Dual (GCC e Clang):**
    - O código deve compilar sem nenhum aviso (`-Wall -Wextra -Werror -pedantic -std=c99`) sob ambos os compiladores:
        ```sh
        CC=gcc make clean all
        CC=clang make clean all
        ```

3. **Validação Instrumentalizada com Sanitizers:**
    - Executar compilação de depuração com AddressSanitizer (ASan) e UndefinedBehaviorSanitizer (UBSan):
        ```sh
        CC=clang make clean debug
        ```
    - Todos os testes de execução devem passar sem emissão de relatórios de vazamento de memória ou comportamentos indefinidos.

---

## 2. Checklist Canônico de Auditoria de Segurança SUID

Para utilitários de sistema com privilégios elevados, audite obrigatoriamente:

1. **Higiene de Credenciais e Memória:**
    - Chamada obrigatória de `explicit_bzero` em todos os buffers que contenham senhas ou tokens antes do término de seu escopo.
2. **Isolamento de Grupos:**
    - Invocação mandatória de `initgroups("root", 0)` antes de assumir privilégio via `setuid(0)` e `setgid(0)`.
3. **Higienização do Ambiente:**
    - Expurgar variáveis perigosas (`LD_PRELOAD`, `LD_LIBRARY_PATH`, `IFS`) antes de qualquer chamada `exec*`.
    - Garantir `PATH` padrão do sistema imune a diretórios inseguros.
4. **Integridade da TTY e Captura de Sinais:**
    - Manipuladores de sinal (`SIGINT`, `SIGQUIT`, `SIGTERM`) devem restaurar os atributos originais da TTY (`tcsetattr`) caso o terminal tenha operado sem echo.

---

## 3. Checklist de Clean Code e Idioma Canônico

1. **Código Autoexplicativo:**
    - Ausência total de comentários redundantes que narrem código óbvio. Comentários justificam apenas decisões não-triviais de arquitetura ou restrições de kernel.
2. **Nomenclatura e Mensagens em Inglês:**
    - Todos os identificadores (funções, variáveis, macros, tipos) e saídas de terminal (`Usage: ...`, mensagens em `stderr`) devem ser estritamente em inglês.
3. **Estrutura e Abstração:**
    - Ausência de `#include` órfãos, variáveis não utilizadas ou código morto.
    - Funções pequenas com Princípio da Responsabilidade Única (SRP).
    - Função `main()` estruturada em Alto Nível de Abstração Único (SLAP) com cláusulas de guarda (_fail-fast_).

---

## 4. Literatura de Referência & Especificações Oficiais

- **LLVM / Clang Sanitizers:** <https://clang.llvm.org/docs/AddressSanitizer.html>
- **CERT C Coding Standard (SEI):** <https://wiki.sei.cmu.edu/confluence/display/c>
- **Segurança em Sistemas C:** _Secure Coding in C and C++_ (Robert C. Seacord, 2ª edição, Addison-Wesley / CERT)
- **Engenharia de Software UNIX:** _The Practice of Programming_ (Brian W. Kernighan & Rob Pike, 1999, Addison-Wesley)
