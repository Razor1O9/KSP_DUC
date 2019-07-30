#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "stack8.h"

Stackslot *stack;
char *ziel_halbspeicher;
char *quell_halbspeicher;
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

ObjRef copyObjectToFreeMem(ObjRef orig);

void garbagecollector();

// ToDo
/* Garbage Collector starts here */
ObjRef relocate(ObjRef orig) {
    ObjRef copy;
    if (orig == NULL) {
/* relocate(nil) = nil */
        copy = NULL;
    } else if (BROKENHEART(orig)) {
/* Objekt ist bereits kopiert , Forward -Pointer gesetzt */
        copy = FORWARDPOINTER(orig); // + (ObjRef heap??)
    } else {
/* Objekt muss noch kopiert werden */
        copy = copyObjectToFreeMem (orig );
/* im Original: setze Broken -Heart -Flag und Forward -Pointer */
        orig -> size = SBIT;
        orig -> size = ((char*)copy - heap | SBIT);
    }
/* Adresse des kopierten Objektes zurück */
    return copy;
}

ObjRef copyObjectToFreeMem(ObjRef orig) {

    if (!BROKENHEART(orig)) {
        if (IS_PRIM(orig)) {
            memcpy(heap + nextPointer, orig, (GET_SIZE(orig) + sizeof(unsigned int)));
            nextPointer = (GET_SIZE(orig) + sizeof(unsigned int) + nextPointer);
        } else {
            memcpy(heap + nextPointer, orig, (sizeof(ObjRef)*GET_SIZE(orig) + sizeof(unsigned int)));
            nextPointer = (nextPointer + sizeof(ObjRef)*(GET_SIZE(orig) + sizeof(unsigned int))));
        }
    }
    return orig;
}
// ToDo replace all malloc (except heap and stack) with allocate
void *allocate(size_t size){
    unsigned int halfheapsize;
    char *temp_heap;
    temp_heap = heap + nextPointer;
    nextPointer += size;
    if(temp_heap >=  heap + halfheapsize){
        garbagecollector();
    }
    return temp_heap;
}

void garbagecollector() {

}
/* Garbage Collector ends here */

ObjRef newCompoundObject(int objRefSize) {
    //ObjRef objRef = malloc(sizeof(unsigned int) + objRefSize * sizeof(ObjRef));
    ObjRef objRef = allocate(objRefSize * sizeof(ObjRef) + sizeof(unsigned int));
    objRef->size = objRefSize | MSB;

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
