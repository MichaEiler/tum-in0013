#include <cairomm/context.h>
#include <glibmm/main.h>
#include <gtkmm/application.h>
#include <gtkmm/drawingarea.h>
#include <gtkmm/window.h>
#include <gtkmm.h>
#include <sys/time.h>
#include "jpegdecoder.h"
#include <iostream>
using namespace std;
using namespace Cairo;
using namespace Gtk;

class Timer
{
private:
        long currentTime;
public:
        static Timer& get() {
                static Timer t;
                return t;
        }
        void start() {
                struct timeval tv;
                gettimeofday(&tv, nullptr);
                currentTime = static_cast<long>(tv.tv_sec*1000 + (tv.tv_usec / 1000));
        }
        long stop() {
                long oldTime = currentTime;
                start();
                return currentTime - oldTime;
        }

        
};

class JPEGViewer : public Gtk::DrawingArea
{
private:
        RefPtr<ImageSurface> surface;
        RefPtr<Context> context;
protected:
        bool on_draw(const Cairo::RefPtr<Cairo::Context>& cr);
public:
        JPEGViewer(Picture& picture) {
                Pixel p;
                surface = ImageSurface::create(Format::FORMAT_RGB24, picture.getWidth(), picture.getHeight());
                context = Context::create(surface);
                
                for(int x = 0; x < picture.getWidth(); x++) {
                        for (int y = 0; y < picture.getHeight(); y++) {
                                p = picture.getPixel(x, y);
                                context->set_source_rgb(p.red/255.0, p.green/255.0, p.blue/255.0);
                                context->rectangle(x, y, 1, 1);
                                context->fill();
                        }
                }
        }
};

bool JPEGViewer::on_draw(const Cairo::RefPtr<Cairo::Context>& cr)
{
        cr->set_source(surface, 0, 0);
        cr->rectangle(0, 0, surface->get_width(), surface->get_height());
        cr->fill();

        return true;
}

int main(int argc, char** argv)
{
        if (argc != 2) {
                cout << "Usage: ./jpegdecode filename" << endl;
                return -1;
        }

        JpegDecoder jpegDecoder;

        if (!jpegDecoder.read(argv[1]))
        {
                cout << "Could not read file!" << endl;
                return -1;
        }
        Timer::get().start();
        int errcode = jpegDecoder.decode();
        if (errcode != 0) {
                cout << "Could not process raw data!" << endl << "Error code:" << errcode << endl;
                return errcode;
        }
        cout << "Decoding took " << Timer::get().stop() << "ms." << endl;

        Picture& picture = jpegDecoder.getPicture();

        // Show GTK Window
        Gtk::Main main;
        Gtk::Window window;
        Gtk::ScrolledWindow scrolledWindow;

        Timer::get().start();
        JPEGViewer viewer(picture);
        cout << "Converting to cairo surface took " << Timer::get().stop() << "ms." << endl;

        window.set_size_request(640, 480);
        scrolledWindow.set_size_request(640, 480);
        viewer.set_size_request(picture.getWidth(), picture.getHeight());

        window.set_title("JPEG Decoder");
        scrolledWindow.add(viewer);
        window.add(scrolledWindow);
        viewer.show();
        scrolledWindow.show();
        Gtk::Main::run(window);

        return 0;
}

