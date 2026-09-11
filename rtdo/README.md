# 🔒 rtdo (Root Do)

> Executor minimalista e seguro de comandos com privilégio elevado mediante autenticação interativa de senha via `/dev/tty`, restrito ao grupo administrativo `wheel`.

---

## 1. Como Funciona Agora (Implementação Atual)

O `rtdo` implementa uma cadeia completa de autenticação local, isolamento de terminal e elevação de privilégios:

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
3. Verificação de Grupo Autorizado (is_authorized_group)
   - Valida se o RUID pertence a 'wheel' (ou 'sudo')
    │
    ▼
4. Resolução da Identidade Real do Chamador
   - pwd = getpwuid(getuid())  <-- Usa o RUID, nunca confia em $USER
    │
    ▼
5. Autenticação Interativa (authenticate_user)
   - get_user_hash: lê hash de /etc/shadow (Linux) ou /etc/master.passwd (BSD)
   - is_account_locked: barra contas bloqueadas ('!', '*') ou vazias
   - read_password_tty:
       * Abre /dev/tty com O_NOCTTY
       * Desabilita flag ECHO via tcsetattr
       * Registra tratadores de sinal (SIGINT, SIGQUIT, SIGTERM)
       * Lê senha char a char, restaura terminal e handlers originais
   - crypt(): calcula hash derivado da senha informada
   - explicit_bzero(): zera imediatamente a senha da memória
   - Compara hash calculado com hash do banco de credenciais
    │
    ▼
6. Higienização de Ambiente (sanitize_environment)
   - unsetenv: LD_PRELOAD, LD_LIBRARY_PATH, IFS
   - Validação e imposição de PATH padrão seguro
    │
    ▼
7. Isolamento e Transição de Privilégios (assume_root)
   - initgroups("root", 0): elimina grupos secundários do chamador
   - setgid(0) & setuid(0): fixa RUID/EUID/SUID para root
    │
    ▼
8. execvp(argv[1], &argv[1]) -> Transfere controle ao comando alvo
```

---

## 2. A Versão Mínima Viável de um Executor com Senha

Em utilitários que exigem autenticação de senha, o volume de código é naturalmente maior que o do `rtgo` devido à manipulação da TTY e criptografia. No entanto, se eliminarmos abstrações defensivas redundantes, a versão mínima essencial gira em torno de ~90 linhas:

```c
#ifndef _DEFAULT_SOURCE
#define _DEFAULT_SOURCE
#endif
#ifndef _XOPEN_SOURCE
#define _XOPEN_SOURCE 700
#endif

#include <fcntl.h>
#include <grp.h>
#include <pwd.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <termios.h>
#include <unistd.h>

#if defined(__linux__)
#include <shadow.h>
#endif

static struct termios g_saved_termios;
static int g_tty_fd = -1;

static void signal_handler(int sig)
{
	(void)sig;
	if (g_tty_fd >= 0)
	{
		tcsetattr(g_tty_fd, TCSANOW, &g_saved_termios);
	}
	_exit(130);
}

static const char *get_user_hash(const struct passwd *pwd)
{
#if defined(__linux__)
	struct spwd *sp = getspnam(pwd->pw_name);
	return (sp != NULL) ? sp->sp_pwdp : NULL;
#else
	return pwd->pw_passwd;
#endif
}

static int is_account_locked(const char *hash)
{
	if (hash == NULL || hash[0] == '\0')
	{
		return 1;
	}
	return (hash[0] == '!' || hash[0] == '*' || hash[0] == 'x');
}

static int read_password(char *buffer, size_t size)
{
	struct termios no_echo;
	struct sigaction sa;
	size_t i = 0;
	char c;

	g_tty_fd = open("/dev/tty", O_RDWR | O_NOCTTY);
	if (g_tty_fd < 0)
	{
		return -1;
	}

	if (tcgetattr(g_tty_fd, &g_saved_termios) != 0)
	{
		close(g_tty_fd);
		g_tty_fd = -1;
		return -1;
	}

	memset(&sa, 0, sizeof(sa));
	sa.sa_handler = signal_handler;
	sigaction(SIGINT, &sa, NULL);

	no_echo = g_saved_termios;
	no_echo.c_lflag &= ~(tcflag_t)ECHO;
	tcsetattr(g_tty_fd, TCSAFLUSH, &no_echo);

	while (i < size - 1)
	{
		if (read(g_tty_fd, &c, 1) <= 0 || c == '\n' || c == '\r')
		{
			break;
		}
		buffer[i++] = c;
	}
	buffer[i] = '\0';

	tcsetattr(g_tty_fd, TCSANOW, &g_saved_termios);
	close(g_tty_fd);
	g_tty_fd = -1;
	return 0;
}

