#include <gtkmm/window.h>
#include <gtkmm/main.h>

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

    Gtk::Main::run (window);
}
