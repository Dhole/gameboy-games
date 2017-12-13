BIN=../../bin
OBJ=./obj



build:
	mkdir -p $(OBJ)
	$(BIN)/gbdk-n-compile.sh gol.c -o $(OBJ)/gol.rel
	$(BIN)/gbdk-n-link.sh $(OBJ)/gol.rel -o $(OBJ)/gol.ihx
	$(BIN)/gbdk-n-make-rom.sh $(OBJ)/gol.ihx gol.gb

clean:
	rm -rf $(OBJ)
	rm -f gol.gb
