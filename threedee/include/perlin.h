#pragma once


typedef int Permutation[512];

void init_perlin(Permutation p);

float perlin(float x, float y, float z, Permutation p, int repeat);

float octave_perlin(float x, float y, float z, Permutation p, int repeat, int octaves, float persistence);
