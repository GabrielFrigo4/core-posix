---
name: unix-audit
description: Runbook cognitivo para auditoria estática, conformidade POSIX e validação de segurança em utilitários Unix C.
---

# Unix Audit & Security Verification

Use esta skill para verificar a conformidade de código C e segurança de binários no repositório `unix`.

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
