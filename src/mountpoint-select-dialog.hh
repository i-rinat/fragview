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

#pragma once

#include <gtkmm/dialog.h>
#include <gtkmm/liststore.h>
#include <gtkmm/treeview.h>
#include <stdint.h>

class MountpointSelectDialog : public Gtk::Dialog
{
public:
    MountpointSelectDialog();

    ~MountpointSelectDialog();

    Glib::ustring &
    get_path();

protected:
    class ModelColumns : public Gtk::TreeModelColumnRecord
    {
    public:
        ModelColumns()
        {
            add(mountpoint);
            add(size);
            add(used);
            add(available);
            add(type);
            add(used_percentage);
        }

        Gtk::TreeModelColumn<Glib::ustring> mountpoint;
        Gtk::TreeModelColumn<uint64_t> size;
        Gtk::TreeModelColumn<uint64_t> used;
        Gtk::TreeModelColumn<uint64_t> available;
        Gtk::TreeModelColumn<Glib::ustring> type;
        Gtk::TreeModelColumn<int> used_percentage;
    };

    ModelColumns columns_;
    Gtk::TreeView tv_;
    Glib::RefPtr<Gtk::ListStore> liststore_;
    Glib::ustring selected_path_;

    void
    on_list_selection_changed();

    void
    on_list_row_activated(const Gtk::TreeModel::Path &path, Gtk::TreeViewColumn *column);

    void
    cell_data_func_size(Gtk::CellRenderer *cell, const Gtk::TreeModel::iterator &iter,
                        Gtk::TreeModelColumn<uint64_t> *column);
};
