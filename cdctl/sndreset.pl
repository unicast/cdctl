#!/usr/bin/perl
# sndreset.pl -- perl version of sndreset utility
require "ioctl.ph";

open(DSP, "/dev/dsp");
ioctl(DSP, &SOUND_MIXER_READ_CD, $vol);
print("$vol\n");
