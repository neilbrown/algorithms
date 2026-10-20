#line 187 "../mdcode.mdc"
struct text {
	char *txt;
	int len;
};

struct section {
	struct text section_name;
	struct code_node *code;
	struct section *next;
#line 215 "../mdcode.mdc"
	struct code_node *last;
	int refcnt;
	int indent;
#line 197 "../mdcode.mdc"
};

struct code_node {
	struct text code;
	struct code_node *next;
	struct section *child;
#line 225 "../mdcode.mdc"
	int line_no;
	int strip_size;
#line 245 "../mdcode.mdc"
	int indent;
#line 204 "../mdcode.mdc"
};
#line 745 "../mdcode.mdc"
typedef void (*code_err_fn)(char *msg);
#line 293 "../mdcode.mdc"
void code_free(struct code_node *code);
#line 370 "../mdcode.mdc"
int text_cmp(struct text a, struct text b);
#line 800 "../mdcode.mdc"
struct section *code_extract(char *pos, char *end, code_err_fn error);
#line 866 "../mdcode.mdc"
void code_node_print(FILE *out, struct code_node *node, char *fname);
#line 128 "../mdcode.mdc"

