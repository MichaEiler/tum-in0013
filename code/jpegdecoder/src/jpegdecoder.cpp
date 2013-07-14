#include "jpegdecoder.h"
#include <vector>
#include <fstream>
#include <string.h>

#if DEBUG
#include <iostream>
#endif

#include "color.h"
#include "dct.h"
using namespace std;

#define JFIF_SOI                0xD8    // Start of Image
#define JFIF_SOS                0xDA    // Start of Scan - start of image data
#define JFIF_SOF0               0xC0    // Start of Frame 0 if Baseline DCT is used
                                        // it also contains the width and height of the image
                                        // and color scheme information (note: only YCbCr sup.)
#define JFIF_SOF2               0xC2    // progressive dct (not supported)
#define JFIF_DHT                0xC4    // Definition of huffman tables
#define JFIF_DAC                0xCC    // Definition of arithmetic codec (not supported)
#define JFIF_DQT                0xDB    // Definition of quantization tables
#define JFIF_DRI                0xDD    // Definition for restart interval (optional?)
#define JFIF_EOI                0xD9    // End of Image

#define COLOR_Y                   0x01
#define COLOR_CB                  0x02
#define COLOR_CR                  0x03

#define ERROR_NOIMAGEDATA       0x10
#define ERROR_OUTOFRANGE        0x11
#define ERROR_NOTSUPPORTED      0x12
#define ERROR_COLORSCHEME       0x13    // only YCbCr is supported
#define ERROR_DATAPRECISION     0x14    // only 8bit data precision is supported
#define ERROR_ARITHMETIC        0x15    // contains additional arithmetic
                                                                        // codecs which aren't supported
#define ERROR_INVALIDDRI        0x16    // invalid DRI/RST segment
#define ERROR_PDCT              0X17    // progressive dct is not supported
#define ERROR_DHTOVERFLOW       0x18    // to many entries in the DHT table (max. 256 are allowed)
#define ERROR_NOEOIMARKER       0x19    // no end of image marker found in image
#define ERROR_INVALIDQTNR       0x1A    // invalid quantization table number

#define ERROR_HUFFMANPREFIX     0x100   // bitmask added to error codes produced by the huffmantree
                                        // algorithm so that the error codes can be distinguished
#define ERROR_BITSTREAMPREFIX   0x200   // bitmask added to error codes produced by the bitstream

#define CHECK_RANGE(a,b,c)      if ((unsigned int)a+(unsigned int)b >= (unsigned int)c.size()) return ERROR_OUTOFRANGE;
#define CHECK_ERROR(a)          if (a != 0) return a;
#define CHECK_ERROR_HUFFMAN(a)  if (a != 0) return a ^ ERROR_HUFFMANPREFIX;
#define CHECK_ERROR_BITSTREAM(a) if (a != 0) return a ^ ERROR_BITSTREAMPREFIX;

// numbers in the array for the inverse zigzag algorithm
static unsigned char zz[64] =   {  0,  1,  8, 16,  9,  2,  3, 10,
                                  17, 24, 32, 25, 18, 11,  4,  5,
                                  12, 19, 26, 33, 40, 48, 41, 34,
                                  27, 20, 13,  6,  7, 14, 21, 28,
                                  35, 42, 49, 56, 57, 50, 43, 36,
                                  29, 22, 15, 23, 30, 37, 44, 51,
                                  58, 59, 52, 45, 38, 31, 39, 46,
                                  53, 60, 61, 54, 47, 55, 62, 63 };

JpegDecoder::JpegDecoder()
{
        position = 0;
        width = -1;
        height = -1;
        useRST = false;
        restartInterval = -1;
}

JpegDecoder::~JpegDecoder()
{
        // thank's to the shared-ptr's we don't have to free any memory
}

bool JpegDecoder::read(std::string path)
{
        // the fastest method to read a file, according to:
        // http://insanecoding.blogspot.co.at/2011/11/how-to-read-in-file-in-c.html

        ifstream pictureStream (path.c_str(), ios::in | ios::binary);

        if ( pictureStream ) {
                pictureStream.seekg(0, ios::end);
                // tell the string how much it has to store
                raw.resize(pictureStream.tellg());
                pictureStream.seekg(0, ios::beg);
                pictureStream.read(&raw[0], raw.size());
                pictureStream.close();
                return true;
        }
        return false;
}

