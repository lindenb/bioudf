#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <limits.h>
#include "taxonstruct.h"

	
static int order_by_id(const void *ptr1, const void *ptr2)
	{
	return (((TaxonPtr)ptr1)->id) -  (((TaxonPtr)ptr2)->id) ;
	}
	
int main(int argc,char** argv)
	{
	int i;
	char line[BUFSIZ];
	FILE* in;
	FILE* out;
	TaxonPtr taxons=NULL;
	int nTaxons=0;

	
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
			fprintf(stderr,"Out of memory cannot realloc for %d\n",nTaxons);
			return EXIT_FAILURE;
			}
		memset(&taxons[nTaxons],0,sizeof(Taxon));
		taxons[nTaxons].id=atoi(line);
		if(taxons[nTaxons].id==0 || taxons[nTaxons].id==INT_MAX || taxons[nTaxons].id==INT_MIN )
			{
			fprintf(stderr,"bad taxon-id number %s\n",line);
			return EXIT_FAILURE;
			}
		taxons[nTaxons].parent_id=atoi(pipe1+3);
		if(taxons[nTaxons].parent_id==0 || taxons[nTaxons].parent_id==INT_MAX || taxons[nTaxons].parent_id==INT_MIN )
			{
			fprintf(stderr,"bad parent_id number %s\n",line);
			return EXIT_FAILURE;
			}
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
		if(strlen(taxon->name)!=0)
			{
			if(strstr( (pipe2+1),"scientific name")==NULL)
				{
				continue;
				}
			}
		int len= strlen(pipe1+3);
		if(len+1>=MAX_TAXON_NAME)
			{
			fprintf(stderr,
				"Ok here we have a problem the max length of a taxon name was defined as %d but here we have len=%d. Please adjust taxonstruct.h", MAX_TAXON_NAME,len);
			}
		strcpy(taxon->name,pipe1+3);
		}
	fclose(in);
	
	
	for(i=0;i< nTaxons;++i)
		{
		if(strlen(taxons[i].name)==0)
			{
			fprintf(stderr,"Cannot get name for taxon id %d\n",taxons[i].id);
			return EXIT_FAILURE;
			}
		}
	
	printf("#ifndef NCBI_TAXON_HEADER\n#define NCBI_TAXON_HEADER\n");
	printf("#include \"taxonstruct.h\"\n");
	printf("#define TAXON_FILE \"%s\"\n",argv[3]);
	printf("#define TAXON_COUNT %d\n",nTaxons);
	printf("#endif\n");
	
	
	
	errno=0;
	out=fopen(argv[3],"wb");
	if(out==NULL)
		{
		fprintf(stderr,"Cannot open %s (%s)\n",argv[3],strerror(errno));
		return EXIT_FAILURE;
		}
	for(i=0;i< nTaxons;++i)
		{
		if(fwrite(&taxons[i],sizeof(Taxon),1,out)!=1)
			{
			fprintf(stderr,"fwrite failed.\n");
			return EXIT_FAILURE;
			}
		}
	
	fflush(out);
	fclose(out);
	free(taxons);
	
	
	
	return 0;
	}

