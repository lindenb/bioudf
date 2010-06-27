/** 

Author:	Pierre Lindenbaum PhD
Mail:	plindenbaum@yahoo.fr
WWW:	http://plindenbaum.blogspot.com

*/
#include <mysql.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <limits.h>



#ifndef GETSEQ_PREFIX
	#define GETSEQ_PREFIX getSeq
#endif

#ifndef GETSEQ_BUFFER_SIZE
	#define GETSEQ_BUFFER_SIZE BUFSIZ 
#endif

#ifdef MYSQL_VERSION
	#define GETSEQ_MAX_LENGTH_ERR_MESSAGE MYSQL_ERRMSG_SIZE
#endif

#ifndef GETSEQ_MAX_LENGTH_ERR_MESSAGE
	#define GETSEQ_MAX_LENGTH_ERR_MESSAGE 100
#endif

#ifndef MIN
	#define MIN(a,b) (a<b?(a):(b))
#endif

/** number of distinct fasta files */
#define GETSEQ_NUM_FILES	<xsl:value-of select="count(/rdf:RDF/fi:Index/fi:source[generate-id() = generate-id(key('filenames',@rdf:resource))])"/>


/** FILE to fasta file */
typedef struct file_array
	{
	/** flag -1 if file couldn't be opened */
	int flag[GETSEQ_NUM_FILES];
	/** fasta file */
	FILE* file[GETSEQ_NUM_FILES];
	}FileArray,*FileArrayPtr;
#ifdef XXXXXXXXXXXXXXXXXXXXX
/** indexes for a sequence */
typedef struct fasta_index_t
	{
	/** fastaFile index */
	int fileIndex; 
	/** offset sequence start */
	long seqStart;
	/** offset sequence end */
	long seqEnd;
	/** length of a fasta line */
	int lineSize;
	/** line of a fasta sequence */
	int length;
	}FastaIndex,*FastaIndexPtr;

/** all the FastaIndex for each sequence */
static const FastaIndex  FASTA_INDEXES[]={
	{-1,0L,0L,0,0}
	};
typedef struct title2fastaIndex
	{
	char title[GETSEQ_MAX_TITLE_LENGTH];
	int fastaIndex;
	}Title2FastaIndex,*Title2FastaIndexPtr;
#endif



/** mapping dc:title to index in FASTA_INDEXES ordered by name for bsearch */
static const Title2FastaIndex name2index[GETSEQ_COUNT_NAMES]=
	{
	
	};

/** comparator for bsearch */
static int _cmpFastaIndex( const void* a, const void* b) 
	{
	/* title is the first member, we can cast this to (const char*) */
	return strcmp((const char*)a,(const char*)b);	
	}

