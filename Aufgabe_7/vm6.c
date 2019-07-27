#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <bigint.h>
#include "stack6.h"

#define IMMEDIATE(x) ((x) & 0x00FFFFFF)
#define SIGN_EXTEND(i) ((i) & 0x00800000 ? (i) | 0xFF000000 : (i)) 

#define HALT		0
#define PUSHC		1
#define ADD 		2
#define SUB 		3
#define MUL 		4
#define DIV 		5
#define MOD 		6
#define RDINT		7
#define WRINT 		8
#define RDCHR 		9
#define WRCHR		10

#define PUSHG		11
#define POPG 		12
#define ASF		13
#define RSF		14
#define PUSHL		15
#define POPL		16

#define EQ      	17
#define NE      	18
#define LT      	19
#define LE      	20
#define GT      	21
#define GE      	22

#define JMP     	23
#define BRF     	24
#define BRT     	25

#define CALL		26
#define RET		27
#define DROP		28
#define PUSHR		29
#define POPR		30

#define DUP		31

#define NEW             32
#define GETF            33
#define PUTF            34

#define NEWA            35
#define GETFA           36
#define PUTFA           37

#define GETSZ           38

#define PUSHN           39
#define REFEQ           40
#define REFNE           41

/*the current version of VM*/
#define VERSION 	7

// DEBUG-option
#define INSPECT		1
#define LIST		2
#define BREAKPOINT	3
#define STEP		4
#define RUN		5
#define QUIT		6

// for instance read input
#define MAX 		11

// program memory
unsigned int *ps;
// break up program
static int halt = 0;

// program counter
static int pc = 0;
// current instruktion in debug mode
static unsigned int index = 0;
// set breakpoint in debug mode
static int breakpoint = -1;
// next instruktion in debug mode
static int step = 0; 

// it used in function break_point(char *x)
static int value = 0;

// it used in function read_line(char *str)
static int count = 0;

// it used in function run()
static int switcher = 1;

// Header control structures
typedef struct {
	char name[4];
	int version;
	int noi;
	int sda;
} header_t;

static header_t buffer;

void instruktion(int i) {
	switch(ps[i] >> 24) {
		case HALT: printf("%.4d\thalt\n", i); break;
		case PUSHC: printf("%.4d\tpushc\t%d\n", i, SIGN_EXTEND(IMMEDIATE(ps[i]))); break;
		case ADD: printf("%.4d\tadd\n", i); break;
		case SUB: printf("%.4d\tsub\n", i); break;
		case MUL: printf("%.4d\tmul\n", i); break;
		case DIV: printf("%.4d\tdiv\n", i); break;
		case MOD: printf("%.4d\tmod\n", i); break;
		case RDINT: printf("%.4d\trdint\n", i); break;
		case WRINT: printf("%.4d\twrint\n", i); break;
		case RDCHR: printf("%.4d\trdchr\n", i); break;
		case WRCHR: printf("%.4d\twrchr\n", i); break;
		case PUSHG: printf("%.4d\tpushg\t%d\n", i, IMMEDIATE(ps[i])); break;
		case POPG: printf("%.4d\tpopg\t%d\n", i, IMMEDIATE(ps[i])); break;
		case ASF: printf("%.4d\tasf\t%d\n", i, IMMEDIATE(ps[i])); break;
		case RSF: printf("%.4d\trsf\t\n", i); break;
		case PUSHL: printf("%.4d\tpushl\t%d\n", i, SIGN_EXTEND(IMMEDIATE(ps[i]))); break;
		case POPL: printf("%.4d\tpopl\t%d\n", i, IMMEDIATE(ps[i])); break;
		case EQ: printf("%.4d\teq\t\n", i); break;
		case NE: printf("%.4d\tne\t\n", i); break;
		case LT: printf("%.4d\tlt\t\n", i); break;
		case LE: printf("%.4d\tls\t\n", i); break;
		case GT: printf("%.4d\tgt\t\n", i); break;
		case GE: printf("%.4d\tge\t\n", i); break;
		case JMP: printf("%.4d\tjmp\t\n", i); break;
		case BRF: printf("%.4d\tbrf\t\n", i); break;
		case BRT: printf("%.4d\tbrt\t\n", i); break;
		case CALL: printf("%.4d\tcall\t%d\n", i, IMMEDIATE(ps[i])); break;
		case RET: printf("%.4d\tret\t\n", i); break;
		case DROP: printf("%.4d\tdrop\t%d\n", i, IMMEDIATE(ps[i])); break;
		case PUSHR: printf("%.4d\tpushr\t\n", i); break;
		case POPR:  printf("%.4d\tpopr\t\n", i); break;
		case DUP: printf("%.4d\tdup\t\n", i); break;
		default: printf("ir is something else\n"); exit(1); break;
	}
}

