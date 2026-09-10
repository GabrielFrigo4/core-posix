#ifndef _DEFAULT_SOURCE
#define _DEFAULT_SOURCE
#endif
#ifndef _XOPEN_SOURCE
#define _XOPEN_SOURCE 700
#endif

#include <fcntl.h>
#include <grp.h>
#include <paths.h>
#include <pwd.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/types.h>
#include <termios.h>
#include <unistd.h>

#if defined(__linux__)
#include <shadow.h>
#endif

#define MAX_PASSWORD_LEN 128
#define MAX_PROMPT_LEN 128
#define MAX_SUPP_GROUPS 64

static volatile sig_atomic_t g_interrupted = 0;
static struct termios g_saved_termios;
static int g_tty_fd = -1;

static void signal_handler(int sig)
{
	(void)sig;
	g_interrupted = 1;
	if (g_tty_fd >= 0)
	{
		tcsetattr(g_tty_fd, TCSANOW, &g_saved_termios);
	}
	_exit(130);
}

static int verificar_grupo_autorizado(void)
{
	if (getuid() == 0)
	{
		return 1;
	}

	struct group *gr = getgrnam("wheel");
	if (gr == NULL)
	{
		/* Fallback para distros Linux onde 'wheel' nao existe */
		gr = getgrnam("sudo");
	}

	if (gr == NULL)
	{
		return 0;
	}

	gid_t gid_alvo = gr->gr_gid;
	if (getgid() == gid_alvo)
	{
		return 1;
	}

	gid_t groups[MAX_SUPP_GROUPS];
	int ngroups = getgroups(MAX_SUPP_GROUPS, groups);
	if (ngroups <= 0)
	{
		return 0;
	}

	for (int i = 0; i < ngroups; i++)
	{
		if (groups[i] == gid_alvo)
		{
			return 1;
		}
	}

	return 0;
}

static const char *obter_hash_usuario(struct passwd *pwd)
{
#if defined(__linux__)
	struct spwd *sp = getspnam(pwd->pw_name);
	if (sp == NULL)
	{
		return NULL;
	}
	return sp->sp_pwdp;
#else
	return pwd->pw_passwd;
#endif
}

static int conta_bloqueada(const char *hash)
{
	if (hash == NULL || hash[0] == '\0')
	{
		return 1;
	}
	/* Contas bloqueadas ou desabilitadas em sistemas Unix */
	if (hash[0] == '!' || hash[0] == '*' || hash[0] == 'x')
	{
		return 1;
	}
	return 0;
}

static int ler_senha_tty(const char *prompt, char *buffer, size_t tamanho)
{
	struct termios new_term;
	struct sigaction sa, old_sa_int, old_sa_quit, old_sa_term;
	size_t i = 0;
	char c;
	ssize_t n;

	g_tty_fd = open("/dev/tty", O_RDWR | O_NOCTTY);
	if (g_tty_fd < 0)
	{
		perror("rtdo: open /dev/tty");
		return -1;
	}

	if (tcgetattr(g_tty_fd, &g_saved_termios) != 0)
	{
		perror("rtdo: tcgetattr");
		close(g_tty_fd);
		g_tty_fd = -1;
		return -1;
	}

	/* Configurar tratadores de sinal para restaurar TTY em caso de interrupcao */
	memset(&sa, 0, sizeof(sa));
	sa.sa_handler = signal_handler;
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = 0;

	sigaction(SIGINT, &sa, &old_sa_int);
	sigaction(SIGQUIT, &sa, &old_sa_quit);
	sigaction(SIGTERM, &sa, &old_sa_term);

	new_term = g_saved_termios;
	new_term.c_lflag &= ~(tcflag_t)ECHO;
	tcsetattr(g_tty_fd, TCSAFLUSH, &new_term);

	if (write(g_tty_fd, prompt, strlen(prompt)) < 0)
	{
		/* falha de escrita tratada silenciosamente */
	}

	while (i < tamanho - 1 && !g_interrupted)
	{
		n = read(g_tty_fd, &c, 1);
		if (n <= 0)
		{
			break;
		}
		if (c == '\n' || c == '\r')
		{
			break;
		}
		buffer[i++] = c;
	}
	buffer[i] = '\0';

	if (write(g_tty_fd, "\n", 1) < 0)
	{
		/* falha de escrita tratada silenciosamente */
	}

	tcsetattr(g_tty_fd, TCSANOW, &g_saved_termios);

	/* Restaurar handlers de sinal originais */
	sigaction(SIGINT, &old_sa_int, NULL);
	sigaction(SIGQUIT, &old_sa_quit, NULL);
	sigaction(SIGTERM, &old_sa_term, NULL);

	close(g_tty_fd);
	g_tty_fd = -1;

	return g_interrupted ? -1 : 0;
}

