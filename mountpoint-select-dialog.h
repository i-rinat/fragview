#pragma once
#ifndef __MOUNTPOINT_SELECT_DIALOG_H__
#define __MOUNTPOINT_SELECT_DIALOG_H__

#include <gtkmm/dialog.h>
#include <gtkmm/treeview.h>
#include <gtkmm/liststore.h>
#include <stdint.h>

class MountpointSelectDialog : public Gtk::Dialog
{
    public:
        MountpointSelectDialog (void);
        ~MountpointSelectDialog (void);

    protected:
        class ModelColumns : public Gtk::TreeModelColumnRecord
        {
        public:
            ModelColumns ()
            {
                add (id);
                add (mountpoint);
                add (size);
                add (used);
                add (available);
                add (type);
                add (used_percentage);
            }

            Gtk::TreeModelColumn<int> id;
            Gtk::TreeModelColumn<Glib::ustring> mountpoint;
            Gtk::TreeModelColumn<uint64_t> size;
            Gtk::TreeModelColumn<uint64_t> used;
            Gtk::TreeModelColumn<uint64_t> available;
            Gtk::TreeModelColumn<Glib::ustring> type;
            Gtk::TreeModelColumn<int> used_percentage;
        };

        ModelColumns columns;
        Gtk::TreeView   tv;
        Glib::RefPtr<Gtk::ListStore> liststore;
};

#endif
