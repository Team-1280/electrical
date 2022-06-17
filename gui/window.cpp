#include "window.hpp"
#include "cairomm/context.h"
#include "geom.hpp"
#include "lib.hpp"
#include "unit.hpp"

#include <gtkmm/enums.h>
#include <gtkmm/builder.h>

#include <iostream>

MainWindow::MainWindow() : 
    m_layout{Gtk::Orientation::VERTICAL},
    m_toolbar{Gtk::Orientation::HORIZONTAL},
    m_button{"Button"},
    m_graph{"./assets/boards/board.json"},
    m_render{this->m_graph}
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

GraphRender::GraphRender(BoardGraph& graph) : 
        Gtk::DrawingArea{},
        m_drag_event{Gtk::GestureDrag::create()},
        m_scroll_event{Gtk::EventControllerScroll::create()},
        m_motion_event{Gtk::EventControllerMotion::create()},
        m_pxpmeter{static_cast<double>(this->smallest_dim())},
        m_campos{0._m, 0._m},
        graph{graph}
    {
    this->set_draw_func(sigc::mem_fun(*this, &GraphRender::on_draw));
    this->m_drag_event->signal_drag_begin().connect([this](double,double){ this->m_dragoffset = Point{};});
    this->m_drag_event->signal_drag_update().connect([this](double x, double y) {
        x = this->px_to_meters(x);
        y = this->px_to_meters(y);
        auto newdragpos = Point{Length{static_cast<float>(x)}, Length{static_cast<float>(y)}};
        this->m_campos += newdragpos - this->m_dragoffset;
        this->m_dragoffset = newdragpos;
        this->queue_draw();
    });

    this->m_scroll_event->set_flags(Gtk::EventControllerScroll::Flags::VERTICAL);
    this->m_scroll_event->signal_scroll().connect([this](double dx, double dy) {
            dy = std::exp((dy < 0) ? 0.1 : -0.2);
            this->m_pxpmeter *= dy;
            auto newpos = this->m_mousepos / dy;
            this->m_campos -= this->m_mousepos - newpos;
            this->m_mousepos = newpos;
            this->m_dragoffset /= dy;
            this->queue_draw();
            return true;
        },
        true
    );

    this->m_motion_event->signal_motion().connect([this](double x, double y) { 
        this->m_mousepos.x.normalized() = this->px_to_meters(x - this->get_width() / 2.);
        this->m_mousepos.y.normalized() = this->px_to_meters(y - this->get_height() / 2.);
        for(const auto& [_, node] : this->graph.nodes()) {
            if(node->type()->footprint().aabb().contains(this->m_mousepos + this->m_campos)) {
                this->m_hovered = WeakRef<ComponentNode>{node};
                this->queue_draw();
                return;
            }
        }

        this->m_hovered = WeakRef<ComponentNode>{};
        this->queue_draw();
    });

    this->add_controller(this->m_drag_event);
    this->add_controller(this->m_scroll_event);
    this->add_controller(this->m_motion_event);

    this->signal_resize().connect([this](int w, int h) {
        this->m_pxpmeter = this->smallest_dim();
    });

    this->queue_draw();
}

void GraphRender::on_draw(const Cairo::RefPtr<Cairo::Context>& cairo, int w, int h) {
    cairo->save();
    cairo->set_source_rgb(0.9, 0.9, 0.9);
    cairo->paint();
    cairo->restore();
    
    cairo->save();
    cairo->scale(this->m_pxpmeter, this->m_pxpmeter);
    cairo->translate(this->m_campos.x.normalized(), this->m_campos.y.normalized());
    cairo->translate(w / 2. / this->m_pxpmeter, h / 2. / this->m_pxpmeter);


    cairo->save();

    cairo->set_line_width(0.0001);
    cairo->set_source_rgba(0., 0., 0., 0.3);
    
    double startx = -this->m_campos.x.normalized() - this->meters_wide() / 2.;
    double starty = -this->m_campos.y.normalized() - this->meters_tall() / 2.;
    double h_m = starty + this->meters_tall();
    double w_m = startx + this->meters_wide();
 
    for(
        double i = std::round(startx / 0.01) * 0.01;
        i < w_m;
        i += 0.01
    ) {
        cairo->move_to(i, starty);
        cairo->line_to(i, h_m);
        cairo->stroke();
    }

    for(
        double i = std::round(starty / 0.01) * 0.01;
        i < h_m;
        i += 0.01
    ) {
        cairo->move_to(startx, i);
        cairo->line_to(w_m, i);
        cairo->stroke();
    }

    cairo->restore();

    
    for(const auto& [id, node] : this->graph.nodes()) {
        this->draw_node(cairo, node);
    }

    for(const auto& [id, edge] : this->graph.edges()) {
        cairo->save();
        cairo->set_line_cap(Cairo::Context::LineCap::ROUND);
        cairo->set_line_join(Cairo::Context::LineJoin::ROUND);
        cairo->set_line_width(0.001);
        cairo->set_source_rgb(0., 0., 0.);
        
        const auto& left = edge->side(WireEdge::Side::LEFT);
        cairo->move_to(left.pos().x.normalized(), left.pos().y.normalized());
        for(const auto& point : *edge) {
            cairo->line_to(point.x.normalized(), point.y.normalized());
        }

        const auto& right = edge->side(WireEdge::Side::RIGHT);
        cairo->line_to(right.pos().x.normalized(), right.pos().y.normalized());

        cairo->stroke();
        
        cairo->restore();
    }
    cairo->restore();
}

void GraphRender::draw_node(const Cairo::RefPtr<Cairo::Context>& cairo, Ref<ComponentNode> node) {
    cairo->save();
    
    cairo->set_line_cap(Cairo::Context::LineCap::ROUND);
    cairo->set_line_join(Cairo::Context::LineJoin::ROUND);
    cairo->set_line_width(0.01);
    if(!this->m_hovered.expired() && this->m_hovered.lock().get() == node.get()) {
        cairo->set_source_rgb(0., 0., 0.5);
    } else {
        cairo->set_source_rgb(.0, .0, 0.0);
    }

    const auto& fp = node->type()->footprint();
    const auto& pos = node->pos();
    
    cairo->move_to(fp.first().x.normalized(), fp.first().y.normalized());
    for(const auto& pt : fp) {
        cairo->line_to(
            pt.x.normalized(),
            pt.y.normalized()
        );
    }

    cairo->line_to(fp.first().x.normalized(), fp.first().y.normalized());
    cairo->stroke();

    for(const auto& [id, port] : *node->type()) {
        cairo->arc(
            (port.pos().x + pos.x).normalized(),
            (port.pos().y + pos.y).normalized(),
            0.001,
            0,
            2 * std::numbers::pi
        );
        cairo->fill_preserve();
    }

    cairo->restore();
}
