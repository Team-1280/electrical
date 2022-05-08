#include "window.hpp"
#include "geom.hpp"
#include "unit.hpp"

#include <gtkmm/enums.h>
#include <gtkmm/builder.h>

#include <iostream>

MainWindow::MainWindow() : 
    m_layout{Gtk::Orientation::VERTICAL},
    m_toolbar{Gtk::Orientation::HORIZONTAL},
    m_button{"Button"},
    m_render{} 
{
    this->set_title("Electrical");
    this->set_default_size(1280, 720);
    this->set_resizable();

    this->m_button.set_margin(5);
    this->m_button.signal_clicked().connect(sigc::mem_fun(*this, &MainWindow::on_click));
    this->m_button.set_size_request(-1, 50);
    
    this->m_toolbar.append(this->m_button);
    this->m_render.set_expand();

    this->m_layout.append(this->m_toolbar);

    this->m_layout.append(this->m_render);
    this->set_child(this->m_layout);
}

void MainWindow::on_click() {

}

GraphRender::GraphRender() : 
        Gtk::DrawingArea{},
        m_drag_event{Gtk::GestureDrag::create()},
        m_scroll_event{Gtk::EventControllerScroll::create()},
        m_motion_event{Gtk::EventControllerMotion::create()},
        m_pxpmeter{static_cast<double>(this->smallest_dim())},
        m_campos{0._m, 0._m},
        m_oldcampos{this->m_campos}
    {
    this->set_draw_func(sigc::mem_fun(*this, &GraphRender::on_draw));
    this->m_drag_event->signal_drag_begin().connect([this](double,double){ this->m_oldcampos = this->m_campos; });
    this->m_drag_event->signal_drag_update().connect([this](double x, double y) {
        x = this->px_to_meters(x);
        y = this->px_to_meters(y);

        this->m_campos = this->m_oldcampos + model::Point{model::Length{x}, model::Length{y}};
        this->queue_draw();
    });

    this->m_scroll_event->set_flags(Gtk::EventControllerScroll::Flags::VERTICAL);
    this->m_scroll_event->signal_scroll().connect([this](double dx, double dy) {
            double old_pxpmeter = this->m_pxpmeter; 

            this->m_pxpmeter += (this->m_pxpmeter / 10.) * dy;
            this->m_campos *= old_pxpmeter / this->m_pxpmeter;
            this->m_oldcampos *= old_pxpmeter / this->m_pxpmeter;
            this->m_mousepos *= old_pxpmeter / this->m_pxpmeter;

            this->queue_draw();
            return true;
        },
        true
    );

    this->m_motion_event->signal_motion().connect([this](double x, double y) {
        this->m_mousepos.x.raw_val() = this->px_to_meters(x);
        this->m_mousepos.y.raw_val() = this->px_to_meters(y);
    });

    this->add_controller(this->m_drag_event);
    this->add_controller(this->m_scroll_event);
    this->add_controller(this->m_motion_event);

    this->signal_resize().connect([this](int w, int h) {
        this->m_pxpmeter = this->smallest_dim();
    });
}


void GraphRender::on_draw(const Cairo::RefPtr<Cairo::Context>& cairo, int w, int h) {
    cairo->save();
    cairo->set_source_rgb(0.9, 0.9, 0.9);
    cairo->paint();
    cairo->restore();
    
    cairo->save();
    cairo->scale(this->m_pxpmeter, this->m_pxpmeter);
    cairo->translate(-0.5, -0.5);
    cairo->translate(this->m_campos.x.default_unit(), this->m_campos.y.default_unit());
    //this->draw_node(cairo, fp);

    cairo->restore();
}

void GraphRender::draw_node(const Cairo::RefPtr<Cairo::Context>& cairo, Ref<model::ComponentNode> node) {
    cairo->save();
    
    cairo->set_line_cap(Cairo::Context::LineCap::ROUND);
    cairo->set_line_width(0.01);
    cairo->set_source_rgba(.0, .0, 0.0, 1.);

    const auto& fp = node->type()->footprint();
    const auto& pos = node->pos();
    
    cairo->move_to(fp.first().x.default_unit(), fp.first().y.default_unit());
    for(const auto& pt : fp) {
        cairo->line_to(
            pt.x.default_unit(),
            pt.y.default_unit()
        );
    }

    cairo->line_to(fp.first().x.default_unit(), fp.first().y.default_unit());
    cairo->stroke();

    for(const auto& [id, port] : *node->type()) {
        cairo->arc(
            (port.pos().x + pos.x).default_unit(),
            (port.pos().y + pos.y).default_unit(),
            0.001,
            0,
            2 * std::numbers::pi
        );
        cairo->fill_preserve();
    }

    cairo->restore();
}

