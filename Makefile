FILES=main.cpp

BIN_FILES=main.o

all: clean bin build

clean : 
	rm -rf ./proxy $(BIN_FILES) log.txt

bin :
	g++ -c -I./boost_1_81_0 $(FILES)

build :
	g++ $(BIN_FILES) -L ./boost_1_81_0/stage/lib/ -o proxy

run:
	./proxy