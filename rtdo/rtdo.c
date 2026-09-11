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

static int is_authorized_group(void)
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

	gid_t target_gid = gr->gr_gid;
	if (getgid() == target_gid)
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
		if (groups[i] == target_gid)
		{
			return 1;
		}
	}

	return 0;
}

static const char *get_user_hash(const struct passwd *pwd)
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

static int is_account_locked(const char *hash)
{
	if (hash == NULL || hash[0] == '\0')
	{
		return 1;
	}

	return (hash[0] == '!' || hash[0] == '*' || hash[0] == 'x');
}

static void write_tty(int fd, const char *str)
{
	size_t len = strlen(str);
	ssize_t written = write(fd, str, len);
	(void)written;
}

static void setup_signals(
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

static void restore_signals(
    const struct sigaction *old_int, const struct sigaction *old_quit,
    const struct sigaction *old_term
)
{
	sigaction(SIGINT, old_int, NULL);
	sigaction(SIGQUIT, old_quit, NULL);
	sigaction(SIGTERM, old_term, NULL);
}

static int read_password_tty(const char *prompt, char *buffer, size_t size)
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

	setup_signals(&old_sa_int, &old_sa_quit, &old_sa_term);

	new_term = g_saved_termios;
	new_term.c_lflag &= ~(tcflag_t)ECHO;
	tcsetattr(g_tty_fd, TCSAFLUSH, &new_term);

	write_tty(g_tty_fd, prompt);

	while (i < size - 1 && !g_interrupted)
	{
		n = read(g_tty_fd, &c, 1);
		if (n <= 0 || c == '\n' || c == '\r')
		{
			break;
		}
		buffer[i++] = c;
	}
	buffer[i] = '\0';

	write_tty(g_tty_fd, "\n");
	tcsetattr(g_tty_fd, TCSANOW, &g_saved_termios);
	restore_signals(&old_sa_int, &old_sa_quit, &old_sa_term);

	close(g_tty_fd);
	g_tty_fd = -1;

	return g_interrupted ? -1 : 0;
}

static void sanitize_environment(void)
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

static int authenticate_user(const struct passwd *pwd)
{
	char password[MAX_PASSWORD_LEN];
	char prompt[MAX_PROMPT_LEN];
	const char *target_hash;
	char *computed_hash;

	target_hash = get_user_hash(pwd);
	if (target_hash == NULL)
	{
		fprintf(stderr, "rtdo: failed to access user credentials.\n");
		return -1;
	}

	if (is_account_locked(target_hash))
	{
		fprintf(stderr, "rtdo: account locked or password disabled.\n");
		return -1;
	}

	snprintf(prompt, sizeof(prompt), "[rtdo] Password for %s: ", pwd->pw_name);
	if (read_password_tty(prompt, password, sizeof(password)) != 0)
	{
		fprintf(stderr, "rtdo: error interacting with terminal.\n");
		return -1;
	}

	computed_hash = crypt(password, target_hash);
	explicit_bzero(password, sizeof(password));

	if (computed_hash == NULL || strcmp(computed_hash, target_hash) != 0)
	{
		fprintf(stderr, "rtdo: incorrect password.\n");
		return -1;
	}

	return 0;
}

static int assume_root(void)
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
		fprintf(stderr, "Usage: %s <command> [arguments...]\n", argv[0]);
		return 1;
	}

	if (geteuid() != 0)
	{
		fprintf(stderr, "rtdo: error: binary requires SUID root (chmod 4750).\n");
		return 1;
	}

	if (!is_authorized_group())
	{
		fprintf(stderr, "rtdo: access denied: caller must belong to 'wheel' group.\n");
		return 1;
	}

	struct passwd *pwd = getpwuid(getuid());
	if (pwd == NULL)
	{
		perror("rtdo: getpwuid");
		return 1;
	}

	if (authenticate_user(pwd) != 0)
	{
		return 1;
	}

	sanitize_environment();

	if (assume_root() != 0)
	{
		perror("rtdo: failed to assume root credentials");
		return 1;
	}

	execvp(argv[1], &argv[1]);
	perror("rtdo: execvp");
	return 1;
}
