#define _DEFAULT_SOURCE
#define _XOPEN_SOURCE 700

#include <errno.h>
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

static volatile sig_atomic_t g_interrupted = 0;
static struct termios g_saved_termios;
static int g_tty_fd = -1;

static void signal_handler(int sig) {
    (void)sig;
    g_interrupted = 1;
    if (g_tty_fd >= 0) {
        tcsetattr(g_tty_fd, TCSANOW, &g_saved_termios);
    }
    _exit(130);
}

static const char *obter_hash_usuario(struct passwd *pwd) {
#if defined(__linux__)
    struct spwd *sp = getspnam(pwd->pw_name);
    if (sp == NULL) {
        return NULL;
    }
    return sp->sp_pwdp;
#else
    return pwd->pw_passwd;
#endif
}

static int conta_bloqueada(const char *hash) {
    if (hash == NULL || hash[0] == '\0') {
        return 1;
    }
    /* Contas bloqueadas ou desabilitadas em sistemas Unix */
    if (hash[0] == '!' || hash[0] == '*' || hash[0] == 'x') {
        return 1;
    }
    return 0;
}

static int ler_senha_tty(const char *prompt, char *buffer, size_t tamanho) {
    struct termios new_term;
    struct sigaction sa, old_sa_int, old_sa_quit, old_sa_term;
    size_t i = 0;
    char c;
    ssize_t n;

    g_tty_fd = open("/dev/tty", O_RDWR | O_NOCTTY);
    if (g_tty_fd < 0) {
        perror("msud: open /dev/tty");
        return -1;
    }

    if (tcgetattr(g_tty_fd, &g_saved_termios) != 0) {
        perror("msud: tcgetattr");
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
    new_term.c_lflag &= ~(ECHO);
    tcsetattr(g_tty_fd, TCSAFLUSH, &new_term);

    if (write(g_tty_fd, prompt, strlen(prompt)) < 0) {
        /* write falhou */
    }

    while (i < tamanho - 1 && !g_interrupted) {
        n = read(g_tty_fd, &c, 1);
        if (n <= 0) {
            break;
        }
        if (c == '\n' || c == '\r') {
            break;
        }
        buffer[i++] = c;
    }
    buffer[i] = '\0';

    if (write(g_tty_fd, "\n", 1) < 0) {
        /* write newline falhou */
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

static void sanitizar_ambiente(void) {
    /* Variaveis comumente abusadas para hijacking de execucao */
    unsetenv("LD_PRELOAD");
    unsetenv("LD_LIBRARY_PATH");
    unsetenv("IFS");

    /* Garantir PATH seguro caso esteja vazio ou nulo */
    const char *path = getenv("PATH");
    if (path == NULL || path[0] == '\0') {
#if defined(_PATH_DEFPATH)
        setenv("PATH", _PATH_DEFPATH, 1);
#else
        setenv("PATH", "/usr/local/sbin:/usr/local/bin:/usr/sbin:/usr/bin:/sbin:/bin", 1);
#endif
    }
}

int main(int argc, char *argv[]) {
    char senha_digitada[MAX_PASSWORD_LEN];
    char prompt[MAX_PROMPT_LEN];
    struct passwd *pwd;
    const char *hash_alvo;
    char *hash_calculado;

    if (argc < 2) {
        fprintf(stderr, "Uso: %s <comando> [argumentos...]\n", argv[0]);
        return 1;
    }

    /* msud precisa ter permissao SUID root para ler shadow e alterar uid/gid */
    if (geteuid() != 0) {
        fprintf(stderr, "msud: erro: binario requer SUID root (chmod 4755).\n");
        return 1;
    }

    pwd = getpwuid(getuid());
    if (pwd == NULL) {
        perror("msud: getpwuid");
        return 1;
    }

    hash_alvo = obter_hash_usuario(pwd);
    if (hash_alvo == NULL) {
        fprintf(stderr, "msud: falha ao acessar credenciais do usuario.\n");
        return 1;
    }

    if (conta_bloqueada(hash_alvo)) {
        fprintf(stderr, "msud: conta bloqueada ou desprovida de senha.\n");
        return 1;
    }

    snprintf(prompt, sizeof(prompt), "[msud] Senha para %s: ", pwd->pw_name);
    if (ler_senha_tty(prompt, senha_digitada, sizeof(senha_digitada)) != 0) {
        fprintf(stderr, "msud: erro ao interagir com o terminal.\n");
        return 1;
    }

    hash_calculado = crypt(senha_digitada, hash_alvo);
    explicit_bzero(senha_digitada, sizeof(senha_digitada));

    if (hash_calculado == NULL || strcmp(hash_calculado, hash_alvo) != 0) {
        fprintf(stderr, "msud: senha incorreta.\n");
        return 1;
    }

    /* Higienizar ambiente antes de assumir root e executar comando */
    sanitizar_ambiente();

    /* Assumir grupos e identidade de root */
    if (initgroups("root", 0) != 0 || setgid(0) != 0 || setuid(0) != 0) {
        perror("msud: falha ao assumir credenciais de root");
        return 1;
    }

    execvp(argv[1], &argv[1]);
    perror("msud: execvp");
    return 1;
}
