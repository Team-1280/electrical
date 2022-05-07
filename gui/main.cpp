#include "window.hpp"
#include <lib.hpp>
#include <FL/Fl.H>

int main(int argc, char * argv[]) {
    MainWindow win{};
    
    win.end();
    win.show(argc, argv);
    return Fl::run();
}
