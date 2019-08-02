#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "stack8.h"

Stackslot *stack;
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
unsigned int nextPointer = 0;

char *ziel_halbspeicher;
char *quell_halbspeicher;
char *heap;
unsigned int set_stack_size = 0;
unsigned int set_heap_size = 0;
unsigned int halfsize = 0;

header_t buffer;

char *temp_heap;

int button = 1;

ObjRef copyObjectToFreeMem(ObjRef orig){
		if(IS_PRIM(orig)){
			memcpy((ziel_halbspeicher + nextPointer), orig, (GET_SIZE(orig) + sizeof(unsigned int)));
			nextPointer += (GET_SIZE(orig) + sizeof(unsigned int));
		}
		else{
			memcpy(ziel_halbspeicher + nextPointer, orig, (sizeof(ObjRef)*(GET_SIZE(orig) + sizeof(unsigned int))));
			nextPointer += (sizeof(ObjRef)*(GET_SIZE(orig) + sizeof(unsigned int)));
		}
	return orig;
}

ObjRef relocate(ObjRef orig) {
	ObjRef copy;
	if(orig == NULL) {
		copy = NULL;
	}
	else if((orig->size & SECBIT) == 1) {
		copy->size = FORWARDPOINTER(orig);
	}
	else
	{
		copy = copyObjectToFreeMem(orig);
		
		orig->size = SECBIT;
		orig->size = FORWARDPOINTER(copy);
	}
	return copy;
}

void garbagecollector(){
	char* scan = 0;
	char *temp;
	temp = ziel_halbspeicher;
	ziel_halbspeicher = quell_halbspeicher;
	quell_halbspeicher = temp;
	nextPointer = 0;
	for(int i = 0; i < buffer.sda; i++){
		static_data_area[i] = relocate(static_data_area[i]);
	}
	r[1] = relocate(r[1]);
	for(int j = 0; j < sp; j++){
		if(stack[j].isObjRef){
			stack[j].u.objRef = relocate(stack[j].u.objRef);
		}
	}
	
	bip.op1 = relocate(bip.op1);
	bip.op2 = relocate(bip.op2);
	bip.res = relocate(bip.res);
	bip.rem = relocate(bip.rem);

	scan = ziel_halbspeicher;
	while(scan != ziel_halbspeicher + nextPointer) {
		if(!IS_PRIM((ObjRef)scan)) {
			for(int k = 0; k < GET_SIZE((ObjRef)scan); k++) {
				GET_REFS((ObjRef)scan)[k] = relocate(GET_REFS((ObjRef)scan)[k]);
			}
		}
		scan +=  (GET_SIZE((ObjRef)scan) + sizeof(unsigned int));
	}
	nextPointer = 0;
	if(button)
		button = 0;
	else
		button = 1;
}

void *allocate_header(size_t size) {
	temp_heap = ziel_halbspeicher + nextPointer;
	nextPointer += size;
	if(nextPointer >= halfsize) {
		printf("Ninja Virtual Machine started\n");
		return temp_heap = NULL;
	}
	return temp_heap;
}

void *allocate_data(size_t size){
	char *x;
	if(button) {
		temp_heap = ziel_halbspeicher + nextPointer;
		nextPointer += size;	
		x = temp_heap;
		if(x + size > quell_halbspeicher) {
			garbagecollector();	
		}
	} else {
		temp_heap = ziel_halbspeicher - nextPointer;
		nextPointer += size;	
		x = temp_heap;
		if(x - size < quell_halbspeicher) {
			garbagecollector();	
		}
	} 
	return temp_heap;
}

ObjRef newCompoundObject(int objRefSize) {
    	ObjRef objRef = malloc(sizeof(unsigned int) + objRefSize * sizeof(ObjRef));
	objRef->size = objRefSize | MSB;
	if (objRef == NULL) {
   		 fatalError("newCompoundObject() got no memory");
  	}
	for(int i = 0; i < GET_SIZE(objRef); i++)
		GET_REFS(objRef)[i] = NULL;

    return objRef;
}

int is_objRef(int i) {
	if(stack[i].isObjRef)
		return 1;
	return 0;
}

void pushNumber(int x) {
	if(sp >= max_size) {
		printf("Error: stack overflow\n");
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
		printf("Error: stack underflow\n");
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
		printf("Error: stack overflow\n");
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
		printf("Error: stack underflow\n");
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