unsigned char JpegDecoder::seekNextSegment()
{
        while ((unsigned int)position < raw.size()) {
                if ((unsigned char)raw[position] == 0xFF) {
                        position += 2;
                        return (unsigned char)raw[position - 1];
                }
                
                position++;
        }
        return JFIF_EOI;
}

int JpegDecoder::decode()
{
        // search for the beginning of the image in the raw data
        unsigned char symbol = 0x00;

        while (symbol != JFIF_SOI) { 
                symbol = seekNextSegment();
                if (symbol == JFIF_EOI) {
                        return ERROR_NOIMAGEDATA;
                }
        }
        
        int errcode = 0;
        while ((symbol = seekNextSegment()) != JFIF_EOI) {
                switch (symbol) {
                case JFIF_SOS:
                        errcode = parseSOS(); break;
                case JFIF_SOF0:
                        errcode = parseSOF0(); break;
                case JFIF_DHT:
                        errcode = parseDHT(); break;
                case JFIF_DQT:
                        errcode = parseDQT(); break;
                case JFIF_DRI:
                        errcode = parseDRI(); break;
                case JFIF_DAC:
                        return ERROR_ARITHMETIC;
                case JFIF_SOF2:
                        return ERROR_PDCT;
                default:
                        if (symbol >= 0xE0 && symbol <= 0xEF) {
                                errcode = parseEXIF();
                        }
                        break;
                }

                CHECK_ERROR(errcode)
        }

        return 0;
}

unsigned short JpegDecoder::parseUShort()
{
        unsigned short result = (unsigned short)raw[position++];
        result *= 256;
        result += (unsigned short)(unsigned char)raw[position++];
        return result;
}

int JpegDecoder::parseEXIF()
{
        CHECK_RANGE(position, 2, raw)
        unsigned short length = parseUShort() - 2;
        // skip exif meta-data
        CHECK_RANGE(position, length, raw);
        position += length;
        return 0;
}

int JpegDecoder::parseSOF0()
{
        // parse segment length and remove two because the size for the length itself is included
        CHECK_RANGE(position, 2, raw)
        unsigned short length = parseUShort() - 2;
        
        // check total length assumed that the image uses the YCbCr color scheme
        if (length < 15) {
                return ERROR_COLORSCHEME;
        }

        // check for data precision
        CHECK_RANGE(position, 1, raw)
        if (raw[position++] != 0x08) {
                return ERROR_DATAPRECISION;
        }

        // parse image height
        CHECK_RANGE(position, 2, raw);
        height = parseUShort();

        // parse image width
        CHECK_RANGE(position, 2, raw);
        width = parseUShort();

        // init the picture
        picture.init(width, height);

        // parse color scheme
        CHECK_RANGE(position, 1, raw);
        if (raw[position++] != 0x03) {
                return ERROR_COLORSCHEME;
        }

        // parse color scheme components
        for (int i = 0; i < 3; i++) {
                CHECK_RANGE(position, 3, raw);
                unsigned char id = raw[position++];
                unsigned char vsf = ((unsigned char)raw[position]) & 0x0F;
                unsigned char hsf = ((unsigned char)raw[position]) >> 4;

                if (!((vsf == 1 || vsf == 2) && (hsf == 1 || hsf == 2)))
                        return ERROR_NOTSUPPORTED;

                unsigned char qt = raw[++position];
                position++;
                ColorComponent* component = nullptr;

                switch (id) {
                case 1:
                        component = &color_y; break;
                case 2:
                        component = &color_cb; break;
                default:
                        component = &color_cr; break;
                }

                component->hsf = hsf;
                component->vsf = vsf;
                component->qt = qt;
        }

#if DEBUG
        cout << "Width: " << width << endl;
        cout << "Height: " << height << endl;
        cout << "YCBCR_Y, hsf: " << (unsigned int)color_y.hsf << ", vsf: " << (unsigned int)color_y.vsf << ", qtn: " << (unsigned int)color_y.qt  << endl;
        cout << "YCBCR_CB, hsf: " << (unsigned int)color_cb.hsf << ", vsf: " << (unsigned int)color_cb.vsf << ", qtn: " << (unsigned int)color_cb.qt  << endl;
        cout << "YCBCR_CR, hsf: " << (unsigned int)color_cr.hsf << ", vsf: " << (unsigned int)color_cr.vsf << ", qtn: " << (unsigned int)color_cr.qt  << endl;
#endif

        return 0;
}

