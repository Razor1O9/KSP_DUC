/*Datei stack.h*/

#ifndef vm_H
#define vm_H

extern char *ziel_halbspeicher;
extern char *quell_halbspeicher;
extern unsigned  int nextPointer;

void *allocate(size_t);

#endif

