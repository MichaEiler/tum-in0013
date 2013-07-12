#ifndef __DCT_H
#define __DCT_H

#include <cmath>
#include <math.h>
#include <iostream>
using namespace std;

#define W1  2841
#define W2  2676
#define W3  2408
#define W5  1609
#define W6  1108
#define W7  565
#define CLIP(x) ((x < 0) ? 0 : ((x > 0xFF) ? 0xFF : x));

class DCT
{
public:
        // 2d dct for 8x8 matrix in form of an array with 64 values
        // DCT is a lot slower than the FDCT, this method has just been used to validating the FDCT algorithm
        /*static inline void transform(float* values)
        {
                float* result = new float[64];
                int x,y;
                for(int i = 0; i < 64; i++) {
                        x = i % 8;
                        y = i / 8;

                        float sum = 0;
                        float cm = 1.0 / sqrt(2);
                        float cn = cm;
                        for (int m = 0; m < 8; m++) {
                                if (m != 0)
                                        cm = 1;
                                for (int n = 0; n < 8; n++)
                                {
                                        if (n != 0)
                                                cn = 1;
                                        sum += values[n*8+m]*cm*cn*cos((2.0*x+1.0)*m*M_PI/16.0)*cos((2.0*y+1.0)*n*M_PI/16.0);
                                }
                                cn = 1 / sqrt(2);
                        }
                        result[i] = sum / 4.0;
                }

                for(int i = 0; i < 64; i++)
                {
                        values[i] = round(result[i]) + 128;
                }

                delete result;
        }*/

        // FDCT based on ujpeg.c
        static inline void rowTransform(int* values)
        {
                int x1 = ((int)values[4]) << 11;
                int x2 = (int)values[6];
                int x3 = (int)values[2];
                int x4 = (int)values[1];
                int x5 = (int)values[7];
                int x6 = (int)values[5];
                int x7 = (int)values[3];

                if (!(x1 | x2 | x3 | x4 | x5 | x6 | x7)) {
                        values[0] = values[1] = values[2] = values[3]
                                = values[4] = values[5] = values[6]
                                = values[7] = ((int)values[0]) << 3;
                        return;
                }

                int x0 = (((int)values[0]) << 11) + 128;
                int x8 = W7 * (x4 + x5);
       
                x4 = x8 + (W1 - W7) * x4;
                x5 = x8 - (W1 + W7) * x5;
                x8 = W3 * (x6 + x7);
                x6 = x8 - (W3 - W5) * x6;
                x7 = x8 - (W3 + W5) * x7;
                x8 = x0 + x1;
                x0 -= x1;
                x1 = W6 * (x3 + x2);
                x2 = x1 - (W2 + W6) * x2;
                x3 = x1 + (W2 - W6) * x3;
                x1 = x4 + x6;
                x4 -= x6;
                x6 = x5 + x7;
                x5 -= x7;
                x7 = x8 + x3;
                x8 -= x3;
                x3 = x0 + x2;
                x0 -= x2;
                x2 = (181 * (x4 + x5) + 128) >> 8;
                x4 = (181 * (x4 - x5) + 128) >> 8;

                values[0] = (x7 + x1) >> 8;
                values[1] = (x3 + x2) >> 8;
                values[2] = (x0 + x4) >> 8;
                values[3] = (x8 + x6) >> 8;
                values[4] = (x8 - x6) >> 8;
                values[5] = (x0 - x4) >> 8;
                values[6] = (x3 - x2) >> 8;
                values[7] = (x7 - x1) >> 8;
        }

        static inline void columnTransform(int* values, int* result, int column)
        {
                int x1 = ((int)values[8*4]) << 8;
                int x2 = (int)values[8*6];
                int x3 = (int)values[8*2];
                int x4 = (int)values[8*1];
                int x5 = (int)values[8*7];
                int x6 = (int)values[8*5];
                int x7 = (int)values[8*3];

                if (!(x1 | x2 | x3 | x4 | x5 | x6 | x7)) {
                        x1 = CLIP((( ((int)values[0]) + 32) >> 6) + 128);
                        result[0 + column] = result[8 + column] = result[16 + column]
                                = result[24 + column] = result[32 + column] = result[40 + column]
                                = result[48 + column] = result[56 + column] = x1;
                        return;
                }

                int x0 = (((int)values[0]) << 8) + 8192;
                int x8 = W7 * (x4 + x5) + 4;
       
                x4 = (x8 + (W1 - W7) * x4) >> 3;
                x5 = (x8 - (W1 - W7) * x5) >> 3;
                x8 = W3 * (x6 + x7) + 4;
                x6 = (x8 - (W3 - W5) * x6) >> 3;
                x7 = (x8 - (W3 + W5) * x7) >> 3;
                x8 = x0 + x1;
                x0 -= x1;
                x1 = W6 * (x3 +x2) + 4;
                x2 = (x1 - (W2 + W6) * x2) >> 3;
                x3 = (x1 + (W2 - W6) * x3) >> 3;
                x1 = x4 + x6;
                x4 -= x6;
                x6 = x5 + x7;
                x5 -= x7;
                x7 = x8 + x3;
                x8 -= x3;
                x3 = x0 + x2;
                x0 -= x2;
                x2 = (181 * (x4 + x5) + 128) >> 8;
                x4 = (181 * (x4 - x5) + 128) >> 8;

                result[0 + column] =  CLIP(((x7 + x1) >> 14) + 128);
                result[8 + column] =  CLIP(((x3 + x2) >> 14) + 128);
                result[16 + column] = CLIP(((x0 + x4) >> 14) + 128);
                result[24 + column] = CLIP(((x8 + x6) >> 14) + 128);
                result[32 + column] = CLIP(((x8 - x6) >> 14) + 128);
                result[40 + column] = CLIP(((x0 - x4) >> 14) + 128);
                result[48 + column] = CLIP(((x3 - x2) >> 14) + 128);
                result[56 + column] = CLIP(((x7 - x1) >> 14) + 128);
        }

        // same operation, but uses the FDCT algorithm
        static inline void fastTransform(int* values)
        {
                static int tmp[64];

                for (int row = 0; row < 64; row += 8) {
                        rowTransform(&values[row]);
                }

                for (int column = 0; column < 8; column++) {
                        columnTransform(&values[column], tmp, column);
                }

                for(int i = 0; i < 64; i++) {
                        values[i] = tmp[i];
                }
        }
};

#endif // __DCT_H
