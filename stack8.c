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
unsigned int set_stack_size = 0;
unsigned int set_heap_size = 0;
unsigned int halfsize = 0;

header_t buffer;

ObjRef copyObjectToFreeMem(ObjRef orig){
/*	printf("Speicherstelle:  %p, Source:   %p,    Große Objekt:    %i, next Pointer:         %i\n",heap + nextPointer, orig ,(GET_SIZE(orig) + sizeof(unsigned int)),nextPointer);*/
	if(!BROKEN_HEART(orig)){
		printf("BROKEN_HEART\n");
		if(IS_PRIM(orig)){
			printf("IS_PRIM\n");
			printf("nextPointer = %d und ziel_halbspeicher = %p und orig = %p\n", nextPointer, ziel_halbspeicher + nextPointer, (void*) orig);
	/*		printf("is prim\n");*/
			memcpy((ziel_halbspeicher + nextPointer), orig, (GET_SIZE(orig) + sizeof(unsigned int)));
			printf("nach IS_PRIM\n");
		/*	printf("%i             old prim\n", nextPointer);*/
			nextPointer += (GET_SIZE(orig) + sizeof(unsigned int));
			/*printf("%i            prim\n", nextPointer);*/
		}
		else{
			printf("not IS_PRIM\n");
		/*	printf("not prim\n");*/
			memcpy(ziel_halbspeicher + nextPointer, orig, (sizeof(ObjRef)*(GET_SIZE(orig) + sizeof(unsigned int))));
		/*	printf("not prim 2\n");*/
			nextPointer += (sizeof(ObjRef)*(GET_SIZE(orig) + sizeof(unsigned int)));
		/*	printf("%i                not prim\n", nextPointer);*/
			printf("nach not IS_PRIM\n");

		}
	}
	printf("nach copy\n");
	return orig;
}

ObjRef relocate(ObjRef orig) {
	printf("relocate\n");
	ObjRef copy;
	if(orig == NULL) {
		printf("orig = NULL\n");
		/*printf("%i      Null\n", nextPointer);*/
		copy = NULL;
	}
	else if((orig->size & SECBIT) == 1) {
		printf("realocate BROKEN_HEART\n");
		/*printf("%i           Broken_heart\n", nextPointer);*/
		copy = (ObjRef)(ziel_halbspeicher + FORWARDPOINTER(orig));
	}
	else
	{
		printf("realocate else = %p\n", (void*) orig);
	/*	printf("%i    %p      obj kopiert\n", nextPointer, objRef);*/
		copy = copyObjectToFreeMem(orig);
	/*	printf("%i    %p      copy kopiert\n", nextPointer, copy);*/

		orig->size = SECBIT;
		orig->size = FORWARDPOINTER(copy);
	}
	printf("nach relocate\n");
	return copy;
}

void garbagecollector(){
	char* scan = 0;
	int i = 0;
	int k = 0;
	int j = 0;
	char *temp_heap;
	temp_heap = ziel_halbspeicher;
	ziel_halbspeicher = quell_halbspeicher;
	quell_halbspeicher = temp_heap;
	nextPointer = 0;
	printf("vor stack\n");
	for(i = 0; i < sp; i++){
		if(stack[i].isObjRef){
			stack[i].u.objRef = relocate(stack[i].u.objRef);
			printf("stack[i].u.objRef = %p\n", (void*) stack[i].u.objRef);
		}
	}
	printf("nach stack\n");
	bip.op1 = relocate(bip.op1);
	bip.op2 = relocate(bip.op2);
	bip.res = relocate(bip.res);
	bip.rem = relocate(bip.rem);
	printf("bip\n");
	r[1] = relocate(r[1]);
	printf("returnregister\n");
	for(j = 0; j < buffer.sda; j++){
		static_data_area[j] = relocate(static_data_area[j]);
	}
	printf("static_data_area\n");
	scan = ziel_halbspeicher;
	while(scan != ziel_halbspeicher + nextPointer) {
/* es gibt noch Objekte, die gescannt werden müssen */
		if(!IS_PRIM((ObjRef)scan)) {
			for(k = 0; k < GET_SIZE((ObjRef)scan); k++) {
				GET_REFS((ObjRef)scan)[k] = relocate(GET_REFS((ObjRef)scan)[k]);
			}
		//scan += GET_SIZE((ObjRef)scan) * 8 + sizeof(unsigned int);
		}
		//else{
		scan +=  (GET_SIZE((ObjRef)scan) + sizeof(unsigned int));
		//}
	}
}

void *allocate(size_t size){
	char *temp_heap;
	
	/*printf("ziel_halbspeicher = %p\n", ziel_halbspeicher);
	printf("ziel_halbspeicher + nextPointer = %p\n", ziel_halbspeicher + nextPointer);
	printf("ziel_halbspeicher + halfsize = %p\n", ziel_halbspeicher + halfsize);*/
	//temp_heap = ziel_halbspeicher;
	//temp_heap = temp_heap + + nextPointer;
	temp_heap = ziel_halbspeicher + nextPointer;
	
	printf("Vergleich von Zeigern = %d\n", temp_heap >= (ziel_halbspeicher + halfsize));
	if(temp_heap >= (ziel_halbspeicher + halfsize)){
		printf("gc\n");
	
		printf("garbagecollector11111111111111111111111111111111111111111111111111111111111111111111\n");
		garbagecollector();
	}
	nextPointer += size;
	//printf("Vergleich von Zeigern 2 = %d\n", (temp_heap + size) >= (ziel_halbspeicher + halfsize));
	if((temp_heap + size) >= (ziel_halbspeicher + halfsize)) {
		printf("gc\n");
		garbagecollector();	
	//printf("nextPointer >= halfsize = %d\n", nextPointer >= halfsize);
	} else if(nextPointer >= halfsize) {
			return temp_heap = NULL;
	printf("temp_heap = %p\n", temp_heap);
	printf("ziel_halbspeicher + halfsize = %p\n", ziel_halbspeicher + halfsize);
	}	
	//printf("halfsize = %d\n", halfsize);
	
	return temp_heap;
}

ObjRef newCompoundObject(int objRefSize) {
    	ObjRef objRef = allocate(sizeof(unsigned int) + objRefSize * sizeof(ObjRef));
	objRef->size = objRefSize | MSB;
	if (objRef == NULL) {
   		 fatalError("newCompoundObject() got no memory");
  	}
	for(int i = 0; i < objRefSize; i++)
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
