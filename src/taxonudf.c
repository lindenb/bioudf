/*
Copyright (c) 2006-2010 Pierre Lindenbaum PhD

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




mysql> create function taxon_name returns string soname "taxonudf.o";
mysql> create function taxon_id returns integer soname "taxonudf.o";
mysql> create aggregate function taxon_com returns integer soname "taxonudf.o";

mysql> select taxon_childof(taxon_id("Homo"),taxon_id("Homo Sapiens")) as "is Homo child of Homo.Sapiens ?";
+---------------------------------+
| is Homo child of Homo.Sapiens ? |
+---------------------------------+
|                               0 |
+---------------------------------+
1 row in set (0,00 sec)


mysql> select taxon_childof(taxon_id("Homo Sapiens"),taxon_id("Homo")) as "Is Homo.Sapiens descendant of Homo ?";
+--------------------------------------+
| Is Homo.Sapiens descendant of Homo ? |
+--------------------------------------+
|                                    1 |
+--------------------------------------+
1 row in set (0,00 sec)



mysql> create temporary table t1(cluster varchar(20),taxon int);

insert into t1(cluster,taxon) values("A",251093),("A",9781), ("A",37348),("B",9605),("B",9606),("B",63221),("C",32523),("C",33154),("C",7776),("C",9443);

mysql> select cluster,taxon as "ncbi-id",taxon_name(taxon) as "Name",taxon_childof(taxon,taxon_id("Primates")) as "Is_Primate" from t1;
+---------+---------+-------------------------------+------------+
| cluster | ncbi-id | Name                          | Is_Primate |
+---------+---------+-------------------------------+------------+
| A       |  251093 | Elephas antiquus              |          0 |
| A       |    9781 | Elephantidae gen. sp.         |          0 |
| A       |   37348 | Mammuthus                     |          0 |
| B       |    9605 | Homo                          |          1 |
| B       |    9606 | Homo sapiens                  |          1 |
| B       |   63221 | Homo sapiens neanderthalensis |          1 |
| C       |   32523 | Tetrapoda                     |          0 |
| C       |   33154 | Fungi/Metazoa group           |          0 |
| C       |    7776 | Gnathostomata                 |          0 |
| C       |    9443 | Primates                      |          1 |
+---------+---------+-------------------------------+------------+
10 rows in set (0.00 sec)

mysql> select taxon,taxon_name(taxon) from t1 where taxon_childof(taxon,taxon_id("Hominidae"));
+-------+-------------------------------+
| taxon | taxon_name(taxon)             |
+-------+-------------------------------+
|  9605 | Homo                          |
|  9606 | Homo sapiens                  |
| 63221 | Homo sapiens neanderthalensis |
+-------+-------------------------------+
3 rows in set (0.00 sec)

mysql> select cluster,taxon_com(taxon) as ncbi_id from t1 group by cluster;
+---------+---------+
| cluster | ncbi_id |
+---------+---------+
| A       |    9780 |
| B       |    9605 |
| C       |   33154 |
+---------+---------+
3 rows in set (0.00 sec)

*/
#include <my_global.h>
#include <m_ctype.h>
#include <mysql.h>
#include <m_string.h>
#include <stdio.h>
#include <stdlib.h>

#include "taxon.h"




/**
Binary search for a Taxon from its id
returns 0 if taxon was found
*/

