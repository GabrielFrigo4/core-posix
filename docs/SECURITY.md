# 🔐 Modelo de Segurança e Ameaças

Diretrizes de segurança para a suíte de ferramentas do repositório `unix`.

---

## 🎯 Modelo de Ameaças

Utilitários que operam com bit SUID root ativo (`4755`) constituem pontos de máxima criticidade em sistemas Unix, pois qualquer vulnerabilidade de corrupção de memória ou herança de ambiente pode levar a escalonamento local de privilégios (*LPE*).

### 1. Injeção de Ambiente (`LD_PRELOAD`, `LD_LIBRARY_PATH`)
- **Ameaça:** Usuários não privilegiados podem configurar variáveis de ambiente que forçam a carga de bibliotecas compartilhadas arbitrárias durante a execução do comando alvo como root.
- **Mitigação:** `msud` limpa explicitamente `LD_PRELOAD`, `LD_LIBRARY_PATH` e `IFS` via `unsetenv()`, e garante que `PATH` aponte para um caminho de sistema seguro caso esteja vazio.

### 2. Vazamento de Segredos em Memória
- **Ameaça:** Despejos de memória (*core dumps*) ou inspeção residual de pilha podem expor a senha digitada em texto puro.
- **Mitigação:** Imediatamente após a comparação de hash, a função `explicit_bzero()` sobrescreve todo o buffer da senha, prevenindo que o compilador elimine a limpeza por otimização (*Dead Store Elimination*).

### 3. Ataques de Tempo e Contas Desativadas
- **Ameaça:** Tentativa de autenticação em contas de sistema (`daemon`, `bin`) ou desativadas (`!` ou `*` no shadow).
- **Mitigação:** A função `conta_bloqueada()` rejeita antecipadamente qualquer conta cujo hash não represente uma credencial válida antes de invocar algoritmos criptográficos.

---

## 📋 Checklist de Auditoria Contínua

- [x] Ausência de `malloc`/`free` em caminhos críticos de autenticação.
- [x] Checagem de limites estritos em todas as operações de string.
- [x] Restauração de manipuladores de sinal e atributos da TTY.
- [x] Invocação de `initgroups` para isolamento de privilégios.
- [x] Compilação limpa sob `-Wall -Wextra -Werror -pedantic`.
- [x] Validação com AddressSanitizer e UndefinedBehaviorSanitizer no CI.
