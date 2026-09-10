# 🏛️ Arquitetura e Ciclo de Execução

Visão aprofundada da mecânica de funcionamento, transições de privilégios e ciclo de vida dos processos na suíte `unix`, cobrindo os utilitários `rtdo` e `rtgo`.

---

## ⚡ Fluxo de Execução do `rtdo` e `rtgo`

```mermaid
flowchart TD
    A["Chamador: Executa binário SUID"] --> B{"geteuid() == 0 ?"}
    B -- Não --> B1["Erro: Requer SUID root (chmod 4750)"]
    B -- Sim --> C{"verificar_grupo_autorizado(): Caller in 'wheel' or root?"}
    C -- Não --> C1["Erro: Acesso negado (não pertence ao grupo wheel)"]
    C -- Sim --> D{"Utilitário"}

    subgraph RTDO_FLOW ["🔒 rtdo (Com Senha)"]
        D -- rtdo --> E["Acesso seguro ao shadow via getspnam/getpwuid"]
        E --> F{"Conta ativa e válida?"}
        F -- Não --> F1["Erro: Conta bloqueada ou sem senha"]
        F -- Sim --> G["Captura TTY (/dev/tty, O_NOCTTY)"]
        G --> H["Desabilita ECHO & Registra Signals (SIGINT/SIGQUIT)"]
        H --> I["Lê senha na TTY & Restaura Atributos"]
        I --> J["crypt(): Compara hash calculado vs alvo"]
        J --> K["explicit_bzero(): Zera buffer de senha"]
        K --> L{"Hash confere?"}
        L -- Não --> L1["Erro: Senha incorreta"]
    end

    subgraph RTGO_FLOW ["⚡ rtgo (Sem Senha)"]
        D -- rtgo --> M["Elevação imediata para membros do wheel"]
    end

    L -- Sim --> N["sanitizar_ambiente(): Expura LD_* e valida PATH"]
    M --> N
    N --> O["initgroups('root', 0) & setgid(0) & setuid(0)"]
    O --> P["execvp(argv[1], ...): Transfere execução final"]
```

---

## 🛡️ Decisões de Design de Baixo Nível

### 1. Restrição Estrita ao Grupo `wheel` (4750)

Para evitar que qualquer usuário sem privilégios possa explorar ou abusar dos utilitários:

- **Permissão de Arquivo:** `chmod 4750 root:wheel`. Usuários fora de `wheel` recebem `Permission Denied` diretamente no nível do kernel ao tentar ler ou invocar o binário.
- **Validação Programática:** Inspeciona o `getgid()` e a lista de grupos secundários retornada por `getgroups()` contra o GID de `wheel` (com fallback para `sudo` em distros como Ubuntu/Debian).

### 2. Acesso Direto à TTY (`/dev/tty`) em `rtdo`

A leitura de credenciais acessa diretamente o descritor `/dev/tty` com `O_NOCTTY`. Isso impede injeção via pipes (`stdin`) e garante que a entrada só venha do usuário interativo.

### 3. Captura Atômica de Sinais

Em `rtdo`, se o usuário interromper com `Ctrl+C` enquanto a flag `ECHO` estiver desligada, `SIGINT` restaura imediatamente o estado original do terminal antes de sair com código 130.

### 4. Isolamento Completo com `initgroups`

Ambos os utilitários invocam `initgroups("root", 0)` antes de `setgid(0)` e `setuid(0)` para garantir que nenhuma permissão de grupo secundário do usuário chamador seja retida no processo root.
