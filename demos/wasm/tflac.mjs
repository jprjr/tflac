/* stores a global Module and Instance of tflac,
 * we use this first instance to calculate the needed
 * memory pages when creating a new instance.
 *
 * An alternative would be to just load the wasm module,
 * then grow the memory of the loaded instance. */
let mod = null;

function calc_needed_pages(block_size,channels,bitdepth) {

    /* WASM uses flat memory - just one big buffer of data. So
     * we need to calculate the total amount of memory, then request
     * the needed pages. */

    const struct_size = mod.instance.exports.tflac_size();
    const memory_size = mod.instance.exports.tflac_size_memory(block_size);
    const frame_size = mod.instance.exports.tflac_size_frame(block_size, channels, bitdepth);
    const samples_size = Int32Array.BYTES_PER_ELEMENT * block_size * channels;
    const total_size =
      struct_size + // storage for the tflac struct
      memory_size + // storage for tflac internal memory (residuals)
      frame_size +  // storage for the output frame
      Int32Array.BYTES_PER_ELEMENT + // to ensure we align Int32Array on 4-byte boundary
      samples_size + // storage for our incoming audio samples
      (1 * Uint32Array.BYTES_PER_ELEMENT) + // single-element Uint32Array for used ptr
      mod.instance.exports.__heap_base.value; // base offset of where heap memory starts:w

    const pages = Math.ceil(total_size / 65536); // convert to pages, each page is 64k
    return pages;
}

class TFlac {

    /* static method to load the global module from a buffer, this needs to be called
     * before creating any instances of the class. */
    static async load(buf) {
        mod = await WebAssembly.instantiate(buf);
    }

    constructor(block_size, channels, bitdepth, samplerate) {
        if(mod === null) throw new Error("WASM module not loaded");

        this.chunks = []; // we'll store things like the fLaC stream marker, STREAMINFO block and audio frames
                          // as an array of Uint8Array

        /* Note: In a "real" version of this I wouldn't store chunks, but instead
         * emit them as needed, or return them from calls to encode, etc.
         * The main point of this demo was to figure out how memory management works with WASM
         * so I'm just going to collect them all and let the caller get them at the end. */

        /* calculate how much memory we'll need, request it, create a new instance */
        const needed_pages = calc_needed_pages(block_size, channels, bitdepth);
        const memory = new WebAssembly.Memory({ initial: needed_pages });
        this.wasm = new WebAssembly.Instance(mod.module, { env: { memory: this.memory }});

        /* grab a bunch of needed functions and values */
        const {
            __heap_base,
            tflac_size,
            tflac_size_memory,
            tflac_size_frame,
            tflac_init,
            tflac_validate,
            tflac_set_blocksize,
            tflac_set_channels,
            tflac_set_bitdepth,
            tflac_set_samplerate,
            tflac_encode_streaminfo,
        } = this.wasm.exports;

        /* Start sectioning off our memory buffer.
         * For each thing we allocate (tflac struct, memory, buffers, etc)
         * we'll also create a "pointer" - which is just a byte offset we provide
         * to our WASM calls. */
        let offset = __heap_base.value;
        const buffer = this.wasm.exports.memory.buffer;

        /* our tflac struct and pointer */
        this.tflac = new Uint8Array(buffer, offset, tflac_size());
        this.tflac_ptr = this.tflac.byteOffset;

        offset += this.tflac.byteLength;

        /* our tflac's residual memory and pointer */
        this.tflac_memory = new Uint8Array(buffer, offset, tflac_size_memory(block_size));
        this.tflac_memory_ptr = this.tflac_memory.byteOffset;

        offset += this.tflac_memory.byteLength;

        /* our output buffer and pointer */
        this.tflac_frame = new Uint8Array(buffer, offset, tflac_size_frame(block_size,channels,bitdepth));
        this.tflac_frame_ptr = this.tflac_frame.byteOffset;

        offset += this.tflac_frame.byteLength;

        /* make sure offset is aligned on an element boundary */
        if(offset % Int32Array.BYTES_PER_ELEMENT != 0) {
            offset += Int32Array.BYTES_PER_ELEMENT - (offset % Int32Array.BYTES_PER_ELEMENT);
        }

        /* our incoming samples and pointer */
        this.samples = new Int32Array(buffer, offset, block_size * channels);
        this.samples_ptr = this.samples.byteOffset;

        offset += this.samples.byteLength;

        /* a single Uint32Array and pointer for getting out bytes-used outvar */
        this.used = new Uint32Array(buffer, offset, 1);
        this.used_ptr = this.used.byteOffset;

        /* Let's get it all set up! */
        tflac_init(this.tflac_ptr);

        tflac_set_blocksize(this.tflac_ptr,block_size);
        tflac_set_channels(this.tflac_ptr,channels);
        tflac_set_bitdepth(this.tflac_ptr,bitdepth);
        tflac_set_samplerate(this.tflac_ptr,samplerate);

        /* tflac_validate returns non-zero on error so just throw an error if that happens. */
        if(tflac_validate(this.tflac_ptr,this.tflac_memory_ptr,this.tflac_memory.byteLength)) {
            throw new Error("Error validating tflac");
        }

        /* we'll add our "fLaC" stream marker to our first chunk */
        this.chunks.push(new Uint8Array([0x66, 0x4c, 0x61, 0x43]));

        /* we'll also add a placeholder STREAMINFO block */
        tflac_encode_streaminfo(this.tflac_ptr, 1, this.tflac_frame_ptr, this.tflac_frame.byteLength, this.used_ptr);
        this.chunks.push(this.tflac_frame.slice(0,this.used[0]));
    }

    /* A more prodution-y version of this would have a nicer method for
     * accepting audio samples - like allow the caller to provide any
     * number of samples, and split them up into blocks for the
     * caller.
     *
     * For this demo we just require the caller to always provide a whole block of
     * samples, every block should be the same size except the final block. */

    encode_s32i(block_size, samples) {
        this.samples.set(samples); // copy caller-provided samples to our internal buffer

        // tflac_encode_s32i returns non-zero on error so just throw an error if that happens.
        if(this.wasm.exports.tflac_encode_s32i(
          this.tflac_ptr,
          block_size,
          this.samples_ptr,
          this.tflac_frame_ptr,
          this.tflac_frame.byteLength,
          this.used_ptr)) {
            throw new Error("Error encoding frame");
        }
        this.chunks.push(this.tflac_frame.slice(0,this.used[0]));
    }

    /* tflac_finalize that also updates the STREAMINFO block in memory */
    finalize() {
        this.wasm.exports.tflac_finalize(this.tflac_ptr);
        this.wasm.exports.tflac_encode_streaminfo(this.tflac_ptr, 1, this.tflac_frame_ptr, this.tflac_frame.byteLength, this.used_ptr);
        this.chunks[1] = this.tflac_frame.slice(0,this.used[0]);
    }

}

export { TFlac };
