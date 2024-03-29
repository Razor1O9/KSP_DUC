#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "bigint/build/include/bigint.h"
#include "stack8.h"

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
#define VERSION 	8

// DEBUG-option
#define INSPECT		1
#define LIST		2
#define BREAKPOINT	3
#define STEP		4
#define RUN		5
#define QUIT		6

// for instance read input
#define MAX 		11

// Macros for Garbage Collector
#define BROKENHEART(objRef) (((objRef)->size & SBIT) == 1)
#define FORWARDPOINTER(objRef) (((objRef)->size & ~(MSB | SBIT)))
#define MSB (1 << (8 * sizeof(unsigned int) - 1))
#define SBIT (1 <<(8* sizeof(unsigned int )- 2))

// program memory
unsigned int *ps;
// break up program
static int halt = 0;

// program counter
static int pc = 0;
// current instruktion in debug mode
static unsigned int debugCount = 0;
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

char *ziel_halbspeicher;
char *quell_halbspeicher;

int set_stack_size = 0;
int set_heap_size = 0;
unsigned int nextPointer = 0;

ObjRef copyObjectToFreeMem(ObjRef orig) {
    printf("copy\n");
    if(!BROKENHEART(orig)) {
        if(IS_PRIM(orig)) {
            memcpy(ziel_halbspeicher + nextPointer, orig, GET_SIZE(orig) + sizeof(unsigned int));
            nextPointer += GET_SIZE(orig) + sizeof(unsigned int);
        } else {
            memcpy(ziel_halbspeicher + nextPointer, orig, sizeof(ObjRef) * GET_SIZE(orig) + sizeof(unsigned int));
            nextPointer += sizeof(ObjRef) * (GET_SIZE(orig) + sizeof(unsigned int));
        }
    }

    return orig;
}

ObjRef relocate(ObjRef orig) {
    ObjRef copy;
    if (orig == NULL) {
        copy = NULL;
        printf("re\n");
    } else if (BROKENHEART(orig)) {
        printf("re1\n");
        copy = (ObjRef) (ziel_halbspeicher + FORWARDPOINTER(orig)); // + (ObjRef heap??)
    } else {
        printf("re2\n");
        copy = copyObjectToFreeMem (orig );

        orig->size = SBIT;
        orig->size = ((char*)copy - ziel_halbspeicher) | SBIT;
    }
    printf("re3\n");
    return copy;
}

void garbagecollector() {
    printf("print");
    char* scan;
    char *temp;
    temp = ziel_halbspeicher;
    ziel_halbspeicher = quell_halbspeicher;
    quell_halbspeicher = temp;
    nextPointer = 0;

    for(int j = 0; j < buffer.sda; j++)
        static_data_area[j] = relocate(static_data_area[j]);

    bip.op1 = relocate(bip.op1);
    bip.op2 = relocate(bip.op2);
    bip.res = relocate(bip.res);
    bip.rem = relocate(bip.res);

    r[0] = relocate(r[0]);

    for(int i = 0; i < sp; i++)
        if(stack[i].isObjRef)
            stack[i].u.objRef = relocate(stack[i].u.objRef);

    scan = ziel_halbspeicher;

    while(scan != ziel_halbspeicher + nextPointer) {
        if (!IS_PRIM((ObjRef) scan)) {
            for (int k = 0; k < GET_SIZE((ObjRef) scan); k++) {
                GET_REFS((ObjRef) scan)[k] = relocate(GET_REFS((ObjRef) scan)[k]);
            }
            scan += (GET_SIZE((ObjRef) scan) * 8 + sizeof(unsigned int));
        } else {
            scan += (GET_SIZE((ObjRef) scan) + sizeof(unsigned int));
        }
        printf("nach while\n");
    }
}

