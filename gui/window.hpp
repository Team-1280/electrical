#pragma once

#include "FL/Fl_Group.H"
#include "FL/Fl_Menu_Bar.H"
#include "FL/Fl_Widget.H"
#include "FL/Fl_Window.H"
#include <FL/Fl_Double_Window.H>

/**
 * \brief Class deriving from FL_Double_Window that contains all widgets for the application plus GUI
 * state
 */
class MainWindow : public Fl_Double_Window {
public:
    MainWindow() : 
        Fl_Double_Window{0, 0, 1280, 720}, 
        m_menubar(0, 0, 0, 0)
    {
        Fl_Menu_Item items[] = {
            {"&File", 0, 0, 0, FL_SUBMENU},
                {"New", 0, 0, 0},
                {"Open", 0, 0, 0},
                {"Save", 0, 0, 0},
                {0},
            {0}
        };


        this->resizable(*this); //Enable window resizing
        
        this->m_menubar.resize(0, 0, this->w(), 50);
        this->m_menubar.copy(items);

        this->add(this->m_menubar);
    }

private:
    /** \brief Main menu bar with the usual New Open Save buttons*/
    Fl_Menu_Bar m_menubar;
    
};
