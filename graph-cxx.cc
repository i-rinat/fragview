#include <gtkmm/window.h>
#include <gtkmm/main.h>
#include "clusters-cxx.h"
#include <iostream>

class GraphWindow : public Gtk::Window {
    public:
        GraphWindow ();
        virtual ~GraphWindow ();

    protected:
};

GraphWindow::GraphWindow () {
}

GraphWindow::~GraphWindow () {
}


int main (int argc, char *argv[]) {
    Gtk::Main kit(argc, argv);

    GraphWindow window;

    Clusters cl;

    cl.collect_fragments("/home");

    uint64_t ds = cl.get_device_size_in_blocks ("/home");

    clock_t clock_start = clock();
    std::cout << "before __fill_clusters" << std::endl;
    cl.__fill_clusters (ds, 1000, 2);
    std::cout << "after __fill_clusters " << std::endl;
    std::cout << "duration: " << (clock() - clock_start) / CLOCKS_PER_SEC << std::endl;
    std::cout << "ds = " << ds << std::endl;

    Gtk::Main::run (window);
}
