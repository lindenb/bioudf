#ifndef TERM_H
#define TERM_H

#define MAX_TERM_LENGTH 12




typedef int go_id;

typedef struct Term_t
	{	
	go_id child_id;
	go_id parent_id;
	} Term,*TermPtr;

typedef struct Termdb
	{	
	TermPtr terms;
	int n_terms;
	} TermDB,*TermDBPtr;


#endif
