# 🔐 Modelo de Segurança e Ameaças

Diretrizes de segurança para a suíte de ferramentas do repositório `unix` (`rtdo` e `rtgo`).

---

## 🎯 Modelo de Ameaças

Utilitários que operam com bit SUID root ativo constituem pontos de máxima criticidade em sistemas Unix. O modelo de segurança adota uma postura de defesa em camadas (*defense-in-depth*).

### 1. Restrição de Execução ao Grupo Administrativo (`wheel`)
- **Ameaça:** Usuários comuns não privilegiados ou contas de serviço (`nobody`, `www-data`) tentando executar binários SUID para escalar privilégios.
- **Mitigação em 2 Camadas:**
  - **Camada 1 (Kernel/VFS):** Permissões `chmod 4750 root:wheel`. O bit de execução para `others` é explicitamente desabilitado. Contas fora de `wheel` são barradas antes do `execve` do sistema operacional.
  - **Camada 2 (Runtime C):** Checagem no código-fonte via `getgroups()`. Mesmo que as permissões do arquivo sejam corrompidas acidentalmente para `4755`, a execução aborta imediatamente se o chamador não pertencer ao grupo `wheel`.

### 2. Injeção de Ambiente (`LD_PRELOAD`, `LD_LIBRARY_PATH`)
- **Ameaça:** Usuários configurando variáveis que forçam a injeção de bibliotecas compartilhadas arbitrárias durante a execução do comando alvo como root.
- **Mitigação:** Ambos os utilitários removem explicitamente `LD_PRELOAD`, `LD_LIBRARY_PATH` e `IFS` via `unsetenv()`, e garantem `PATH` seguro do sistema caso esteja vazio ou nulo.

### 3. Retenção de Grupos Secundários
- **Ameaça:** O processo root herdar grupos secundários do usuário chamador, permitindo privilégios incongruentes ou vazamento de dados.
- **Mitigação:** Chamada obrigatória a `initgroups("root", 0)` antes de `setgid(0)` e `setuid(0)`.

### 4. Vazamento de Senhas em Memória (`rtdo`)
- **Ameaça:** Despejos de memória (*core dumps*) ou inspeção residual da pilha expondo senhas em texto puro.
- **Mitigação:** `explicit_bzero()` sobrescreve todo o buffer da senha imediatamente após a comparação criptográfica.

---

## 📋 Checklist de Auditoria Contínua

- [x] Permissão `chmod 4750 root:wheel` nos alvos de instalação.
- [x] Validação programática de `wheel`/`root` com fallback seguro para `sudo`.
- [x] Ausência de alocações dinâmicas desnecessárias (`malloc`/`free`).
- [x] Checagem de limites estritos em todas as operações de string.
- [x] Restauração de manipuladores de sinal e atributos da TTY em `rtdo`.
- [x] Invocação de `initgroups` para isolamento de privilégios.
- [x] Compilação limpa sob `-Wall -Wextra -Werror -pedantic`.
- [x] Validação com AddressSanitizer e UndefinedBehaviorSanitizer no CI.