void *allocate(size_t size) {
    char *temp_heap;
    temp_heap = ziel_halbspeicher + nextPointer;
    nextPointer += size;
    if (temp_heap >= ziel_halbspeicher + set_heap_size) {
        garbagecollector();
    }
    return temp_heap;
}

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
        case NEW: printf("%.4d\tnew\t\n", i); break;
        case GETF: printf("%.4d\tgetf\t\n", i); break;
        case PUTF: printf("%.4d\tputf\t\n", i); break;
        case NEWA: printf("%.4d\tnewa\t\n", i); break;
        case GETFA: printf("%.4d\tgetfa\t\n", i); break;
        case PUTFA: printf("%.4d\tputfa\t\n", i); break;
        case GETSZ: printf("%.4d\tgetsz\t\n", i); break;
        case PUSHN: printf("%.4d\tpushn\t\n", i); break;
        case REFEQ: printf("%.4d\trefeq\t\n", i); break;
        case REFNE: printf("%.4d\trefne\t\n", i); break;
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
            //printf("Fehler halt\n");
            halt = 1; break;
        case PUSHC: // push value ir ^ (1 << 24)
            //printf("Fehler pushc\n");
            bigFromInt(SIGN_EXTEND(IMMEDIATE(ir)));
            pushObjRef(bip.res);
            break;
        case ADD:
            //	printf("Fehler add\n");
            bip.op1 = popObjRef();
            bip.op2 = popObjRef();
            bigAdd();
            pushObjRef(bip.res);
            break;
        case SUB:
            //	printf("Fehler sub\n");
            bip.op2 = popObjRef();
            bip.op1 = popObjRef();
            bigSub();
            pushObjRef(bip.res);
            break;
        case MUL:
            //	printf("Fehler mul\n");
            bip.op1 = popObjRef();
            bip.op2 = popObjRef();
            bigMul();
            pushObjRef(bip.res);
            break;
        case DIV:
            //	printf("Fehler div\n");
            bip.op2 = popObjRef();
            bip.op1 = popObjRef();
            bigDiv();
            pushObjRef(bip.res);
            break;
        case MOD:
            //	printf("Fehler mod\n");
            bip.op2 = popObjRef();
            bip.op1 = popObjRef();
            bigDiv();
            pushObjRef(bip.rem);
            break;
        case RDINT: // scan return 1 if read value successfully
            //	printf("Fehler rdint\n");
            if(scanf("%d", &value) == 1) {
                bigFromInt(value);
                pushObjRef(bip.res);
            } else {
                printf("Error: no digits in input\n");
                exit(1);
            }
            break;
        case WRINT:
            //	printf("Fehler wrint\n");
            bip.op1 = popObjRef();
            bigPrint(stdout);
            break;
        case RDCHR:
            //	printf("Fehler rdchr\n");
            value = getchar();
            bigFromInt(value);
            pushObjRef(bip.res);
            break;
        case WRCHR:
            //	printf("Fehler wrchr\n");
            bip.op1 = popObjRef();
            printf("%c", bigToInt());
            break;
        case PUSHG:
            //	printf("Fehler pushg\n");
            pushg(SIGN_EXTEND(IMMEDIATE(ir))); break;
        case POPG:
            //	printf("Fehler popg\n");
            popg(SIGN_EXTEND(IMMEDIATE(ir))); break;
        case ASF:
            //	printf("Fehler asf\n");
            pushNumber(fp);
            fp = sp;
            for(int i = 0; i < IMMEDIATE(ir); i++)
                pushObjRef(NULL);
            //sp = sp + IMMEDIATE(ir);
            break;
        case RSF:
            //	printf("Fehler rsf\n");
            sp = fp;
            fp = popNumber();
            break;
        case PUSHL:
            //	printf("Fehler pushl\n");
            pushl(SIGN_EXTEND(IMMEDIATE(ir)) + fp); break;
        case POPL:
            //	printf("Fehler popl\n");
            popl(fp+SIGN_EXTEND(IMMEDIATE(ir))); break;
        case EQ: // value1 == value2
            //	printf("Fehler eq\n");
            bip.op2 = popObjRef();
            bip.op1 = popObjRef();
            if(bigCmp() == 0)
                bigFromInt(1);
            else
                bigFromInt(0);
            pushObjRef(bip.res);
            break;
        case NE: // value1 != value2
            //	printf("Fehler ne\n");
            bip.op2 = popObjRef();
            bip.op1 = popObjRef();
            if(bigCmp() == 0)
                bigFromInt(0);
            else
                bigFromInt(1);
            pushObjRef(bip.res);
            break;
        case LT: // value1 < value2
            //	printf("Fehler lt\n");
            bip.op2 = popObjRef();
            bip.op1 = popObjRef();
            if(bigCmp() < 0)
                bigFromInt(1);
            else
                bigFromInt(0);
            pushObjRef(bip.res);
            break;
        case LE: // value1 <= value2
            //	printf("Fehler le\n");
            bip.op2 = popObjRef();
            bip.op1 = popObjRef();
            if(bigCmp() <= 0)
                bigFromInt(1);
            else
                bigFromInt(0);
            pushObjRef(bip.res);
            break;
        case GT: // value1 > value2
            //	printf("Fehler gt\n");
            bip.op2 = popObjRef();
            bip.op1 = popObjRef();
            if(bigCmp() > 0)
                bigFromInt(1);
            else
                bigFromInt(0);
            pushObjRef(bip.res);
            break;
        case GE: // value1 >= value2
            //	printf("Fehler ge\n");
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
            //	printf("Fehler brf\n");
            bip.op1 = popObjRef();
            if(bigToInt() == 0)
                pc = IMMEDIATE(ir);
            break;
        case BRT:
            //	printf("Fehler brt\n");
            bip.op1 = popObjRef();
            if(bigToInt() == 1)
                pc = IMMEDIATE(ir);
            break;
        case CALL:
            //	printf("Fehler call\n");
            pushNumber(pc);
            pc = IMMEDIATE(ir);
            break;
        case RET:
            //	printf("Fehler ret\n");
            pc = popNumber();
            break;
        case DROP:
            //	printf("Fehler drop\n");
            value = IMMEDIATE(ir);
            while(value > 0) {
                popObjRef();
                value--;
            }
            break;
        case PUSHR:
            //	printf("Fehler pushr\n");
            pushObjRef(r[0]); break;
        case POPR:
            //	printf("Fehler popr\n");
            r[0] = popObjRef(); break;
        case DUP:
            //	printf("Fehler dup\n");
            bip.res = popObjRef();
            pushObjRef(bip.res);
            pushObjRef(bip.res);
            break;
        case NEW:
            //	printf("Fehler new\n");
            pushObjRef(newCompoundObject(SIGN_EXTEND(IMMEDIATE(ir))));
            break;
        case GETF:
            //	printf("Fehler getf\n");
            pushObjRef(GET_REFS(popObjRef())[SIGN_EXTEND(IMMEDIATE(ir))]);
            break;
        case PUTF:
            //	printf("Fehler putf\n");
            bip.op1 = popObjRef();
            bip.op2 = popObjRef();
            GET_REFS(bip.op2)[SIGN_EXTEND(IMMEDIATE(ir))] = bip.op1;
            break;
        case NEWA:
            //	printf("Fehler newa\n");
            bip.op1 = popObjRef();
            bip.op2 = newCompoundObject(bigToInt());
            pushObjRef(bip.op2);
            break;
        case GETFA:
            //	printf("Fehler getfa\n");
            bip.op1 = popObjRef();
            bip.op2 = popObjRef();
            pushObjRef(GET_REFS(bip.op2)[bigToInt()]);
            break;
        case PUTFA:
            //	printf("Fehler putfa\n");
            bip.res = popObjRef();
            bip.op1 = popObjRef();
            bip.op2 = popObjRef();
            GET_REFS(bip.op2)[bigToInt()] = bip.res;
            break;
        case GETSZ:
            //	printf("Fehler getsz\n");
            bip.op1 = popObjRef();
            if(IS_PRIM(bip.op1))
                bigFromInt(-1);
            else
                bigFromInt(GET_SIZE(bip.op1));
            pushObjRef(bip.res);
            break;
        case PUSHN:
            //	printf("Fehler pushn\n");
            pushObjRef(NULL);
            break;
        case REFEQ:
            //	printf("Fehler refeq\n");
            bip.op1 = popObjRef();
            bip.op2 = popObjRef();
            if(bip.op2 == bip.op1)
                bigFromInt(1);
            else
                bigFromInt(0);;
            pushObjRef(bip.res);
            break;
        case REFNE:
            //	printf("Fehler refne\n");
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

