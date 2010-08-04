/**

Author:	Pierre Lindenbaum PhD
Mail:	plindenbaum@yahoo.fr
WWW:	http://plindenbaum.blogspot.com

Copyright (c) 2010 Pierre Lindenbaum PhD

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
#include <mysql.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <limits.h>

#include "fastaindexer.h"


#ifndef GETSEQ_BUFFER_SIZE
	#define GETSEQ_BUFFER_SIZE BUFSIZ 
#endif


#ifndef MIN
	#define MIN(a,b) (a<b?(a):(b))
#endif


/** FILE to fasta file */
typedef struct file_array
	{
	/** flag -1 if file couldn't be opened */
	int flag[FASTA_FILE_COUNT];
	/** fasta file */
	FILE* file[FASTA_FILE_COUNT];
	}FileArray,*FileArrayPtr;


/** comparator for bsearch */
static int _cmpFastaIndex( const void* a, const void* b) 
	{
	/* title is the first member, we can cast this to (const char*) */
	return strcmp((const char*)a,(const char*)b);	
	}

/** returns a FastaIndexPtr from the title of a sequence */
static const FastaIndexPtr  fastaIndexFromName(const char* seqName,char *errMsg)
	{
	FastaIndexPtr ptr; 
	if(seqName==NULL)
		{
		snprintf(errMsg,MYSQL_ERRMSG_SIZE,"name is nil\n");
		return NULL;
		}
	ptr=(FastaIndexPtr)bsearch((void*)seqName,(void*)fastaIndexes,FASTA_SEQUENCE_COUNT,sizeof(FastaIndex),_cmpFastaIndex);
	if(ptr==NULL)
		{
		snprintf(errMsg,MYSQL_ERRMSG_SIZE,"unknown sequence \"%s\" ?\n",seqName);
		return NULL;
		}
	return ptr;
	}

