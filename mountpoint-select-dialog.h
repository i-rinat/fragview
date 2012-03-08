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
        Glib::ustring& get_path (void);

    protected:
        class ModelColumns : public Gtk::TreeModelColumnRecord
        {
            public:
            ModelColumns ()
            {
                add (mountpoint);
                add (size);
                add (used);
                add (available);
                add (type);
                add (used_percentage);
            }

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
        Glib::ustring selected_path;

        void on_list_selection_changed (void);
        void on_list_row_activated (const Gtk::TreeModel::Path& path, Gtk::TreeViewColumn* column);
        void cell_data_func_size (Gtk::CellRenderer *cell, const Gtk::TreeModel::iterator &iter,
                                  Gtk::TreeModelColumn<uint64_t> *column);
};

#endif
