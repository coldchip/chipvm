#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>
#include "chipvm.h"
#include "list.h"

int main(int argc, char const *argv[]) {
	/* code */
	char *data = read_file_into_buffer("/home/ryan/compiler/data/code.S");
	if(!data) {
		error("Unable to parse asm");
	}
	parse_asm(data);
	free(data);

	dump_asm("tmp.bin");
	char *binary = load_asm("./tmp.bin");

	execute(binary);
	return 0;
}

void error(char *format, ...) {
	va_list args;
	va_start(args, format);

	char fmt[1000];
	snprintf(fmt, sizeof(fmt), "[ChipCode] %s\n", format);
	vprintf(fmt, args);

	va_end(args);
	exit(1);
}

char *read_file_into_buffer(char *file) {
	FILE *infp = fopen(file, "rb");
	if (!infp) {
		printf("Cannot open %s\n", file);
		exit(0);
	}
	fseek(infp, 0, SEEK_END);
	long fsize = ftell(infp);
	char *p = malloc(fsize + 1);
	fseek(infp, 0, SEEK_SET);

	if(fread((char*)p, 1, fsize, infp) != fsize) {
		error("Unable to load file");
	}
	fclose(infp);
	*(p + fsize) = '\0';


	for(char *check = p; check < p + fsize; check++) {
		if(*check == '\0') {
			error("Cannot compile because file %s contains NULL character(s)\n", file);
		}
	}
	return p;
}

List labels; // temporary container for resolving labels into index
List codes;  // container to store operation codes

int locate_label(char *name) {
	for(ListNode *i = list_begin(&labels); i != list_end(&labels); i = list_next(i)) {
		Label *label = (Label*)i;
		if(strcmp(name, label->name) == 0) {
			return label->index;
		}
	}
	return -1;
}

void parse_asm(char *data) {
	list_clear(&labels); 
	list_clear(&codes);

	// resolve labels into index

	char *data_tmp = strdup(data);

	int i = 0;

	char *token = strtok(data_tmp, "\n");
	while(token != NULL) {
		if(!starts_with(token, "\t") == true) {
			// a label
			token[strlen(token) - 1] = '\0'; // remove leading ':'

			Label *label = malloc(sizeof(Label));
			label->index = i;
			label->name  = strdup(token);

			list_insert(list_end(&labels), label);
		} else {
			i++;
		}
		token = strtok(NULL, "\n");

	}

	free(data_tmp);

	// parsing of instruction codes

	data_tmp = strdup(data);
	token = strtok(data_tmp, "\n");
	while(token != NULL) {
		if(starts_with(token, "\t") == true) {
			resolve_code(token + 1); // +1 to remove [TABS]
		}
		token = strtok(NULL, "\n");
	}

	free(data_tmp);
}

void resolve_code(char *code) {
	if(strlen(code) > 0) {
		char *save_ptr = code;

		char *op_c = strtok_r(code, " ", &save_ptr);
		char *left_c  = strtok_r(NULL, " ", &save_ptr);
		char *right_c = strtok_r(NULL, " ", &save_ptr);

		if(!op_c) {
			error("operation code undefined");
		}

		int op = get_op_by_string(op_c);
		if(op < 0) {
			error("unknown opcode %s", op_c);
		}

		Code *code = malloc(sizeof(Code));
		code->op = op;
		if(op == OP_CALL || op == OP_JE || op == OP_JMP) {
			if(!left_c) {
				error("opcode call requires a left oprand");
			}
			int index = locate_label(left_c);
			if(index < 0) {
				error("unable to find label %s", left_c);
			}
			code->left = index;
			if(right_c) {
				code->right = atoi(right_c);
			}
		} else {
			if(left_c) {
				code->left = atoi(left_c);
			}
			if(right_c) {
				code->right = atoi(right_c);
			}
		}

		list_insert(list_end(&codes), code);
	}
}