// stack operation
void exec(int ir) {
	/**value2 = Devisor --> z.B. Quotient = Dividend : Devisor*/
	/*or it used for buffer*/
	int value = 0;
	// int value2 = 0;
		switch((ir >> 24)) {
			case HALT: 
				halt = 1; break;
			case PUSHC: // push value ir ^ (1 << 24)
				bigFromInt(SIGN_EXTEND(IMMEDIATE(ir)));		
				pushObjRef(bip.res);
				break; 				
			case ADD: 
				bip.op1 = popObjRef();
				bip.op2 = popObjRef();
				bigAdd();
				pushObjRef(bip.res); 
				break;
			case SUB: 
				bip.op2 = popObjRef();
				bip.op1 = popObjRef();
				bigSub();
				pushObjRef(bip.res); 
				break;
			case MUL: 
				bip.op1 = popObjRef();
				bip.op2 = popObjRef();
				bigMul();
				pushObjRef(bip.res);
				break;
			case DIV: 
				bip.op2 = popObjRef();
				bip.op1 = popObjRef();
				bigDiv();
				pushObjRef(bip.res);
				break;
			case MOD: 
				bip.op2 = popObjRef();
				bip.op1 = popObjRef();
				bigDiv();
				pushObjRef(bip.rem);
				break;
			case RDINT: // scan return 1 if read value successfully
				if(scanf("%d", &value) == 1) {
					bigFromInt(value);		
					pushObjRef(bip.res);
				} else {
					printf("Error: no digits in input\n");
					exit(1);				
				}
				break;
			case WRINT: 
				bip.op1 = popObjRef();
				bigPrint(stdout);
				break;
			case RDCHR: 
				value = getchar();
				bigFromInt(value);		
				pushObjRef(bip.res);
				break;
			case WRCHR:  
				bip.op1 = popObjRef();
				printf("%c", bigToInt()); 
				break;
			case PUSHG:
				pushg(SIGN_EXTEND(IMMEDIATE(ir))); break;
			case POPG:
				popg(SIGN_EXTEND(IMMEDIATE(ir))); break;
			case ASF:
				pushNumber(fp);
				fp = sp;
				sp = sp + IMMEDIATE(ir);
				break;
			case RSF:
				sp = fp;
				fp = popNumber();
				break;
			case PUSHL:
				pushl(SIGN_EXTEND(IMMEDIATE(ir)) + fp); break;
			case POPL:
				popl(fp+SIGN_EXTEND(IMMEDIATE(ir))); break;
			case EQ: // value1 == value2
				bip.op2 = popObjRef();
				bip.op1 = popObjRef();
				if(bigCmp() == 0)
					bigFromInt(1);
				else
					bigFromInt(0);		
				pushObjRef(bip.res);
				break;
			case NE: // value1 != value2
				bip.op2 = popObjRef();
				bip.op1 = popObjRef();
				if(bigCmp() == 0)
					bigFromInt(0);
				else
					bigFromInt(1);		
				pushObjRef(bip.res);
				break;
			case LT: // value1 < value2
				bip.op2 = popObjRef();
				bip.op1 = popObjRef();
				if(bigCmp() < 0)
					bigFromInt(1);
				else
					bigFromInt(0);		
				pushObjRef(bip.res);
				break;
			case LE: // value1 <= value2
				bip.op2 = popObjRef();
				bip.op1 = popObjRef();
				if(bigCmp() <= 0)
					bigFromInt(1);
				else
					bigFromInt(0);		
				pushObjRef(bip.res);
				break;
			case GT: // value1 > value2
				bip.op2 = popObjRef();
				bip.op1 = popObjRef();
				if(bigCmp() > 0)
					bigFromInt(1);
				else
					bigFromInt(0);		
				pushObjRef(bip.res);
				break;
			case GE: // value1 >= value2
				bip.op2 = popObjRef();
				bip.op1 = popObjRef();
				if(bigCmp() >= 0)
					bigFromInt(1);
				else
					bigFromInt(0);		
				pushObjRef(bip.res);
				break;
			case JMP: pc = IMMEDIATE(ir); break;
			case BRF: 
				bip.op1 = popObjRef();
				if(bigToInt() == 0)
					pc = IMMEDIATE(ir);
				break;
			case BRT: 
				bip.op1 = popObjRef();
				if(bigToInt() == 1)
					pc = IMMEDIATE(ir);
				break;
			case CALL:
				bigFromInt(pc);		
				pushObjRef(bip.res);
				pc = IMMEDIATE(ir);
				break;
			case RET:
				bip.op1 = popObjRef();
				pc = bigToInt();
				break;
			case DROP: 
				value = IMMEDIATE(ir);
				while(value > 0) {
					popObjRef();
					value--;
				}
				break;
			case PUSHR:
				pushObjRef(r[0]); break;
			case POPR:
				r[0] = popObjRef(); break;
			case DUP:
				bip.res = popObjRef();		
				pushObjRef(bip.res);
				pushObjRef(bip.res);
				break;	
			case NEW:
				pushObjRef(newCompoundObject(SIGN_EXTEND(IMMEDIATE(ir))));	
				break;
			case GETF:
				pushObjRef(GET_REFS(popObjRef())[SIGN_EXTEND(IMMEDIATE(ir))]);
				break;
			case PUTF:
				bip.op1 = popObjRef();
				bip.op2 = popObjRef();
				GET_REFS(bip.op2)[SIGN_EXTEND(IMMEDIATE(ir))] = bip.op1;
				break;
			case NEWA:
				break;
			case GETFA:
				bip.op1 = popObjRef();
				bip.op2 = popObjRef();
				pushObjRef(GET_REFS(bip.op2)[bigToInt()]);
				break;
			case PUTFA:
				break;	
			case GETSZ:
				bip.op1 = popObjRef();
				if(IS_PRIM(bip.op1))
					bigFromInt(-1);
				else
					bigFromInt(GET_SIZE(bip.op1));
				pushObjRef(bip.res);
				break;
			case PUSHN:
				pushObjRef(NULL);
				break;
			case REFEQ:
				break;
			case REFNE:
				bip.op1 = popObjRef();
				bip.op2 = popObjRef();
				if(bip.op2 != bip.op1)
					bigFromInt(1);
				else
					bigFromInt(0);
				pushObjRef(bip.res);
				break;
			default: printf("Error in void exec(int ir): ir is something else\n"); break;
		}
}

