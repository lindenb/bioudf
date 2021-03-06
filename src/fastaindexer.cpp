/**
* Author:
*  Pierre Lindenbaum PhD
*  plindenbaum@yahoo.fr
* Simple FASTA file indexer
*
*/
#include<cstdio>
#include<cstdlib>
#include<cerrno>
#include<cstring>
#include<cctype>
#include<vector>
#include<string>
#include<algorithm>
#include<iostream>

class Index
	{
	public:
		std::string name;
		int file_index;
		int indexLineSize;
		long offsetNameStart;
		long offsetNameEnd;
		long offsetSeqStart;
		long offsetSeqEnd;
		int seqLength;
		
		Index():file_index(0),
			indexLineSize(0),
			offsetNameStart(-1L),
			offsetNameEnd(-1L),
			offsetSeqStart(-1L),
			offsetSeqEnd(-1L),
			seqLength(0)
			{
			}
	static bool lt (const  Index* a,const Index* b) { return (a->name<b->name); }

	};



int main(int argc,char **argv)
	{
	int optind=1;
	std::vector<Index*> indexes;
	std::string::size_type max_name_length(0);

	for(int i=optind; i<argc;++i)
		{
		if(argv[i][0]!='/')
			{
			std::cerr << "expected a full path name but got "<< argv[i] << std::endl;
			return EXIT_FAILURE;
			}
		}


	std::cout << "#ifndef FAIDX_H\n#define FAIDX_H\n";
	std::cout << "#define FASTA_FILE_COUNT \t"
		<< (argc-optind)
		<< std::endl;
	std::cout << "static const char* fasta_filenames[FASTA_FILE_COUNT]={" << std::endl;
	
	
	for(int i=optind; i<argc;++i)
		{
		std::cout << "\t\"" << argv[i]<< "\" " <<(i+1==argc?"":",") <<" /* "<< (i-optind) << "th" << " */" << std::endl; 
		}
	std::cout << "};\n" << std::endl;
	
	for(int i=optind; i<argc;++i)
		{
		errno=0;
		FILE* in;
		long offset=-1L;
		int curr_line_size=0;
		Index* current=NULL;
		
		if(argv[i][0]!='/')
			{
			std:: cerr << "file " << argv[i]
				<< " should be a full unix path starting with '/'"
				<< std::endl;
			std::exit(EXIT_FAILURE);
			}
		
		in=std::fopen(argv[i],"r");
		if(in==NULL)
			{
			std:: cerr << "cannot open " << argv[i]
				<< " " << std::strerror(errno)
				<< std::endl;
			exit(EXIT_FAILURE);
			}
		
		for(;;)
			{
			int c=std::fgetc(in);
			if(c!=EOF) offset++;
			//end of line or end of file
			if(current!=NULL && (c==EOF || (c=='\n')))
				{
				current->indexLineSize= (
					current->indexLineSize > curr_line_size?
					current->indexLineSize:
					curr_line_size
					);
				curr_line_size=0;
				}
			
			if(c==-1 || c=='>')
				{
				if(current!=NULL)
					{
					indexes.push_back(current);
					current=NULL;
					}
				if(c==-1) break;
				current=new Index;
				current->file_index=(i-optind);
				current->offsetNameStart= offset;
				for(;;)
					{
					c=std::fgetc(in);
					if(c==EOF)
						{
						std::cerr << "unfinished name at offset "<< offset << std::endl;
						exit(EXIT_FAILURE);
						}
					offset++;
					if(c=='\n') break;
					current->name+=((char)c);
					}
				max_name_length=std::max(max_name_length,current->name.size());
				current->offsetNameEnd=offset;
				current->offsetSeqStart=offset+1;
				current->offsetSeqEnd=offset+1;
				current->seqLength=0;
				curr_line_size=0;
				}
			else if(current!=NULL)
				{
				if(c=='\n')
					{
					//already processed ignore
					}
				else if(std::isalpha(c))
					{
					current->offsetSeqEnd= offset+1L;
					current->seqLength++;
					curr_line_size++;
					}
				else
					{
					std::cerr << "Illegal character "
						<< (char)c<< " at "
						<< offset << std::endl;
					std::exit(EXIT_FAILURE);
					}
				}
			}
		std::fclose(in);
		}
	std::sort(indexes.begin(),indexes.end(),Index::lt);
	std::cout << "#define FASTA_SEQUENCE_MAX_NAME_LENGTH\t"
		<< (max_name_length+1)
		<< std::endl;
	std::cout << "typedef struct FastaIndex_t\n"
		<< "\t{\n"
		<< "\tchar name[FASTA_SEQUENCE_MAX_NAME_LENGTH];\n"
		<< "\tint fileIndex;\n"
		<< "\tunsigned long seqLength;\n"
		<< "\tunsigned long offsetSeqStart;\n"
		<< "\tunsigned long offsetSeqEnd;\n"
		<< "\tint lineSize;\n"
		<< "\t}FastaIndex,*FastaIndexPtr;"
		<< std::endl;
	std::cout << "#define FASTA_SEQUENCE_COUNT\t"
		<< (indexes.size())
		<< std::endl;
	std::cout << "static const FastaIndex fastaIndexes[FASTA_SEQUENCE_COUNT]={\n";
	for(std::vector<Index*>::size_type i=0;i< indexes.size();++i)
		{
		Index* curr= indexes.at(i);
		if(i>0) std::cout << ",\n"; 
		std::cout << "\t{";
		std::cout << "\"" << curr->name << "\","
			<< curr->file_index << ","
			<< curr->seqLength << ","
			<< curr->offsetSeqStart<< ","
			<< curr->offsetSeqEnd << ","
			<< curr->indexLineSize
			;
		std::cout << "}";
		delete curr;
		}
	std::cout << "\n};" << std::endl;
	std::cout << "#endif" << std::endl;
	return EXIT_SUCCESS;
	}

