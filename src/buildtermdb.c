#include <libxml/parser.h>
#include <libxml/tree.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "term.h"

static const char GO_NS[]="http://www.geneontology.org/dtds/go.dtd#";
static const char RDF_NS[]="http://www.w3.org/1999/02/22-rdf-syntax-ns#";

static TermDB termdb={NULL,0};


static void scanterm(xmlNode *term)
	{
	xmlAttrPtr rsrc;
	xmlNode *n=NULL;
	xmlChar * acn=NULL;
	for(n=term->children; n!=NULL; n=n->next)
		{
		if (n->type != XML_ELEMENT_NODE) continue;
		if(strcmp(n->name,"accession")==0 &&
			n->ns!=NULL && n->ns->href!=NULL &&
			strcmp(n->ns->href,GO_NS)==0)
			{
			acn= xmlNodeGetContent(n);
			break;
			}
		}
	if(acn==NULL) return;
	
	for(n=term->children; n!=NULL; n=n->next)
		{
		if (n->type != XML_ELEMENT_NODE) continue;
		if(strcmp(n->name,"is_a")==0 &&
			n->ns!=NULL && n->ns->href!=NULL &&
			strcmp(n->ns->href,GO_NS)==0
			)
			{
			xmlChar* is_a=xmlGetNsProp(n,"resource",RDF_NS);
			if(is_a!=NULL)
				{
				char* p=strstr(is_a,"#GO:");
				if(p!=NULL)
					{
					++p;
					termdb.terms=(TermPtr)realloc(termdb.terms,sizeof(Term)*(termdb.n_terms+1));
					if(termdb.terms==NULL)
						{
						fprintf(stderr,"out of memory\n");
						exit(EXIT_FAILURE);
						}
					strncpy(termdb.terms[termdb.n_terms].parent,p,MAX_TERM_LENGTH);
					strncpy(termdb.terms[termdb.n_terms].child,acn,MAX_TERM_LENGTH);
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
	xmlElement* e;
	for(n=root_element->children; n!=NULL; n=n->next)
		{
		if (n->type != XML_ELEMENT_NODE) continue;
		if(strcmp(n->name,"term")==0 &&
			n->ns!=NULL && n->ns->href!=NULL && strcmp(n->ns->href,GO_NS)==0)
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
	xmlNode *n=NULL;
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
	int i=strcmp(a->child,b->child);
	if(i!=0) return i;
	return strcmp(a->parent,b->parent);
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
	return 0;
	}
