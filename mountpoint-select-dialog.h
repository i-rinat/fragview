#pragma once
#ifndef __MOUNTPOINT_SELECT_DIALOG_H__
#define __MOUNTPOINT_SELECT_DIALOG_H__

#include <gtkmm/dialog.h>
#include <gtkmm/treeview.h>
#include <stdint.h>

class MountpointSelectDialog : public Gtk::Dialog
{
    public:
        MountpointSelectDialog (void);
        ~MountpointSelectDialog (void);

    protected:
        Gtk::TreeView   tv;

        class ModelColumns : public Gtk::TreeModelColumnRecord
        {
        public:
            ModelColumns ()
            {
                add (col_id);
                add (col_mountpoint);
                add (col_size);
                add (col_free);
                add (col_available);
                add (col_type);
                add (col_available_percentage);
            }

            Gtk::TreeModelColumn<int> col_id;
            Gtk::TreeModelColumn<Glib::ustring> col_mountpoint;
            Gtk::TreeModelColumn<uint64_t> col_size;
            Gtk::TreeModelColumn<uint64_t> col_free;
            Gtk::TreeModelColumn<uint64_t> col_available;
            Gtk::TreeModelColumn<Glib::ustring> col_type;
            Gtk::TreeModelColumn<int> col_available_percentage;
        };
};

#endif
