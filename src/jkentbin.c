/**
 * inspired from Kent, et. al. Genome Research 2002.12:996-1006
 * http://genome.cshlp.org/content/12/6/996.full
 * http://genomewiki.ucsc.edu/index.php/Bin_indexing_system 
 * Bin
 */

typedef struct KentBin_t
	{
	int childrenCount;
	int maxLevel;
	int maxGenomicLengthLevel;
	}KentBin,*KentBinPtr;


	
static int calcBin(
	const KentBinPtr params,
	int chromStart,
	int chromEnd,
	int binId,
	int level,
	int binRowStart,
	int rowIndex,
	int binRowCount,
	int genomicPos,
	int genomicLength
	)
	{

	if( 
		chromStart>=genomicPos &&
		chromEnd<= (genomicPos+genomicLength))
		{
		if(level>=params->maxLevel) return binId;
		
		int childLength=genomicLength/params->childrenCount;
		int childBinRowCount=binRowCount*params->childrenCount;
		int childRowBinStart=binRowStart+binRowCount;
		int firstChildIndex=rowIndex*params->childrenCount;
		int firstChildBin=childRowBinStart+firstChildIndex;
		int i;
		for(i=0;i< params->childrenCount;++i)
			{
			//System.out.println("i:"+i+"=>"+(firstChildBin+i));
			//System.out.println(" "+(genomicPos+i*childLength)+" <=" +chromStart+" < "+chromEnd+"<"+(genomicPos+(i+1)*childLength));
			int n= calcBin(
					params,
					chromStart,
					chromEnd,
					firstChildBin+i,
					level+1,
					childRowBinStart,
					firstChildIndex+i,
					childBinRowCount,
					genomicPos+i*childLength,
					childLength
					);
			if(n!=-1)
				{
				return n;
				}
			}
		return binId;
		}
	
	return -1;
	}
	
	
/** Given start,end in chromosome coordinates assign it
	* a bin.   There's a bin for each 128k segment, for each
	* 1M segment, for each 8M segment, for each 64M segment,
	* and for each chromosome (which is assumed to be less than
	* 512M.)  A range goes into the smallest bin it will fit in. */
static int ucscBin(const KentBinPtr params,int chromStart,int chromEnd)
	{
	int genomicLength=params->maxGenomicLengthLevel;
	if(chromEnd<chromStart) return -1;
	return calcBin(params,chromStart,chromEnd,0,0,0,0,1,0,genomicLength);
	}
	


static KentBin Ucsc512=
	{
	8,
	4,
	536870912 //2^29
	};


static int ucsc512Bin(int chromStart,int chromEnd)
	{
	return ucscBin(&Ucsc512,chromStart,chromEnd);
	}

my_bool jKentBin_init(UDF_INIT *initid, UDF_ARGS *args, char *message);
	{
	if (args->arg_count != 2)
		{
		strcpy(message,"jKentBin(start,end) requires two arguments");
		return 1;
		}
	if(args->arg_type[0] != INT_RESULT ||
	   args->arg_type[1] != INT_RESULT)
		{
		strcpy(message,"jKentBin(start,end) requires two integers");
		return 1;
		}
	initid->maybe_null=1;
	return 0;
	}

void jKentBin_deinit(UDF_INIT *initid)
	{
	
	}

long long jKentBin(UDF_INIT *initid, UDF_ARGS *args,
              char *is_null, char *error)
        {
        long long start_val;
        long long end_val;
        if(args->args[0]==NULL || args->args[1]==NULL)
        	{
        	*is_null=1;
        	return -1;
        	}
        
        start_val = *((long long*) args->args[0]);
	end_val = *((long long*) args->args[1]);
	
	return ucsc512Bin((int)start_val,(int)end_val);
        }



