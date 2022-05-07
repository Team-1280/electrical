#pragma once

#include <gtkmm.h>

class MainWindow : public Gtk::Window {
public:
    MainWindow();
private:

    void on_click();
    
    Gtk::Button m_button;
};
