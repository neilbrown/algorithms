#line 135 "../mdcode.mdc"
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>

#include "mdcode.h"
#line 507 "../mdcode.mdc"
#include  <ctype.h>
#include  <string.h>
#line 255 "../mdcode.mdc"
static void code_linearize(struct code_node *code)
{
	struct code_node *t;
	for (t = code; t; t = t->next)
		t->indent = 0;
	for (; code; code = code->next)
		if (code->child) {
			struct code_node *next = code->next;
			struct section *child = code->child;
			int indent = child->indent;

			code->next = code->child->code;
			code->child->code = NULL;
			code->child = NULL;
			for (t = code; t->next; t = t->next)
				t->next->indent = code->indent + indent;
			t->next = next;
		}
}
#line 279 "../mdcode.mdc"
void code_free(struct code_node *code)
{
	while (code) {
		struct code_node *this;
		if (code->child)
			code_linearize(code);
		this = code;
		code = code->next;
		free(this);
	}
}
#line 307 "../mdcode.mdc"
static void code_add_text(struct section *where, struct text txt,
                          int line_no, bool was_marked)
{
	struct code_node *n;
	if (txt.len == 0)
		return;
	n = calloc(1, sizeof(*n));
	n->code = txt;
	n->line_no = line_no;
	if (was_marked) {
		n->strip_size = 0;
	} else {
		if (txt.txt[0] == '\t')
			n->strip_size = 8;
		else
			n->strip_size = 4;
	}
	if (where->last)
		where->last->next = n;
	else
		where->code = n;
	where->last = n;
}
#line 334 "../mdcode.mdc"
void code_add_link(struct section *where, struct section *to,
		   int indent)
{
	struct code_node *n;

	to->indent = indent;
	to->refcnt++;	// this will be checked elsewhere
	if (where->last && where->last->child == NULL) {
		where->last->child = to;
		return;
	}
	n = calloc(1, sizeof(*n));
	n->child = to;
	if (where->last)
		where->last->next = n;
	else
		where->code = n;
	where->last = n;
}
#line 374 "../mdcode.mdc"
int text_cmp(struct text a, struct text b)
{
	int len = a.len;
	if (len > b.len)
		len = b.len;
	int cmp = strncmp(a.txt, b.txt, len);
	if (cmp)
		return cmp;
	else
		return a.len - b.len;
}

static struct section *section_find(struct section **list, struct text name)
{
	struct section *new;
	for (; *list; list = &(*list)->next) {
		int cmp = text_cmp((*list)->section_name, name);
		if (cmp == 0)
			return *list;
		if (cmp > 0)
			break;
	}
	/* Add the section here */
	new = calloc(1, sizeof(*new));
	new->next = *list;
	*list = new;
	new->section_name = name;
	return new;
}
#line 456 "../mdcode.mdc"
static char *skip_lws(char *pos, char *end)
{
	while (pos < end && (*pos == ' ' || *pos == '\t'))
		pos++;
	return pos;
}

static char *skip_line(char *pos, char *end)
{
	while (pos < end && *pos != '\n')
		pos++;
	if (pos < end)
		pos++;
	return pos;
}

static char *skip_para(char *pos, char *end, int *line_no)
{
	/* Might return a pointer to a blank line, as only
	 * one trailing blank line is skipped
	 */
	if (*pos == '#') {
		pos = skip_line(pos, end);
		(*line_no) += 1;
		return pos;
	}
	while (pos < end &&
	       *pos != '#' &&
	       *(pos = skip_lws(pos, end)) != '\n') {
		pos = skip_line(pos, end);
		(*line_no) += 1;
	}
	if (pos < end && *pos == '\n') {
		pos++;
		(*line_no) += 1;
	}
	return pos;
}
#line 512 "../mdcode.mdc"
static struct text take_header(char *pos, char *end)
{
	struct text section;

	while (pos < end && *pos == '#')
		pos++;
	while (pos < end && *pos == ' ')
		pos++;
	section.txt = pos;
	while (pos < end && *pos != '\n')
		pos++;
	// A section heading can have trailing #
	while (pos > section.txt &&
	       (pos[-1] == '#' || pos[-1] == ' '))
		pos--;
	section.len = pos - section.txt;
	return section;
}

static int is_list(char *pos, char *end)
{
	if (strchr("-*+", *pos))
		return 1;
	if (isdigit(*pos)) {
		while (pos < end && isdigit(*pos))
			pos += 1;
		if  (pos < end && *pos == '.')
			return 1;
	}
	return 0;
}

static int matches(char *start, char *pos, char *end)
{
	if (start == NULL)
		return matches("\t", pos, end) ||
		       matches("    ", pos, end);
	return (pos + strlen(start) < end &&
		strncmp(pos, start, strlen(start)) == 0);
}
#line 591 "../mdcode.mdc"
static int count_space(char *sol, char *p)
{
	int c = 0;
	while (sol < p) {
		if (sol[0] == ' ')
			c++;
		if (sol[0] == '\t')
			c+= 8;
		sol++;
	}
	return c;
}

