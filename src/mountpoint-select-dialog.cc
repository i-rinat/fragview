#include <gtkmm/box.h>
#include <gtkmm/button.h>
#include <gtkmm/stock.h>
#include <gtkmm/treemodelcolumn.h>
#include <gtkmm/cellrendererprogress.h>
#include <iostream>
#include <fstream>
#include <sys/vfs.h>
#include "util.hh"
#include "mountpoint-select-dialog.hh"


MountpointSelectDialog::MountpointSelectDialog(void)
{
    set_title("Select mountpoint");
    set_size_request(400, 300);
    set_border_width(10);

    add_button(Gtk::Stock::CANCEL, Gtk::RESPONSE_CANCEL);
    add_button(Gtk::Stock::OK, Gtk::RESPONSE_OK);
    get_content_area()->pack_start(tv, Gtk::PACK_EXPAND_WIDGET, 10);
    show_all_children();

    // prepare table
    liststore = Gtk::ListStore::create(columns);
    tv.set_model(liststore);

    tv.append_column("Mount point", columns.mountpoint);
    tv.set_search_column(columns.mountpoint);

    {
        Gtk::CellRenderer *renderer = Gtk::manage(new Gtk::CellRendererText());
        int column_id = tv.append_column("Size", *renderer) - 1;
        tv.get_column(column_id)->set_cell_data_func(*renderer,
            sigc::bind(sigc::mem_fun(*this, &MountpointSelectDialog::cell_data_func_size),
                       &columns.size));
    }

    {
        Gtk::CellRenderer *renderer = Gtk::manage(new Gtk::CellRendererText());
        int column_id = tv.append_column("Used", *renderer) - 1;
        tv.get_column(column_id)->set_cell_data_func(*renderer,
            sigc::bind(sigc::mem_fun(*this, &MountpointSelectDialog::cell_data_func_size),
                       &columns.used));
    }

    {
        Gtk::CellRenderer *renderer = Gtk::manage(new Gtk::CellRendererText());
        int column_id = tv.append_column("Available", *renderer) - 1;
        tv.get_column(column_id)->set_cell_data_func(*renderer,
            sigc::bind(sigc::mem_fun(*this, &MountpointSelectDialog::cell_data_func_size),
                       &columns.available));
    }

    tv.append_column("Type", columns.type);

    {
        Gtk::CellRendererProgress *renderer = Gtk::manage(new Gtk::CellRendererProgress());
        int column_id = tv.append_column("Used %", *renderer) - 1;
        tv.get_column(column_id)->add_attribute(renderer->property_value(),
                                                columns.used_percentage);
    }

    tv.get_selection()->signal_changed().connect(
        sigc::mem_fun(*this, &MountpointSelectDialog::on_list_selection_changed));
    tv.signal_row_activated().connect(sigc::mem_fun(*this,
                                      &MountpointSelectDialog::on_list_row_activated));

    // populate
    std::ifstream m_f;
    m_f.open("/proc/mounts");
    std::locale clocale("C");
    m_f.imbue(clocale);
    std::string m_device, m_mountpoint, m_type, m_options, m_freq, m_passno;

    while (!m_f.eof()) {
        m_f >> m_device >> m_mountpoint >> m_type >> m_options >> m_freq >> m_passno;
        struct statfs64 sfsb;
        struct stat64 sb;
        if (0 != statfs64(m_mountpoint.c_str(), &sfsb))
            continue;
        if (0 != lstat64(m_mountpoint.c_str(), &sb))
            continue;
        if (sfsb.f_blocks == 0)
            continue; // pseudo-fs's have zero size
        if ("tmpfs" == m_type)
            continue; // tmpfs has neither FIEMAP not FIBMAP
        if ("devtmpfs" == m_type)
            continue; // the same as tmpfs

        Gtk::TreeModel::Row row = *(liststore->append());
        row[columns.mountpoint] = m_mountpoint;
        row[columns.type] = m_type;
        row[columns.size] = sfsb.f_blocks * sb.st_blksize;
        row[columns.used] = (sfsb.f_blocks - sfsb.f_bfree) * sb.st_blksize;
        row[columns.available] = sfsb.f_bavail * sb.st_blksize;
        row[columns.used_percentage] = 100 * (sfsb.f_blocks - sfsb.f_bfree) / sfsb.f_blocks;
    }

    m_f.close();
}

MountpointSelectDialog::~MountpointSelectDialog(void)
{
}

Glib::ustring&
MountpointSelectDialog::get_path(void)
{
    return selected_path;
}

void
MountpointSelectDialog::on_list_selection_changed(void)
{
    Gtk::TreeIter iter = tv.get_selection()->get_selected();
    if (0 != iter) {
        selected_path = (*iter)[columns.mountpoint];
    }
}

void
MountpointSelectDialog::on_list_row_activated(const Gtk::TreeModel::Path &path,
                                              Gtk::TreeViewColumn *column)
{
    Gtk::TreeModel::iterator iter = liststore->get_iter(path);
    if (0 != iter) {
        selected_path = (*iter)[columns.mountpoint];
        response(Gtk::RESPONSE_OK);
    }
}

void
MountpointSelectDialog::cell_data_func_size(Gtk::CellRenderer *cell,
                                            const Gtk::TreeModel::iterator &iter,
                                            Gtk::TreeModelColumn<uint64_t> *column)
{
    Gtk::CellRendererText *renderer = dynamic_cast<Gtk::CellRendererText *>(cell);
    std::string filesize_string;
    uint64_t size = (*iter)[*column];

    Util::format_filesize(size, filesize_string);
    renderer->property_text() = filesize_string;
}
