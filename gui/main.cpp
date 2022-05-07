#include "window.hpp"
#include <lib.hpp>

int main(int argc, char * argv[]) {
    auto application = Gtk::Application::create("t1280.electrical");

    return application->make_window_and_run<MainWindow>(argc, argv);
}
