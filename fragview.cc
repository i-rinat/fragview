#include <gtkmm/window.h>
#include <gtkmm/main.h>
#include <gtkmm/button.h>
#include <gtkmm/box.h>
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

    Gtk::VBox *vbox = Gtk::manage (new Gtk::VBox);
    Gtk::Button *btn = Gtk::manage (new Gtk::Button ("Hello, world!"));

    vbox->pack_start (fragmap, true, true);
    vbox->pack_start (*btn, false, false);
    vbox->pack_start (filelist, true, true);

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
