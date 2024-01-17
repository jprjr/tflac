# Even Simpler Encoder

Reads in raw audio samples, assuming signed, 16-bit little endian, 2 channels, interleaved.

You can produce raw audio samples with ffmpeg, then use this encoder to produce a basic
FLAC file:

```bash
ffmpeg -i source.flac -ar 44100 -ac 2 -f s16le pipe:1 | ./encoder - destination.flac
```


