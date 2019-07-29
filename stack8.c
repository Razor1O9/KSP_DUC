#include <stdio.h>
#include <stdlib.h>
#include "stack8.h"

Stackslot *stack;
char *heap1;
char *heap2;
ObjRef *static_data_area;
// stackpointer
unsigned int sp = 0;
// framepointer
unsigned int fp = 0;
// ask for funktion with arguments and return value!
// instruktion popr save in stack or r?
// return register
ObjRef *r;
int max_size = 0;

ObjRef newCompoundObject(int objRefSize) {
    ObjRef objRef = malloc(sizeof(unsigned int) + objRefSize * sizeof(ObjRef));
    int counter = 0;
    objRef->size = objRefSize | MSB;

    do{
        *((ObjRef *)objRef->data + counter++) = NULL;
    } while(counter < GET_SIZE(objRef));

    return objRef;
}

int is_objRef(int i) {
	if(stack[i].isObjRef)
		return 1;
	return 0;
}

void pushNumber(int x) {
	if(sp >= max_size) {
		printf("stackoverflow.\n");
		exit(1);
	} else {
		stack[sp].isObjRef = 0;
		stack[sp].u.number = x;
		sp++;
	}
}

int popNumber(void) {
	int pop_wert = stack[sp-1].u.number;
	if(sp < 0) {
		printf("Stackunderflow.\n");
		exit(1);
	} else {
		sp--;
		stack[sp].u.number = 0;
		stack[sp].u.objRef = NULL;
	}
	return pop_wert;
}

void pushObjRef(ObjRef x) {
	if(sp >= max_size) {
		printf("stackoverflow.\n");
		exit(1);
	} else {
		stack[sp].isObjRef = 1;
		stack[sp].u.objRef = x;
		sp++;
	}
}

ObjRef popObjRef(void) {
	ObjRef o = stack[sp-1].u.objRef;
	if(sp < 0) {
		printf("Stackunderflow.\n");
		exit(1);
	} else {
		sp--;
		stack[sp].u.objRef = NULL;
	}
	return o;
}

// push local variable
void pushl(int location) {
	pushObjRef(stack[location].u.objRef);
}

// pop local variable
void popl(int location) {
	stack[location].u.objRef = popObjRef();
}

// push global variable
void pushg(int location) {
	pushObjRef(static_data_area[location]);
}

// pop global variable
void popg(int location) {
	static_data_area[location] = popObjRef();
}
