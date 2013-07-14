#ifndef __HUFFMANTREE_H
#define __HUFFMANTREE_H

#include <vector>
#include <queue>
#include <cmath>
#include "bitstream.h"

#define ERROR_NOSYMBOLFOUND     0x50    // thrown if the next bits in the bitstream
                                        // point to a non existing entry in the huffmantree
#define ERROR_ROWOVERFLOW       0x51    // to many values for the current row
#define ERROR_ENDOFSOURCESTREAM 0x52    // end of source stream

struct HuffmanNode
{
        HuffmanNode* left;              // I don't use shared_ptr here as this results in an additional
                                        // 300ms (from 500ms to 8xx ms for decoding a big picture) just
                                        // because we have the overhead of the shared_ptr class and 
                                        // therefor multiple lookups to get address for the child-node.
        HuffmanNode* right;
        unsigned char value;
        bool hasValue;
};

class HuffmanTree
{
private:
        HuffmanNode* root;
        std::vector<HuffmanNode*> previousRow;  // contains only the "non-leaf"-elements
        void deleteNode(HuffmanNode* node);
        unsigned char getValue(BitStream& stream, HuffmanNode* node, int& result);

        int currentRow; 

public:
        explicit HuffmanTree();
        virtual ~HuffmanTree();
        int insertNextRow(char* values, unsigned int n);
        unsigned char getValue(BitStream& stream, int& result);
};

#endif // __HUFFMANTREE_H
