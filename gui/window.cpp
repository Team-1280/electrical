#include "window.hpp"
#include "gtkmm/builder.h"

MainWindow::MainWindow() : m_button{"Button"} {
    this->set_title("Electrical");
    this->set_default_size(1280, 720);
    this->set_resizable();

    this->m_button.set_margin(5);
    this->m_button.signal_clicked().connect(sigc::mem_fun(*this, &MainWindow::on_click));

    this->set_child(this->m_button);
    
}

void MainWindow::on_click() {

}
