#include <my_global.h>
#include <m_ctype.h>
#include <mysql.h>
#include <m_string.h>

#ifndef GENETIC_CODE_STRING
	#define GENETIC_CODE_STRING "FFLLSSSSYY**CC*WLLLLPPPPHHQQRRRRIIIMTTTTNNKKSSRRVVVVAAAADDEEGGGG"
#endif

static const char* STANDARD_GENETIC_CODE = GENETIC_CODE_STRING;


static int base2index(char c)
	{
	switch(tolower(c))
		{
		case 't': return 0;
		case 'c': return 1;
		case 'a': return 2;
		case 'g': return 3;
		default: return -1;
		}
	}

/* a function translating 3 bases into an amino acid */
static char translation(char base1,char base2,char base3)
	{
	int base1= base2index(a);
	int base2= base2index(b);
	int base3= base2index(c);
	if(base1==-1 || base2==-1 || base3==-1)
		{
		return '?';
		}
	else
		{
		return STANDARD_GENETIC_CODE[base1*16+base2*4+base3];
		}
	}


/* The initialization function */
my_bool translate_init(
	UDF_INIT *initid,
	UDF_ARGS *args,
	char *message
	)
	{
	/* check the args */
	if (!(args->arg_count == 1 && args->arg_type[0] == STRING_RESULT ))
		{
		strncpy(message,"Bad parameter expected a DNA",MYSQL_ERRMSG_SIZE);
		return 1;
		}
	initid->maybe_null=1;
	initid->ptr= (char*)malloc(0);
	
	if(initid->ptr==NULL)
		{
		strncpy(message,"Out Of Memory",MYSQL_ERRMSG_SIZE);
		return 1;
		}
	return 0;
	}

/* The deinitialization function */
void translate_deinit(UDF_INIT *initid)
	{
	/* free the memory **/
	if(initid->ptr!=NULL) free(initid->ptr);
	initid->ptr=NULL;
	}

/* The main function. This is where the function result is computed */
char *translate(UDF_INIT *initid, UDF_ARGS *args, char *result,
unsigned long *length, char *is_null, char *error)
	{
	long i;
	long dnaLength= args->lengths[0];
	const char *dna=args->args[0];
	char *ptr=NULL;
	
	if (dna==NULL) /* Null argument */
		{
		*is_null=1;
		return NULL;
		}
	*length=dnaLength/3;
	ptr= (char*)realloc(initid->ptr,sizeof(char)*(*length));
	if(ptr==NULL)
		{
		*is_null=1;
		*error=1;
		strncpy(error,"Out Of Memory",MYSQL_ERRMSG_SIZE);
		return NULL;
		}
	initid->ptr=ptr;
	/* loop over the codons of the sequence */
	int j=0;
	for(i=0;i+2< dnaLength;i+=3)
		{
		initid->ptr[j++]=translation(dna[i],dna[i+1],dna[i+2]);
		}
	
	return initid->ptr;
	}


