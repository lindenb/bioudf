#ifndef TERM_H
#define TERM_H

#define MAX_TERM_LENGTH 12

typedef struct Term_t
	{	
	char child[MAX_TERM_LENGTH];
	char parent[MAX_TERM_LENGTH];
	} Term,*TermPtr;

typedef struct Termdb
	{	
	TermPtr terms;
	int n_terms;
	} TermDB,*TermDBPtr;



#endif
