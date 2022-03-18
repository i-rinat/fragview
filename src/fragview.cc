/*
 * Copyright © 2011-2014  Rinat Ibragimov
 *
 * This file is part of "fragview" software suite.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include "clusters.hh"
#include "filelist-widget.hh"
#include "fragmap-widget.hh"
#include "mountpoint-select-dialog.hh"
#include <cassert>
#include <gtkmm/actiongroup.h>
#include <gtkmm/box.h>
#include <gtkmm/button.h>
#include <gtkmm/filechooserdialog.h>
#include <gtkmm/main.h>
#include <gtkmm/paned.h>
#include <gtkmm/scrolledwindow.h>
#include <gtkmm/statusbar.h>
#include <gtkmm/stock.h>
#include <gtkmm/uimanager.h>
#include <gtkmm/window.h>
#include <iostream>
#include <locale>
#include <pwd.h>

class GraphWindow : public Gtk::Window
{
public:
    GraphWindow();

    ~GraphWindow() noexcept override;

    void
    scan_dir(const Glib::ustring &dir);

    void
    show_directory_in_title(const Glib::ustring &dir);

protected:
    Fragmap fragmap;
    Clusters cl;
    FilelistView filelistview;
    std::string initial_dir;
    Glib::RefPtr<Gtk::ActionGroup> action_group_ref;
    Glib::RefPtr<Gtk::UIManager> ui_manager_ref;
    Gtk::Statusbar statusbar;
    unsigned int statusbar_context;

    void
    on_action_view_most_fragmented();

    void
    on_action_view_most_severe();

    void
    on_action_view_restore();

    void
    on_action_main_open();

    void
    on_action_main_open_mountpoint();

    void
    on_action_main_quit();
};

void
GraphWindow::on_action_view_most_fragmented()
{
    Clusters::file_list &fl = cl.get_files();

    // sort files
    std::vector<std::pair<uint64_t, uint64_t>> mapping(fl.size());
    for (unsigned int k = 0; k < fl.size(); k++) {
        mapping[k].first = k;
        mapping[k].second = fl[k].extents.size();
    }
    std::sort(mapping.begin(), mapping.end(),
              [](const std::pair<uint64_t, uint64_t> &a, const std::pair<uint64_t, uint64_t> &b) {
                  return a.second > b.second;
              });

    // fill filelistview widget with 'n' most fragmented
    filelistview.clear();
    for (unsigned int k = 0; k < std::min((size_t)40, fl.size()); k++) {
        uint64_t idx = mapping[k].first;
        filelistview.add_file_info(idx, fl[idx].extents.size(), fl[idx].severity, fl[idx].filetype,
                                   fl[idx].size, fl[idx].name);
    }
}

void
GraphWindow::on_action_view_most_severe()
{
    Clusters::file_list &fl = cl.get_files();

    std::vector<std::pair<uint64_t, double>> mapping(fl.size());
    for (unsigned int k = 0; k < fl.size(); k++) {
        mapping[k].first = k;
        mapping[k].second = fl[k].severity;
    }
    std::sort(mapping.begin(), mapping.end(),
              [](const std::pair<uint64_t, double> &a, const std::pair<uint64_t, double> &b) {
                  return a.second > b.second;
              });

    // fill filelistview widget with 'n' most fragmented (by severity)
    filelistview.clear();
    for (unsigned int k = 0; k < std::min((size_t)40, fl.size()); k++) {
        uint64_t idx = mapping[k].first;
        filelistview.add_file_info(idx, fl[idx].extents.size(), fl[idx].severity, fl[idx].filetype,
                                   fl[idx].size, fl[idx].name);
    }
}

void
GraphWindow::on_action_view_restore()
{
    fragmap.set_mode(Fragmap::FRAGMAP_MODE_SHOW_ALL);
    filelistview.clear();
    fragmap.queue_draw();
}

void
GraphWindow::on_action_main_quit()
{
    hide();
}

void
GraphWindow::on_action_main_open()
{
    Gtk::FileChooserDialog dialog("Select directory", Gtk::FILE_CHOOSER_ACTION_SELECT_FOLDER);
    dialog.set_transient_for(*this);
    dialog.add_button(Gtk::Stock::CANCEL, Gtk::RESPONSE_CANCEL);
    dialog.add_button(Gtk::Stock::OK, Gtk::RESPONSE_OK);

    int result = dialog.run();
    if (result == Gtk::RESPONSE_OK) {
        scan_dir(dialog.get_filename());
    }
}

void
GraphWindow::on_action_main_open_mountpoint()
{
    MountpointSelectDialog msd;
    int result = msd.run();
    if (result == Gtk::RESPONSE_OK) {
        scan_dir(msd.get_path());
    }
}

void
GraphWindow::scan_dir(const Glib::ustring &dir)
{
    show_directory_in_title(dir);
    filelistview.clear();
    cl.collect_fragments(dir);
    cl.create_coarse_map(2000);
    fragmap.recalculate_sizes();
    fragmap.queue_draw();
}

void
GraphWindow::show_directory_in_title(const Glib::ustring &dir)
{
    set_title(Glib::ustring("fragview - ") + dir);
}

GraphWindow::GraphWindow()
{
    show_directory_in_title("void");
    set_default_size(800, 560);
    fragmap.attach_clusters(cl);
    fragmap.attach_filelist_widget(filelistview);
    statusbar_context = statusbar.get_context_id("main");
    fragmap.attach_statusbar(&statusbar, statusbar_context);

    // set up menus
    action_group_ref = Gtk::ActionGroup::create();

    action_group_ref->add(Gtk::Action::create("MenuMain", "Fragview"));
    action_group_ref->add(Gtk::Action::create("MainOpen", "Open directory..."),
                          sigc::mem_fun(*this, &GraphWindow::on_action_main_open));
    action_group_ref->add(Gtk::Action::create("MainOpenMountpoint", "Open mountpoint..."),
                          sigc::mem_fun(*this, &GraphWindow::on_action_main_open_mountpoint));
    action_group_ref->add(Gtk::Action::create("MainQuit", "Quit"),
                          sigc::mem_fun(*this, &GraphWindow::on_action_main_quit));

    action_group_ref->add(Gtk::Action::create("MenuView", "View"));
    action_group_ref->add(Gtk::Action::create("ViewFragmented", "Most fragmented files"),
                          sigc::mem_fun(*this, &GraphWindow::on_action_view_most_fragmented));
    action_group_ref->add(Gtk::Action::create("ViewSevere", "Files with highest severity"),
                          sigc::mem_fun(*this, &GraphWindow::on_action_view_most_severe));
    action_group_ref->add(Gtk::Action::create("ViewRestore", "Restore regular view"),
                          sigc::mem_fun(*this, &GraphWindow::on_action_view_restore));

    ui_manager_ref = Gtk::UIManager::create();
    ui_manager_ref->insert_action_group(action_group_ref);
    add_accel_group(ui_manager_ref->get_accel_group());

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

    ui_manager_ref->add_ui_from_string(ui_info);
    Gtk::Widget *menubar = ui_manager_ref->get_widget("/MenuBar");

    Gtk::ScrolledWindow *scrolled_window = Gtk::manage(new Gtk::ScrolledWindow);
    scrolled_window->add(filelistview);

    Gtk::Paned *vpaned = Gtk::manage(new Gtk::Paned);
    vpaned->set_orientation(Gtk::ORIENTATION_VERTICAL);
    vpaned->pack1(fragmap, true, false);
    vpaned->pack2(*scrolled_window, true, false);

    Gtk::VBox *vbox = Gtk::manage(new Gtk::VBox);
    vbox->pack_start(*menubar, false, false);
    vbox->pack_start(*vpaned, true, true);
    vbox->pack_start(statusbar, false, false);

    add(*vbox);
    show_all_children();
}

GraphWindow::~GraphWindow()
{
}

class numpunct_spacets : public std::numpunct_byname<char>
{
public:
    numpunct_spacets(const char *name)
        : std::numpunct_byname<char>(name)
    {
    }

private:
    char
    do_thousands_sep() const
    {
        return ' ';
    }
};

int
main(int argc, char *argv[])
{
    Gtk::Main kit(argc, argv);

    // current libstdc++ implementation have a bug with UTF-8 locale
    // as it's byte-based and therefore two-byte nonbreakable space
    // character truncated to one byte.
    // Using own numpunct implementation with plain space as thousand
    // separator.
    std::locale current_locale("");
    if (std::use_facet<std::numpunct<char>>(current_locale).thousands_sep() == '\xc2') {
        std::locale modified_locale(current_locale, new numpunct_spacets(""));
        std::locale::global(modified_locale);
    } else {
        std::locale::global(current_locale);
    }

    GraphWindow window;

    std::string initial_dir;
    if (argc > 1) {
        initial_dir = argv[1];
        window.scan_dir(initial_dir);
    }

    Gtk::Main::run(window);
}
