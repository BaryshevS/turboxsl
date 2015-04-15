/*
 *  TurboXSL XML+XSLT processing library
 *  Main include for internal use. Interface functions and types
 *  are in global turboxsl.h include
 *
 *  (c) Egor Voznessenski, voznyak@mail.ru
 *
 *  $Id$
 *
**/

#ifndef LTR_XSL_H_
#define LTR_XSL_H_

#include <stdio.h>
#include <pthread.h>

#include "turboxsl.h"
#include "xmldict.h"
#include "threadpool.h"
#include "allocator.h"
#include "external_cache.h"
#include "concurrent_dictionary.h"
#include "logger.h"

typedef enum {EMPTY_NODE=0, ELEMENT_NODE, TEXT_NODE, ATTRIBUTE_NODE, PI_NODE, COMMENT_NODE, INFO_NODE, XPATH_NODE_VAR, XPATH_NODE_NOT,
XPATH_NODE_OR, XPATH_NODE_AND, XPATH_NODE_EQ, XPATH_NODE_NE, XPATH_NODE_LT, XPATH_NODE_GT, XPATH_NODE_CALL, KEY_NODE,
XPATH_NODE_LE, XPATH_NODE_GE, XPATH_NODE_ADD, XPATH_NODE_SUB, XPATH_NODE_MUL, XPATH_NODE_DIV, XPATH_NODE_MOD,
XPATH_NODE_FILTER, XPATH_NODE_ALL, XPATH_NODE_ATTR, XPATH_NODE_ROOT, XPATH_NODE_PARENT, XPATH_NODE_SELF, XPATH_NODE_ANCESTOR,
XPATH_NODE_FOLLOWING_SIBLING, XPATH_NODE_PRECEDING_SIBLING, XPATH_NODE_PRECEDING, XPATH_NODE_FOLLOWING, XPATH_NODE_SELECT,
XPATH_NODE_ATTR_ALL, XPATH_NODE_INT, XPATH_NODE_CONTEXT, XPATH_NODE_DESCENDANT, XPATH_NODE_DESCENDANT_OR_SELF,
XPATH_NODE_UNION, XPATH_NODE_ANCESTOR_OR_SELF
} NODETYPE;

typedef enum {VAL_NULL=0, VAL_BOOL, VAL_INT, VAL_NUMBER, VAL_STRING, VAL_NODESET} RVALUE_TYPE;

typedef struct _rval {
  RVALUE_TYPE type;
  union {
    long integer;
    double number;
    char *string;
    XMLNODE *nodeset;
  } v;
} RVALUE;

typedef XMLNODE XPATH_EXPR;

typedef enum {XML_FLAG_CDATA=1,XML_FLAG_NOESCAPE=2,XML_FLAG_SORTNUMBER=4,XML_FLAG_DESCENDING=8,XML_FLAG_LOWER=16} XML_FLAG;

struct _xmlnode {
  struct _xmlnode *parent;
  struct _xmlnode *next;
  struct _xmlnode *prev;
  struct _xmlnode *children;
  struct _xmlnode *attributes;
  XML_FLAG flags;
  unsigned position;
  unsigned order;
  void *uid;
  XPATH_EXPR *compiled;
  RVALUE extra;
  unsigned line;
  char *file;
  char *name;
  char *content;
  NODETYPE type;
  memory_allocator *allocator;
};

typedef struct _cbt {
    char *name;
    void (*func)();
}CB_TABLE;

typedef struct _var {
  char *name;
  RVALUE extra;
} XSL_VARIABLE;

struct _globaldata {
  memory_allocator *allocator;
  concurrent_dictionary *urldict;
  XMLDICT *revisions;
  CB_TABLE *perl_functions;  // linear search for functions - small number and sorted by usage statistics
  unsigned perl_cb_max;
  unsigned perl_cb_ptr;
  char *(*perl_cb_dispatcher)(void *fun,char **args,void *interpreter);
  void (*perl_urlcode)();
  void *interpreter;
  external_cache *cache;
  XSL_VARIABLE *vars;
  unsigned var_max;
  unsigned var_pos;
  unsigned initialized:1;
};

typedef enum {TMATCH_NONE, TMATCH_ALWAYS, TMATCH_ROOT, TMATCH_SELECT} MATCH_TYPE;

typedef struct {
  char *name;
  char *match;
  char *mode;
  XMLNODE *expr;
  MATCH_TYPE matchtype;
  XMLNODE *content;
  double priority;
} TEMPLATE;

