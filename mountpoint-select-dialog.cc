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

    Gtk::Box *box = Gtk::manage (new Gtk::Box (Gtk::ORIENTATION_HORIZONTAL));
    Gtk::Button *button_ok = Gtk::manage (new Gtk::Button (Gtk::Stock::OK));
    Gtk::Button *button_cancel = Gtk::manage (new Gtk::Button (Gtk::Stock::CANCEL));

    box->pack_end (*button_ok, Gtk::PACK_SHRINK);
    box->pack_end (*button_cancel, Gtk::PACK_SHRINK, 10);

    get_vbox ()->pack_start (tv, Gtk::PACK_EXPAND_WIDGET, 10);
    get_vbox ()->pack_start (*box, Gtk::PACK_SHRINK);

    show_all_children ();
}

MountpointSelectDialog::~MountpointSelectDialog (void)
{

}
