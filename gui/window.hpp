#pragma once

#include "geom.hpp"
#include <gtkmm/eventcontrollerscroll.h>
#include <gtkmm/gesturedrag.h>
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
    GraphRender();

protected:
    void on_draw(const Cairo::RefPtr<Cairo::Context>& cairo, int w, int h);
    
private:
    void draw_fp(const Cairo::RefPtr<Cairo::Context>& cairo, model::Footprint& fp);

    inline double px_to_meters(double px) const noexcept { return px / this->m_pxpmeter; }
    inline double meters_to_px(double m) const noexcept { return m * this->m_pxpmeter; }

    /** \brief Get the smallest dimension of this drawing area */
    inline int smallest_dim() const { return std::min(this->get_width(), this->get_height()); }

    /** \brief Receive events when the user pans the view around */
    Glib::RefPtr<Gtk::GestureDrag> m_drag_event;
    /** \brief Receive scroll events, same behavior as m_zoom_event */
    Glib::RefPtr<Gtk::EventControllerScroll> m_scroll_event;
    
    /** \brief Pixels per meter */
    double m_pxpmeter;

    /** \brief Camera position in the workspace*/
    model::Point m_campos;
    
    /** \brief Used while dragging to store the original camera postition */
    model::Point m_oldcampos;
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


