BIN=../../bin
OBJ=./obj



build:
	mkdir -p $(OBJ)
	$(BIN)/gbdk-n-compile.sh snake.c -o $(OBJ)/snake.rel
	$(BIN)/gbdk-n-link.sh $(OBJ)/snake.rel -o $(OBJ)/snake.ihx
	$(BIN)/gbdk-n-make-rom.sh $(OBJ)/snake.ihx snake.gb

clean:
	rm -rf $(OBJ)
	rm -f snake.gb
