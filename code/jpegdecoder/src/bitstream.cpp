#include "bitstream.h"
#ifdef DEBUG
#include <iostream>
using namespace std;
#endif

BitStream::BitStream(char* raw, unsigned int length)
{
        position = 0;
        this->length = length * 8;
        this->raw = raw;
}

BitStream::~BitStream()
{

}

int BitStream::nextNoSkip()
{
        if (position == (length-1)) {
                return BITSTREAM_EOS;
        }
        unsigned char c = raw[position/8];
        c = c >> (7-(position%8));
        position++;
        return (c&0x01) == 0x01;
}

int BitStream::next()
{
        if (position == (length-1)) {
                return BITSTREAM_EOS;
        }
        unsigned char c = raw[position/8];

        // skip bytes used for bytestuffing
        if (c == 0x00) {
                unsigned char cprev = raw[(position-8)/8];
                if (cprev == 0xFF) {
                        position += 8;
                        c = raw[position/8];
                }       
        }

        c = c >> (7-(position%8));
        c = c & 0x01;
        position++;
        return c == 0x01;
}

int BitStream::next(unsigned char n, int& error)
{
        if (n <= 0 || n > 16) return 0;

        int result = 0;
        while (n>0) {
                if (isEnd()) {
                        error = BITSTREAM_EOS;
                        break;
                }

                result = result << 1;
                if (next())
                        result++;
                n--;
        }
        return result;
}

unsigned char BitStream::nextByte(bool skip)
{
        unsigned char result = 0;
        for (int i = 0; i < 8; i++) {
                if (position == (length-1)) {
                        return BITSTREAM_EOS;
                }

                result = result << 1;
                if((skip && next()) || (!skip && nextNoSkip()))
                        result++;
        }
        return result;
}