/** the core of the program, fseek to the correct index in the file , reads the sequence and returns it */
static char* fetch_sequence(
		const FastaIndexPtr fastaIndex,
		FILE* file,
		int start,
		int length,
		char *errMsg
		)
		{
		char *seq=NULL;
		char *buffer=NULL;
		int row_index=-1;
		int index_in_row=-1;
		int buffer_length=-1;
		int index_in_buffer=-1;
		int index_in_seq=-1;
		#ifdef GETSEQ_MAX_LENGTH
		if(length>=GETSEQ_MAX_LENGTH)
			{
			snprintf(errMsg,MYSQL_ERRMSG_SIZE,
				"length(%s)>=max-length(%ld)\n",
				length,
				MYSQL_ERRMSG_SIZE
				);
			return NULL;
			}
		#endif
		if(start < 0)
			{
			snprintf(errMsg,MYSQL_ERRMSG_SIZE,"start<0:%d\n",start);
			return NULL;
			}
		if(start+length> fastaIndex->seqLength)
			{
			snprintf(errMsg,MYSQL_ERRMSG_SIZE,"start=%d length=%d > size=%ld\n",start,length,fastaIndex->seqLength);
			return NULL;
			}
		/* the array of byte where we store the final sequence */
		seq=malloc(sizeof(char)*(length+1));
		if(seq==NULL)
			{
			snprintf(errMsg,MYSQL_ERRMSG_SIZE,"out of memory=%d\n",length);
			return NULL;
			}
		
		/** the fasta row index */
		row_index=start/fastaIndex->lineSize;
		/** index of 'start' in this current row */
		index_in_row=start-row_index*fastaIndex->lineSize;
		/** prepare a buffer to read some bytes from the fasta file */
		buffer=malloc(sizeof(char)*GETSEQ_BUFFER_SIZE);
		if(buffer==NULL)
			{
			free(seq);
			snprintf(errMsg,MYSQL_ERRMSG_SIZE,"out of memory=%d\n",GETSEQ_BUFFER_SIZE);
			return NULL;
			}
		/**  number of bytes in the buffer */
		buffer_length=0;
		/** current position in the buffer */
		index_in_buffer=0;
		/** current position in the final array of bytes (sequence) */
		index_in_seq=0;
		/** move the IO cursor to the beginning of the sequence */
		if(fseek(	file,
			fastaIndex->offsetSeqStart +
			row_index*(fastaIndex->lineSize+1)+
			index_in_row,
			SEEK_SET
			)!=0)
			{
			free(seq);
			free(buffer);
			snprintf(errMsg,MYSQL_ERRMSG_SIZE,"fseek failed.\n");
			return NULL;
			}
		while(length > 0)
			{
			/* buffer empty ? fill it */
			if(buffer_length==0)
				{
				buffer_length=fread(buffer,sizeof(char),GETSEQ_BUFFER_SIZE,file);
				index_in_buffer=0;
				if(buffer_length<=0)
					{
					free(seq);
					free(buffer);
					snprintf(errMsg,MYSQL_ERRMSG_SIZE,"cannot fill buffer.\n");
					return NULL;
					}
				}
			/* number of byte to copy into the sequence */
			int n_to_copy= MIN(buffer_length-index_in_buffer,MIN(length,fastaIndex->lineSize-index_in_row));
			
			
			/* copy the bytes from the buffer to the sequence */
			memcpy(&seq[index_in_seq],&buffer[index_in_buffer],n_to_copy);
			
			/* move the offsets */
			index_in_seq+=n_to_copy;
			length-=n_to_copy;
			index_in_buffer+=n_to_copy;
			index_in_row=(index_in_row+n_to_copy)%fastaIndex->lineSize;
			
			/* check the next  input is a CR/LF */  
			if(length>0)
				{
				if(index_in_row==0)
					{
					/* buffer is filled, read from input stream */
					if(index_in_buffer==buffer_length)
						{
						if(fgetc(file)!='\n')
							{
							free(seq);
							free(buffer);
							snprintf(errMsg,MYSQL_ERRMSG_SIZE,"I/O error expected a carriage return.\n");
							return NULL;
							}
						}
					else /* check the buffer */
						{
						if(buffer[index_in_buffer]!='\n')
							{
							free(seq);
							free(buffer);
							snprintf(errMsg,MYSQL_ERRMSG_SIZE,"I/O error expected a carriage return.\n");
							return NULL;
							}
						index_in_buffer++;
						}
					}
					
				/* reset the buffer it is filled */
				if(index_in_buffer==buffer_length)
					{
					buffer_length=0;
					index_in_buffer=0;
					}
				}
			
			
			}
		
		
		free(buffer);
		seq[index_in_seq]=0;
		return seq;
		}

/** returns the sequence as a C-String for this sequence name/range. Return NULL or a string that should
 be free-ed with free */
static char* getSequence(FileArrayPtr fileArray,const char* seqName,int start,int length,char *errMsg)
	{
	FastaIndexPtr ptr=NULL;
	
	#ifdef GETSEQ_MAX_LENGTH
	if(length>=GETSEQ_MAX_LENGTH)
		{
		snprintf(errMsg,MYSQL_ERRMSG_SIZE,"length(%s)>=max-length(%d)\n",length,MYSQL_ERRMSG_SIZE);
		return NULL;
		}
	#endif
	ptr=fastaIndexFromName(seqName,errMsg);
	if(ptr==NULL) return NULL;
	//file flagged as bad forever ?
	if(fileArray->flag[ptr->fileIndex]==-1) return NULL;
	//file need to be open ?
	if(fileArray->file[ptr->fileIndex]==NULL)
		{
		int err;
		errno=0;
	 	fileArray->file[ptr->fileIndex]=fopen(fasta_filenames[ptr->fileIndex],"r");
		err=errno;
		if(fileArray->file[ptr->fileIndex]==NULL)
			{
			snprintf(errMsg,MYSQL_ERRMSG_SIZE,"Cannot open file(index:%d) \"%s\" %s.\n",
				ptr->fileIndex,
				fasta_filenames[ptr->fileIndex],
				strerror(err)
				);
			fileArray->flag[ptr->fileIndex]=-1;
			return NULL;
			}
		}
	return fetch_sequence(
		ptr,
		fileArray->file[ptr->fileIndex],
		start,
		length,
		errMsg
		);
	}