int JpegDecoder::parseDRI()
{
        CHECK_RANGE(position, 2, raw);
        unsigned short length = parseUShort();
        if (length != 4) {
                return ERROR_INVALIDDRI;
        }

        CHECK_RANGE(position, 2, raw);
        restartInterval = parseUShort();
        useRST = true;

#if DEBUG
        cout << "UseRST: " << useRST << ", Intervall: " << restartInterval << endl;
#endif

        return 0;
}

int JpegDecoder::parseDHT()
{
        CHECK_RANGE(position, 2, raw);
        unsigned short length = parseUShort() - 2;

        while (length > 16) {
                CHECK_RANGE(position, 1, raw);
                unsigned char information = raw[position++];
                length--;

                shared_ptr<HuffmanTree> huffmanTree(new HuffmanTree());
                unsigned int nr = information & 0x0F; // 0-3. bit: nr
                bool isDC = (information & 0x10) == 0;

                CHECK_RANGE(position, 16, raw);
                char* nodeCounters = &raw[position];
                position += 16;
                length -= 16;

                // there is a maximum of 256 entries in the specification for the dht table
                int sum = 0;
                for (int i = 0; i < 16; i++)
                        sum += (int)nodeCounters[i];
                if (sum > 256)
                        return ERROR_DHTOVERFLOW;
                
#if DEBUG
                cout<<"Huffman-Table, Nr: "<<nr<<", DC: "<<isDC<<endl;
#endif

                int errcode = 0;
                for (int i = 0; i < 16; i++) {
                        CHECK_RANGE(position, nodeCounters[i], raw);

                        if((errcode = huffmanTree->insertNextRow(&raw[position], nodeCounters[i])) != 0) {
                                return errcode ^ ERROR_HUFFMANPREFIX;
                        }

                        length -= nodeCounters[i];
                        position += nodeCounters[i];
                }
                if (nr == 0x01|| nr == 0x00) {
                        if (isDC) {
                                hTablesDC[nr] = huffmanTree;
                        } else {
                                hTablesAC[nr] = huffmanTree;
                        }
                } else {
                        return ERROR_NOTSUPPORTED;
                }
        }
        position += length;

        return 0;
}

int JpegDecoder::parseDQT()
{
        CHECK_RANGE(position, 2, raw);
        unsigned short length = parseUShort() - 2;

        while (length > 0) {
                shared_ptr<QTable> qTable(new QTable());
                CHECK_RANGE(position, 1, raw);
                unsigned char information = (unsigned char)raw[position++];
                length--;

                qTable->precision = information >> 4;
                qTable->id = information & 0x0F;

                if (qTable->precision == 0) { // 8bit precision
                        CHECK_RANGE(position, 64, raw)
                        for (int i = 0; i < 64; i++) {
                                qTable->values[i] = (unsigned char)raw[position++];
                        }
                        length -= 64;
                } else { // 16bit precision
                        CHECK_RANGE(position, 128, raw)
                        for (int i = 0; i < 64; i++) {
                                qTable->values[i] = parseUShort();
                        }
                        length -= 128;
                }
                
                if (!(qTable->id == 0x00 || qTable->id == 0x01)) {
                        return ERROR_NOTSUPPORTED;
                }

                qTables[qTable->id] = qTable;
        
#if DEBUG
                cout << "QuantationTable-Nr. " << (unsigned int)qTable->id << ", Precision: " << (unsigned int)qTable->precision << endl;
                for (int i = 0; i < 64; i++)
                        cout << " " << qTable->values[i];
                cout << endl;
#endif
        }

        return 0;
}