OPCode get_op_by_string(char *name) {
	if(strcmp(name, "push_int") == 0) {
		return OP_PUSH;
	} else if(strcmp(name, "load") == 0) {
		return OP_LOAD;
	} else if(strcmp(name, "store") == 0) {
		return OP_STORE;
	} else if(strcmp(name, "lda") == 0) {
		return OP_LDA;
	} else if(strcmp(name, "deref") == 0) {
		return OP_DEREF;
	} else if(strcmp(name, "sta") == 0) {
		return OP_STA;
	} else if(strcmp(name, "add") == 0) {
		return OP_ADD;
	} else if(strcmp(name, "sub") == 0) {
		return OP_SUB;
	} else if(strcmp(name, "mul") == 0) {
		return OP_MUL;
	} else if(strcmp(name, "div") == 0) {
		return OP_DIV;
	} else if(strcmp(name, "cmplt") == 0) {
		return OP_CMPLT;
	} else if(strcmp(name, "cmpgt") == 0) {
		return OP_CMPGT;
	} else if(strcmp(name, "je") == 0) {
		return OP_JE;
	} else if(strcmp(name, "jmp") == 0) {
		return OP_JMP;
	} else if(strcmp(name, "call") == 0) {
		return OP_CALL;
	} else if(strcmp(name, "syscall") == 0) {
		return OP_SYSCALL;
	} else if(strcmp(name, "ret") == 0) {
		return OP_RET;
	} else {
		return -1;
	}
}

void dump_asm(char *file) {
	FILE *fp = fopen(file, "wb");
	for(ListNode *i = list_begin(&codes); i != list_end(&codes); i = list_next(i)) {
		Code *code = (Code*)i;
		fwrite(&code->op, sizeof(OPCode), 1, fp);
		fwrite(&code->left, sizeof(int), 1, fp);
		fwrite(&code->right, sizeof(int), 1, fp);
	}

	fclose(fp);
}

char *load_asm(char *file) {
	FILE *infp = fopen(file, "rb");
	if (!infp) {
		printf("Cannot open %s\n", file);
		exit(0);
	}
	fseek(infp, 0, SEEK_END);
	long fsize = ftell(infp);
	char *p = malloc(fsize);
	fseek(infp, 0, SEEK_SET);

	if(fread((char*)p, 1, fsize, infp) != fsize) {
		error("Unable to load file");
	}
	fclose(infp);
	
	return p;
}

bool starts_with(const char *str, const char *pre) {
	size_t lenpre = strlen(pre), lenstr = strlen(str);
	return lenstr < lenpre ? false : memcmp(pre, str, lenpre) == 0;
}

