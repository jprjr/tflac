# Simple Encoder with stereo decorrelation

This builds on ["Even Simpler Encoder"](../encoder-raw) - it creates
4 instances of a tflac encoder, each with a different stereo decorrelation
mode set. It runs audio through all 4 encoders sequentially and picks
the smallest audio frame produced by each one.

This does make the time needed to encode increase dramatically. See
["Simple Encoder with stereo decorrelation, multithreaded"](../encoder-raw-mt)
for a multithreaded version, where each encoder instance is in its own thread.

Like the other raw-file encodes, this assumes you're reading a file of
signed, 16-bit, little-endian, 2-channel, interleaved audio.

You can produce raw audio samples with ffmpeg, then use this encoder to produce a basic
FLAC file:

```bash
ffmpeg -i source.flac -ar 44100 -ac 2 -f s16le pipe:1 | ./encoder-raw-serial - destination.flac
```

