#include "util.h"
#include "math.h"
#include "perlin.h"


void init_perlin(Permutation p) {
    for (int i = 0; i < 256; i++) {
        p[i] = i;
    }
    permute(p, 256);
    for (int i = 0; i < 256; i++) {
        p[i + 256] = p[i];
    }
}


float fade(float t) {
    return t * t * t * (t * (t * 6.0 - 15.0) + 10.0);
}


float grad(int hash, float x, float y, float z) {
    switch (hash & 0xF) {
        case 0x0: return x + y;
        case 0x1: return -x + y;
        case 0x2: return x - y;
        case 0x3: return -x - y;
        case 0x4: return x + z;
        case 0x5: return -x + z;
        case 0x6: return x - z;
        case 0x7: return -x - z;
        case 0x8: return y + z;
        case 0x9: return -y + z;
        case 0xA: return y - z;
        case 0xB: return -y - z;
        case 0xC: return y + x;
        case 0xD: return -y + z;
        case 0xE: return y - x;
        case 0xF: return -y - z;
        default: return 0;
    }
}


int inc(int num, int repeat) {
    num++;
    if (repeat > 0) {
        num %= repeat;
    }
    return num;
}


float perlin(float x, float y, float z, Permutation p, int repeat) {
    // https://adrianb.io/2014/08/09/perlinnoise.html

    if (repeat > 0) {
        x = mod(x, (float) repeat);
        y = mod(y, (float) repeat);
        z = mod(z, (float) repeat);
    }

    int xi = mod(x, 255);
    int yi = mod(y, 255);
    int zi = mod(z, 255);

    float xf = x - floorf(x);
    float yf = y - floorf(y);
    float zf = z - floorf(z);

    float u = fade(xf);
    float v = fade(yf);
    float w = fade(zf);

    int aaa = p[p[p[xi] + yi] + zi];
    int aba = p[p[p[xi] + inc(yi, repeat)] + zi];
    int aab = p[p[p[xi] + yi] + inc(zi, repeat)];
    int abb = p[p[p[xi] + inc(yi, repeat)] + inc(zi, repeat)];
    int baa = p[p[p[inc(xi, repeat)] + yi] + zi];
    int bba = p[p[p[inc(xi, repeat)] + inc(yi, repeat)] + zi];
    int bab = p[p[p[inc(xi, repeat)] + yi] + inc(zi, repeat)];
    int bbb = p[p[p[inc(xi, repeat)] + inc(yi, repeat)] + inc(zi, repeat)];

    float x1 = lerp(grad(aaa, xf, yf, zf),
                    grad(baa, xf - 1.0, yf, zf),
                    u);

    float x2 = lerp(grad(aba, xf, yf - 1.0, zf),
                    grad(bba, xf - 1.0, yf - 1.0, zf),
                    u);

    float y1 = lerp(x1, x2, v);

    x1 = lerp(grad(aab, xf, yf, zf - 1.0),
              grad(bab, xf - 1.0, yf, zf - 1.0),
                   u);

    x2 = lerp(grad(abb, xf, yf - 1.0, zf - 1.0),
              grad(bbb, xf - 1.0, yf - 1.0, zf - 1.0),
              u);

    float y2 = lerp(x1, x2, v);

    return 0.5 * (lerp(y1, y2, w) + 1.0);
}


float octave_perlin(float x, float y, float z, Permutation p, int repeat, int octaves, float persistence) {
    float total = 0.0;
    float frequency = 1.0;
    float amplitude = 1.0;
    float max_value = 0.0;

    for (int i = 0; i < octaves; i++) {
        total += perlin(x * frequency, y * frequency, z * frequency, p, repeat) * amplitude;

        max_value += amplitude;

        amplitude *= persistence;
        frequency *= 2;
    }

    return total / max_value;
}
