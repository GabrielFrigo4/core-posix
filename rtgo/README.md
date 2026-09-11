# ⚡ rtgo (Root Go)

> Executor imediato de privilégios com latência ultrabaixa (< 1ms), sem solicitação de senha, restrito ao grupo administrativo `wheel`.

---

## 1. Como Funciona Agora (Implementação Atual)

O `rtgo` opera sob o princípio de **Defesa em Profundidade** (_Defense-in-Depth_). O fluxo de execução segue uma cadeia linear, estrita e sem concessões:

```
[Invocação]
    │
    ▼
1. Validação de Argumentos (argc >= 2)
    │
    ▼
2. Checagem de SUID Ativo (geteuid() == 0)
    │
    ▼
3. Verificação em Runtime de Grupo (is_authorized_group)
   - getuid() == 0 (root imediato)
   - getgid() == GID de 'wheel' (ou 'sudo')
   - getgroups() iterando grupos suplementares
    │
    ▼
4. Higienização de Ambiente (sanitize_environment)
   - unsetenv: LD_PRELOAD, LD_LIBRARY_PATH, IFS
   - Validação e imposição de PATH padrão seguro
    │
    ▼
5. Isolamento e Transição de Privilégios (assume_root)
   - initgroups("root", 0): zera grupos suplementares herdados
   - setgid(0): fixa RGID/EGID/SGID para root
   - setuid(0): fixa RUID/EUID/SUID para root
    │
    ▼
6. execvp(argv[1], &argv[1]) -> Substitui imagem do processo
```

### Decisões de Design Atuais

- **Dupla camada de autorização:**
    1. _Camada do Sistema de Arquivos:_ Permissões `4750 root:wheel` no binário. O kernel rejeita chamadas de usuários que não pertençam ao grupo `wheel` com `EACCES` (_Permission Denied_) antes mesmo de carregar o executável em memória.
    2. _Camada em Runtime C:_ A função `is_authorized_group()` valida manualmente a filiação ao grupo `wheel` (ou `sudo`). Se alguém alterar erroneamente as permissões do binário para `4755`, usuários comuns ainda serão barrados pelo código.
- **Fail-Closed:** Qualquer falha de chamada de sistema (`initgroups`, `setgid`, `setuid`, `execvp`) encerra o processo imediatamente com código de erro não-zero.

---

## 2. O Mínimo Viável (~20 Linhas)

Se optarmos pelo princípio **KISS** (_Keep It Simple, Stupid_), delegando 100% do controle de acesso à camada de permissões do Kernel (`chmod 4750 root:wheel`), o código C pode ser reduzido ao núcleo inegociável:

```c
#ifndef _DEFAULT_SOURCE
#define _DEFAULT_SOURCE
#endif
#ifndef _XOPEN_SOURCE
#define _XOPEN_SOURCE 700
#endif

#include <grp.h>
#include <stdlib.h>
#include <unistd.h>

int main(int argc, char *argv[])
{
	if (argc < 2)
	{
		return 1;
	}

	unsetenv("LD_PRELOAD");
	unsetenv("LD_LIBRARY_PATH");
	unsetenv("IFS");

	if (initgroups("root", 0) != 0 || setgid(0) != 0 || setuid(0) != 0)
	{
		return 1;
	}

	execvp(argv[1], &argv[1]);
	return 127;
}
```

### O que foi removido e por que ainda é seguro?

1. **`is_authorized_group()` removido:** A proteção `chmod 4750 root:wheel` já impede fisicamente que quem não é `wheel` execute o binário. O kernel valida isso no VFS.
2. **`geteuid() == 0` removido:** Se o bit SUID não estiver ativo, a chamada `setuid(0)` falhará com `EPERM` (_Operation not permitted_) e o `return 1` abortará o processo.
3. **`fprintf`/`perror` removidos:** Filosofia clássica Unix: utilitários de sistema expressam falhas retornando códigos de erro (`1`, `127`) sem poluir a saída desnecessariamente.

---

## 3. Fundamentos do Modelo de Segurança Unix

Para compreender a segurança de utilitários de elevação de privilégio, é essencial dominar como o Kernel Unix gerencia identidades de processos e descritores de segurança.

### A Tríade de UIDs (User Identifiers)

Todo processo em sistemas Unix possui não apenas um, mas **três UIDs** mantidos pelo kernel:

