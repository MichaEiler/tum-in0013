/*

  Definitions of JFIF File Format implemented according to:
        * http://webtweakers.com/swag/GRAPHICS/0143.PAS.html
        * http://de.wikipedia.org/wiki/JPEG_File_Interchange_Formata
        * http://en.wikipedia.org/wiki/Jpeg
        * http://www.impulseadventure.com/photo/jpeg-huffman-coding.html
        * http://www.media.mit.edu/pia/Research/deepview/exif.html
        * http://www.w3.org/Graphics/JPEG/itu-t81.pdf

 */

#ifndef __JPEGDECODER_H
#define __JPEGDECODER_H

#include <string>
#include "picture.h"

#include "huffmantree.h"

struct ColorComponent
{
        unsigned char vsf;              // verticalSamplingFactor;
        unsigned char hsf;              // horicontalSamplingFactor;
        unsigned char qt;               // Number of Quantization table according to SOF0 tag
        unsigned char htac;             // huffman table number according to SOS tag (AC)
        unsigned char htdc;             // same for DC
};

struct QTable
{
        unsigned char id;
        unsigned char precision;        // 8 or 16
        unsigned short values[64];
        
};

class JpegDecoder
{
private:
        std::string raw;
        int position;

        // image data
        unsigned short width;
        unsigned short height;

        ColorComponent color_y;
        ColorComponent color_cb;
        ColorComponent color_cr;

        unsigned short restartInterval; // used for RSTn markers (=FFDn) n = [0..7]
                                        // one RSTn marker is in the content after
                                        // every restartInterval MCU blocks
        bool useRST;                    // true if the file contained 0xFFDD
        QTable* qTables[4];             // used to store the quantization tables

        HuffmanTree* hTablesDC[3];      // used to store the huffman tables
        HuffmanTree* hTablesAC[3];

        Picture picture;                // final picture data
        
        // private methods for parser
        unsigned char seekNextSegment();
        int parseSOF0();                // parse the parameters for the baseline dct algorithm
        int parseDRI();
        int parseDHT();                 // parse huffman table
        int parseDQT();                 // parse quantization table
        int parseEXIF();                // will just read the length and skip the EXIF content
        int parseSOS();                 // parsing of image data

        int parseBlock(BitStream& stream, HuffmanTree* dcTable, HuffmanTree* acTable,
                       QTable* qTable, int& previousDC, int* values);
        int parseScanHeader(ColorComponent* components, int** coef,
                            int* cy, int* ccb, int* ccr);
        void skipRST(BitStream& stream);
        void scaleHorizontal(int hsf_max, int vsf_max, int hsf, int vsf, int* values);
        void scaleVertical(int hsf_max, int vsf_max, int hsf, int vsf, int* values);

        // general parsing methods
        unsigned short parseUShort();

public:
        explicit JpegDecoder();
        virtual ~JpegDecoder();
        bool read(std::string path);
        int decode();
        Picture& getPicture() { return picture; }
};

#endif // __JPEGDECODER_H

