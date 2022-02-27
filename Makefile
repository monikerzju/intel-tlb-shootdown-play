CC     := gcc
CFLAGS := -O1
CLIBS  := -lm -lpthread
DEST_D := build/
SRC_D  := src/
INC_D  := include/
SRC     := $(wildcard $(SRC_D)*.c)
OBJ     := $(patsubst $(SRC_D)%.c,$(DEST_D)%.o,$(SRC))

default: run

$(DEST_D)%.o: $(SRC_D)%.c
	mkdir -p $(DEST_D)
	$(CC) $(CFLAGS) -I$(INC_D) -c $< -o $@ $(CLIBS)

all: $(SRC_D) $(INC_D) $(OBJ)
	$(CC) $(CFLAGS) -I$(INC_D) $(OBJ) -o $(DEST_D)testelf $(CLIBS)

run: all
	./run.sh

clean:
	rm -rf $(DEST_D)