/**

Copyright (c) 2006 Pierre Lindenbaum PhD

Permission is hereby granted, free of charge, to any person obtaining
a copy of this software and associated documentation files (the
``Software''), to deal in the Software without restriction, including
without limitation the rights to use, copy, modify, merge, publish,
distribute, sublicense, and/or sell copies of the Software, and to
permit persons to whom the Software is furnished to do so, subject to
the following conditions:

The above copyright notice and this permission notice shall be included
in all copies or substantial portions of the Software.

The name of the authors when specified in the source files shall be
kept unmodified.

THE SOFTWARE IS PROVIDED ``AS IS'', WITHOUT WARRANTY OF ANY KIND, EXPRESS
OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL 4XT.ORG BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE
USE OR OTHER DEALINGS IN THE SOFTWARE.

*/
#include <my_global.h>
#include <m_ctype.h>
#include <mysql.h>

#include <m_string.h>
/* this function is called by mysql to initialize it */
my_bool revcomp_init(UDF_INIT *initid, UDF_ARGS *args, char *message);
/* this function is called by mysql to dispose it */
void revcomp_deinit(UDF_INIT *initid);
/* the main function with get the reverse complement of a dna */
char *revcomp(UDF_INIT *initid, UDF_ARGS *args, char *result,
               unsigned long *length, char *is_null, char *error);

/* a trivial function returning the complementary base of an acid nucleic */
static char complement(char b)
	{
	switch(b)
		{
                case 'A': return 'T';
                case 'T': return 'A';
                case 'G': return 'C';
                case 'C': return 'G';

                case 'a': return 't';
                case 't': return 'a';
                case 'g': return 'c';
                case 'c': return 'g';

                case 'w': return 'w';
                case 'W': return 'W';

                case 's': return 's';
                case 'S': return 'S';

                case 'y': return 'r';
                case 'Y': return 'R';

                case 'r': return 'y';
                case 'R': return 'Y';

                case 'k': return 'm';
                case 'K': return 'M';

                case 'm': return 'k';
                case 'M': return 'K';

                case 'b': return 'v';
                case 'd': return 'h';
                case 'h': return 'd';
                case 'v': return 'b';


                case 'B': return 'V';
                case 'D': return 'H';
                case 'H': return 'D';
                case 'V': return 'B';

                case 'N': return 'N';
                case 'n': return 'n';

		}
	return '?';
	}


/** this function is called by mysql to initialize our revcomp function */
my_bool revcomp_init(
        UDF_INIT *initid,
        UDF_ARGS *args,
        char *message
        )
  {
 /* check we have one STRING argument */
  if (!(args->arg_count == 1 && args->arg_type[0] == STRING_RESULT ))
    {
    strncpy(message,"Bad parameter, expected a DNA",MYSQL_ERRMSG_SIZE);
    return 1;
    }
  initid->maybe_null=1;
  /* initid->ptr will be used to store the transformed sequence */
  initid->ptr= (char*)malloc(0);
  /* out of memory ? */
  if(initid->ptr==NULL)
        {
        strncpy(message,"Out Of Memory",MYSQL_ERRMSG_SIZE);
        return 1;
        }
  return 0;
  }

/** this function is called by mysql to dispose our revcomp function */
void revcomp_deinit(UDF_INIT *initid)
        {
	/* free the user ptr */
        if(initid->ptr!=NULL) free(initid->ptr);
        }

/** this is the function called by mysql to reverse-complement a DNA */
char *revcomp(UDF_INIT *initid, UDF_ARGS *args, char *result,
               unsigned long *length, char *is_null, char *error)
 {
  long i;
  /* the size of the input */
  long size= args->lengths[0];
  /* the DNA given as input */
  const char *dna=args->args[0];
  char *ptr=NULL;

  if (dna==NULL) // DNA is a null argument
   {
    *is_null=1;
    return NULL;
   }
/* the length of the returned string will be 'size' */
*length=size;


/** try to reallocate our memory to store the new transformed DNA sequence */
ptr= (char*)realloc(initid->ptr,sizeof(char)*(size));//no need (size+1) 

/* out of memory ? */
if(ptr==NULL)
        {
        *is_null=1;
        *error=1;
        strncpy(error,"Out Of Memory",MYSQL_ERRMSG_SIZE);
        return NULL;
        }
initid->ptr=ptr;
*is_null=0;        
*error=0;

/* build the reverse complement */
for(i=0;i< size;++i)
        {
	initid->ptr[i] = complement( dna[(size-1)-i] );
        }

/* return our pointer */
return initid->ptr;
}