int JpegDecoder::parseSOS()
{
        int error;
        // temporary arrays for data
        ColorComponent components[3];
        int* coef[3];
        int coefy[256]; // 4 * 64, maximum amount of values to remember in case of supersampling
        int coefcb[256];
        int coefcr[256];

        // parse scan header and sort color scheme components
        error = parseScanHeader(components, coef, coefy, coefcb, coefcr);
        CHECK_ERROR(error);

        BitStream stream(&raw[position], (raw.size()-position));
        int previousDC[3] = { 0, 0, 0};
        int posx = 0;
        int posy = 0;
        int mcu = 0;

        int vsf_max = components[0].vsf + components[1].vsf + components[2].vsf;
        int hsf_max = components[0].hsf + components[1].hsf + components[2].hsf;

        vsf_max = vsf_max > 3 ? 2 : 1;
        hsf_max = hsf_max > 3 ? 2 : 1;

        while ( posy < height) {
                // reset previousDC array after #-MCU's (amount of MCU's defined by DRI-marker)
                if (useRST && mcu % restartInterval == 0) {
                        previousDC[0] = previousDC[1] = previousDC[2] = 0;
                }


                for (int cid = 0; cid < 3; cid++) {
                        for (int v = 0; v < components[cid].vsf; v++) { 
                                for (int h = 0; h < components[cid].hsf; h++) {
                                        error = parseBlock(stream, hTablesDC[components[cid].htdc],
                                                   hTablesAC[components[cid].htac],
                                                   qTables[components[cid].qt], previousDC[cid],
                                                   coef[cid] + (128 * v + 64 * h));
                                        CHECK_ERROR(error);
                
                                        // apply IDCT onto values
                                        // DCT::transform(coef[cid] + (128 * v + 64 * h));
                                        DCT::fastTransform(coef[cid] + (128 * v + 64 * h));
                                        // check for reset-marker and skip those two bytes
                                        skipRST(stream);
                                }
                        }
                        // scale
                        scaleHorizontal(hsf_max, vsf_max, components[cid].hsf, components[cid].vsf, coef[cid]);
                        scaleVertical(hsf_max, vsf_max, components[cid].hsf, components[cid].vsf, coef[cid]);
                }
                
                // store pixel-data
                for (int v = 0; v < vsf_max; v++) {
                        for (int h = 0; h < hsf_max; h++) {
                                for (int k = 0; k < 64; k++) {
                                        int index = (v * 128 + h * 64) + k;
                                        int red = Color::toRed(coefy[index], coefcb[index], coefcr[index]);
                                        int green = Color::toGreen(coefy[index], coefcb[index], coefcr[index]);
                                        int blue = Color::toBlue(coefy[index], coefcb[index], coefcr[index]);
                                        int x = posx + h * 8 + (k % 8);
                                        int y = posy + v * 8 + (k / 8);
                                        picture.setPixel(x, y, red, green, blue);
                                }
                        }
                }

                mcu++;
                posx += 8 * hsf_max;
                if (posx >= width) { posx = 0; posy += 8 * vsf_max; }
        }

        // check if the last two bytes are FF D9 = EOI

        if ( (unsigned char)raw[raw.size()-2] != 0xFF || (unsigned char)raw[raw.size()-1] != JFIF_EOI) {
                return ERROR_NOEOIMARKER;
        }

        return 0;
}

void JpegDecoder::scaleHorizontal(int hsf_max, int vsf_max, int hsf, int vsf, int* values)
{
        if (hsf_max == hsf)
                return;
        for (int j = 0; j < 2; ++j) {
                for (int i = 63; i >= 0; --i) {
                        values[2 * i] = values[i];
                        values[2 * i + 1] = values[i];
                }
                if (vsf_max != vsf)
                        return;
                else
                        values += 128;
        }
}

void JpegDecoder::scaleVertical(int hsf_max, int vsf_max, int hsf, int vsf, int* values)
{
        if (vsf_max == vsf)
                return;
        for (int v = 7; v >= 0; --v) {
                for (int h = 0; h < 16; ++h) {
                        values [v * 2 * 16 + h] = values[v * 16 + h];
                        values [(v * 2 + 1) * 16 + h] = values [v * 16 + h];
                }
        }
}

