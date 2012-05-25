.PHONY: clean

all: sample

sample: main.o unpack.o offsets.o
	${CC} $^ -lz -o sample

pack: pack.o
	${CC} $^ -lz -o $@

offsets.c: pack data1.txt data2.txt data3.txt
	./pack DATA1:data1.txt DATA2:data2.txt DATA3:data3.txt > offsets.c || rm offsets.c

%.gz: %.txt
	gzip -cn $< > $@

%.o: %.c
	${CC} -Wall -std=c99 -g -c $< -o $@

clean:
	rm -rf *.o sample pack offsets.c
