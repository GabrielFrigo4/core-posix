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

static const char *obter_hash_usuario(const struct passwd *pwd)
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

	return (hash[0] == '!' || hash[0] == '*' || hash[0] == 'x');
}

static void escrever_tty(int fd, const char *str)
{
	size_t len = strlen(str);
	ssize_t written = write(fd, str, len);
	(void)written;
}

static void configurar_sinais(
    struct sigaction *old_int, struct sigaction *old_quit, struct sigaction *old_term
)
{
	struct sigaction sa;
	memset(&sa, 0, sizeof(sa));
	sa.sa_handler = signal_handler;
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = 0;

	sigaction(SIGINT, &sa, old_int);
	sigaction(SIGQUIT, &sa, old_quit);
	sigaction(SIGTERM, &sa, old_term);
}

static void restaurar_sinais(
    const struct sigaction *old_int, const struct sigaction *old_quit,
    const struct sigaction *old_term
)
{
	sigaction(SIGINT, old_int, NULL);
	sigaction(SIGQUIT, old_quit, NULL);
	sigaction(SIGTERM, old_term, NULL);
}

static int ler_senha_tty(const char *prompt, char *buffer, size_t tamanho)
{
	struct termios new_term;
	struct sigaction old_sa_int, old_sa_quit, old_sa_term;
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

	configurar_sinais(&old_sa_int, &old_sa_quit, &old_sa_term);

	new_term = g_saved_termios;
	new_term.c_lflag &= ~(tcflag_t)ECHO;
	tcsetattr(g_tty_fd, TCSAFLUSH, &new_term);

	escrever_tty(g_tty_fd, prompt);

	while (i < tamanho - 1 && !g_interrupted)
	{
		n = read(g_tty_fd, &c, 1);
		if (n <= 0 || c == '\n' || c == '\r')
		{
			break;
		}
		buffer[i++] = c;
	}
	buffer[i] = '\0';

	escrever_tty(g_tty_fd, "\n");
	tcsetattr(g_tty_fd, TCSANOW, &g_saved_termios);
	restaurar_sinais(&old_sa_int, &old_sa_quit, &old_sa_term);

	close(g_tty_fd);
	g_tty_fd = -1;

	return g_interrupted ? -1 : 0;
}

static void sanitizar_ambiente(void)
{
	unsetenv("LD_PRELOAD");
	unsetenv("LD_LIBRARY_PATH");
	unsetenv("IFS");

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

static int autenticar_usuario(const struct passwd *pwd)
{
	char senha_digitada[MAX_PASSWORD_LEN];
	char prompt[MAX_PROMPT_LEN];
	const char *hash_alvo;
	char *hash_calculado;

	hash_alvo = obter_hash_usuario(pwd);
	if (hash_alvo == NULL)
	{
		fprintf(stderr, "rtdo: falha ao acessar credenciais do usuario.\n");
		return -1;
	}

	if (conta_bloqueada(hash_alvo))
	{
		fprintf(stderr, "rtdo: conta bloqueada ou desprovida de senha.\n");
		return -1;
	}

	snprintf(prompt, sizeof(prompt), "[rtdo] Senha para %s: ", pwd->pw_name);
	if (ler_senha_tty(prompt, senha_digitada, sizeof(senha_digitada)) != 0)
	{
		fprintf(stderr, "rtdo: erro ao interagir com o terminal.\n");
		return -1;
	}

	hash_calculado = crypt(senha_digitada, hash_alvo);
	explicit_bzero(senha_digitada, sizeof(senha_digitada));

	if (hash_calculado == NULL || strcmp(hash_calculado, hash_alvo) != 0)
	{
		fprintf(stderr, "rtdo: senha incorreta.\n");
		return -1;
	}

	return 0;
}

static int assumir_root(void)
{
	if (initgroups("root", 0) != 0 || setgid(0) != 0 || setuid(0) != 0)
	{
		return -1;
	}
	return 0;
}

int main(int argc, char *argv[])
{
	if (argc < 2)
	{
		fprintf(stderr, "Uso: %s <comando> [argumentos...]\n", argv[0]);
		return 1;
	}

	if (geteuid() != 0)
	{
		fprintf(stderr, "rtdo: erro: binario requer SUID root (chmod 4750).\n");
		return 1;
	}

	if (!verificar_grupo_autorizado())
	{
		fprintf(stderr, "rtdo: acesso negado: requer pertencer ao grupo 'wheel'.\n");
		return 1;
	}

	struct passwd *pwd = getpwuid(getuid());
	if (pwd == NULL)
	{
		perror("rtdo: getpwuid");
		return 1;
	}

	if (autenticar_usuario(pwd) != 0)
	{
		return 1;
	}

	sanitizar_ambiente();

	if (assumir_root() != 0)
	{
		perror("rtdo: falha ao assumir credenciais de root");
		return 1;
	}

	execvp(argv[1], &argv[1]);
	perror("rtdo: execvp");
	return 1;
}
