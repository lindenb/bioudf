#include <libxml/parser.h>
#include <libxml/tree.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include "term.h"




static const char GO_NS[]="http://www.geneontology.org/dtds/go.dtd#";
static const char RDF_NS[]="http://www.w3.org/1999/02/22-rdf-syntax-ns#";

static TermDB termdb={NULL,0};

#define SAME(a,b) (xmlStrcmp(BAD_CAST  a,BAD_CAST   b)==0)

static go_id goId(const char* s)
	{
	go_id id;
	if(strncmp(s,"GO:",3)!=0)
		{
		fprintf(stderr,"Bad go identifier \"%s\".",s);
		exit(EXIT_FAILURE);
		}
	id=atoi(&s[3]);
	if(id==0 || id==INT_MAX || id==INT_MIN)
		{
		fprintf(stderr,"Bad go identifier \"%s\".",s);
		exit(EXIT_FAILURE);
		}
	return id;
	}

static void scanterm(xmlNode *term)
	{
	xmlNode *n=NULL;
	xmlChar * acn=NULL;
	for(n=term->children; n!=NULL; n=n->next)
		{
		if (n->type != XML_ELEMENT_NODE) continue;
		if(SAME( n->name,"accession") &&
			n->ns!=NULL && n->ns->href!=NULL &&
			SAME(n->ns->href,GO_NS))
			{
			acn= xmlNodeGetContent(n);
			break;
			}
		}
	if(acn==NULL) return;
	
	for(n=term->children; n!=NULL; n=n->next)
		{
		if (n->type != XML_ELEMENT_NODE) continue;
		if(SAME(n->name,"is_a") &&
			n->ns!=NULL && n->ns->href!=NULL &&
			SAME(n->ns->href,GO_NS)
			)
			{
			xmlChar* is_a=xmlGetNsProp(n,BAD_CAST "resource",BAD_CAST RDF_NS);
			if(is_a!=NULL)
				{
				char* p=strstr((const char*)is_a,"#GO:");
				if(p!=NULL)
					{
					++p;
					termdb.terms=(TermPtr)realloc(termdb.terms,sizeof(Term)*(termdb.n_terms+1));
					if(termdb.terms==NULL)
						{
						fprintf(stderr,"out of memory\n");
						exit(EXIT_FAILURE);
						}
					
					termdb.terms[termdb.n_terms].parent_id=goId(p);
					termdb.terms[termdb.n_terms].child_id=goId((const char*)acn);
					//fprintf(stdout,"%s\t%s\n",termdb.terms[termdb.n_terms].child,termdb.terms[termdb.n_terms].parent);
					++termdb.n_terms;				
					}
				xmlFree(is_a);
				}
			else
				{
				//fprintf(");
				}
			}
		}
	xmlFree(acn);
	}



static void scanterms(xmlNode *root_element)
	{
	xmlNode *n=NULL;
	for(n=root_element->children; n!=NULL; n=n->next)
		{
		if (n->type != XML_ELEMENT_NODE) continue;
		if(SAME(n->name,"term") &&
			n->ns!=NULL && n->ns->href!=NULL &&
			SAME(n->ns->href,GO_NS))
			{
			scanterm(n);
			}
		else
			{
			scanterms(n);
			}
		}
	}

static void readRDF(const char* filename)
	{
	xmlDoc *doc = xmlReadFile(filename,NULL,0);
	xmlNode *root_element = NULL;
	if(doc==NULL)
		{
		fprintf(stderr,"Cannot read %s\n",filename);
		exit(EXIT_FAILURE);
		}
	root_element = xmlDocGetRootElement(doc);
	

	if(root_element ==NULL)
		{
		fprintf(stderr,"No root in %s\n",filename);
		exit(EXIT_FAILURE);
		}
	scanterms(root_element);
	xmlFreeDoc(doc);
	}



static int cmp_term(const void * o1, const void * o2)
	{
	TermPtr a=(TermPtr)o1;
	TermPtr b=(TermPtr)o2;
	int i=a->child_id - b->child_id;
	if(i!=0) return i;
	return a->parent_id - b->parent_id;
	}

int main(int argc, char** argv)
	{
	FILE* out;
	int i=0;
	if(argc<3)
		{
		fprintf(stderr,"usage: %s output.db go1.rdf go2.rdf ....\n",argv[0]);
		exit(EXIT_FAILURE);
		}
	LIBXML_TEST_VERSION	
	for(i=2;i< argc;++i)
		{
		readRDF(argv[2]);
		}
	qsort(termdb.terms,termdb.n_terms,sizeof(Term),cmp_term);
	
	out= fopen(argv[1],"w");
	if(out==NULL)
		{
		fprintf(stderr,"Cannot open %s\n",argv[1]);
		return -1;
		}
	fwrite(&(termdb.n_terms),sizeof(int),1,out);
	fwrite(termdb.terms,sizeof(Term),termdb.n_terms,out);
	fflush(out);
	fclose(out);
	free(termdb.terms);
	printf("#ifndef GO_DATABASE_H\n#define GO_DATABASE_H\n");
	printf("#include \"term.h\"\n");
	printf("#define TERMDB_SIZE %d\n",termdb.n_terms);
	printf("#define TERMDB_FILENAME \"%s\"\n",argv[1]);
	printf("#endif\n");
	return 0;
	}
