include make.properties
DBOUT='"/tmp/terms.bin"'
OPTCFLAGS=-O3 -Wall
CFLAGS=$(OPTCFLAGS) -fPIC -shared `mysql_config --cflags`

all:
	echo "make translate"
	echo "make taxonomy"
	echo "make jkentbin"
	echo "make go"
	echo "make faidx"
	echo "make revcomp"

revcomp:${mysql.libdir}/revcomp.so
${mysql.libdir}/revcomp.so: src/revcomp.c
	$(CC)  $(CFLAGS) -o$@ src/revcomp.c 


translate:${mysql.libdir}/translate.so
${mysql.libdir}/translate.so: src/translate.c
	$(CC)  $(CFLAGS) -o$@ src/translate.c 

taxonomy:${mysql.libdir}/taxonudf.so 
${mysql.libdir}/taxonudf.so:src/taxon.h src/taxonudf.c
	$(CC)  $(CFLAGS) -o$@ src/taxonudf.c 

src/taxon.h:bin/readtaxdump
	if [ ! -f taxdump.tar.gz ]; then wget -O taxdump.tar.gz "ftp://ftp.ncbi.nih.gov/pub/taxonomy/taxdump.tar.gz" ; fi
	tar xfz taxdump.tar.gz names.dmp
	tar xfz taxdump.tar.gz nodes.dmp
	mkdir -p bin
	bin/readtaxdump nodes.dmp names.dmp ${taxonomy.dir}/taxonomy.bin > $@
	rm names.dmp nodes.dmp taxdump.tar.gz taxdump.tar.gz
	
bin/readtaxdump:bin src/readtaxdump.c
	$(CC) $(OPTCFLAGS)  -o $@ src/readtaxdump.c
	
jkentbin:${mysql.libdir}/jkentbin.so
${mysql.libdir}/jkentbin.so:src/jkentbin.c
	$(CC) $(CFLAGS) -o $@  src/jkentbin.c

go:${mysql.libdir}/go.so
${mysql.libdir}/go.so:src/go_udf.c ${go.dir}/terms.bin
	mkdir -p lib
	$(CC) $(CFLAGS) -o $@ src/go_udf.c

${go.dir}/terms.bin:bin/buildtermdb ${go.dir}/go_daily-termdb.rdf-xml
	bin/buildtermdb  $@ ${go.dir}/go_daily-termdb.rdf-xml > src/go_database.h

bin/buildtermdb: src/buildtermdb.c  src/term.h 
	mkdir -p bin
	$(CC) $(OPTCFLAGS)  -o $@ src/buildtermdb.c  `xml2-config --cflags --libs`

${go.dir}/go_daily-termdb.rdf-xml:
	mkdir -p ${go.dir}
	wget -O $@.gz ${gene.ontology.url}
	gunzip $@.gz

faidx:${mysql.libdir}/faidx.so
${mysql.libdir}/faidx.so:src/fastaindexer.h src/getsequence.c
	$(CC) $(CFLAGS) -o $@  src/getsequence.c
src/fastaindexer.h:bin/fastaindexer
	bin/fastaindexer ${fasta.files} > $@
bin/fastaindexer:src/fastaindexer.cpp
	mkdir -p bin
	$(CPP) $(OPTCFLAGS) -o $@ src/fastaindexer.cpp

clean:
	rm -f bin/readtaxdump  taxdump.tar.gz src/taxon.h
	rm -f ${taxonomy.dir}/taxonomy.bin
	rm -f ${mysql.libdir}/translate.so
	rm -f data/go_daily-termdb.rdf-xml