typedef enum {XSL_FLAG_OUTPUT=1, XSL_FLAG_OMIT_DESC=2, XSL_FLAG_STANDALONE=4, XSL_FLAG_INDENT=8, XSL_FLAG_MODE_SET=16} XSL_FLAG;
typedef enum {MODE_NONE=0, MODE_XML, MODE_HTML, MODE_TEXT} OUTPUT_MODE;

struct _context {
  XSLTGLOBALDATA *gctx;
  struct threadpool *pool;
  memory_allocator *allocator;
  char *cache_key_prefix;
  char *url_local_prefix;
  char *docname;
  char *fnbuf;
  char *local_part;
  TEMPLATE *templtab;   // templates used in matching
  unsigned templno;
  unsigned templcnt;
  XMLDICT *named_templ; // templates for call'ing
  XMLNODE *root_node;
  XMLNODE *stylesheet;
  CB_TABLE *functions;  // linear search for functions - small number and sorted by usage statistics
  unsigned cb_max;
  unsigned cb_ptr;
  XSL_FLAG flags;
  unsigned rawout;
  XSL_VARIABLE *vars;   // Global (per conversion context) variables
  unsigned var_max;     // +
  unsigned var_pos;     // +
  concurrent_dictionary *expressions;
  char **sort_keys;     // sort tables for reuse. XXX must be rewritten to avoid conflicts (currently works because of rare use)
  XMLNODE **sort_nodes; // +
  unsigned sort_size;   // +
  char *charset;        // Just links to strings allocated and freed with global hash. No need to free
  char *doctype_public; // +
  char *doctype_system; // +
  char *media_type;     // +
  char *encoding;       // + -- This one is not really useful now since only UTF-8 is supported
  XMLNODE *keys;        // xsl:key data
  XMLNODE *formats;     // xsl:decimal-format data
  pthread_mutex_t lock;
  OUTPUT_MODE outmode;
};


XMLNODE *xml_parse_file(XSLTGLOBALDATA *gctx, char *file, int has_cache);
XMLNODE *xml_parse_string(XSLTGLOBALDATA *gctx, char *string, int has_cache);

/********************** nodes.c -- nodes operations *****************/
void xml_unlink_node(XMLNODE *node);
void xml_clear_node(TRANSFORM_CONTEXT *pctx, XMLNODE *node);
XMLNODE *xml_new_node(TRANSFORM_CONTEXT *pctx, char *name, NODETYPE type);
XMLNODE *xml_append_child(TRANSFORM_CONTEXT *pctx, XMLNODE *node, NODETYPE type);
void xml_add_child(TRANSFORM_CONTEXT *pctx, XMLNODE *node,XMLNODE *child);

/********************** transform.c -- XSLT mainloop ****************/
void apply_xslt_template(TRANSFORM_CONTEXT *pctx, XMLNODE *ret, XMLNODE *source, XMLNODE *templ, XMLNODE *params, XMLNODE *locals);
void process_one_node(TRANSFORM_CONTEXT *pctx, XMLNODE *ret, XMLNODE *source, XMLNODE *params, XMLNODE *locals, char *mode);
void apply_default_process(TRANSFORM_CONTEXT *pctx, XMLNODE *ret, XMLNODE *source, XMLNODE *params, XMLNODE *locals, char *mode);
XMLNODE *find_first_node(XMLNODE *n);
char *xml_eval_string(TRANSFORM_CONTEXT *pctx, XMLNODE *locals, XMLNODE *source, XMLNODE *foreval);

char *xml_get_attr(XMLNODE *node, char *name);
char *get_abs_name(TRANSFORM_CONTEXT *pctx, char *fname);

XMLNODE *find_template(TRANSFORM_CONTEXT *pctx, XMLNODE *source, char *mode);
XMLNODE *template_byname(TRANSFORM_CONTEXT *pctx, char *name);
void precompile_templates(TRANSFORM_CONTEXT *pctx, XMLNODE *node);
void precompile_variables(TRANSFORM_CONTEXT *pctx, XMLNODE *stylesheet, XMLNODE *doc);
char *get_variable_str(TRANSFORM_CONTEXT *pctx, XMLNODE *locals, char *name);
void get_variable_rv(TRANSFORM_CONTEXT *pctx, XMLNODE *vars, char *name, RVALUE *rv);
void  free_variables(TRANSFORM_CONTEXT *pctx);

void do_local_var(TRANSFORM_CONTEXT *pctx, XMLNODE *vars, XMLNODE *doc, XMLNODE *var);
void add_local_var(TRANSFORM_CONTEXT *pctx, XMLNODE *vars, char *name, XMLNODE *params);
char *xsl_get_global_key(TRANSFORM_CONTEXT *pctx, char *first, char *second);

