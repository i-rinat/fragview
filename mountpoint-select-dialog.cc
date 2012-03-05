#include "mountpoint-select-dialog.h"
#include <gtkmm/box.h>
#include <gtkmm/button.h>
#include <gtkmm/stock.h>
#include <gtkmm/treemodelcolumn.h>
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
    tv.append_column ("id", columns.id);
    tv.append_column ("Mount point", columns.mountpoint);
    tv.append_column ("Size", columns.size);
    tv.append_column ("Used", columns.used);
    tv.append_column ("Available", columns.available);
    tv.append_column ("Type", columns.type);
    tv.append_column ("Used %", columns.used_percentage);

    // populate
    std::ifstream m_f;
    m_f.open("/proc/mounts");
    std::string m_device, m_mountpoint, m_type, m_options, m_freq, m_passno;

    int id = 0;
    while (! m_f.eof ()) {
        m_f >> m_device >> m_mountpoint >> m_type >> m_options >> m_freq >> m_passno;
        Gtk::TreeModel::Row row = *(liststore->append ());
        row[columns.id] = id ++;
        row[columns.mountpoint] = m_mountpoint;
        row[columns.type] = m_type;

        struct statfs sfsb;
        struct stat64 sb;
        if (0 != statfs (m_mountpoint.c_str(), &sfsb)) continue;
        if (0 != lstat64 (m_mountpoint.c_str(), &sb)) continue;

        row[columns.size] = sfsb.f_blocks * sb.st_blksize;
        row[columns.used] = (sfsb.f_blocks - sfsb.f_bfree) * sb.st_blksize;
        row[columns.available] = sfsb.f_bavail * sb.st_blksize;
        if (sfsb.f_blocks == 0) sfsb.f_blocks = 1;
        row[columns.used_percentage] = 100 * (sfsb.f_blocks - sfsb.f_bfree) / sfsb.f_blocks;
    }

    m_f.close ();
}

MountpointSelectDialog::~MountpointSelectDialog (void)
{

}
