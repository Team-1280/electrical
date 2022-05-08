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

void GraphRender::on_draw(const Cairo::RefPtr<Cairo::Context>& cairo, int w, int h) {
    /*cairo->save();
    cairo->set_source_rgb(0.7, 0.7, 1.);
    cairo->paint();
    cairo->restore();*/

    model::Footprint fp{std::vector{
        model::Point{model::Length{0.}, model::Length{0.}},
        model::Point{model::Length{0.5}, model::Length{0.8}},
        model::Point{model::Length{-0.5}, model::Length{0.8}}
    }};
    
    this->m_pxpmeter = this->smallest_dim();
    std::cout << this->m_pxpmeter << std::endl;
    cairo->scale(this->m_pxpmeter, this->m_pxpmeter);
    cairo->translate(this->m_campos.x.default_unit(), this->m_campos.y.default_unit());
    this->draw_fp(cairo, fp);
}

void GraphRender::draw_fp(const Cairo::RefPtr<Cairo::Context>& cairo, model::Footprint& fp) {
    cairo->save();
    
    cairo->set_line_cap(Cairo::Context::LineCap::ROUND);
    cairo->set_line_width(0.01);
    cairo->set_source_rgba(.0, .0, 1.0, 0.8);
    
    cairo->move_to(fp.first().x.default_unit(), fp.first().y.default_unit());
    for(const auto& pt : fp) {
        cairo->line_to(
            pt.x.default_unit(),
            pt.y.default_unit()
        );
    }

    cairo->line_to(fp.first().x.default_unit(), fp.first().y.default_unit());
    cairo->stroke();

    cairo->restore();
}

void GraphRender::on_drag_update(double x, double y) {
    x = this->px_to_meters(x);
    y = this->px_to_meters(y);

    this->m_campos = this->m_oldcampos + model::Point{x, y};
    std::cout << this->m_campos.to_json() << " " << this->m_pxpmeter << std::endl;
    this->queue_draw();
}
