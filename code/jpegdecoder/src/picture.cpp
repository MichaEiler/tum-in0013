#include "picture.h"
#include <iostream>
using namespace std;

Picture::Picture()
{
        data = nullptr;
}

Picture::~Picture()
{
        for(int i = 0; i < height; i++) {
                delete[] data[i];
        }
        delete[] data;
}

void Picture::init(int width, int height)
{
        this->width = width;
        this->height = height;

        data = new Pixel*[height];
        for (int i = 0; i < height; i++) {
                data[i] = new Pixel[width];
        }
}

