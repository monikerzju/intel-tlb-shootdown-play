CC     := gcc
CFLAGS := -O0
CLIBS  := -lm -lpthread
SRC    := mprotect.c
DEST_D := build/

default: run

all: mprotect.c
	mkdir -p $(DEST_D)
	$(CC) $(CFLAGS) -DWMATH $(SRC) -o $(DEST_D)math_basic $(CLIBS)
	$(CC) $(CFLAGS) -DWMATH -DSWITCH $(SRC) -o $(DEST_D)math_mprot $(CLIBS)
	$(CC) $(CFLAGS) $(SRC) -o $(DEST_D)basic $(CLIBS)
	$(CC) $(CFLAGS) -DSWITCH $(SRC) -o $(DEST_D)mprot $(CLIBS)
	$(CC) $(CFLAGS) -DWTHREAD -DWMATH $(SRC) -o $(DEST_D)thread_math_basic $(CLIBS)
	$(CC) $(CFLAGS) -DWTHREAD -DWMATH -DSWITCH $(SRC) -o $(DEST_D)thread_math_mprot $(CLIBS)
	$(CC) $(CFLAGS) -DWTHREAD $(SRC) -o $(DEST_D)thread_basic $(CLIBS)
	$(CC) $(CFLAGS) -DWTHREAD -DSWITCH $(SRC) -o $(DEST_D)thread_mprot $(CLIBS)

run: all
	./run.sh

clean:
	rm -rf $(DEST_D)