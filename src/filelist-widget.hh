/*
 * Copyright © 2011-2014  Rinat Ibragimov
 *
 * This file is part of "fragview" software suite.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#ifndef FRAGVIEW_FILELIST_WIDGET_HH
#define FRAGVIEW_FILELIST_WIDGET_HH

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
                add(col_fileid);
                add(col_fragments);
                add(col_severity);
                add(col_name);
                add(col_dir);
                add(col_filetype);
                add(col_size);
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
        FilelistView();
        ~FilelistView();

        void clear();
        void add_file_info(int id, int fragments, double severity, int filetype, uint64_t size,
                           const std::string& name, const std::string& dir);
        void add_file_info(int id, int fragments, double severity, int filetype, uint64_t size,
                           const std::string& full_path);
        void attach_fragmap(Fragmap *fm) { fragmap = fm; }

    protected:
        ModelColumns columns;
        Glib::RefPtr<Gtk::ListStore> liststore;
        std::map<void *, enum Gtk::SortType> default_sort_order;
        std::map<void *, Gtk::TreeModelColumnBase *> view_to_model;
        Fragmap *fragmap;

        virtual void on_filelist_header_clicked(Gtk::TreeViewColumn *column);
        virtual void on_selection_changed(void);

        void cell_data_func_filetype(Gtk::CellRenderer *cell, const Gtk::TreeModel::iterator &iter);
        void cell_data_func_size(Gtk::CellRenderer *cell, const Gtk::TreeModel::iterator &iter);
};

#endif // FRAGVIEW_FILELIST_WIDGET_HH
