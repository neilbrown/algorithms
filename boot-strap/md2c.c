#line 156 "../mdcode.mdc"
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>

#include "mdcode.h"

#line 911 "../mdcode.mdc"
#include <fcntl.h>
#include <errno.h>
#include <sys/mman.h>
#include <string.h>
#line 74 "../mdcode.mdc"
static bool is_top_level(struct text section)
{
	int i;
	for (i = 1; i < section.len; i++) {
		if (section.txt[i] == ' ')
			return section.txt[i-1] == ':';
	}
	return false;
}
#line 883 "../mdcode.mdc"
static void copy_fname(char *name, int space, struct text t)
{
	char *sec = t.txt;
	int len = t.len;
	name[0] = 0;
	if (len < 5 || strncmp(sec, "File:", 5) != 0)
		return;
	sec += 5;
	len -= 5;
	while (len && sec[0] == ' ') {
		sec++;
		len--;
	}
	if (len >= space)
		len = space - 1;
	strncpy(name, sec, len);
	name[len] = 0;
}
#line 918 "../mdcode.mdc"
static int errs;
static void pr_err(char *msg)
{
	errs++;
	fprintf(stderr, "%s\n", msg);
}

int main(int argc, char *argv[])
{
	int fd;
	size_t len;
	char *file;
	struct text section = {};
	struct section *table, *s, *prev;

	errs = 0;
	if (argc != 2 && argc != 3) {
		fprintf(stderr, "Usage: mdcode file.mdc [section]\n");
		exit(2);
	}
	if (argc == 3) {
		section.txt = argv[2];
		section.len = strlen(argv[2]);
	}

	fd = open(argv[1], O_RDONLY);
	if (fd < 0) {
		fprintf(stderr, "mdcode: cannot open %s: %s\n",
			argv[1], strerror(errno));
		fprintf(stderr, "Usage: mdcode file.mdc [section]\n");
		exit(1);
	}
	len = lseek(fd, 0, 2);
	file = mmap(NULL, len, PROT_READ, MAP_SHARED, fd, 0);
	table = code_extract(file, file+len, pr_err);

	for (s = table; s;
		(code_free(s->code), prev = s, s = s->next, free(prev))) {
		FILE *fl;
		char fname[1024];

		if (is_top_level(s->section_name)) {
			if (strncmp(s->section_name.txt, "File: ", 6) != 0 &&
			    (section.txt == NULL ||
			     text_cmp(s->section_name, section) != 0))
				/* Ignore this section */
				continue;
		} else {
			fprintf(stderr, "Code in unreferenced section that is not ignored or a file name: %.*s\n",
				s->section_name.len, s->section_name.txt);
			errs++;
			continue;
		}
		if (section.txt) {
			if (text_cmp(s->section_name, section) == 0)
				code_node_print(stdout, s->code, argv[1]);
			break;
		}
		copy_fname(fname, sizeof(fname), s->section_name);
		if (fname[0] == 0) {
			fprintf(stderr, "Missing file name at:%.*s\n",
				s->section_name.len, s->section_name.txt);
			errs++;
			continue;
		}
		fl = fopen(fname, "w");
		if (!fl) {
			fprintf(stderr, "Cannot create %s: %s\n",
				fname, strerror(errno));
			errs++;
			continue;
		}
		code_node_print(fl, s->code, argv[1]);
		fclose(fl);
	}
	exit(!!errs);
}
#line 164 "../mdcode.mdc"