/** Data Handler for MYSQL */
typedef struct mysqlFasta_t
	{
	FileArray fileArray;
	char errMsg[MYSQL_ERRMSG_SIZE];
	char* sequence;
	} MysqlFasta,*MysqlFastaPtr;

/* The initialization function */
my_bool faidx_init(
        UDF_INIT *initid,
        UDF_ARGS *args,
        char *message
        )
  {
  MysqlFastaPtr fastaptr=NULL;
  /* check the args */
  if (!(args->arg_count == 3 &&
        args->arg_type[0] == STRING_RESULT &&
        args->arg_type[1] == INT_RESULT &&
        args->arg_type[1] == INT_RESULT
        ))
    {
    strncpy(message,"Bad parameter expected a chrom-name,start,end",MYSQL_ERRMSG_SIZE);
    return 1;
    }
  initid->maybe_null=1;
  initid->ptr= NULL;

  fastaptr=(MysqlFastaPtr)malloc(sizeof(MysqlFasta));
  if(fastaptr==NULL)
        {
        strncpy(message,"Out Of Memory",MYSQL_ERRMSG_SIZE);
        return 1;
        }
  memset(fastaptr,0,sizeof(MysqlFasta));
  initid->ptr=(void*)fastaptr;
  return 0;
  }

/* The deinitialization function */
void  faidx_deinit(UDF_INIT *initid)
        {
        /* free the memory **/
        if(initid->ptr!=NULL)
                {
		int i;
                MysqlFastaPtr fastaptr=(MysqlFastaPtr)initid->ptr;
                for(i=0;i< FASTA_FILE_COUNT;++i)
			{
			if(fastaptr->fileArray.file[i]!=NULL)
				{
				fclose(fastaptr->fileArray.file[i]);
				}
			}
		free(fastaptr->sequence);
                free(fastaptr);
                initid->ptr=NULL;
                }
        }

/* The main function. This is where the function result is computed */
char *faidx(
	UDF_INIT *initid, UDF_ARGS *args, char *result,
	unsigned long *length, char *is_null, char *error)
 {
 MysqlFastaPtr fastaptr=(MysqlFastaPtr)initid->ptr;
 long namesize=  args->lengths[0];
 long long start_val;
 long long end_val;
 char  seqName[FASTA_SEQUENCE_MAX_NAME_LENGTH+1];
 //bad Name 
 if(args->args[0]==NULL || args->args[1]==NULL || args->args[2]==NULL ||
    namesize==0 || namesize>FASTA_SEQUENCE_MAX_NAME_LENGTH)
  	{
  	*is_null=1;
    	return NULL;
  	}
  *is_null=0;

  //create a NULL terminated string
  memcpy(seqName,args->args[0],args->lengths[0]);
  seqName[args->lengths[0]]=0;
 
  start_val = *((long long*) args->args[1]);
  end_val = *((long long*) args->args[2]);
  if(start_val<0 || end_val<start_val
	#ifdef GETSEQ_MAX_LENGTH
	|| (end_val-start_val)> GETSEQ_MAX_LENGTH+1
	#else
	|| (end_val-start_val)> INT_MAX-1
	#endif
  	)
  	{
  	*is_null=1;
    	return NULL;
  	}
  //remove previous result if any
  if(fastaptr->sequence!=NULL)
  	{
  	free(fastaptr->sequence);
  	fastaptr->sequence=NULL;
  	}
  fastaptr->sequence=getSequence(
  	&(fastaptr->fileArray),
  	seqName,
  	(int)start_val,
  	(int)(end_val-start_val),
  	fastaptr->errMsg
  	);
  if(fastaptr->sequence==NULL)
  	{
  	*length=0;
  	*is_null=1;
    	return NULL;
	//*length=strlen(fastaptr->errMsg);
	//return fastaptr->errMsg;
  	}
  *length=(end_val-start_val);
  return fastaptr->sequence;
  }

