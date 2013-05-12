#ifndef __COLOR_H
#define __COLOR_H

/*!
 * y ... luminance
 * cb ... chrominance blue
 * cr ... chromiance red
 *
 * JPEG File Interchange Format 1.02, Eric Hamilton, 1992, Page 3
 */
namespace Color
{
        inline int toRed(int y, int cb, int cr) {
                int yi = y << 8;
                int cri = cr - 128;
                return ((yi + 359 * cri + 128) >> 8);
        }
        
        inline int toGreen(int y, int cb, int cr) {
                int yi = y << 8;
                int cbi = cb - 128;
                int cri = cr - 128;
                return ((yi - 88 * cbi - 183 *cri + 128) >> 8);
        } 

        inline int toBlue(int y, int cb, int cr) {
                int yi = y << 8;
                int cbi = cb - 128;
                return ((yi + 454 * cbi + 128) >> 8); 
        }
};

#endif // __COLOR_H
