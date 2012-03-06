#include <gtkmm/window.h>
#include <gtkmm/main.h>
#include <gtkmm/button.h>
#include <gtkmm/box.h>
#include <gtkmm/paned.h>
#include <gtkmm/scrolledwindow.h>
#include <gtkmm/actiongroup.h>
#include <gtkmm/uimanager.h>
#include <gtkmm/filechooserdialog.h>
#include <gtkmm/stock.h>
#include "clusters.h"
#include "fragmap-widget.h"
#include "filelist-widget.h"
#include "mountpoint-select-dialog.h"
#include <iostream>
#include <locale>
#include <pwd.h>
#include <cassert>

class GraphWindow : public Gtk::Window {
    public:
        GraphWindow (const std::string& initial_dir);
        virtual ~GraphWindow ();

    protected:
        Fragmap fragmap;
        Clusters cl;
        FilelistView filelistview;
        std::string initial_dir;
        Glib::RefPtr<Gtk::ActionGroup> action_group_ref;
        Glib::RefPtr<Gtk::UIManager> ui_manager_ref;

        void on_action_view_most_fragmented (void);
        void on_action_view_most_severe (void);
        void on_action_view_restore (void);
        void on_action_main_open (void);
        void on_action_main_open_mountpoint (void);
        void on_action_main_quit (void);
        class sorter {
            public:
            bool operator() (const std::pair<uint64_t, uint64_t> &a, const std::pair<uint64_t, uint64_t> &b) const {
                return a.second > b.second;
            }
        } sorter_object_desc;
        class sorter_severity {
            public:
            bool operator() (const std::pair<uint64_t, double> &a, const std::pair<uint64_t, double> &b) const {
                return a.second > b.second;
            }
        } sorter_object_severity_desc;
};

void
GraphWindow::on_action_view_most_fragmented (void)
{
    Clusters::file_list &fl = cl.get_files();

    // sort files
    std::vector<std::pair<uint64_t, uint64_t> > mapping (fl.size());
    for (unsigned int k = 0; k < fl.size(); k ++) {
        mapping[k].first = k;
        mapping[k].second = fl[k].extents.size();
    }
    std::sort (mapping.begin(), mapping.end(), sorter_object_desc);

    // fill filelistview widget with 'n' most fragmented
    filelistview.clear ();
    for (unsigned int k = 0; k < std::min ((size_t)40, fl.size()); k ++) {
        uint64_t idx = mapping[k].first;
        filelistview.add_file_info (idx, fl[idx].extents.size(), fl[idx].severity,
            fl[idx].filetype, fl[idx].size, fl[idx].name);
    }
}

void
GraphWindow::on_action_view_most_severe (void)
{
    Clusters::file_list &fl = cl.get_files();

    std::vector<std::pair<uint64_t, double> > mapping (fl.size());
    for (unsigned int k = 0; k < fl.size(); k ++) {
        mapping[k].first = k;
        mapping[k].second = fl[k].severity;
    }
    std::sort (mapping.begin(), mapping.end(), sorter_object_severity_desc);

    // fill filelistview widget with 'n' most fragmented (by severity)
    filelistview.clear ();
    for (unsigned int k = 0; k < std::min ((size_t)40, fl.size()); k ++) {
        uint64_t idx = mapping[k].first;
        filelistview.add_file_info (idx, fl[idx].extents.size(), fl[idx].severity,
            fl[idx].filetype, fl[idx].size, fl[idx].name);
    }
}

void
GraphWindow::on_action_view_restore (void)
{
    fragmap.set_mode (Fragmap::FRAGMAP_MODE_SHOW_ALL);
    filelistview.clear ();
    fragmap.queue_draw ();
}

void
GraphWindow::on_action_main_quit (void)
{
    this->hide ();
}

