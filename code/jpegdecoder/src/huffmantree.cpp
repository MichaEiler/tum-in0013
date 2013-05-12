#include "huffmantree.h"
#if DEBUG
#include <iostream>
#endif
using namespace std;

HuffmanTree::HuffmanTree()
{
        root = new HuffmanNode();
        root->hasValue = false;
        root->left = nullptr;
        root->right = nullptr;
        previousRow.push_back(root);
        currentRow = 0;
}

HuffmanTree::~HuffmanTree()
{
        deleteNode(root);
        root = nullptr;
}

void HuffmanTree::deleteNode(HuffmanNode* node)
{
        if (node != nullptr) {
                deleteNode(node->left);
                deleteNode(node->right);
                delete node;
        }
}

int HuffmanTree::insertNextRow(char* values, unsigned int n)
{
        currentRow++;
#if DEBUG
        if (n != 0) {
                // first row: code length: 1bit, second row: code length: 2bit and so on...
                cout << currentRow << " bit: ";
        }
#endif

        if ( n > pow(2, currentRow)) {
                return ERROR_ROWOVERFLOW;
        }

        vector<HuffmanNode*> newRow;
        queue<HuffmanNode*> newNodes;
    
        for (auto node : previousRow) {
                node->left = new HuffmanNode();
                node->left->left = nullptr;
                node->left->right = nullptr;
                node->left->hasValue = false;
                node->right = new HuffmanNode();
                node->right->left = nullptr;
                node->right->right = nullptr;
                node->right->hasValue = false;
                newNodes.push(node->left);
                newNodes.push(node->right); 
        }

        while (n > 0) {
                HuffmanNode* node = newNodes.front();
                newNodes.pop();
                node->value = (unsigned char)*values;
                node->hasValue = true;
                n--;

#if DEBUG
                cout << " " << (int)(unsigned char)*values;
                if (n == 0)
                        cout << endl;
#endif
                values++;

        }

        while (newNodes.size() != 0) {
                newRow.push_back(newNodes.front());
                newNodes.pop();
        }

        previousRow = newRow;

        return 0;
}

unsigned char HuffmanTree::getValue(BitStream& stream, int& result) {
        HuffmanNode* node = root;
        result = 0;
        while (node != nullptr) {
                if (node->hasValue) {
                        return node->value;
                }
                
                if (stream.isEnd()) {
                        result = ERROR_ENDOFSOURCESTREAM;
                        break;
                }

                if (stream.next()) {
                        node = node->right;
                } else {
                        node = node->left;
                }
        }
        result = ERROR_NOSYMBOLFOUND;
        return 0x00;
}