void load_data(char file[]) { 
	r = malloc(sizeof(ObjRef));

	/*create file pointer*/
	FILE *fp;	

	/*open file*/
	fp = fopen(file, "r");
	if(fp == NULL) {
		printf("Error: cannot open code file '%s'\n", file);
		exit(1);
	} else {
		// read header of the file fp
		if(fread(&buffer, sizeof(header_t), 1, fp) != 1) {
			printf("Error: reading\n");
			exit(1);
		}

		/*Verify the format identifier*/
		if(!strncmp(buffer.name, "NJBF", 4) == 0) {
			printf("identifier invalid\n");
			exit(1);
		}

		/*Verify that this matches the current VM's version number*/
		if(buffer.version != VERSION) {
			printf("Version number doesn't match the version number of VM\n");
			exit(1);
		}
				
		// allocatre memory for Static Data Area
		// warum sizeof(static_data_area) = 8 anstatt 108?
		static_data_area = malloc(buffer.sda * sizeof(ObjRef));
		if(static_data_area == NULL) {
			printf("memory is full.");
			exit(1);
		}

		// allocate memory program memory
		// warum sizeof(ps) = 8 anstatt 108?
		ps = malloc(buffer.noi * sizeof(int));
		
		if(ps == NULL) {
			printf("memory is full.");
			exit(1);
		}
				
		// read Instruktionen
		if(fread(ps, sizeof(int), buffer.noi, fp) != buffer.noi) {
			printf("Error: reading\n");
			exit(1);
		}

		/*close file*/
		fclose(fp);
	}
}

