#include <gtkmm/window.h>
#include <gtkmm/main.h>
#include <gtkmm/button.h>
#include <gtkmm/box.h>
#include <gtkmm/paned.h>
#include <gtkmm/scrolledwindow.h>
#include <gtkmm/actiongroup.h>
#include <gtkmm/uimanager.h>
#include "clusters.h"
#include "fragmap-widget.h"
#include "filelist-widget.h"
#include <iostream>
#include <pwd.h>
#include <cassert>

class GraphWindow : public Gtk::Window {
    public:
        GraphWindow (const std::string& initial_dir);
        virtual ~GraphWindow ();

    protected:
        Fragmap fragmap;
        Clusters cl;
        FilelistView filelist;
        std::string initial_dir;
        Glib::RefPtr<Gtk::ActionGroup> action_group_ref;
        Glib::RefPtr<Gtk::UIManager> ui_manager_ref;
};

GraphWindow::GraphWindow (const std::string& initial_dir) {
    set_title ("graph");
    set_default_size (800, 560);
    fragmap.attach_clusters (cl);
    fragmap.attach_filelist_widget (filelist);

    cl.collect_fragments (initial_dir);
    cl.create_coarse_map (1000);

    // set up menus
    action_group_ref = Gtk::ActionGroup::create ();

    // TODO: WIP

    Gtk::ScrolledWindow *scrolled_window = Gtk::manage (new Gtk::ScrolledWindow);
    scrolled_window->add (filelist);

    Gtk::VPaned *vpaned = Gtk::manage (new Gtk::VPaned);
    vpaned->pack1 (fragmap, true, false);
    vpaned->pack2 (*scrolled_window, true, false);

    Gtk::VBox *vbox = Gtk::manage (new Gtk::VBox);
    vbox->pack_start (*vpaned, true, true);
    Gtk::Button *btn = Gtk::manage (new Gtk::Button ("Button that do nothing"));
    vbox->pack_start (*btn, false, false);

    add (*vbox);
    show_all_children ();
}

GraphWindow::~GraphWindow () {
}


int main (int argc, char *argv[]) {
    Gtk::Main kit(argc, argv);

    std::string initial_dir;
    if (argc > 1) {
        initial_dir = argv[1];
    } else {
        struct passwd *pw = getpwuid(getuid());
        assert(pw != NULL);
        initial_dir = pw->pw_dir;
    }

    GraphWindow window (initial_dir);

    Gtk::Main::run (window);
}
