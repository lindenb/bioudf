#ifndef taxonstruct_h
#define taxonstruct_h

#define MAX_TAXON_NAME 120
typedef int taxon_id_t;

typedef struct
	{
	taxon_id_t id;
	taxon_id_t parent_id;
	char name[MAX_TAXON_NAME];
	}Taxon,*TaxonPtr;

static const taxon_id_t NO_TAXON=-1;
static const taxon_id_t TAXON_ROOT=1;

#endif
