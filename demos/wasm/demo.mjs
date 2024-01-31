/* our FLAC file audio properties */
const BLOCK_SIZE = 1152;
const CHANNELS = 2;
const BITDEPTH = 16;
const SAMPLE_RATE = 44100;

/* We'll generate a sine wave */
const LENGTH = 10; // in seconds
const FREQ = 440;  // tone frequency
const AMP  = Math.ceil(0x7FFF * .5); // limit to about half of max volume
const STEP = FREQ * 2 * Math.PI/SAMPLE_RATE;

import { writeFile, readFile } from 'node:fs/promises';
import { TFlac } from './tflac.mjs';

/* load up the WASM module */
const wasmBuffer = await readFile('tflac.wasm');
await TFlac.load(wasmBuffer);

/* create our encoder */
const tflac = new TFlac(BLOCK_SIZE, CHANNELS, BITDEPTH, SAMPLE_RATE);

/* generate a sine wave, chunk it into blocks, and encode */
const samples = new Int32Array(BLOCK_SIZE * CHANNELS);
let o = 0;
let p = 0;
let i = 0;
while(i < SAMPLE_RATE * LENGTH) {
    o = i % BLOCK_SIZE;

    if(i > 0 && o === 0) {
        // we're starting a new block, encode previous block */
        tflac.encode_s32i(BLOCK_SIZE,samples);
    }

    for(let j = 0; j < CHANNELS; j++) {
        samples[ (o * CHANNELS) + j] = AMP * Math.sin(p);
    }

    p += STEP;
    i++;
}

/* check for final block */
o = i % BLOCK_SIZE;
if(o) {
    tflac.encode_s32i(o,samples);
}

/* finalize MD5 and update internal STREAMINFO block */
tflac.finalize();

/* we'll collect all the encoded chunks, turn it into a blob, write it out.
 * I'm sure there's better ways to do this, I went with Blob because
 * I think that's what I'd want to do in a browser - create a Blob and
 * download it. */
const b = new Blob(tflac.chunks, {
    type: 'audio/flac',
});
const buf = await b.arrayBuffer();

await writeFile('demo.flac',Buffer.from(buf));
console.log('successfully wrote demo.flac');
