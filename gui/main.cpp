#define DOCTEST_CONFIG_IMPLEMENT
#include "unit.hpp"
#include "util/log.hpp"
#include <lib.hpp>

#include "FL/Fl.H"
#include <FL/Fl_Window.H>

int main(int argc, char * argv[]) {
    logger::init("./log.txt");
    Fl_Window win{1280, 720};
    win.end();
    win.show(argc, argv);
    return Fl::run();
}
