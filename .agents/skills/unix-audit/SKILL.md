---
name: unix-audit
description: Runbook cognitivo para auditoria estática, conformidade POSIX, Clean Code e validação de segurança em utilitários Unix C.
---

# Unix Audit & Security Verification

Use esta skill para verificar a conformidade de código C, padrões Clean Code e segurança de binários no repositório `unix`.

## Procedimento de Auditoria

1. **Validação de Formatação:**
   Execute o verificador de conformidade com o `.clang-format`:

    ```bash
    make format-check
    ```

    Se houver divergências, aplique a formatação com:

    ```bash
    make format
    ```

2. **Compilação Estrita com GCC e Clang:**
   Verifique que ambos os compiladores constroem o código sem alertas:

    ```bash
    CC=gcc make clean all
    CC=clang make clean all
    ```

3. **Validação com Sanitizers:**
   Execute o build de debug instrumentalizado com AddressSanitizer e UndefinedBehaviorSanitizer:

    ```bash
    CC=clang make clean debug
    ```

4. **Verificação de Segurança SUID:**
    - Conferir se `explicit_bzero` é chamado para todos os buffers contendo credenciais.
    - Conferir se `initgroups()` é chamado antes de `setuid(0)`.
    - Conferir se variáveis de ambiente perigosas são expurgadas antes de qualquer `exec*`.
    - Conferir se o sinal `SIGINT` restaura os atributos da TTY.

5. **Auditoria de Clean Code e Idioma Inglês:**
    - Verificar ausência de comentários redundantes nos arquivos `.c`.
    - Confirmar que todos os identificadores (funções, variáveis, macros, tipos) e saídas de terminal estão estritamente em inglês.
    - Checar ausência de `#include` órfãos ou variáveis não utilizadas.
    - Conferir se funções possuem responsabilidade única e `main` mantém nível uniforme de abstração.

---

## 📚 Literatura de Referência & Ferramentas Oficiais

Recomenda-se enfaticamente a consulta às fontes canônicas de segurança e boas práticas em C:

- **LLVM / Clang Sanitizers:** <https://clang.llvm.org/docs/AddressSanitizer.html>
- **CERT C Coding Standard (SEI):** <https://wiki.sei.cmu.edu/confluence/display/c>
- **Livro de Referência em Segurança:** _Secure Coding in C and C++_ (Robert C. Seacord, 2ª edição, Addison-Wesley / CERT).
- **Livro Canônico de Engenharia:** _The Practice of Programming_ (Brian W. Kernighan & Rob Pike, 1999, Addison-Wesley).
