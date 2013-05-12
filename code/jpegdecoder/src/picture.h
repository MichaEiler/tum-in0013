#ifndef __PICTURE_H
#define __PICTURE_H

struct Pixel
{
        int red;
        int green;
        int blue;
};

class Picture
{
private:
        Pixel** data;
        int width;
        int height;
public:
        explicit Picture();
        virtual ~Picture();
        void init(int width, int height);
        int getWidth() { return width; }
        int getHeight() { return height; }
        inline void setPixel(int x, int y, int red, int green, int blue)
        {
                if ( x >= 0 && x < width && y >= 0 && y < height) {
                        data[y][x].red = red;
                        data[y][x].green = green;
                        data[y][x].blue = blue;
                }
        }
        inline Pixel& getPixel(int x, int y)
        {
                return data[y][x];
        }
};

#endif // __PICTURE_H
