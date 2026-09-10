#define _DEFAULT_SOURCE
#define _XOPEN_SOURCE 700

#include <errno.h>
#include <fcntl.h>
#include <grp.h>
#include <paths.h>
#include <pwd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

#define MAX_SUPP_GROUPS 64

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
	if (argc < 2)
	{
		fprintf(stderr, "Uso: %s <comando> [argumentos...]\n", argv[0]);
		return 1;
	}

	/* Validar SUID root ativo */
	if (geteuid() != 0)
	{
		fprintf(stderr, "rtgo: erro: binario requer SUID root (chmod 4750).\n");
		return 1;
	}

	/* Validar restricao ao grupo wheel/root */
	if (!verificar_grupo_autorizado())
	{
		fprintf(stderr, "rtgo: acesso negado: requer pertencer ao grupo 'wheel'.\n");
		return 1;
	}

	/* Higienizar ambiente antes de assumir root e executar comando */
	sanitizar_ambiente();

	/* Assumir grupos e identidade de root */
	if (initgroups("root", 0) != 0 || setgid(0) != 0 || setuid(0) != 0)
	{
		perror("rtgo: falha ao assumir credenciais de root");
		return 1;
	}

	execvp(argv[1], &argv[1]);
	perror("rtgo: execvp");
	return 1;
}
