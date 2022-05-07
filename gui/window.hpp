#pragma once

#include "geom.hpp"
#include <cairomm/context.h>
#include <cairomm/refptr.h>
#include <gtkmm/drawingarea.h>
#include <gtkmm/box.h>
#include <gtkmm/button.h>
#include <gtkmm/window.h>

/** 
 * \brief Class that is responsible for rendering a board graph, storing all 
 * state like camera position and zoom level
 */
class GraphRender : public Gtk::DrawingArea {
public:
    GraphRender() : Gtk::DrawingArea{} {
        this->set_draw_func(sigc::mem_fun(*this, &GraphRender::on_draw));
    }
protected:
    void on_draw(const Cairo::RefPtr<Cairo::Context>& cairo, int w, int h);
    void draw_fp(const Cairo::RefPtr<Cairo::Context>& cairo, model::Footprint& fp);

    double zoom = 0.7;
};

class MainWindow : public Gtk::Window {
public:
    MainWindow();
private:

    void on_click();
    
    /** \brief Top level layout of this window */
    Gtk::Box m_layout;
    /** \brief A toolbar containing multiple buttons for editing */
    Gtk::Box m_toolbar; 
    Gtk::Button m_button;
    
    /** \brief Widget to draw the board graph in */
    GraphRender m_render;
};


