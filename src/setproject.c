#include <stdio.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#include <fcntl.h>
#include <getopt.h>
#include <libgen.h>
#include <unistd.h>
#include <stdlib.h>
#include "project.h"

int fsetproject(const char *name, int project)
{
	struct stat buf;
	int fd;
	int ret;
	int save_errno = 0;
	struct fsxattr fsxattr;

	if (stat(name, &buf) == -1)
		return -1;

	fd = open(name, OPEN_FLAGS);
	if (fd == -1)
		return -1;

	ret = ioctl(fd, EXT4_IOC_FSGETXATTR, &fsxattr);
	if (ret < 0) {
		save_errno = errno;
		ret = -1;
		goto out;
	}

	fsxattr.fsx_projid = project;

	ret = ioctl(fd, EXT4_IOC_FSSETXATTR, &fsxattr);
	if (ret < 0) {
		save_errno = errno;
		ret = -1;
		goto out;
	}

out:
	close(fd);
	if (save_errno)
		errno = save_errno;
	return ret;
}

struct option long_options[] = {
	{ "help",		0, 0, 'H' },
	{ "value",		1, 0, 'v' },
	{ "version",		0, 0, 'V' },
	{ NULL,			0, 0, 0 }
};

const char *progname;

void help(void)
{
	printf("%s %s -- set project indentifier\n", progname, VERSION);
	printf("Usage: %s [-v value] [-h] file...\n", progname);
	printf(
"  -v, --value=value       use value as the project indentifier\n"
"      --version           print version and exit\n"
"      --help              this help text\n");
}

int main(int argc, char **argv)
{
	int project;
	int project_set = 0;
	int ret;
	int error_number = 0;
	int opt;
	char *tmp;

	progname = basename(argv[0]);

	while ((opt = getopt_long(argc, argv, "Hv:V",
				  long_options, NULL)) != -1) {
		switch(opt) {
		case 'v':
			if (project_set) {
				help();
				return 2;
			}
			project = strtoul(optarg, &tmp, 0);;
			project_set++;
			break;
		case 'V':
			printf("%s " VERSION "\n", progname);
			return 0;
		case 'H':
			help();
			return 0;
		default:
			help();
			return 2;
		}
	}

	if (optind >= argc || project_set == 0) {
		help();
		return 2;
	}

	while (optind < argc) {
		ret = fsetproject(argv[optind], project);
		if (ret < 0) {
			fprintf(stderr, "failed to set project of %s: %s\n",
				argv[optind], strerror(errno));
			error_number++;
		}
		optind++;
	}

	return error_number ? 1 : 0;
}