static char *take_code(char *pos, char *end, char *marker,
		       struct section **table, struct text section,
		       int *line_nop)
{
	char *start = pos;
	int line_no = *line_nop;
	int start_line = line_no;
	struct section *sect;

	sect = section_find(table, section);

	while (pos < end) {
		char *sol, *t;
		struct text ref;

		if (marker && matches(marker, pos, end))
			break;
		if (!marker &&
		    (skip_lws(pos, end))[0] != '\n' &&
		    !matches(NULL, pos, end))
			/* Paragraph not indented */
			break;

		/* Still in code - check for reference */
		sol = pos;
		if (!marker) {
			if (*sol == '\t')
				sol++;
			else if (strcmp(sol, "    ") == 0)
				sol += 4;
		}
		t = skip_lws(sol, end);
		if (t[0] != '#' || t[1] != '#') {
			/* Just regular code here */
			pos = skip_line(sol, end);
			line_no++;
			continue;
		}

		if (pos > start) {
			struct text txt;
			txt.txt = start;
			txt.len = pos - start;
			code_add_text(sect, txt, start_line, !!marker);
		}
		ref = take_header(t, end);
		if (ref.len) {
			struct section *refsec = section_find(table, ref);
			code_add_link(sect, refsec, count_space(sol, t));
		}
		pos = skip_line(t, end);
		line_no++;
		start = pos;
		start_line = line_no;
	}
	if (pos > start) {
		struct text txt;
		txt.txt = start;
		txt.len = pos - start;
		/* strip trailing blank lines */
		while (!marker && txt.len > 2 &&
		       start[txt.len-1] == '\n' &&
		       start[txt.len-2] == '\n')
			txt.len -= 1;

		code_add_text(sect, txt, start_line, !!marker);
	}
	if (marker) {
		pos = skip_line(pos, end);
		line_no++;
	}
	*line_nop = line_no;
	return pos;
}
#line 688 "../mdcode.mdc"
static struct section *code_find(char *pos, char *end)
{
	struct section *table = NULL;
	bool in_list = false;
	int line_no = 1;
	struct text section = {};

	while (pos < end) {
		if (pos[0] == '#') {
			section = take_header(pos, end);
			in_list = false;
			pos = skip_line(pos, end);
			line_no++;
		} else if (is_list(pos, end)) {
			in_list = true;
			pos = skip_para(pos, end, &line_no);
		} else if (!in_list && matches(NULL, pos, end)) {
			pos = take_code(pos, end, NULL, &table,
					section, &line_no);
		} else if (matches("```", pos, end)) {
			in_list = false;
			pos = skip_line(pos, end);
			line_no++;
			pos = take_code(pos, end, "```", &table,
					section, &line_no);
		} else if (matches("~~~", pos, end)) {
			in_list = false;
			pos = skip_line(pos, end);
			line_no++;
			pos = take_code(pos, end, "~~~", &table,
					section, &line_no);
		} else {
			if (!isspace(*pos))
				in_list = false;
			pos = skip_para(pos, end, &line_no);
		}
	}
	return table;
}
#line 749 "../mdcode.mdc"
struct section *code_extract(char *pos, char *end, code_err_fn error)
{
	struct section *table;
	struct section *result = NULL;
	struct section *tofree = NULL;

	table = code_find(pos, end);

	while (table) {
		struct section *t = table->next;
		if (table->last == NULL) {
			char *msg;
			asprintf(&msg,
				"Section \"%.*s\" is referenced but not declared",
				 table->section_name.len, table->section_name.txt);
			error(msg);
			free(msg);
		}
		if (table->refcnt == 0) {
			/* Root-section,  return it */
			table->next = result;
			result = table;
			code_linearize(result->code);
		} else {
			table->next = tofree;
			tofree = table;
			if (table->refcnt > 1) {
				char *msg;
				asprintf(&msg,
					 "Section \"%.*s\" referenced multiple times (%d).",
					 table->section_name.len, table->section_name.txt,
					 table->refcnt);
				error(msg);
				free(msg);
			}
		}
		table = t;
	}
	/* Free all the non-top-level section - their code will
	 * be moved to a top-level.
	 */
	while (tofree) {
		struct section *t = tofree->next;
		free(tofree);
		tofree = t;
	}
	return result;
}
#line 827 "../mdcode.mdc"
void code_node_print(FILE *out, struct code_node *node, char *fname)
{
	for (; node; node = node->next) {
		char *c = node->code.txt;
		int len = node->code.len;

		if (!len)
			continue;

		if (fname)
			fprintf(out, "#line %d \"%s\"\n",
				node->line_no, fname);
		while (len && *c) {
			if (node->indent >= 8)
				fprintf(out, "\t%*s", node->indent - 8, "");
			else
				fprintf(out, "%*s", node->indent, "");
			if (node->strip_size && len > 1) {
				if (*c == '\t') {
					c++;
					len--;
				} else if (*c == ' ') {
					int s = node->strip_size;
					if (s > len)
						s = len;
					c += s;
					len-= s;
				}
			}
			do {
				fputc(*c, out);
				c++;
				len--;
			} while (len && c[-1] != '\n');
		}
	}
}
#line 142 "../mdcode.mdc"

