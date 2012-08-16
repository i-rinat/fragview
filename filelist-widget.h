#pragma once
#ifndef __FILELIST_WIDGET_H__
#define __FILELIST_WIDGET_H__

#include <gtkmm/treeview.h>
#include <gtkmm/liststore.h>
#include <stdint.h>
#include <map>

class Fragmap;

class FilelistView : public Gtk::TreeView
{
    class ModelColumns : public Gtk::TreeModelColumnRecord {
        public:
            ModelColumns ()
            {
                add (col_fileid);
                add (col_fragments);
                add (col_severity);
                add (col_name);
                add (col_dir);
                add (col_filetype);
                add (col_size);
            };

            Gtk::TreeModelColumn<int> col_fileid;
            Gtk::TreeModelColumn<int> col_fragments;
            Gtk::TreeModelColumn<double> col_severity;
            Gtk::TreeModelColumn<Glib::ustring> col_name;
            Gtk::TreeModelColumn<Glib::ustring> col_dir;
            Gtk::TreeModelColumn<int> col_filetype;
            Gtk::TreeModelColumn<uint64_t> col_size;
    };

    public:
        FilelistView ();
        ~FilelistView ();

        void clear ();
        void add_file_info (int id, int fragments, double severity, int filetype, uint64_t size, const std::string& name, const std::string& dir);
        void add_file_info (int id, int fragments, double severity, int filetype, uint64_t size, const std::string& full_path);
        void attach_fragmap (Fragmap *fm) { fragmap = fm; }

    protected:
        ModelColumns columns;
        Glib::RefPtr<Gtk::ListStore> liststore;
        std::map<void *, enum Gtk::SortType> default_sort_order;
        std::map<void *, Gtk::TreeModelColumnBase *> view_to_model;
        Fragmap *fragmap;

        virtual void on_filelist_header_clicked (Gtk::TreeViewColumn *column);
        virtual void on_selection_changed (void);

        void cell_data_func_filetype (Gtk::CellRenderer *cell, const Gtk::TreeModel::iterator &iter);
        void cell_data_func_size (Gtk::CellRenderer *cell, const Gtk::TreeModel::iterator &iter);
};

#endif
