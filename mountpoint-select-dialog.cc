#include "mountpoint-select-dialog.h"
#include <gtkmm/box.h>
#include <gtkmm/button.h>
#include <gtkmm/stock.h>
#include <gtkmm/treemodelcolumn.h>


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
    tv.append_column ("Available", columns.available_percentage);
}

MountpointSelectDialog::~MountpointSelectDialog (void)
{

}