| Identificador | Nome Completo       | Finalidade e Comportamento                                                                                                                                                                                                                                                                                     |
| :------------ | :------------------ | :------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| **RUID**      | _Real User ID_      | **Quem invocou o processo.** Representa a identidade real do usuário logado que iniciou a execução. Determina a quem pertence a sessão e quais processos podem receber sinais (`kill`) deste processo.                                                                                                         |
| **EUID**      | _Effective User ID_ | **Qual autoridade o processo tem no momento.** É o UID que o kernel consulta em cada chamada de sistema para validar permissões de leitura, escrita, execução de arquivos, abertura de sockets em portas privilegiadas (< 1024) ou montagem de sistemas de arquivos.                                           |
| **SUID**      | _Saved Set-User-ID_ | **Identidade de retorno.** Quando um programa SUID é carregado via `execve`, o kernel copia o novo `EUID` para o `SUID`. Isso permite que processos com privilégios transitórios rebaixem temporariamente o `EUID` para o `RUID` e, mais tarde, recuperem os privilégios restaurando o `SUID` via `seteuid()`. |

### O que é o Bit SUID (`04000` / `chmod u+s`)?

No sistema de arquivos Unix, arquivos regulares possuem bits de permissão especiais. O bit **SUID** (`Set-User-ID`, máscara octal `04000`):

- Em um executável comum (`0755`), ao executar o arquivo, o kernel cria um processo onde:
  $$\text{RUID} = \text{EUID} = \text{SUID} = \text{UID do chamador}$$
- Em um executável com bit SUID pertencente ao `root` (`4750`):
  $$\text{RUID} = \text{UID do chamador} \quad|\quad \text{EUID} = 0 \text{ (root)} \quad|\quad \text{SUID} = 0 \text{ (root)}$$

O chamador ganha o poder de root em seu `EUID`, mas seu `RUID` continua sendo o usuário não-privilegiado.

---

### A Tríade de GIDs e Grupos Suplementares

Assim como os UIDs, os processos possuem identidades de grupo:

- **RGID** (_Real Group ID_), **EGID** (_Effective Group ID_) e **SGID** (_Saved Set-Group-ID_).
- **Grupos Suplementares (_Supplementary Groups_):** Uma lista de grupos adicionais retornada por `getgroups(2)` a que o usuário pertence (ex: `wheel`, `docker`, `video`, `audio`).

---

### Os Perigos Críticos e Como o Código se Protege

#### 1. Por que `initgroups("root", 0)` é obrigatório?

Se um usuário comum no grupo `docker` invocar o binário e executarmos apenas `setuid(0)`, o processo root resultante continuará com a lista de grupos suplementares do usuário original.
Se esse comando root criar arquivos ou acessar recursos, esses grupos suplementares herdados podem causar vazamento de permissões ou confusão de privilégios (_Group Confusion Attack_).
A chamada `initgroups("root", 0)` redefine a lista de grupos suplementares para conter exclusivamente os grupos do usuário root.

#### 2. Por que `setuid(0)` e `setgid(0)` são cruciais antes de `execvp`?

Quando o `execve` do sistema operacional carrega um binário SUID, `RUID != EUID`.
Diversas shells modernas (`bash`, `dash`, `zsh`) possuem proteções internas contra SUID: ao detectarem que `RUID != EUID`, elas **descartam automaticamente os privilégios de root** (_privilege dropping_), forçando `EUID = RUID`.
Ao invocar `setuid(0)` e `setgid(0)` no código C, igualamos todas as três identidades:
$$\text{RUID} = \text{EUID} = \text{SUID} = 0$$
O processo passa a ser um processo root verdadeiro e permanente, impedindo que comandos filhos façam o descarte involuntário de privilégios.

#### 3. Por que limpar `LD_PRELOAD`, `LD_LIBRARY_PATH` e `IFS`?

- **`LD_PRELOAD` e `LD_LIBRARY_PATH`:** Permitem que um usuário instrua o dynamic linker (`ld.so`) a carregar bibliotecas dinâmicas compartilhadas arbitrárias antes das bibliotecas de sistema, podendo sequestrar funções da libc como `malloc` ou `getuid`.
    - Embora o kernel ative a flag `AT_SECURE` quando um binário SUID inicia (ignorando essas variáveis na carga inicial), assim que chamamos `setuid(0)` o processo vira root pleno. Se invocarmos um novo comando com `execvp`, o dynamic linker não verá mais uma transição de privilégio e carregará qualquer biblioteca maliciosa que tenha permanecido no vetor de variáveis de ambiente (`environ`).
- **`IFS` (_Internal Field Separator_):** Variável que define separadores de palavras para shells. Se herdada por um script invocado pelo utilitário, pode alterar como argumentos e caminhos são interpretados.