int main(int argc, char *argv[])
{
	char password[128];
	const char *target_hash;
	char *computed_hash;

	if (argc < 2)
	{
		return 1;
	}

	struct passwd *pwd = getpwuid(getuid());
	if (pwd == NULL)
	{
		return 1;
	}

	target_hash = get_user_hash(pwd);
	if (is_account_locked(target_hash))
	{
		return 1;
	}

	if (read_password(password, sizeof(password)) != 0)
	{
		return 1;
	}

	computed_hash = crypt(password, target_hash);
	explicit_bzero(password, sizeof(password));

	if (computed_hash == NULL || strcmp(computed_hash, target_hash) != 0)
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

---

## 3. Fundamentos do Modelo de Segurança Unix & Autenticação

Para implementar utilitários de segurança confiáveis em C, é imperativo compreender o comportamento do Kernel Unix e da libc sob privilégios elevados.

### A Tríade de UIDs (RUID, EUID, SUID)

Todo processo Unix mantém três identificadores de usuário:

| Identificador | Nome Completo       | Significado no `rtdo`                                                                                                                                                                                                                                                                                                    |
| :------------ | :------------------ | :----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| **RUID**      | _Real User ID_      | **Quem você é de verdade.** Determinado no momento do login. O `rtdo` usa exclusivamente `getuid()` (RUID) para saber qual usuário autenticar. Nunca usamos variáveis como `$USER` ou `$LOGNAME`, pois um invasor pode facilmente alterá-las no ambiente antes de executar o comando.                                    |
| **EUID**      | _Effective User ID_ | **Que poder você tem agora.** É o identificador que o kernel usa para conceder permissões. Como o `rtdo` é instalado com `chmod 4750 root:wheel`, o kernel define `EUID = 0` (root). É exatamente por ter `EUID = 0` que o `rtdo` consegue abrir e ler o `/etc/shadow` (que possui permissão restrita `0640` ou `0000`). |
| **SUID**      | _Saved Set-User-ID_ | **A âncora de privilégio.** Quando o `execve` executa um binário SUID, ele salva uma cópia do novo `EUID` (neste caso, `0`) no `SUID`. Isso permite transitar entre privilégios sem perder o direito de reassumir root.                                                                                                  |

### A Tríade de GIDs e Grupos Suplementares

- **RGID** (_Real Group ID_), **EGID** (_Effective Group ID_) e **SGID** (_Saved Set-Group-ID_).
- **Grupos Suplementares (_Supplementary Groups_):** O conjunto de grupos adicionais vinculados ao processo. A chamada `initgroups("root", 0)` zera essa lista para garantir que privilégios de grupos do chamador (ex: `docker`, `lxd`, `wheel`) não vazem para o processo root executado.

---

### Por que a Leitura de Senha é Feita via `/dev/tty` e não `stdin`?

Se um utilitário de senha lesse de `stdin` (`scanf`, `fgets`):

1. **Ataques por Redirecionamento e Pipe:** Um script malicioso ou comando anterior poderia injetar senhas falsas ou capturar entradas via `rtdo < arquivo_senhas` ou `cat script | rtdo`.
2. **Terminal Controlador Direto:** Ao abrir explicitamente `/dev/tty` com a flag `O_NOCTTY`, garantimos conexão direta com a sessão interativa do usuário físico no console, rejeitando injeções por pipes descontrolados.
3. **Controle de Eco (_ECHO_):** Usando `tcgetattr` e `tcsetattr`, desativamos a flag `ECHO` do subsistema `termios`. Os caracteres digitados não são refletidos na tela nem gravados no buffer visível do terminal.

---

### A Armadilha do `SIGINT` (Ctrl+C) e a Restauração de Sinais

Quando a flag `ECHO` do terminal está desativada, se o usuário pressionar `Ctrl+C` (`SIGINT`) ou `Ctrl+\` (`SIGQUIT`), o comportamento padrão do kernel seria abortar o processo imediatamente.
Se isso acontecesse:

- O processo morreria sem reativar o `ECHO`.
- O terminal do usuário ficaria permanentemente "mudo" (o usuário digita comandos no shell mas nada aparece na tela).
- **Solução no `rtdo`:** Registramos tratadores de sinal assíncronos (`setup_signals`). Ao receber uma interrupção, o manipulador restaura imediatamente a configuração original do terminal salva em `g_saved_termios` e encerra com código `130` (padrão POSIX para processo terminado por `SIGINT`).

---

### Higiene de Memória: Por que `explicit_bzero` e não `memset`?

Ao armazenar senhas em buffers locais de memória:

```c
char password[128];
```

Se usássemos:

```c
memset(password, 0, sizeof(password));
```

Compiladores modernos C com otimizações ativadas (`-O2` ou `-O3`) aplicam uma técnica chamada **Dead Store Elimination** (Eliminação de Escritas Mortas). Como o buffer `password` não é mais lido após o `memset` e está prestes a sair do escopo, o otimizador do compilador pode descartar a instrução de `memset` por considerá-la "inútil".
O resultado: **a senha do usuário permaneceria intacta na pilha de memória RAM**, vulnerável a extração via despejos de memória (_core dumps_) ou exploits de leitura de pilha.
A função `explicit_bzero()` (ou `memset_s`) garante ao compilador uma barreira de memória (_compiler memory barrier_), impedindo que a limpeza seja descartada na otimização.

---

### Transição Final com `setuid(0)` e `setgid(0)`

Ao carregar o comando final via `execvp`, precisamos garantir que o processo filho seja root pleno:

1. Shells como `bash` descartam privilégios se detectarem `RUID != EUID`.
2. Ao executar `setuid(0)` e `setgid(0)`, convertemos todas as três identidades para zero:
   $$\text{RUID} = \text{EUID} = \text{SUID} = 0$$
3. As variáveis perigosas `LD_PRELOAD`, `LD_LIBRARY_PATH` e `IFS` são removidas previamente para impedir sequestro de chamadas dinâmicas durante a carga do novo binário.
