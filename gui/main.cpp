#include <giomm/resource.h>
#include <gtkmm/application.h>

#include "unit.hpp"
#include "util/log.hpp"
#include "window.hpp"
#include <lib.hpp>


int main(int argc, char * argv[]) {
    logger::init("./log.txt");
    Length l;
    Length::from_string(l, "1337 in");
    BoardGraph b{"./assets/boards/board.json"};
    auto application = Gtk::Application::create("t1280.electrical");
    
    return application->make_window_and_run<MainWindow>(argc, argv);
}
