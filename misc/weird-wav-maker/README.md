# weird-wav-maker

This is a tool to create WAVE files with weird bitdepths.

It supports reading from WAVE files with non-floating-point audio,
and will downscale to any bitdepth between 4 and 32 (any bitdepth
supported by FLAC).

I tried looking into dithering and honestly have no idea if and
how that should be applied. Since this tool is mostly just for
creating WAVE files for testing, that probably doesn't really
matter anyway. The point is to test that sources with strange
bit depths are handled correctly, not to produce good-sounding
4-bit audio files.

The code is terrible and pretty slapped together but it works.

## Usage

```
./weird-wav-maker -(bitdepth) source.wav dest.wav
```

Example, to convert a 16-bit WAVE to a 15-bit WAVE:

```
./weird-wav-maker -15 source.wav dest.wav
```

The default output is a 5-bit WAVE.

The source file needs to be a higher bitdepth than your destination.
For example, you can't use this tool to convert a 16-bit WAVE to a 17-bit WAVE.

I recommend creating a source 32-bit WAVE file since that can
be converted to any supported bitdepth with this tool.
