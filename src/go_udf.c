#include <my_global.h>
#include <m_ctype.h>
#include <mysql.h>
#include <m_string.h>
#include <stdio.h>
#include <ctype.h>
#include "go_database.h"


/*
create function go_isa RETURNS INTEGER SONAME 'go_udf.so';

mysql>  select LEFT(T.name,20) as name,T.acc from go_latest.term as T where go_isa(T.acc,"GO:0016859");
+----------------------+------------+
| name                 | acc        |
+----------------------+------------+
| peptidyl-prolyl cis- | GO:0003755 | 
| retinal isomerase ac | GO:0004744 | 
| maleylacetoacetate i | GO:0016034 | 
| cis-trans isomerase  | GO:0016859 | 
| cis-4-[2-(3-hydroxy) | GO:0018839 | 
| trans-geranyl-CoA is | GO:0034872 | 
| carotenoid isomerase | GO:0046608 | 
| 2-chloro-4-carboxyme | GO:0047466 | 
| 4-hydroxyphenylaceta | GO:0047467 | 
| farnesol 2-isomerase | GO:0047885 | 
| furylfuramide isomer | GO:0047907 | 
| linoleate isomerase  | GO:0050058 | 
| maleate isomerase ac | GO:0050076 | 
| maleylpyruvate isome | GO:0050077 | 
| retinol isomerase ac | GO:0050251 | 
+----------------------+------------+
15 rows in set (1.27 sec)



drop function go_isa;

*/

/* The initialization function */
my_bool go_isa_init(UDF_INIT *initid, UDF_ARGS *args, char *message);
/* The deinitialization function */
void go_isa_deinit(UDF_INIT *initid);
/* The main function. This is where the function result is computed */
long long go_isa(UDF_INIT *initid, UDF_ARGS *args,
              char *is_null, char *error);

static go_id goID(const char* s)
	{
	char *p2=NULL;
	go_id id;
	if(!(toupper(s[0])=='G' && toupper(s[1])=='O' && s[2]==':'))
		{
		return -1;
		}
	id=strtol(&s[3],&p2,10);
	if(*p2!=0 || id==0 || id==INT_MAX || id==INT_MIN)
		{
		return -1;
		}
	return id;
	}

static int lower_bound(const TermDBPtr termsdb, go_id id)
	{
	int low = 0;
	int len= termsdb->n_terms;

	while(len>0)
		{
		int half=len/2;
		int mid=low+half;
		if( termsdb->terms[mid].child_id - id <0)
			{
			low=mid;
			++low;
			len=len-half-1;
			}
		else
			{
			len=half;
			}
		}
	return low;
	}


static int termdb_findIndexById(const TermDBPtr termsdb,go_id id)
	{
	int i=0;
	if(id<0 || termsdb==NULL || termsdb->terms==NULL || termsdb->n_terms==0) return -1;
	i= lower_bound(termsdb,id);
	if(i<0 || i  >= termsdb->n_terms || termsdb->terms[i].child_id!=id) return -1;
	return i;	
	}

static int recursive_search(const TermDBPtr db,int index, go_id parent_id,int depth)
	{
	int rez=0;
	int start=index;
	int parent_idx=0;
	
	if(start<0 || start>=db->n_terms) return 0;
	
	if(db->terms[index].child_id==parent_id) return 1;
	while(index < db->n_terms)
		{
		if( db->terms[index].child_id != db->terms[start].child_id) break;
		if(db->terms[index].parent_id==parent_id) return 1;
		parent_idx= termdb_findIndexById(db,db->terms[index].parent_id);
		
		rez= recursive_search(db,parent_idx,parent_id,depth+1);
		if(rez==1 )  return 1;
		++index;
		}
	return 0;
	}


/* The initialization function */
my_bool go_isa_init(
        UDF_INIT *initid,
        UDF_ARGS *args,
        char *message
        )
  {
  TermDBPtr termdb;
  FILE* in=NULL;
  /* check the args */
  if (!(args->arg_count == 2 &&
	args->arg_type[0] == STRING_RESULT &&
	args->arg_type[1] == STRING_RESULT))
    {
    strncpy(message,"Bad parameter expected a DNA",MYSQL_ERRMSG_SIZE);
    return 1;
    }
  initid->maybe_null=1;
  initid->ptr= NULL;

  termdb=(TermDBPtr)malloc(sizeof(TermDB));
  

  if(termdb==NULL)
        {
        strncpy(message,"Out Of Memory",MYSQL_ERRMSG_SIZE);
        return 1;
        }
  
  if((in=fopen(TERMDB_FILENAME,"r"))==NULL)
        {
        snprintf(message,MYSQL_ERRMSG_SIZE,"Cannot open %s.",TERMDB_FILENAME );
	free(termdb);
        return 1;
        }
  if(fread(&(termdb->n_terms),sizeof(int),1,in)!=1)
	{
	strncpy(message,"Cannot read n_items",MYSQL_ERRMSG_SIZE);
	fclose(in);
	free(termdb);
	return 1;
	}
  termdb->terms=malloc(sizeof(Term)*(termdb->n_terms));

	
  if(termdb->terms==NULL)
	{
	strncpy(message,"Cannot alloc terms",MYSQL_ERRMSG_SIZE);
	fclose(in);
	free(termdb);
	return 1;
	}
  if(fread(termdb->terms,sizeof(Term),termdb->n_terms,in)!=termdb->n_terms)
	{
	strncpy(message,"Cannot read items",MYSQL_ERRMSG_SIZE);
	fclose(in);
	free(termdb->terms);
	free(termdb);
	return 1;
	}
  fclose(in);
  initid->ptr=(void*)termdb;
  return 0;
  }

/* The deinitialization function */
void  go_isa_deinit(UDF_INIT *initid)
        {
        /* free the memory **/
        if(initid->ptr!=NULL)
		{
		TermDBPtr termdb=(TermDBPtr)initid->ptr;
		if(termdb->terms!=NULL) free(termdb->terms);
		free(termdb);
		initid->ptr=NULL;
		}
        }


/* The main function. This is where the function result is computed */
long long go_isa(UDF_INIT *initid, UDF_ARGS *args,
              char *is_null, char *error)
 {
  //long dnaLength= args->lengths[0];
  //const char *child=args->args[0];
  //const char *parent=args->args[0];
  char name1[MAX_TERM_LENGTH];
  char name2[MAX_TERM_LENGTH];
  TermDBPtr termdb=(TermDBPtr)initid->ptr;
  int index;

  *is_null=0;

  if (args->args[0]==NULL || args->args[1]==NULL
	|| args->lengths[0]>=MAX_TERM_LENGTH
	|| args->lengths[1]>=MAX_TERM_LENGTH
	) /* Null argument */
   {
    *is_null=1;
    return -1;
   }
  strncpy(name1,args->args[0],args->lengths[0]);
  name1[args->lengths[0]]=0;
  strncpy(name2,args->args[1],args->lengths[1]);	
  name2[args->lengths[1]]=0;

 index=termdb_findIndexById(termdb,goID(name1));
 if(index==-1)
	{
    	return 0;
	}
 return recursive_search(termdb,index,goID(name2),0);
 }


