#pragma once

#include "geom.hpp"
#include "lib.hpp"
#include "ser/store.hpp"
#include <gtkmm/eventcontrollermotion.h>
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
    GraphRender(BoardGraph& graph);

protected:
    void on_draw(const Cairo::RefPtr<Cairo::Context>& cairo, int w, int h);
    
private:
    void draw_node(const Cairo::RefPtr<Cairo::Context>& cairo, Ref<ComponentNode> node);
    
    inline double px_to_meters(double px) const noexcept { return px / this->m_pxpmeter; }
    inline double meters_to_px(double m) const noexcept { return m * this->m_pxpmeter; }

    /** \brief Get the smallest dimension of this drawing area */
    inline int smallest_dim() const { return std::min(this->get_width(), this->get_height()); }

    inline double meters_wide() const { return this->get_width() / this->m_pxpmeter; }
    inline double meters_tall() const { return this->get_height() / this->m_pxpmeter; }

    /** \brief Receive events when the user pans the view around */
    Glib::RefPtr<Gtk::GestureDrag> m_drag_event;
    /** \brief Receive scroll events, same behavior as m_zoom_event */
    Glib::RefPtr<Gtk::EventControllerScroll> m_scroll_event;
    /** \brief Receive mouse motion events to draw cursor position text */
    Glib::RefPtr<Gtk::EventControllerMotion> m_motion_event;
    
    /** \brief If a component is being hovered over, highlight it */
    WeakRef<ComponentNode> m_hovered;
    
    /** \brief Pixels per meter */
    double m_pxpmeter;

    /** \brief Camera position in the workspace*/
    Point m_campos;
    
    /** \brief Stores the last drag offset from the beginning, used for panning the view */
    Point m_dragoffset;
    
    /** \brief Last recorded position of the mouse */
    Point m_mousepos;
    /** \brief Mouse coordinates in world coordinates */
    Point m_absmousepos;
    
    /** \brief Board graph, shared with the MainWindow */
    BoardGraph& graph;

    /** \brief Lower limit for how many pixels onscreen may represent one meter */
    static constexpr const double MIN_ZOOM = 0.01;
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
    
    /** \brief The graph to modify */
    BoardGraph m_graph;
};