void read_line(char *str) {
	count = 0;
	char c;

	// result: 
	// maximum one blank
	// write big input without care array size
	while((c = getchar()) != '\n') {		// 10 --> character '\n'

		if(count >= MAX)			// do nothing because array is full
			;
		else if(c != ' ' && c != '\t') {	// check for others character
			str[count] = c;
			count++;
		}
	}
	
	if(count)
		str[count] = '\0';			// end of String terminate by '\0'
	else {
		str[count] = '\n';			// if user doesn't input anything example below
		str[++count] = '\0';			// strncmp("\0", "inspect", 0) = 0 == 0 consequences program continue
	}						
}

void inspect(char *x) {
	void *a = NULL;	
	int j = 0;
	printf("DEBUG [inspect]: stack, data, object?\n");
			
	read_line(x);
	

	if(strncmp(x, "data", count) == 0) {
		for(int i = 0; i < buffer.sda; i++)
			printf("data[%.4d]:\t%d\n", i, *(int*) (static_data_area[i]->data));
		printf("\t--- end of code ---\n");
	} else if(strncmp(x, "stack", count) == 0) {
			if(fp == sp) {
				printf("sp, fp  --->\t%.4d:\t(xxxxxx) xxxxxx\n", fp);
				printf("\t\t--- bottom of stack ---\n");
			} else {
				for(int i = sp; i >= 0; i--) {
					if(i == sp)
						printf("sp\t--->\t%.4d\t(xxxxxx) xxxxxx\n", sp);
					else if(i == fp)
						if(is_objRef(i))
							printf("fp\t--->\t%.4d:\t(objref) %p\n", i, (void*) stack[i].u.objRef);
						else
							printf("fp\t--->\t%.4d:\t(number) %d\n", i, stack[i].u.number);
					else 
						if(is_objRef(i))
							printf("\t\t%.4d:\t(objref) %p\n", i, (void*) stack[i].u.objRef);
						else
							printf("\t\t%.4d:\t(number) %d\n", i, stack[i].u.number);
				}
				printf("\t\t--- bottom of stack ---");
			}
	} else if(strncmp(x, "object", count) == 0) {
		printf("object reference\n");
		scanf("%p", &a);
		while(j < buffer.noi) {
			printf("j = %d\n", j);
			if(is_objRef(j))
				if(a == (void*) stack[j].u.objRef)
					break;
			j++;
		}
		printf("Value = %d\n", *(int*) (stack[j].u.objRef->data));
	}
}

void list(int prog_size) {
	for(int i = 0; i < prog_size; i++) {
		instruktion(i);	
	}
	printf("\t--- end of code ---\n");
}

void break_point(char *x) {
	if(value == -1)
		printf("DEBUG [breakpoint]: cleared\n");
	else
		printf("DEBUG [breakpoint]: set at %d\n", value);
	printf("DEBUG [breakpoint]: address to set, -1 to clear, <ret> for no change?\n");
		
	read_line(x);
	// explicit input 0 because function atoi(int) = 0 if parameter is not a number
	if(strcmp(x, "0") == 0) {
		breakpoint = 0;
		printf("DEBUG [breakpoint]: now set at %d\n", breakpoint);
		return;
	} else if(strcmp(x, "-1") == 0) {
		value = -1;
		printf("DEBUG [breakpoint]: now cleared\n");
		return; 
	}

	// does not include character
	for(int i = 0; i < count; i++)  
		if(x[i] < 48 || x[i] > 57) 		// 48 == '0' ... 57 == '9'
			return;
		
	value = atoi(x);
	if(value < 0 || value > buffer.noi)
		printf("number of instruction = %d\n", buffer.noi);
	else if(value != 0) {
		breakpoint = value;		// important for function run()
		printf("DEBUG [breakpoint]: now set at %d\n", breakpoint);	
	}
}

void run(void) {
	int ir = 0;
	while(!halt) {
		if(pc == breakpoint) {
			index = pc;
			return;
		} else 

		if(step) {
			index = pc;
			return;
		}
		
		if(switcher)
			step = 1;
		ir = ps[pc];
		pc++;
		exec(ir);
	}
	printf("Ninja Virtual Machine stopped\n");
}

