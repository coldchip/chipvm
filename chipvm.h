#ifndef CHIPVM_H
#define CHIPVM_H

#include "list.h"

typedef enum {
	OP_PUSH,
	OP_LOAD,
	OP_STORE,
	OP_LDA,
	OP_DEREF,
	OP_STA,

	OP_ADD,
	OP_ADDF,
	OP_SUB,
	OP_SUBF,
	OP_MUL,
	OP_MULF,
	OP_DIV,
	OP_DIVF,

	OP_CMPLT,
	OP_CMPGT,

	OP_JE,
	OP_JMP,
	OP_CALL,
	OP_SYSCALL,

	OP_RET
} OPCode;

typedef struct _Label {
	ListNode node;
	int index;
	char *name;
} Label;

typedef struct _Code {
	ListNode node;
	OPCode op;

	int left;
	int right;
} Code;

typedef struct _ReturnIP {
	ListNode node;
	int index;
	char *prg_stack;
	char *var_stack;
} ReturnIP;

void error(char *format, ...);
char *read_file_into_buffer(char *file);

// parsers
void parse_asm(char *data);
void resolve_code(char *code);

OPCode get_op_by_string(char *name);

void dump_asm(char *file);
char *load_asm(char *file);

bool starts_with(const char *str, const char *pre);

void execute(char *binary);

#endif