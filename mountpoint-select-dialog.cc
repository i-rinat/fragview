#include "mountpoint-select-dialog.h"
#include <gtkmm/box.h>
#include <gtkmm/button.h>
#include <gtkmm/stock.h>
#include <gtkmm/treemodelcolumn.h>
#include <gtkmm/cellrendererprogress.h>
#include <iostream>
#include <fstream>
#include <sys/vfs.h>

MountpointSelectDialog::MountpointSelectDialog (void)
{
    set_title ("Select mountpoint");
    set_size_request (400, 300);
    set_border_width (10);

    Gtk::Button *button_cancel = add_button (Gtk::Stock::CANCEL, Gtk::RESPONSE_CANCEL);
    Gtk::Button *button_ok = add_button (Gtk::Stock::OK, Gtk::RESPONSE_OK);
    get_vbox ()->pack_start (tv, Gtk::PACK_EXPAND_WIDGET, 10);
    show_all_children ();

    // prepare table
    liststore = Gtk::ListStore::create (columns);
    tv.set_model (liststore);
    tv.append_column ("Mount point", columns.mountpoint);
    tv.append_column ("Size", columns.size);
    tv.append_column ("Used", columns.used);
    tv.append_column ("Available", columns.available);
    tv.append_column ("Type", columns.type);

    Gtk::CellRendererProgress *renderer = Gtk::manage (new Gtk::CellRendererProgress ());
    int column_id = tv.append_column ("Used %", *renderer) - 1;
    tv.get_column (column_id)->add_attribute (renderer->property_value (), columns.used_percentage);

    tv.get_selection ()->signal_changed ().connect (
        sigc::mem_fun (*this, &MountpointSelectDialog::on_list_selection_changed));

    // populate
    std::ifstream m_f;
    m_f.open("/proc/mounts");
    std::string m_device, m_mountpoint, m_type, m_options, m_freq, m_passno;

    while (! m_f.eof ()) {
        m_f >> m_device >> m_mountpoint >> m_type >> m_options >> m_freq >> m_passno;
        struct statfs sfsb;
        struct stat64 sb;
        if (0 != statfs (m_mountpoint.c_str(), &sfsb)) continue;
        if (0 != lstat64 (m_mountpoint.c_str(), &sb)) continue;
        if (sfsb.f_blocks == 0) continue; // pseudo-fs's have zero size
        if ("tmpfs" == m_type) continue; // tmpfs has neither FIEMAP not FIBMAP
        if ("devtmpfs" == m_type) continue; // the same as tmpfs

        Gtk::TreeModel::Row row = *(liststore->append ());
        row[columns.mountpoint] = m_mountpoint;
        row[columns.type] = m_type;
        row[columns.size] = sfsb.f_blocks * sb.st_blksize;
        row[columns.used] = (sfsb.f_blocks - sfsb.f_bfree) * sb.st_blksize;
        row[columns.available] = sfsb.f_bavail * sb.st_blksize;
        row[columns.used_percentage] = 100 * (sfsb.f_blocks - sfsb.f_bfree) / sfsb.f_blocks;
    }

    m_f.close ();
}

MountpointSelectDialog::~MountpointSelectDialog (void)
{

}

Glib::ustring&
MountpointSelectDialog::get_path (void)
{
    return selected_path;
}

void
MountpointSelectDialog::on_list_selection_changed (void)
{
    Gtk::TreeModel::Row row = *(tv.get_selection ()->get_selected ());
    selected_path = row[columns.mountpoint];
}