static int getTaxonById(FILE* in, taxon_id_t taxon_id,TaxonPtr taxonPtr)
	{
	long low = 0;
	long len= TAXON_COUNT;
	
	
	if(taxon_id==NO_TAXON) return -1;
	
	
	while(len>0)
		{
		int half=len/2;
		long mid=low+half;
		fseek(in,mid*sizeof(Taxon), SEEK_SET);
		if(fread(taxonPtr,sizeof(Taxon),1,in)!=1)
			{
			return -1;
			}
		
		if(taxonPtr->id == taxon_id)
			{
			return 0;
			}
		else if( taxonPtr->id < taxon_id)
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
    return -1;
    }




static taxon_id_t* getTaxonLineage(FILE* in,taxon_id_t id,int *size)
    {
    taxon_id_t* list=NULL;
    *size=0;
    for(;;)
	{
	Taxon taxon;
	taxon_id_t* ptr;
	if(getTaxonById(in,id,&taxon)!=0)
		{
		free(list);
		*size=0;
		return NULL;
		}
	ptr=realloc(list,sizeof(taxon_id_t)*(*size+1));
	if(ptr==NULL)
		{
		free(list);
		*size=0;
		return NULL;
		}
	list=ptr;
	list[*size]=taxon.id;
	(*size)++;
	if(taxon.id==TAXON_ROOT) break;
	id=taxon.parent_id;
	}

    return list;
    }

static int getCommonAncestor(
	FILE* in,
	taxon_id_t a,
	taxon_id_t b,
	taxon_id_t *common
	)
    {
    int retValue=0;
    int sizea=0;
    int sizeb=0;
    taxon_id_t* lineagea=getTaxonLineage(in,a,&sizea);
    taxon_id_t* lineageb=getTaxonLineage(in,b,&sizeb);
    
    if(lineagea!=NULL && lineageb!=NULL)
	{
	*common=TAXON_ROOT;
	while(sizea>0 && sizeb>0)
		{
		sizea--;
		sizeb--;
		if(lineagea[sizea]==lineageb[sizeb])
			{
			*common=lineagea[sizea];
			}
		else
			{
			break;
			}
		}
	}
   else
	{
	*common=NO_TAXON;
	retValue=-1;
	}

    free(lineagea);
    free(lineageb);
    return retValue;
    }    





/** taxon_name */
my_bool taxon_name_init(UDF_INIT *initid, UDF_ARGS *args, char *message);
void taxon_name_deinit(UDF_INIT *initid);
char *taxon_name(UDF_INIT *initid, UDF_ARGS *args, char *result,
               unsigned long *length, char *is_null, char *error);



typedef struct {
	FILE* in;
	char name[MAX_TAXON_NAME];
	} TaxonName,*TaxonNamePtr;

/** taxon_name */
my_bool taxon_name_init(
        UDF_INIT *initid,
        UDF_ARGS *args,
        char *message
        )
	{
	int err;
	if (!(args->arg_count == 1 && args->arg_type[0] == INT_RESULT ))
		{
		strncpy(message,"Bad parameter expected an integer",MYSQL_ERRMSG_SIZE);
		return 1;
		}
	initid->maybe_null=1;
	initid->ptr= malloc(sizeof(TaxonName));
	if(initid->ptr==NULL)
		{
		strncpy(message,"Out Of Memory",MYSQL_ERRMSG_SIZE);
		return 1;
		}
	memset((void*)initid->ptr,0,sizeof(TaxonName));
	errno=0;	
	((TaxonNamePtr)initid->ptr)->in=fopen(TAXON_FILE,"rb");
	err=errno;
	if(((TaxonNamePtr)initid->ptr)->in==NULL)
		{
		snprintf(message,MYSQL_ERRMSG_SIZE,"Cannot open %s (%s)",
			TAXON_FILE,
			strerror(err)
			);
		
		free(initid->ptr);
		initid->ptr=NULL;
		return 1;
		}
	return 0;
	}


void taxon_name_deinit(UDF_INIT *initid)
        {
        if(initid->ptr!=NULL)
		{
		if(((TaxonNamePtr)initid->ptr)->in!=NULL)
			{
			fclose(((TaxonNamePtr)initid->ptr)->in);
			}
		free(initid->ptr);
		initid->ptr=NULL;
		}
        }

char *taxon_name(UDF_INIT *initid, UDF_ARGS *args, char *result,
               unsigned long *length, char *is_null, char *error)
	{
	TaxonNamePtr env=((TaxonNamePtr)initid->ptr);
	Taxon taxon;
	taxon_id_t taxon_id = NO_TAXON;
	if(args->args[0]==NULL)
		{
		*is_null=1;
		return  NULL;
		}
	 
	 
	taxon_id= (taxon_id_t)(*((long long*) args->args[0]));
	*error = 0;
	
	if(getTaxonById(env->in,taxon_id,&taxon)!=0)
		{
		*is_null=1;
		return NULL; 
		}
	*is_null=0;
	*length = strlen(taxon.name);
	memcpy(env->name, taxon.name,MAX_TAXON_NAME);
	return env->name;
	}


/**
 * taxon_id
 *
 */
my_bool taxon_id_init(UDF_INIT *initid, UDF_ARGS *args, char *message)
	{
	FILE* in=NULL;
	int err=0;
	if (!(	args->arg_count == 1 &&
		args->arg_type[0] == STRING_RESULT
		))
		{
		strncpy(message,"Bad parameter expected one STRING",MYSQL_ERRMSG_SIZE);
		return 1;
		}
	initid->maybe_null=1;
	initid->ptr=NULL;
	errno=0;	
	in=fopen(TAXON_FILE,"rb");
	err=errno;
	if(in==NULL)
		{
		snprintf(message,MYSQL_ERRMSG_SIZE,"Cannot open %s (%s)",TAXON_FILE,strerror(err));
		initid->ptr=NULL;
		return 1;
		}
	initid->ptr=(void*)in;
	
	return 0;
	}

void taxon_id_deinit(UDF_INIT *initid)
	{
	 if(initid->ptr!=NULL)
		{
		FILE* in=((FILE*)initid->ptr);
		if(in!=NULL)
			{
			fclose(in);
			}
		initid->ptr=NULL;
		}
	}

long long taxon_id(UDF_INIT *initid, UDF_ARGS *args,
              char *is_null, char *error)
         {
         Taxon taxon;
	 FILE* in=((FILE*)initid->ptr);
	 long size= args->lengths[0];
	 char name[MAX_TAXON_NAME+1];
	 if(args->args[0]==NULL || in==NULL || size>=MAX_TAXON_NAME)
		{
		*is_null=1;
		return ((long long)-1);
		}
	
	strncpy(name,args->args[0],MAX_TAXON_NAME);
	fseek(in,0L, SEEK_SET);

	while(fread(&taxon,sizeof(Taxon),1,in)==1)
		{
		if(strcasecmp(name,taxon.name)==0)
			{
			*is_null=0;
			return taxon.id;
			}
		}
	*is_null=1;
	return ((long long)-1);
        }
	


/** taxon_childof */

typedef struct
	{
	FILE* in;
	} TaxonChildOf,*TaxonChildOfPtr;





/**
 * expect two taxon_id as parameters
 * select taxon_childof(taxon1,taxon2)
 *
 */
my_bool taxon_childof_init(UDF_INIT *initid, UDF_ARGS *args, char *message)
	{
	int err=0;
	if (!(	args->arg_count == 2 &&
		args->arg_type[0] == INT_RESULT &&
		args->arg_type[1] == INT_RESULT))
		{
		strncpy(message,"Bad parameter expected 2 integer",MYSQL_ERRMSG_SIZE);
		return 1;
		}
	initid->maybe_null=1;
	initid->ptr= malloc(sizeof(TaxonChildOf));
	if(initid->ptr==NULL)
		{
		strncpy(message,"Out Of Memory",MYSQL_ERRMSG_SIZE);
		return 1;
		}
	memset((void*)initid->ptr,0,sizeof(TaxonChildOf));
	errno=0;	
	((TaxonChildOfPtr)initid->ptr)->in=fopen(TAXON_FILE,"rb");
	err=errno;
	if(((TaxonChildOfPtr)initid->ptr)->in==NULL)
		{
		snprintf(message,MYSQL_ERRMSG_SIZE,"Cannot open %s (%s)",TAXON_FILE,strerror(err));
		free(initid->ptr);
		initid->ptr=NULL;
		return 1;
		}
	return 0;
	}

void taxon_childof_deinit(UDF_INIT *initid)
	{
	 if(initid->ptr!=NULL)
		{
		TaxonChildOfPtr env=((TaxonChildOfPtr)initid->ptr);
		if(env->in!=NULL)
			{
			fclose(env->in);
			}
		free(env);
		initid->ptr=NULL;
		}
	}

long long taxon_childof(UDF_INIT *initid, UDF_ARGS *args,
              char *is_null, char *error)
         {
         Taxon child;
	 taxon_id_t child_val = NO_TAXON;
	 taxon_id_t parent_val = NO_TAXON;
	 TaxonChildOfPtr env=((TaxonChildOfPtr)initid->ptr);
	
	if(args->args[0]==NULL ||
	   args->args[1]==NULL)
		{
		*is_null=1;
		return ((long long)-1);
		}
	
	child_val = (taxon_id_t)(*((long long*) args->args[0]));
	parent_val= (taxon_id_t)(*((long long*) args->args[1]));
	*error = 0;

	
	for(;;)
		{
		if(getTaxonById(env->in,child_val,&child)!=0)
			{
			*is_null=1;
			return ((long long)-1);
			}
		if(parent_val==child.id) return ((long long)1);
		if(child.id==TAXON_ROOT)
			{
			return ((long long)0);
			}
		child_val=child.parent_id;
		}
	
	return ((long long)0);
        }
	

/** taxon_com */
typedef struct
	{
	FILE* in;
	taxon_id_t common;
	my_bool is_error;
	}Common,*CommonPtr;
	
my_bool taxon_com_init(UDF_INIT *initid, UDF_ARGS *args, char *message)
	{
	int err=0;
	CommonPtr ptr;
	if (!(	args->arg_count == 1 &&
		args->arg_type[0] == INT_RESULT))
		{
		strncpy(message,"Bad parameter expected one integer",MYSQL_ERRMSG_SIZE);
		return 1;
		}
	initid->maybe_null=1;
	
	ptr= (CommonPtr)malloc(sizeof(Common));
	if(ptr==NULL)
		{
		initid->ptr=NULL;
		strncpy(message,"Out of memory.",MYSQL_ERRMSG_SIZE);
		return 1;
		}
	memset(ptr,0,sizeof(Common));
	ptr->common=NO_TAXON;	
	ptr->in=fopen(TAXON_FILE,"rb");
	err=errno;
	if(ptr->in==NULL)
		{
		snprintf(message,MYSQL_ERRMSG_SIZE,"Cannot open %s (%s)",TAXON_FILE,strerror(err));
		free(ptr);
		initid->ptr=NULL;
		return 1;
		}

	ptr->is_error=0;
	initid->ptr=(char*)ptr;
	return 0;
	}
	
void taxon_com_deinit(UDF_INIT *initid)
	{
	CommonPtr ptr=(CommonPtr)initid->ptr;
	if(ptr!=NULL)
		{
		fclose(ptr->in);
		free(ptr);
		}
	initid->ptr=NULL;
	}


void taxon_com_add( UDF_INIT* initid, UDF_ARGS* args, char* is_null, char* message )
	{
	CommonPtr ptr = (CommonPtr)initid->ptr;
	if(ptr->is_error==1) return;
	if(*is_null==0 && args->args[0]!=NULL)
		{
		taxon_id_t id = (taxon_id_t)( *((long long*) args->args[0]));
		Taxon taxon;
		if(getTaxonById(ptr->in,id,&taxon)!=0)
			{
			ptr->is_error=1;
			ptr->common=NO_TAXON;
			}
		else if(ptr->common==NO_TAXON)
			{
			ptr->common=taxon.id;
			}
		else
			{
			taxon_id_t commid=NO_TAXON;
			if(getCommonAncestor(ptr->in,taxon.id,ptr->common,&commid)!=0)
				{
				ptr->is_error=1;
				ptr->common=NO_TAXON;
				}
			else
				{
				ptr->common=commid;
				}
			}
		}
	else
		{
		ptr->is_error=1;
		ptr->common=NO_TAXON;
		}
	}

void taxon_com_clear(UDF_INIT *initid, char *is_null, char *error)
	{
	CommonPtr data = (CommonPtr)initid->ptr;
	data->common=NO_TAXON;
	data->is_error=0;
	*is_null = 0;
	
	}
	
void taxon_com_reset( UDF_INIT* initid, UDF_ARGS* args,
	char* is_null, char* message )
	{
	taxon_com_clear( initid, is_null, message );
	taxon_com_add( initid, args, is_null, message );
	}
	


long long taxon_com(UDF_INIT *initid, UDF_ARGS *args,
              char *is_null, char *error)
	{
	CommonPtr data = (CommonPtr)initid->ptr;
	if(data->is_error==1)
		{
		*error=1;
		*is_null=1;
		return NO_TAXON;
		}
	else if(data->common==NO_TAXON)
		{
		*error=0;
		*is_null=1;
		return NO_TAXON;
		}
	else
		{
		*is_null=0;
		return data->common;
		}
        }
