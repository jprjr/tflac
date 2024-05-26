# Simple Encoder with stereo decorrelation, multithreaded

This creates
4 instances of a tflac encoder, each with a different stereo decorrelation
mode set. It runs audio through all 4 encoders simultaneously and picks
the smallest audio frame produced by each one.

This is using Linux-specific syscalls (FUTEX) to handle thread sync,
so this demo is Linux-only at the moment.

Like the other raw-file encodes, this assumes you're reading a file of
signed, 16-bit, little-endian, 2-channel, interleaved audio.

You can produce raw audio samples with ffmpeg, then use this encoder to produce a basic
FLAC file:

```bash
ffmpeg -i source.flac -ar 44100 -ac 2 -f s16le pipe:1 | ./encoder-raw-mt - destination.flac
```