static void sanitizar_ambiente(void)
{
	/* Variaveis comumente abusadas para hijacking de execucao */
	unsetenv("LD_PRELOAD");
	unsetenv("LD_LIBRARY_PATH");
	unsetenv("IFS");

	/* Garantir PATH seguro caso esteja vazio ou nulo */
	const char *path = getenv("PATH");
	if (path == NULL || path[0] == '\0')
	{
#if defined(_PATH_DEFPATH)
		setenv("PATH", _PATH_DEFPATH, 1);
#else
		setenv("PATH", "/usr/local/sbin:/usr/local/bin:/usr/sbin:/usr/bin:/sbin:/bin", 1);
#endif
	}
}

int main(int argc, char *argv[])
{
	char senha_digitada[MAX_PASSWORD_LEN];
	char prompt[MAX_PROMPT_LEN];
	struct passwd *pwd;
	const char *hash_alvo;
	char *hash_calculado;

	if (argc < 2)
	{
		fprintf(stderr, "Uso: %s <comando> [argumentos...]\n", argv[0]);
		return 1;
	}

	/* Validar SUID root ativo */
	if (geteuid() != 0)
	{
		fprintf(stderr, "rtdo: erro: binario requer SUID root (chmod 4750).\n");
		return 1;
	}

	/* Validar restricao ao grupo wheel/root */
	if (!verificar_grupo_autorizado())
	{
		fprintf(stderr, "rtdo: acesso negado: requer pertencer ao grupo 'wheel'.\n");
		return 1;
	}

	pwd = getpwuid(getuid());
	if (pwd == NULL)
	{
		perror("rtdo: getpwuid");
		return 1;
	}

	hash_alvo = obter_hash_usuario(pwd);
	if (hash_alvo == NULL)
	{
		fprintf(stderr, "rtdo: falha ao acessar credenciais do usuario.\n");
		return 1;
	}

	if (conta_bloqueada(hash_alvo))
	{
		fprintf(stderr, "rtdo: conta bloqueada ou desprovida de senha.\n");
		return 1;
	}

	snprintf(prompt, sizeof(prompt), "[rtdo] Senha para %s: ", pwd->pw_name);
	if (ler_senha_tty(prompt, senha_digitada, sizeof(senha_digitada)) != 0)
	{
		fprintf(stderr, "rtdo: erro ao interagir com o terminal.\n");
		return 1;
	}

	hash_calculado = crypt(senha_digitada, hash_alvo);
	explicit_bzero(senha_digitada, sizeof(senha_digitada));

	if (hash_calculado == NULL || strcmp(hash_calculado, hash_alvo) != 0)
	{
		fprintf(stderr, "rtdo: senha incorreta.\n");
		return 1;
	}

	/* Higienizar ambiente antes de assumir root e executar comando */
	sanitizar_ambiente();

	/* Assumir grupos e identidade de root */
	if (initgroups("root", 0) != 0 || setgid(0) != 0 || setuid(0) != 0)
	{
		perror("rtdo: falha ao assumir credenciais de root");
		return 1;
	}

	execvp(argv[1], &argv[1]);
	perror("rtdo: execvp");
	return 1;
}
