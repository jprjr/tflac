.PHONY: all clean

all: tflac.wasm

tflac.wasm: tflac.o
	wasm-ld --no-entry --export-all -o $@ $^

tflac.o: tflac.ll
	llc -march=wasm32 -filetype obj -o $@ $^

tflac.ll: tflac.c ../../tflac.h
	clang -DNDEBUG --target=wasm32 -mbulk-memory -emit-llvm -flto -Os -c -S -o $@ $<

clean:
	rm -f tflac.wasm tflac.o tflac.ll
