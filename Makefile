DBOUT='"/tmp/terms.bin"'
CFLAGS=-Wall `mysql_config --cflags`
all:jkentbin
libjkentbin:lib src/jkentbin.c
	gcc $(CFLAGS) src/jkentbin.c
	
lib:
	mkdir -p lib

data/go_daily-termdb.rdf-xml:
	mkdir -p data
	wget -O data/go_daily-termdb.rdf-xml.gz "http://archive.geneontology.org/latest-termdb/go_daily-termdb.rdf-xml.gz"
	gunzip data/go_daily-termdb.rdf-xml.gz

database:bin/buildtermdb data/go_daily-termdb.rdf-xml
	bin/buildtermdb /tmp/terms.bin data/go_daily-termdb.rdf-xml

bin/buildtermdb: src/buildtermdb.c 
	mkdir -p bin
	gcc -o $@ src/buildtermdb.c  `xml2-config --cflags --libs`
lib/go_udf.so:src/go_udf.c
	mkdir -p lib
	gcc -shared  -DGO_PATH=$(DBOUT) -I /usr/include -I /usr/local/include -I /usr/include/mysql  -o $@ src/go_udf.c
test:src/go_udf.c
	gcc -DGO_PATH=$(DBOUT) -I /usr/include -I /usr/local/include -I /usr/include/mysql  -o bin/test src/go_udf.c
clean:
	rm -f data/go_daily-termdb.rdf-xml
