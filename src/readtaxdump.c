#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <limits.h>

typedef struct taxon_t
	{
	int id;
	int parent_id;
	char* name;
	}Taxon,*TaxonPtr;
	
static int order_by_id(const void *ptr1, const void *ptr2)
	{
	return *((int*)ptr1) -  *((int*)ptr2) ;
	}
	
int main(int argc,char** argv)
	{
	int lenRecord=0;
	int max_length=1;
	int i;
	char line[BUFSIZ];
	FILE* in;
	FILE* out;
	TaxonPtr taxons=NULL;
	int nTaxons=0;
	char *ptr;
	
	if(argc!=4)
		{
		fprintf(stderr,"Usage %s nodes.dmp names.dmp <FULL-PATH-FILEOUT>\n",argv[0]);
		return EXIT_FAILURE;
		}
	if(argv[3][0]!='/')
		{
		fprintf(stderr,"%s should be a full unix path name.\n",argv[3]);
		return EXIT_FAILURE;
		}
	/* read nodes.dmp */
	errno=0;
	in=fopen(argv[1],"r");
	if(in==NULL)
		{
		fprintf(stderr,"Cannot open %s %s\n",argv[1],strerror(errno));
		return EXIT_FAILURE;
		}
	while(fgets(line,BUFSIZ,in)!=NULL)
		{
		char* pipe2;
		char* pipe1=strstr(line,"\t|\t");
		if(pipe1==NULL) continue;
		pipe2=strstr((pipe1+3),"\t|\t");
		if(pipe2==NULL) continue;
		*pipe1=0;
		*pipe2=0;
		
		taxons=realloc(taxons,sizeof(Taxon)*(nTaxons+1));
		if(taxons==NULL)
			{
			fprintf(stderr,"Out of memory\n");
			return EXIT_FAILURE;
			}
		taxons[nTaxons].id=atoi(line);
		if(taxons[nTaxons].id==0 || taxons[nTaxons].id==INT_MAX || taxons[nTaxons].id==INT_MIN )
			{
			fprintf(stderr,"bad number %s\n",line);
			return EXIT_FAILURE;
			}
		taxons[nTaxons].parent_id=atoi(pipe1+3);
		if(taxons[nTaxons].parent_id==0 || taxons[nTaxons].parent_id==INT_MAX || taxons[nTaxons].parent_id==INT_MIN )
			{
			fprintf(stderr,"bad number %s\n",line);
			return EXIT_FAILURE;
			}
		
		taxons[nTaxons].name=NULL;
		nTaxons++;
		}
	fclose(in);
	
	/* sort taxon by id */
	qsort(taxons,nTaxons,sizeof(Taxon),order_by_id);
	
	/* read names.dmp */
	errno=0;
	in=fopen(argv[2],"r");
	if(in==NULL)
		{
		fprintf(stderr,"Cannot open %s %s\n",argv[2],strerror(errno));
		return EXIT_FAILURE;
		}
	while(fgets(line,BUFSIZ,in)!=NULL)
		{
		int id;
		char* pipe2=NULL;
		char* pipe1=strstr(line,"\t|\t");
		if(pipe1==NULL) continue;
		pipe2=strstr((pipe1+3),"\t|\t");
		if(pipe2==NULL) continue;
		*pipe1=0;
		*pipe2=0;
		
		id=atoi(line);
		TaxonPtr taxon=(TaxonPtr)bsearch((const void*)&id,(const void*)taxons, nTaxons,sizeof(Taxon),order_by_id );
		if(taxon==NULL)
			{
			fprintf(stderr,"Cannot find %d\n",id);
			continue;
			}
		if(taxon->name!=NULL)
			{
			if(strstr( (pipe2+1),"scientific name")!=NULL)
				{
				free(taxon->name);
				}
			else
				{
				continue;
				}
			}
		int len= strlen(pipe1+3);
		if((taxon->name=malloc(len+1))==NULL)
			{
			fprintf(stderr,"Out of memory");
			continue;
			}
		strcpy(taxon->name,pipe1+3);
		}
	fclose(in);
	
	/* get max length */
	for(i=0;i< nTaxons;++i)
		{
		int L;
		if(taxons[i].name==NULL)
			{
			fprintf(stderr,"Cannot get name for taxon id %d\n",taxons[i].id);
			return EXIT_FAILURE;
			}
		L=strlen(taxons[i].name);
		if(L>max_length) max_length=L+1;
		}
	
	printf("#ifndef NCBI_TAXON_HEADER\n#define NCBI_TAXON_HEADER\n");
	printf("#define TAXON_FILE \"%s\"\n",argv[3]);
	printf("#define TAXON_COUNT %d\n",nTaxons);
	printf("#define MAX_TAXON_NAME %d\n",max_length);
	printf("typedef int taxon_id_t;\n");
	printf("typedef struct\n"
		" {\n"
		" taxon_id_t id;\n"
		" taxon_id_t parent_id;\n"
		" char name[MAX_TAXON_NAME];\n"
		" }Taxon,*TaxonPtr;\n"
		);
	printf("#endif\n");
	/* save all */
	lenRecord=(sizeof(int)*2+sizeof(char)*(max_length));
	ptr=malloc(lenRecord);
	if(ptr==NULL)
		{
		fprintf(stderr,"Out of memory\n");
		return EXIT_FAILURE;
		}
	errno=0;
	out=fopen(argv[3],"wb");
	if(out==NULL)
		{
		fprintf(stderr,"Cannot open %s (%s)\n",argv[3],strerror(errno));
		return EXIT_FAILURE;
		}
	for(i=0;i< nTaxons;++i)
		{
		memset(ptr,0,lenRecord);
		memcpy(&ptr[0],&(taxons[i].id),sizeof(int));
		memcpy(&ptr[sizeof(int)],&(taxons[i].parent_id),sizeof(int));
		memcpy(&ptr[sizeof(int)*2],taxons[i].name,strlen(taxons[i].name)+1);
		if(fwrite(ptr,lenRecord,1,out)!=1)
			{
			fprintf(stderr,"fwrite failed.\n");
			return EXIT_FAILURE;
			}
		free(taxons[i].name);
		}
	free(ptr);
	fflush(out);
	fclose(out);
	free(taxons);
	return 0;
	}

