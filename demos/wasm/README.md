# wasm demo

A small demo of using tflac as a WASM module.

It just creates a 10-second sine wave. Requires NodeJS,
though I think this could be adapted to run in a browser
pretty easily.

I couldn't find a great tutorial covering using memory
the way I wanted to use it - by pre-allocating a chunk
of memory on the JavaScript side and providing pointers
in to WASM.

The point of this demo is to demonstrate how to do exactly that.

Requires NodeJS, clang, lld (for wasm-ld), llvm (for llc).