void memory_is_full(void *x) {
    if(x == NULL) {
        printf("Error: heap overflow\n");
        exit(1);
    }
}

void load_data(char file[]) {
    r = allocate(sizeof(ObjRef));
    printf("r\n");
    memory_is_full(r);

    /*create file pointer*/
    FILE *f;

    /*open file*/
    f = fopen(file, "r");
    if(f == NULL) {
        printf("Error: cannot open code file '%s'\n", file);
        exit(1);
    } else {
        // read header of the file f
        if(fread(&buffer, sizeof(header_t), 1, f) != 1) {
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
        printf("vor static_data_area\n");
        static_data_area = allocate(buffer.sda * sizeof(ObjRef));
        printf("nach static_data_area\n");
        memory_is_full(static_data_area);
        /*if(static_data_area == NULL) {
            printf("memory is full.");
            exit(1);
        }*/

        // allocate memory program memory
        // warum sizeof(ps) = 8 anstatt 108?
        printf("vor ps\n");
        ps = allocate(buffer.noi * sizeof(int));
        printf("nach ps");
        memory_is_full(ps);
        /*if(ps == NULL) {
            printf("memory is full.");
            exit(1);
        }*/

        // read Instruktionen
        if(fread(ps, sizeof(int), buffer.noi, f) != buffer.noi) {
            printf("Error: reading\n");
            exit(1);
        }

        /*close file*/
        fclose(f);
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
            printf("\t\t--- bottom of stack ---\n");
        }
    } else if(strncmp(x, "object", count) == 0) {
        printf("object reference?\n");
        scanf("%p", &a);
        while(j < buffer.noi) {
            if(is_objRef(j))
                if(a == (void*) stack[j].u.objRef)
                    break;
            j++;
        }

        if(IS_PRIM(stack[j].u.objRef)){
            printf("<primitive object>\n");
            bip.op1 = stack[j].u.objRef;
            printf("Value:\t\t%d\n", bigToInt());
            printf("\t--- end of object ---\n");
        }
        getchar();
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
            debugCount = pc;
            return;
        } else

        if(step) {
            debugCount = pc;
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
    // inspect option like (stack, data, object)
    char x[6] = "";
    unsigned int option = 0;

    instruktion(debugCount);
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
            list(buffer.noi);
            break;
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

int counter = 0;
int start_debug = 0;
int bin = 0;
int position = 0;

int argn(int n, char *argv[], char *str[], int max) {
    int i = 0;
    if(!strcmp(argv[n], str[i++])) {
        if(n == max) {
            printf("Error: stack size is missing\n");
            exit(0);
        } else if(atoi(argv[n+1]) != 0) {
            if(atoi(argv[n+1]) < 0)
                ;
            else
                set_stack_size = atoi(argv[n+1]);
            counter++;
        } else {
            printf("Error: illegal stack size\n");
            exit(0);
        }
    } else if(!strcmp(argv[n], str[i++]))
        if(n == max) {
            printf("Error: heap size is missing\n");
            exit(0);
        } else if(atoi(argv[n+1]) != 0) {
            if(atoi(argv[n+1]) < 0)
                ;
            else
                set_heap_size = atoi(argv[n+1]);
            counter++;
        } else {
            printf("Error: illegal heap size\n");
            exit(0);
        }
    else if(!strcmp(argv[n], str[i++]))
        start_debug = 1;
    else if(!strcmp(argv[n], str[i++])) {
        printf("Ninja Virtual Machine version %d (compiled %s, %s)\n", VERSION, __DATE__, __TIME__);
        return 1;
    }
    else if(!strcmp(argv[n], str[i++])) {
        printf("usage: ./njvm [options] <code file>\n");
        printf("Option:\n");
        printf("  --stack <n>\t   set stack size to n KBytes (default: n = 64)\n");
        printf("  --heap <n>\t   set heap size to n KBytes (default: n = 8192)\n");
        printf("  --gcstats\t   show garbage collection statistics\n");
        printf("  --gcpurge\t   purge old objects after collection\n");
        printf("  --debug\t   start virtual machine in debug mode\n");
        printf("  --version\t   show version and exit\n");
        printf("  --help\t   show this help and exit\n");
        return 1;
    } else if(!strncmp(argv[n], "-", 1)) {
        printf("Error: unknown option '%s', try './njvm --help'\n", argv[n]);
        exit(0);
    } else {
        bin++;
        position = n;
        if(bin > 1) {
            printf("Error: more than one code file specified\n");
            return 1;
        }
    }
    return 0;
}

void start(char *argv) {
    if(set_stack_size > 0) {
        printf("stack1\n");
        stack = malloc(set_stack_size * 1024);
        printf("stack2\n");
        max_size = 64 * 1024 / sizeof(Stackslot);
        memory_is_full(stack);
        printf("stack3\n");
    } else {
        printf("stack4\n");
        stack = malloc(64 * 1024);
        printf("stack5\n");
        max_size = 64 * 1024 / sizeof(Stackslot);
        memory_is_full(stack);
        printf("stack6\n");
    }

    if(set_heap_size > 0) {
        printf("heap1\n");
        ziel_halbspeicher = malloc(set_heap_size * 512);
        printf("heap2\n");
        memory_is_full(ziel_halbspeicher);
        printf("heap3\n");
        quell_halbspeicher = malloc(set_heap_size * 512);
        printf("heap4\n");
        memory_is_full(quell_halbspeicher);
        printf("heap5\n");
    } else {
        printf("heap6\n");
        ziel_halbspeicher = malloc(8192 * 512);
        printf("heap7\n");
        memory_is_full(ziel_halbspeicher);
        printf("heap8\n");
        quell_halbspeicher = malloc(8192 * 512);
        printf("heap9\n");
        memory_is_full(quell_halbspeicher);
        printf("heap10\n");
    }

    if(start_debug && bin == 1) {
        load_data(argv);
        printf("DEBUG: file '%s' loaded (code size = ", argv);
        printf("%d, datasize = %d)\n", buffer.noi, buffer.sda);
        printf("Ninja Virtual Machine started\n");
        while(!halt) {
            debug();
        }
    } else if(bin == 1) {
        //printf("start programm\n");
        load_data(argv);

        printf("Ninja Virtual Machine started\n");

        // start program
        step = 0;
        switcher = 0;
        printf("vor run\n");
        run();

        // release memory from ps
        //free(ps);

        // release memory from static_data_area
        //free(static_data_area);
    }
}

int main(int argc, char *argv[]) {
    char *str[5];
    str[0] = "--stack";
    str[1] = "--heap";
    str[2] = "--debug";
    str[3] = "--version";
    str[4] = "--help";
    int stopp = 1;

    if(argc == 1)
        printf("Error: no code file specified\n");
    else if(argc == 2) {
        argn( 1, argv, str, 1);
        start(argv[position]);
    } else if(argc > 2) {
        for(counter = 1; counter < argc; counter++)
            if(argn(counter, argv, str, argc-1)) {
                stopp = 0;
                break;
            }

        if(stopp)
            start(argv[position]);
    }
    return 0;
}
// ToDo
/* Garbage Collector starts here */


