#ifndef __BITSTREAM_H
#define __BITSTREAM_H

#include <vector>
#include <stack>

#define BITSTREAM_EOS -1

class BitStream
{
private:
        unsigned int position;
        unsigned int length;
        char* raw;
        std::stack<unsigned int> storedPositions;
public:
        BitStream(char* raw, unsigned int length);
        ~BitStream();
        int next();
        int nextNoSkip();
        int next(unsigned char n, int& result);
        unsigned char nextByte(bool skip);
        bool isEnd() { return length-1 <= position; }
        void moveBack(int bits) { position -= bits; }
        void remember() { storedPositions.push(position); }
        void forget() { storedPositions.pop(); }
        void rewind() { position = storedPositions.top(); forget(); }
        void skipRest() { position += (8-(position%8))%8; }
        bool available(unsigned int size) { return (position+size) <= length-1; }
};

#endif // __BITSTREAM_H
