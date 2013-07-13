#include "picture.h"
#include <iostream>
using namespace std;

Picture::Picture()
{
        data = nullptr;
        position = 0;
}

Picture::~Picture()
{
        if (data != nullptr)
        {
                delete[] data;
        }
}

void Picture::init(int width, int height)
{
        this->width = width;
        this->height = height;

        data = new Pixel[width * height];
}

