# Regras de Segurança para Utilitários SUID e Sistema

> Requisitos obrigatórios de segurança para ferramentas com privilégios elevados.

---

## 1. Princípio do Menor Privilégio e Ciclo de Vida SUID

1. **Checagem Imediata de Privilégio:**
   - Binários que dependem de privilégios de root para ler bancos de credenciais devem validar `geteuid() == 0` logo no início.
2. **Isolamento de Grupos:**
   - Antes de assumir a identidade final via `setuid(0)` e `setgid(0)`, é OBRIGATÓRIO invocar `initgroups("root", 0)` ou zerar grupos suplementares para impedir que privilégios de grupos do chamador (ex: docker, wheel) vazem para o processo de destino.
3. **Falha Fechada (Fail-Closed):**
   - Se qualquer chamada de verificação de senha, banco de dados ou alteração de privilégio falhar, o processo deve abortar imediatamente com código de erro não-zero e limpar segredos residuais da memória.
4. **Restrição Mandatória a Grupo Administrativo:**
   - Binários executores de privilégio devem ter permissões restritas ao grupo administrativo (`chmod 4750 root:wheel`).
   - O código-fonte C deve obrigatoriamente validar o pertencimento a `wheel` (ou `root`) antes de qualquer ação.

---

## 2. Higienização do Ambiente

1. **Variáveis Perigosas:**
   - Variáveis que alteram a carga dinâmica de bibliotecas ou comportamento de shells DEVEM ser removidas antes do `execvp`:
     - `LD_PRELOAD`
     - `LD_LIBRARY_PATH`
     - `IFS`
2. **Defesa de PATH:**
   - Se a variável de ambiente `PATH` estiver ausente, vazia ou contiver diretórios inseguros (como `.`), redefinir para o caminho padrão seguro do sistema (`_PATH_DEFPATH` ou `/usr/local/sbin:/usr/local/bin:/usr/sbin:/usr/bin:/sbin:/bin`).

---

## 3. Integridade da TTY e Captura de Sinais

1. **Leitura Segura:**
   - Toda leitura interativa de senhas deve ser feita diretamente via `/dev/tty` com `O_NOCTTY` e flags `ECHO` desabilitadas via `tcsetattr`.
2. **Restauração de Terminal em Sinais:**
   - Manipuladores para `SIGINT`, `SIGQUIT` e `SIGTERM` devem restaurar a configuração original do terminal (`tcsetattr`) antes de finalizar o processo, evitando que a sessão do usuário fique com o terminal desconfigurado.