void
GraphWindow::on_action_main_open (void)
{
    Gtk::FileChooserDialog dialog ("Select directory", Gtk::FILE_CHOOSER_ACTION_SELECT_FOLDER);
    dialog.set_transient_for (*this);
    dialog.add_button (Gtk::Stock::OK, Gtk::RESPONSE_OK);
    dialog.add_button (Gtk::Stock::CANCEL, Gtk::RESPONSE_CANCEL);

    int result = dialog.run ();
    if (Gtk::RESPONSE_OK == result) {
        cl.collect_fragments (dialog.get_filename ());
        cl.create_coarse_map (2000);
    }
}

void
GraphWindow::on_action_main_open_mountpoint (void)
{
    MountpointSelectDialog msd;
    int result = msd.run ();
    std::cout << "result = " << result << std::endl;
}

GraphWindow::GraphWindow (const std::string& initial_dir) {
    set_title ("fragview");
    set_default_size (800, 560);
    fragmap.attach_clusters (cl);
    fragmap.attach_filelist_widget (filelistview);

    cl.collect_fragments (initial_dir);
    cl.create_coarse_map (1000);

    // set up menus
    action_group_ref = Gtk::ActionGroup::create ();

    action_group_ref->add (Gtk::Action::create ("MenuMain", "Fragview"));
    action_group_ref->add (Gtk::Action::create ("MainOpen", "Open directory..."),
        sigc::mem_fun (*this, &GraphWindow::on_action_main_open));
    action_group_ref->add (Gtk::Action::create ("MainOpenMountpoint", "Open mountpoint..."),
        sigc::mem_fun (*this, &GraphWindow::on_action_main_open_mountpoint));
    action_group_ref->add (Gtk::Action::create ("MainQuit", "Quit"),
        sigc::mem_fun (*this, &GraphWindow::on_action_main_quit));

    action_group_ref->add (Gtk::Action::create ("MenuView", "View"));
    action_group_ref->add (Gtk::Action::create ("ViewFragmented", "Most fragmented files"),
        sigc::mem_fun (*this, &GraphWindow::on_action_view_most_fragmented));
    action_group_ref->add (Gtk::Action::create ("ViewSevere", "Files with highest severity"),
        sigc::mem_fun (*this, &GraphWindow::on_action_view_most_severe));
    action_group_ref->add (Gtk::Action::create ("ViewRestore", "Restore regular view"),
        sigc::mem_fun (*this, &GraphWindow::on_action_view_restore));

    ui_manager_ref = Gtk::UIManager::create ();
    ui_manager_ref->insert_action_group (action_group_ref);
    add_accel_group (ui_manager_ref->get_accel_group ());

    Glib::ustring ui_info =
        "<ui>"
        "  <menubar name='MenuBar'>"
        "    <menu action='MenuMain'>"
        "      <menuitem action='MainOpen' />"
        "      <menuitem action='MainOpenMountpoint' />"
        "      <separator />"
        "      <menuitem action='MainQuit' />"
        "    </menu>"
        "    <menu action='MenuView'>"
        "      <menuitem action='ViewFragmented' />"
        "      <menuitem action='ViewSevere' />"
        "      <separator />"
        "      <menuitem action='ViewRestore' />"
        "    </menu>"
        "  </menubar>"
        "</ui>";

    ui_manager_ref->add_ui_from_string (ui_info);
    Gtk::Widget *menubar = ui_manager_ref->get_widget ("/MenuBar");

    Gtk::ScrolledWindow *scrolled_window = Gtk::manage (new Gtk::ScrolledWindow);
    scrolled_window->add (filelistview);

    Gtk::VPaned *vpaned = Gtk::manage (new Gtk::VPaned);
    vpaned->pack1 (fragmap, true, false);
    vpaned->pack2 (*scrolled_window, true, false);

    Gtk::VBox *vbox = Gtk::manage (new Gtk::VBox);
    vbox->pack_start (*menubar, false, false);
    vbox->pack_start (*vpaned, true, true);

    add (*vbox);
    show_all_children ();
}

GraphWindow::~GraphWindow () {
}


int main (int argc, char *argv[]) {
    Gtk::Main kit(argc, argv);

    std::locale::global (std::locale (""));

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