void execute(char *binary) {
	char var_stack[65535];
	char prg_stack[65535];

	List return_ip;
	list_clear(&return_ip);

	uint32_t ip = 0;
	uint32_t sp = 0;
	uint32_t fp = 0;

	while(true) {
		uint32_t offset = (ip * (sizeof(OPCode) + sizeof(int) + sizeof(int)));
		OPCode op = (OPCode)*(&binary[offset]);
		int left  = *(int*)(&binary[offset + sizeof(OPCode)]);
		int right = *(int*)(&binary[offset + sizeof(OPCode) + sizeof(int)]);

		//printf("%i %i %i\n", op, left, right);

		switch(op) {
			case OP_PUSH: {
				memcpy(&prg_stack[fp + sp], &left, 4);
				sp += 4;
			}
			break;
			case OP_LOAD: {
				memcpy(&prg_stack[fp + sp], &var_stack[fp + left], 4);
				sp += 4;
			}
			break;
			case OP_STORE: {
				sp -= 4;
				memcpy(&var_stack[fp + left], &prg_stack[fp + sp], 4);
			}
			break;
			case OP_LDA: {
				// load addr(refrence)
				left += fp; // relative to fp
				memcpy(&prg_stack[fp + sp], &left, 4);
				sp += 4;
			}
			break;
			case OP_DEREF: {
				sp -= 4;
				int pop1 = *(int*)&prg_stack[fp + sp];

				memcpy(&prg_stack[fp + sp], &var_stack[pop1], 4);
				sp += 4;
			}
			break;
			case OP_STA: {
				// store addr
				sp -= 4;
				int pop1 = *(int*)&prg_stack[fp + sp];

				sp -= 4;
				int pop2 = *(int*)&prg_stack[fp + sp];

				memcpy(&var_stack[pop1], &pop2, 4);
			}
			break;
			case OP_ADD: {
				sp -= 4;
				int pop1 = *(int*)&prg_stack[fp + sp];

				sp -= 4;
				int pop2 = *(int*)&prg_stack[fp + sp];


				int result = pop1 + pop2;

				memcpy(&prg_stack[fp + sp], &result, 4);
				sp += 4;
			}
			break;
			case OP_SUB: {
				sp -= 4;
				int pop1 = *(int*)&prg_stack[fp + sp];

				sp -= 4;
				int pop2 = *(int*)&prg_stack[fp + sp];

				int result = pop1 - pop2;
				
				memcpy(&prg_stack[fp + sp], &result, 4);
				sp += 4;
			}
			break;
			case OP_MUL: {
				sp -= 4;
				int pop1 = *(int*)&prg_stack[fp + sp];

				sp -= 4;
				int pop2 = *(int*)&prg_stack[fp + sp];

				int result = pop1 * pop2;
				
				memcpy(&prg_stack[fp + sp], &result, 4);
				sp += 4;
			}
			break;
			case OP_DIV: {
				sp -= 4;
				int pop1 = *(int*)&prg_stack[fp + sp];

				sp -= 4;
				int pop2 = *(int*)&prg_stack[fp + sp];

				int result = pop1 / pop2;
				
				memcpy(&prg_stack[fp + sp], &result, 4);
				sp += 4;
			}
			break;
			case OP_CMPLT: {
				sp -= 4;
				int pop1 = *(int*)&prg_stack[fp + sp];

				sp -= 4;
				int pop2 = *(int*)&prg_stack[fp + sp];

				int result = pop1 < pop2;
				
				memcpy(&prg_stack[fp + sp], &result, 4);
				sp += 4;
			}
			break;
			case OP_CMPGT: {
				sp -= 4;
				int pop1 = *(int*)&prg_stack[fp + sp];

				sp -= 4;
				int pop2 = *(int*)&prg_stack[fp + sp];

				int result = pop1 > pop2;
				
				memcpy(&prg_stack[fp + sp], &result, 4);
				sp += 4;
			}
			break;
			case OP_RET: {
				if(list_size(&return_ip) > 0) {
					ReturnIP *rtn = (ReturnIP*)list_remove(list_back(&return_ip));
					ip = rtn->index;
					fp = rtn->fp;
					free(rtn);
					continue;
				} else {
					goto terminate;
				}
			}
			break;
			case OP_JE: {
				sp -= 4;
				int pop1 = *(int*)&prg_stack[fp + sp];

				sp -= 4;
				int pop2 = *(int*)&prg_stack[fp + sp];
				if(pop1 == pop2) {
					ip = left;
					continue;
				}
			}
			break;
			case OP_JMP: {
				ip = left;
				continue;
			}
			break;
			case OP_CALL: {
				sp -= right;

				ReturnIP *rtn = malloc(sizeof(ReturnIP));
				rtn->index = ip + 1;
				rtn->fp = fp;
				fp = sp;

				memcpy(&var_stack[fp + 0], &prg_stack[rtn->fp + sp], right);

				list_insert(list_end(&return_ip), rtn);
				ip = left;
				continue;
			}
			break;
			case OP_SYSCALL: {
				if(left == 0) {
					// print
					sp -= 4;
					int pop1 = *(int*)&prg_stack[fp + sp];
					printf("%i\n", pop1);
				} else {
					printf("unknown syscall code %i\n", left);
					goto terminate;
				}
			}
			break;
			default: {
				printf("unknown op code %i\n", op);
				goto terminate;
			} 
			break;
		}

		ip++;
	}

	terminate:

	printf("prg ended\n");
}