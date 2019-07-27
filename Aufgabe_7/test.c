#include <stdio.h>
#include <stdlib.h>

# define MSB			(1 << (8 * sizeof ( unsigned int) - 1))
# define IS_PRIM(objRef) 	(((objRef)->size & MSB) == 0)

#define GET_SIZE(objRef) ((objRef)->size & ~MSB)
#define GET_REFS(objRef) ((ObjRef *)(objRef)->data)

typedef struct {
unsigned int size;
unsigned char data [1]; 
} * ObjRef ;

typedef struct {
	unsigned int isObjRef;
	union {
		ObjRef objRef;
		int number;
	} u;
} Stackslot;

typedef struct {
int x;
int y;
} Point;

ObjRef newPrimObject(int dataSize) {
  ObjRef objRef;

  objRef = malloc(sizeof(unsigned int) +
                  dataSize * sizeof(unsigned char));
  if (objRef == NULL) {
    printf("newPrimObject() got no memory");
  }
  objRef->size = dataSize;
  return objRef;
}

int main(int argc, char *argv[]) {
	//int k;

	Point p = newPrimObject(2);
	//p = new(Point);
/*	p.x = 3;
	k = p.x;
	p.y = 2 * k;
	
	printf("p.y = %d\n", p.y);
*/	return 0;
}
