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
    cl.collect_fragments ("/var");

    add (fragmap);
    fragmap.show();

    cl.__fill_clusters (1000, 2);
    fragmap.queue_draw ();
}

GraphWindow::~GraphWindow () {
}


int main (int argc, char *argv[]) {
    Gtk::Main kit(argc, argv);

    GraphWindow window;

    Gtk::Main::run (window);
}
