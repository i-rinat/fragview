#include <gtkmm/window.h>
#include <gtkmm/main.h>
#include <gtkmm/button.h>
#include "clusters.h"
#include "fragmap-widget.h"
#include <iostream>

class GraphWindow : public Gtk::Window {
    public:
        GraphWindow ();
        virtual ~GraphWindow ();

    protected:
        Fragmap fragmap;
        Clusters cl;
};

GraphWindow::GraphWindow () {
    set_title ("graph");
    fragmap.attach_clusters (cl);
    cl.collect_fragments("/var");
    uint64_t ds = cl.get_device_size_in_blocks ("/var");
    cl.__fill_clusters (ds, 1000, 2);

    add (fragmap);
    fragmap.show();
}

GraphWindow::~GraphWindow () {
}


int main (int argc, char *argv[]) {
    Gtk::Main kit(argc, argv);

    GraphWindow window;

    Gtk::Main::run (window);
}