inline int JpegDecoder::parseScanHeader(ColorComponent* components, int** coef, int* cy, int* ccb, int* ccr)
{
        // parsing scan header
        CHECK_RANGE(position, 12, raw)
        int length = parseUShort();
        // header length or component number wrong?
        // (this would indicate that another color scheme is used)
        if (length != 12 || raw[position] != 0x03) {
                return ERROR_COLORSCHEME;
        }
        position++; // skip component number

        // parse order of components and the number of the according AC and DC huffman tables
        // 2 bytes per component

        coef[0] = coef[1] = coef[2] = nullptr;

        for(int i = 0; i < 3; i++) {
                unsigned char componentnr = (unsigned char)raw[position++];
                if (coef[i] != nullptr)
                        return ERROR_COLORSCHEME;
                switch(componentnr) {
                case 0x01:
                        components[i] = color_y; coef[i] = cy; break;
                case 0x02:
                        components[i] = color_cb; coef[i] = ccb; break;
                case 0x03:
                        components[i] = color_cr; coef[i] = ccr; break;
                default:
                        return ERROR_COLORSCHEME;
                }
                unsigned char numbers = (unsigned char)raw[position++];
#if DEBUG
                cout << "Scan-Header: ComponentNr: " << (int)componentnr << ", htac: " << (numbers & 0x0F) << ", htdc: " << (numbers >> 4) << endl;
#endif
                components[i].htac = numbers & 0x0F;
                components[i].htdc = numbers >> 4;
                if (!(components[i].htac == 0 || components[i].htdc == 1) || !(components[i].htdc == 0 || components[i].htdc == 1)) {
                        return ERROR_INVALIDQTNR;
                }
        }

        // the following three bytes are always 00 3F xx -> 00 -> first element in the 8x8 matrix
        // 3F=63 -> last element in the 8x8 matrix, and xx is unused in the case of sequential DCT
        if (!(raw[position] == 0x00 && raw[position+1] == 0x3F && raw[position+2] == 0x00)) {
                return ERROR_NOTSUPPORTED;
        }
        position+=3;

        return 0;
}

inline int JpegDecoder::parseBlock(BitStream& stream, shared_ptr<HuffmanTree> dcTable, shared_ptr<HuffmanTree> acTable,
                                   shared_ptr<QTable> qTable, int& previousDC, int* values)
{
        int error = 0;
        int zzpos = 0;
        memset((void*)values, 0, 64 * sizeof(int));
        for(int i = 0; i < 64; i++) {
                unsigned char len;
                if (i == 0)
                        len = dcTable->getValue(stream, error);
                else
                        len = acTable->getValue(stream, error);
                CHECK_ERROR_HUFFMAN(error);

                if (len == 0x00 && i != 0) {
                        break;
                } else if (len == 0xF0 && i != 0) {
                        i += 15;
                        continue;
                }

                if (i != 0) // preceding zeros
                        i += (len >> 4);
                int value = stream.next(len & 0x0F, error);
                CHECK_ERROR_BITSTREAM(error);

                if ((value & (1 << ((len & 0x0F) - 1))) == 0) {
                        value -= (1 << (len & 0x0F)) - 1;
                }
                zzpos = zz[i];
                values[zzpos] = value;
                if (i == 0) {
                        values[zzpos] += previousDC;
                        previousDC = values[zzpos];
                }
                values[zzpos] *= qTable->values[zzpos];
        }
        return 0;
}

inline void JpegDecoder::skipRST(BitStream& stream)
{
        stream.remember();
        stream.skipRest();
        if (!stream.isEnd() && stream.available(16)) {
            unsigned char byte0 = stream.nextByte(false);
            unsigned char byte1 = stream.nextByte(false);
            if (!(byte0 == 0xFF && (byte1 & 0xF0) == 0xD0)) {
                stream.rewind();
            } else {
                stream.forget();
            }

        } else {
            stream.rewind();
        }
}

