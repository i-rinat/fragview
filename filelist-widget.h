#pragma once
#ifndef __FILELIST_WIDGET_H__
#define __FILELIST_WIDGET_H__

#include <gtkmm/treeview.h>
#include <gtkmm/liststore.h>

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
            };

            Gtk::TreeModelColumn<int> col_fileid;
            Gtk::TreeModelColumn<int> col_fragments;
            Gtk::TreeModelColumn<double> col_severity;
            Gtk::TreeModelColumn<Glib::ustring> col_name;
            Gtk::TreeModelColumn<Glib::ustring> col_dir;
    };

    public:
        FilelistView ();
        ~FilelistView ();

        void clear ();
        void add_file_info (int id, int fragments, double severity, const std::string& name, const std::string& dir);
        void add_file_info (int id, int fragments, double severity, const std::string& full_path);

    protected:
        ModelColumns columns;
        Glib::RefPtr<Gtk::ListStore> liststore;

        virtual void on_filelist_header_clicked (int column_id);

};

#endif