/** returns a FastaIndexPtr from the title of a sequence */
static const FastaIndexPtr  fastaIndexFromName(const char* seqName,char *errMsg)
	{
	Title2FastaIndexPtr ptr; 
	if(seqName==NULL)
		{
		snprintf(errMsg,GETSEQ_MAX_LENGTH_ERR_MESSAGE,"name is nil\n");
		return NULL;
		}
	ptr=(Title2FastaIndexPtr)bsearch((void*)seqName,(void*)name2index,GETSEQ_COUNT_NAMES,sizeof(Title2FastaIndex),_cmpFastaIndex);
	if(ptr==NULL)
		{
		snprintf(errMsg,GETSEQ_MAX_LENGTH_ERR_MESSAGE,"unknown sequence \"%s\" ?\n",seqName);
		return NULL;
		}
	return  ( const FastaIndexPtr)&FASTA_INDEXES[ptr->fastaIndex];
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
			snprintf(errMsg,GETSEQ_MAX_LENGTH_ERR_MESSAGE,"length(%s)>=max-length(%d)\n",length,GETSEQ_MAX_LENGTH_ERR_MESSAGE);
			return NULL;
			}
		#endif
		if(start < 0)
			{
			snprintf(errMsg,GETSEQ_MAX_LENGTH_ERR_MESSAGE,"start<0:%d\n",start);
			return NULL;
			}
		if(start+length> fastaIndex->length)
			{
			snprintf(errMsg,GETSEQ_MAX_LENGTH_ERR_MESSAGE,"start=%d length=%d > size=%d\n",start,length,fastaIndex->length);
			return NULL;
			}
		/* the array of byte where we store the final sequence */
		seq=malloc(sizeof(char)*(length+1));
		if(seq==NULL)
			{
			snprintf(errMsg,GETSEQ_MAX_LENGTH_ERR_MESSAGE,"out of memory=%d\n",length);
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
			snprintf(errMsg,GETSEQ_MAX_LENGTH_ERR_MESSAGE,"out of memory=%d\n",GETSEQ_BUFFER_SIZE);
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
			fastaIndex->seqStart +
			row_index*(fastaIndex->lineSize+1)+
			index_in_row,
			SEEK_SET
			)!=0)
			{
			free(seq);
			free(buffer);
			snprintf(errMsg,GETSEQ_MAX_LENGTH_ERR_MESSAGE,"fseek failed.\n");
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
					snprintf(errMsg,GETSEQ_MAX_LENGTH_ERR_MESSAGE,"cannot fill buffer.\n");
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
							snprintf(errMsg,GETSEQ_MAX_LENGTH_ERR_MESSAGE,"I/O error expected a carriage return.\n");
							return NULL;
							}
						}
					else /* check the buffer */
						{
						if(buffer[index_in_buffer]!='\n')
							{
							free(seq);
							free(buffer);
							snprintf(errMsg,GETSEQ_MAX_LENGTH_ERR_MESSAGE,"I/O error expected a carriage return.\n");
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
		snprintf(errMsg,GETSEQ_MAX_LENGTH_ERR_MESSAGE,"length(%s)>=max-length(%d)\n",length,GETSEQ_MAX_LENGTH_ERR_MESSAGE);
		return NULL;
		}
	#endif
	ptr=fastaIndexFromName(seqName,errMsg);
	if(ptr==NULL) return NULL;
	
	if(fileArray->flag[ptr->fileIndex]==-1) return NULL;
	if(fileArray->file[ptr->fileIndex]==NULL)
		{
		errno=0;
		switch(ptr->fileIndex)
			{
			 <xsl:for-each select="/rdf:RDF/fi:Index/fi:source[generate-id() = generate-id(key('filenames',@rdf:resource))]">
			 case <xsl:value-of select="position()-1"/>:
			 	{
			 	fileArray->file[<xsl:value-of select="position()-1"/>]=fopen(&quot;<xsl:value-of select="substring(@rdf:resource,8)"/>&quot;,"r");
				if(fileArray->file[ptr->fileIndex]==NULL)
					{
					snprintf(errMsg,GETSEQ_MAX_LENGTH_ERR_MESSAGE,"Cannot open file(index:%d) <xsl:value-of select="substring(@rdf:resource,8)"/> \"%s\" .\n",ptr->fileIndex,strerror(errno));
					fileArray->flag[ptr->fileIndex]=-1;
					return NULL;
					}			 	
				break;
			 	}</xsl:for-each>
			default:
				{
				snprintf(errMsg,GETSEQ_MAX_LENGTH_ERR_MESSAGE,"How can I open for file Index %d?\n",ptr->fileIndex);
				return NULL;
				}
			}
		if(fileArray->file[ptr->fileIndex]==NULL)
			{
			snprintf(errMsg,GETSEQ_MAX_LENGTH_ERR_MESSAGE,"Cannot open file(index:%d)  \"%s\".\n",ptr->fileIndex,strerror(errno));
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
	char errMsg[GETSEQ_MAX_LENGTH_ERR_MESSAGE];
	char* sequence;
	} MysqlFasta,*MysqlFastaPtr;

/* The initialization function */
my_bool getsequence_init(
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
void  getsequence_deinit(UDF_INIT *initid)
        {
        /* free the memory **/
        if(initid->ptr!=NULL)
                {
		int i;
                MysqlFastaPtr fastaptr=(MysqlFastaPtr)initid->ptr;
                for(i=0;i< GETSEQ_NUM_FILES;++i)
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
char *getsequence(
	UDF_INIT *initid, UDF_ARGS *args, char *result,
	unsigned long *length, char *is_null, char *error)
 {
 MysqlFastaPtr fastaptr=(MysqlFastaPtr)initid->ptr;
 long namesize=  args->lengths[0];
 long long start_val;
 long long end_val;
 char  seqName[GETSEQ_MAX_TITLE_LENGTH+1];
 //bad Name 
 if(args->args[0]==NULL || args->args[1]==NULL || args->args[2]==NULL ||
    namesize==0 || namesize>GETSEQ_MAX_TITLE_LENGTH)
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
  //remove previous result
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
  	//*length=0;
  	//*is_null=1;
    	//return NULL;
	*length=strlen(fastaptr->errMsg);
	return fastaptr->errMsg;
  	}
  *length=(end_val-start_val);
  return fastaptr->sequence;
  }

