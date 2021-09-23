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
	printf("[Linking ASM]\n");
	parse_asm(data);
	free(data);

	dump_asm("tmp.bin");
	char *binary = load_asm("./tmp.bin") + 8;

	printf("[Executing Binary]\n");
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
	if(strcmp(name, "pushi") == 0) {
		return OP_PUSHI;
	} else if(strcmp(name, "loadi") == 0) {
		return OP_LOADI;
	} else if(strcmp(name, "storei") == 0) {
		return OP_STOREI;
	} else if(strcmp(name, "loadc") == 0) {
		return OP_LOADC;
	} else if(strcmp(name, "storec") == 0) {
		return OP_STOREC;
	} else if(strcmp(name, "lda") == 0) {
		return OP_LDA;
	} else if(strcmp(name, "deref") == 0) {
		return OP_DEREF;
	} else if(strcmp(name, "sta") == 0) {
		return OP_STA;
	} else if(strcmp(name, "cf2i32") == 0) {
		return OP_CF2I32;
	} else if(strcmp(name, "ci2f32") == 0) {
		return OP_CI2F32;
	} else if(strcmp(name, "add") == 0) {
		return OP_ADD;
	} else if(strcmp(name, "addf") == 0) {
		return OP_ADDF;
	} else if(strcmp(name, "sub") == 0) {
		return OP_SUB;
	} else if(strcmp(name, "subf") == 0) {
		return OP_SUBF;
	} else if(strcmp(name, "mul") == 0) {
		return OP_MUL;
	} else if(strcmp(name, "mulf") == 0) {
		return OP_MULF;
	} else if(strcmp(name, "div") == 0) {
		return OP_DIV;
	} else if(strcmp(name, "divf") == 0) {
		return OP_DIVF;
	} else if(strcmp(name, "mod") == 0) {
		return OP_MOD;
	} else if(strcmp(name, "shl") == 0) {
		return OP_SHL;
	} else if(strcmp(name, "shr") == 0) {
		return OP_SHR;
	} else if(strcmp(name, "and") == 0) {
		return OP_AND;
	} else if(strcmp(name, "cmplt") == 0) {
		return OP_CMPLT;
	} else if(strcmp(name, "cmpltf") == 0) {
		return OP_CMPLTF;
	} else if(strcmp(name, "cmpgt") == 0) {
		return OP_CMPGT;
	} else if(strcmp(name, "cmpgtf") == 0) {
		return OP_CMPGTF;
	} else if(strcmp(name, "neq") == 0) {
		return OP_NEQ;
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

	char magic[] = "CHIPCODE";
	fwrite(magic, sizeof(char), 8, fp);
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
	char *var_stack = malloc(sizeof(char) * 65535);
	char *prg_stack = malloc(sizeof(char) * 65535);

	List return_ip;
	list_clear(&return_ip);

	uint32_t ip = 0;
	uint32_t sp = 0;

	while(true) {
		uint32_t offset = (ip * (sizeof(OPCode) + sizeof(int) + sizeof(int)));
		OPCode op = (OPCode)*(&binary[offset]);
		int left  = *(int*)(&binary[offset + sizeof(OPCode)]);
		int right = *(int*)(&binary[offset + sizeof(OPCode) + sizeof(int)]);

		switch(op) {
			case OP_PUSHI: {
				memcpy(&prg_stack[sp], &left, 4);
				sp += 4;
			}
			break;
			case OP_LOADI: {
				memcpy(&prg_stack[sp], &var_stack[left], 4);
				sp += 4;
			}
			break;
			case OP_STOREI: {
				sp -= 4;
				memcpy(&var_stack[left], &prg_stack[sp], 4);
			}
			break;
			case OP_LOADC: {
				memcpy(&prg_stack[sp], &var_stack[left], 4);
				sp += 1;
			}
			break;
			case OP_STOREC: {
				sp -= 1;
				memcpy(&var_stack[left], &prg_stack[sp], 4);
			}
			break;
			case OP_LDA: {
				// load addr(refrence)
				uint64_t real_addr = (uint64_t)(left + var_stack);
				memcpy(&prg_stack[sp], &real_addr, 8);
				sp += 8;
			}
			break;
			case OP_DEREF: {
				sp -= 8;
				uint64_t pop1 = *(uint64_t*)&prg_stack[sp];

				memcpy(&prg_stack[sp], (char*)pop1, 4);
				sp += 4;
			}
			break;
			case OP_STA: {
				// store addr
				sp -= 8;
				uint64_t pop1 = *(uint64_t*)&prg_stack[sp];

				sp -= 4;
				int pop2 = *(int*)&prg_stack[sp];

				memcpy((char*)pop1, &pop2, 4);
			}
			break;
			case OP_CF2I32: {
				sp -= 4;
				float pop1 = *(float*)&prg_stack[sp];
				int result = (int)pop1;

				memcpy(&prg_stack[sp], &result, 4);
				sp += 4;
			}
			break;
			case OP_CI2F32: {
				sp -= 4;
				int pop1 = *(int*)&prg_stack[sp];

				float result = (float)pop1;

				memcpy(&prg_stack[sp], &result, 4);
				sp += 4;
			}
			break;
			case OP_ADD: {
				sp -= 4;
				int pop1 = *(int*)&prg_stack[sp];

				sp -= 4;
				int pop2 = *(int*)&prg_stack[sp];


				int result = pop1 + pop2;

				memcpy(&prg_stack[sp], &result, 4);
				sp += 4;
			}
			break;
			case OP_ADDF: {
				sp -= 4;
				float pop1 = *(float*)&prg_stack[sp];

				sp -= 4;
				float pop2 = *(float*)&prg_stack[sp];


				float result = pop1 + pop2;

				memcpy(&prg_stack[sp], &result, 4);
				sp += 4;
			}
			break;
			case OP_SUB: {
				sp -= 4;
				int pop1 = *(int*)&prg_stack[sp];

				sp -= 4;
				int pop2 = *(int*)&prg_stack[sp];

				int result = pop1 - pop2;
				
				memcpy(&prg_stack[sp], &result, 4);
				sp += 4;
			}
			break;
			case OP_SUBF: {
				sp -= 4;
				float pop1 = *(float*)&prg_stack[sp];

				sp -= 4;
				float pop2 = *(float*)&prg_stack[sp];

				float result = pop1 - pop2;
				
				memcpy(&prg_stack[sp], &result, 4);
				sp += 4;
			}
			break;
			case OP_MUL: {
				sp -= 4;
				int pop1 = *(int*)&prg_stack[sp];

				sp -= 4;
				int pop2 = *(int*)&prg_stack[sp];

				int result = pop1 * pop2;
				
				memcpy(&prg_stack[sp], &result, 4);
				sp += 4;
			}
			break;
			case OP_MULF: {
				sp -= 4;
				float pop1 = *(float*)&prg_stack[sp];

				sp -= 4;
				float pop2 = *(float*)&prg_stack[sp];

				float result = pop1 * pop2;
				
				memcpy(&prg_stack[sp], &result, 4);
				sp += 4;
			}
			break;
			case OP_DIV: {
				sp -= 4;
				int pop1 = *(int*)&prg_stack[sp];

				sp -= 4;
				int pop2 = *(int*)&prg_stack[sp];

				int result = pop1 / pop2;
				
				memcpy(&prg_stack[sp], &result, 4);
				sp += 4;
			}
			break;
			case OP_DIVF: {
				sp -= 4;
				float pop1 = *(float*)&prg_stack[sp];

				sp -= 4;
				float pop2 = *(float*)&prg_stack[sp];

				float result = pop1 / pop2;
				
				memcpy(&prg_stack[sp], &result, 4);
				sp += 4;
			}
			break;
			case OP_MOD: {
				sp -= 4;
				int pop1 = *(int*)&prg_stack[sp];

				sp -= 4;
				int pop2 = *(int*)&prg_stack[sp];

				int result = pop1 % pop2;
				
				memcpy(&prg_stack[sp], &result, 4);
				sp += 4;
			}
			break;
			case OP_SHL: {
				sp -= 4;
				int pop1 = *(int*)&prg_stack[sp];

				sp -= 4;
				int pop2 = *(int*)&prg_stack[sp];

				int result = pop1 << pop2;
				
				memcpy(&prg_stack[sp], &result, 4);
				sp += 4;
			}
			break;
			case OP_SHR: {
				sp -= 4;
				int pop1 = *(int*)&prg_stack[sp];

				sp -= 4;
				int pop2 = *(int*)&prg_stack[sp];

				int result = pop1 >> pop2;
				
				memcpy(&prg_stack[sp], &result, 4);
				sp += 4;
			}
			break;
			case OP_AND: {
				sp -= 4;
				int pop1 = *(int*)&prg_stack[sp];

				sp -= 4;
				int pop2 = *(int*)&prg_stack[sp];

				int result = pop1 & pop2;
				
				memcpy(&prg_stack[sp], &result, 4);
				sp += 4;
			}
			break;
			case OP_CMPLT: {
				sp -= 4;
				int pop1 = *(int*)&prg_stack[sp];

				sp -= 4;
				int pop2 = *(int*)&prg_stack[sp];

				int result = pop1 < pop2;
				
				memcpy(&prg_stack[sp], &result, 4);
				sp += 4;
			}
			break;
			case OP_CMPLTF: {
				sp -= 4;
				float pop1 = *(float*)&prg_stack[sp];

				sp -= 4;
				float pop2 = *(float*)&prg_stack[sp];

				int result = pop1 < pop2;
				
				memcpy(&prg_stack[sp], &result, 4);
				sp += 4;
			}
			break;
			case OP_CMPGT: {
				sp -= 4;
				int pop1 = *(int*)&prg_stack[sp];

				sp -= 4;
				int pop2 = *(int*)&prg_stack[sp];

				int result = pop1 > pop2;
				
				memcpy(&prg_stack[sp], &result, 4);
				sp += 4;
			}
			break;
			case OP_CMPGTF: {
				sp -= 4;
				float pop1 = *(float*)&prg_stack[sp];

				sp -= 4;
				float pop2 = *(float*)&prg_stack[sp];

				int result = pop1 > pop2;
				
				memcpy(&prg_stack[sp], &result, 4);
				sp += 4;
			}
			break;
			case OP_NEQ: {
				sp -= 4;
				int pop1 = *(int*)&prg_stack[sp];

				sp -= 4;
				int pop2 = *(int*)&prg_stack[sp];

				int result = pop1 != pop2;
				
				memcpy(&prg_stack[sp], &result, 4);
				sp += 4;
			}
			break;
			case OP_RET: {
				if(list_size(&return_ip) > 0) {

					ReturnIP *rtn = (ReturnIP*)list_remove(list_back(&return_ip));
					ip = rtn->index;

					memcpy(&rtn->prg_stack[sp - left], &prg_stack[sp - left], left);

					free(prg_stack);
					free(var_stack);

					prg_stack = rtn->prg_stack;
					var_stack = rtn->var_stack;

					free(rtn);
					continue;
				} else {
					goto terminate;
				}
			}
			break;
			case OP_JE: {
				sp -= 4;
				int pop1 = *(int*)&prg_stack[sp];

				sp -= 4;
				int pop2 = *(int*)&prg_stack[sp];
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
				rtn->prg_stack = prg_stack;
				rtn->var_stack = var_stack;

				char *prg_stack_t = malloc(sizeof(char) * 65535);
				char *var_stack_t = malloc(sizeof(char) * 65535);

				memcpy(&var_stack_t[0], &prg_stack[sp], right); // copy args

				prg_stack = prg_stack_t;
				var_stack = var_stack_t;

				list_insert(list_end(&return_ip), rtn);
				ip = left;
				continue;
			}
			break;
			case OP_SYSCALL: {
				if(left == 0) {
					// print
					sp -= 4;
					int pop1 = *(int*)&prg_stack[sp];
					printf("%i\n", pop1);
				} else if(left == 1) {
					// print
					sp -= 4;
					float pop1 = *(float*)&prg_stack[sp];
					printf("%f\n", pop1);
				} else if(left == 2) {
					// print
					sp -= 4;
					int pop1 = *(int*)&prg_stack[sp];
					sleep(pop1);
				} else if(left == 3) {
					// socket
					sp -= 4;
					int family = *(int*)&prg_stack[sp];
					sp -= 4;
					int type = *(int*)&prg_stack[sp];
					sp -= 4;
					int protocol = *(int*)&prg_stack[sp];
					
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