#include <giomm/resource.h>
#include <gtkmm/application.h>

#include "util/log.hpp"
#include "window.hpp"
#include <lib.hpp>


int main(int argc, char * argv[]) {
    logger::init("./log.txt");
    auto application = Gtk::Application::create("t1280.electrical");
    
    return application->make_window_and_run<MainWindow>(argc, argv);
}
