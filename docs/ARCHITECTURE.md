# 🏛️ Arquitetura e Ciclo de Execução

Visão aprofundada da mecânica de funcionamento, transições de privilégios e ciclo de vida dos processos na suíte `unix`, com ênfase no utilitário `msud`.

---

## ⚡ Fluxo de Execução do `msud`

O diagrama abaixo ilustra a transição de privilégios e o ciclo de vida defensivo adotado:

```mermaid
flowchart TD
    A["Início: Execução do Binário SUID"] --> B{"geteuid() == 0 ?"}
    B -- Não --> B1["Erro: Requer SUID root (chmod 4755)"]
    B -- Sim --> C["getuid() & getpwuid(): Obtém identidade real do usuário"]
    C --> D["obter_hash_usuario(): Acesso seguro ao shadow"]
    D --> E{"Conta ativa e válida?"}
    E -- Não --> E1["Erro: Conta bloqueada ou sem senha"]
    E -- Sim --> F["Captura TTY (/dev/tty, O_NOCTTY)"]
    F --> G["Desabilita ECHO & Registra Signals (SIGINT/SIGQUIT)"]
    G --> H["Lê senha na TTY & Restaura Atributos"]
    H --> I["crypt(): Compara hash calculado vs alvo"]
    I --> J["explicit_bzero(): Zera buffer de senha"]
    J --> K{"Hash confere?"}
    K -- Não --> K1["Erro: Senha incorreta"]
    K -- Sim --> L["sanitizar_ambiente(): Expura LD_* e valida PATH"]
    L --> M["initgroups('root', 0) & setgid(0) & setuid(0)"]
    M --> N["execvp(argv[1], ...): Transfere execução final"]
```

---

## 🛡️ Decisões de Design de Baixo Nível

### 1. Acesso Direto à TTY (`/dev/tty`)
Diferente de ler de `stdin` (que pode ser redirecionado através de pipes ou arquivos), a leitura de credenciais no `msud` acessa diretamente o nó `/dev/tty` do processo controlador com a flag `O_NOCTTY`. Isso previne:
- Injeção acidental ou maliciosa via pipes (`cat passwords.txt | msud ...`).
- Assunção indevida do terminal como terminal de controle.

### 2. Captura Atômica de Sinais
Se o usuário pressionar `Ctrl+C` no momento exato em que a flag `ECHO` está desativada, o tratador de sinal intercepta `SIGINT`, reaplica imediatamente a estrutura `g_saved_termios` no descritor aberto e conclui a terminação com `_exit(130)`.

### 3. Eliminação Residual com `initgroups`
No modelo Unix, um usuário pode pertencer a grupos secundários (ex: `wheel`, `docker`, `plugdev`). Quando um binário SUID assume a identidade de root via `setuid(0)` e `setgid(0)`, se não chamar explicitamente `initgroups("root", 0)`, o processo continuará pertencendo aos grupos secundários do usuário chamador, abrindo brechas de integridade. A chamada garante alinhamento estrito com os grupos de root.
