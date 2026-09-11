#ifndef _DEFAULT_SOURCE
#define _DEFAULT_SOURCE
#endif
#ifndef _XOPEN_SOURCE
#define _XOPEN_SOURCE 700
#endif

#include <grp.h>
#include <paths.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>

#define MAX_SUPP_GROUPS 64

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
		fprintf(stderr, "rtgo: error: binary requires SUID root (chmod 4750).\n");
		return 1;
	}

	if (!is_authorized_group())
	{
		fprintf(stderr, "rtgo: access denied: caller must belong to 'wheel' group.\n");
		return 1;
	}

	sanitize_environment();

	if (assume_root() != 0)
	{
		perror("rtgo: failed to assume root credentials");
		return 1;
	}

	execvp(argv[1], &argv[1]);
	perror("rtgo: execvp");
	return 1;
}