/************************* xpath.c -- xpath compilator ************************/
void xpath_execute_scalar(TRANSFORM_CONTEXT *pctx, XMLNODE *locals, XMLNODE *etree, XMLNODE *current, RVALUE *res);

void xpath_eval_expression(TRANSFORM_CONTEXT *pctx, XMLNODE *locals, XMLNODE *current, char *expr, RVALUE *res);
int xpath_eval_boolean(TRANSFORM_CONTEXT *pctx, XMLNODE *locals, XMLNODE *current, XMLNODE *expr);
char *xpath_eval_string(TRANSFORM_CONTEXT *pctx, XMLNODE *locals, XMLNODE *current, XMLNODE *expr);
XMLNODE *xpath_eval_selection(TRANSFORM_CONTEXT *pctx, XMLNODE *locals, XMLNODE *current, XMLNODE *expr);
void xpath_eval_node(TRANSFORM_CONTEXT *pctx, XMLNODE *locals, XMLNODE *current, char *expr, RVALUE *this);

XMLNODE *xpath_find_expr(TRANSFORM_CONTEXT *pctx, char *expr);
XMLNODE *xpath_sort_selection(TRANSFORM_CONTEXT *pctx, XMLNODE *locals, XMLNODE *selection, XMLNODE *sort);
XMLNODE *xpath_in_selection(XMLNODE *sel, char *name);
void xpath_free_selection(TRANSFORM_CONTEXT *pctx, XMLNODE *sel);
XMLNODE *xpath_copy_nodeset(XMLNODE *set);
XMLNODE *xpath_compile(TRANSFORM_CONTEXT *pctx, char *expr);
XMLNODE *xpath_filter(TRANSFORM_CONTEXT *pctx, XMLNODE *locals, XMLNODE *nodeset, XMLNODE *expr);
XMLNODE *add_to_selection(XMLNODE *prev, XMLNODE *src, unsigned int *position);
XMLNODE *xpath_nodeset_copy(TRANSFORM_CONTEXT *pctx, XMLNODE *src);

char *xml_process_string(TRANSFORM_CONTEXT *pctx, XMLNODE *locals, XMLNODE *src, char *s);


/************************* rvalue.c -- rvalues operations ************************/
void rval_init(RVALUE *rv);
void rval_free(RVALUE *rv);
int rval2bool(RVALUE *rv);
char *rval2string(RVALUE *rv);
double rval2number(RVALUE *rv);
int rval_equal(RVALUE *left, RVALUE *right, unsigned eq);
int rval_compare(RVALUE *left, RVALUE *right);

typedef struct _xmls {
  char *s;
  unsigned len;
  unsigned allocated;
} *XMLSTRING;

int match(char **eptr, char *str);
int x_can_number(char *p);
int x_is_ws(char c);
int x_is_empty(char *s);
int x_is_namechar(char c);
int x_is_selchar(char c);
int xml_strcmp(const char *l, const char *r);
int xml_strcasecmp(char *l, char *r);
char *xml_strdup(const char *s);
char *xml_new_string(const char *s, size_t length);
char *xml_unescape(char *s);
char *node2string(XMLNODE *node);
char *nodes2string(XMLNODE *node);

XMLSTRING   xmls_new(unsigned bsize);             // allocates xmlstring
void        xmls_add_char(XMLSTRING s, char c);   // appends a char to xmlstring
void        xmls_add_utf(XMLSTRING s, unsigned u);// appends unicode value to xmlstring
void        xmls_add_str(XMLSTRING s, const char *d);   // appends a string to xmlstring
char       *xmls_detach(XMLSTRING s);             // frees xmlstring, returns content to be freed later

short   *utf2ws(char *s);

void output_node_rec(XMLNODE *node, XMLSTRING rtext, TRANSFORM_CONTEXT *ctx);


void    xpath_call_dispatcher(TRANSFORM_CONTEXT *pctx, XMLNODE *locals, char *fname, XMLNODE *args, XMLNODE *current, RVALUE *res);

void threadpool_start_full(void (*routine)(TRANSFORM_CONTEXT *, XMLNODE *, XMLNODE *, XMLNODE *, XMLNODE *, void *), TRANSFORM_CONTEXT *pctx, XMLNODE *ret, XMLNODE *source, XMLNODE *params, XMLNODE *locals, void *mode);

int threadpool_lock_on();
void threadpool_lock_off();

#endif