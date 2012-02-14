#include <gtkmm/window.h>
#include <gtkmm/main.h>
#include <gtkmm/button.h>
#include <gtkmm/box.h>
#include <gtkmm/paned.h>
#include "clusters.h"
#include "fragmap-widget.h"
#include "filelist-widget.h"
#include <iostream>

class GraphWindow : public Gtk::Window {
    public:
        GraphWindow ();
        virtual ~GraphWindow ();

    protected:
        Fragmap fragmap;
        Clusters cl;
        FilelistView filelist;
};

GraphWindow::GraphWindow () {
    set_title ("graph");
    fragmap.attach_clusters (cl);
    cl.collect_fragments ("/var");

    Gtk::VPaned *vpaned = Gtk::manage (new Gtk::VPaned);
    vpaned->pack1 (fragmap, true, false);
    vpaned->pack2 (filelist, true, false);

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

    GraphWindow window;

    Gtk::Main::run (window);
}
