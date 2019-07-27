/*Datei stack.h*/

#ifndef STACK_H
#define STACK_H

#include <support.h>

//is this object a primitive object?
#define MSB		(1 << (8 * sizeof(unsigned int) - 1))
#define IS_PRIM(objRef) (((objRef)->size & MSB) == 0)
//How many objectrefernce are in this Object
#define GET_SIZE(objRef) ((objRef)->size & ~MSB)
//calculate a pointer on the first objectrefernce of object
#define GET_REFS(objRef) ((ObjRef *)(objRef)->data)

#define SIZE	10000

typedef struct {
	unsigned int isObjRef;
	union {
		ObjRef objRef;
		int number;
	} u;
} Stackslot;

extern Stackslot stack[SIZE];
extern ObjRef *static_data_area;
extern unsigned int sp;
extern unsigned int fp;
extern ObjRef *r;

ObjRef createObject(int value);
int is_objRef(int);
void pushNumber(int);
int popNumber(void);
void pushObjRef(ObjRef x);
ObjRef popObjRef(void);
void pushl(int);
void popl(int);
void pushg(int);
void popg(int);
ObjRef newCompoundObject(int numObjRefs);

#endif