void debug(void) {
	// for debug option like (inspect, list, breakpoint, step, run, quit)
	char input[MAX] = "";
	// inspect option like (stack, data)
	char x[6] = "";
	unsigned int option = 0;

	instruktion(index);
	printf("DEBUG: inspect, list, breakpoint, step, run, quit?\n");

	read_line(input);
	
	if(strncmp(input, "inspect", count) == 0)
		option = INSPECT;
	else if(strncmp(input, "list", count) == 0)
		option = LIST;
	else if(strncmp(input, "breakpoint", count) == 0)
		option = BREAKPOINT;
	else if(strncmp(input, "step", count) == 0)
		option = STEP;
	else if(strncmp(input, "run", count) == 0)
		option = RUN;
	else if(strncmp(input, "quit", count) == 0)
		option = QUIT;

	switch(option) {
		case INSPECT:
			inspect(x);
			break;
		case LIST:
			list(buffer.noi); break;
		case BREAKPOINT:
			break_point(x);
			break;
		case STEP:
			breakpoint = -1;
			step = 0;
			switcher = 1;
			run();
			break;
		case RUN:
			step = 0;
			switcher = 0;
			run();
			break;
		case QUIT:
			printf("Ninja Virtual Machine stopped\n");
			halt = 1;
			break;
		default: break;
	}
}

int f(int argc, char *argv[]) {
	if(strcmp(argv[argc], "--version") == 0) {
		printf("Ninja Virtual Machine version %d (compiled %s, %s)\n", VERSION, __DATE__, __TIME__);
		return 1;
	} 
	
	if(strcmp(argv[argc], "--help") == 0) {
		printf("usage: ./njvm [options] <code file>\n");
		printf("Option:\n");
		printf("  --debug\t   start virtual machine in debug mode\n");
   		printf("  --version\t   show version and exit\n");
   		printf("  --help\t   show this help and exit\n");
		return 1;
	}
	
	if(strncmp(argv[argc], "-", 1) == 0 && strcmp(argv[argc], "--debug") != 0) {
		printf("Error: unknown option '%s', try './njvm --help'\n", argv[argc]);
		return 1;
	}
	return 0;
}

int main(int argc, char *argv[]) {
	
	if(argc == 1) 
		printf("Error: no code file specified\n");
	else if(argc == 2) {
		if(f(1, argv) == 1)
			;
		else if(strcmp(argv[1], "--debug") == 0)
			printf("Error: no code file specified\n");
		else {
			load_data(argv[1]);

			printf("Ninja Virtual Machine started\n");
		
			// start program
			step = 0;
			switcher = 0;
			run();
				
			// release memory from ps
			free(ps);

			// release memory from static_data_area
			free(static_data_area);
		}	
	} else if(argc == 3) {
		if(f(1, argv) == 1)
			;
		else if(f(2, argv) == 1)
			;
		else if(strcmp(argv[1], "--debug") == 0) {
			if(f(2, argv) == 0) {
				load_data(argv[2]);
				printf("DEBUG: file '%s' loaded (code size = ", argv[2]);
				printf("%d, datasize = %d)\n", buffer.noi, buffer.sda);
				printf("Ninja Virtual Machine started\n");
				while(!halt) {
					debug();			
				}
			}
		} else if(strcmp(argv[2], "--debug") == 0) {
			if(f(1, argv) == 0) {
				load_data(argv[1]);
				printf("DEBUG: file '%s' loaded (code size = ", argv[2]);
				printf("%d, datasize = %d)\n", buffer.noi, buffer.sda);
				printf("Ninja Virtual Machine started\n");
				while(!halt) {
					debug();			
				}
			}
		} else 
			printf("Error: more than one code file specified\n");		
	} else if(argc > 3) {
		if(f(1, argv) == 1)
			;
		else if(f(2, argv) == 1)
			;
		else if(strcmp(argv[1], "--debug") == 0 || strcmp(argv[2], "--debug") == 0) {
			if(f(3, argv) == 0)
				printf("Error: more than one code file specified\n");
		} else
			printf("Error: more than one code file specified\n");	
				
	}
	
	return 0;
}
