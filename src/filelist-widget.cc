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

#include "filelist-widget.hh"
#include "fragmap-widget.hh"
#include "util.hh"
#include <cassert>
#include <iomanip>
#include <iostream>

FilelistView::FilelistView()
    : fragmap_(nullptr)
{
    liststore_ = Gtk::ListStore::create(columns_);
    set_model(liststore_);

    Gtk::TreeViewColumn *column;
    Gtk::CellRendererText *renderer;

    column = get_column(append_column("Fragments", columns_.fragments) - 1);
    default_sort_order_[column] = Gtk::SORT_DESCENDING;
    view_to_model_[column] = &columns_.fragments;

    column = get_column(append_column_numeric("Severity", columns_.severity, "%.1f") - 1);
    default_sort_order_[column] = Gtk::SORT_DESCENDING;
    view_to_model_[column] = &columns_.severity;

    column = get_column(append_column("Name", columns_.name) - 1);
    set_search_column(columns_.name);
    default_sort_order_[column] = Gtk::SORT_ASCENDING;
    view_to_model_[column] = &columns_.name;

    column = get_column(append_column("Dir", columns_.dir) - 1);
    default_sort_order_[column] = Gtk::SORT_ASCENDING;
    view_to_model_[column] = &columns_.dir;

    renderer = Gtk::manage(new Gtk::CellRendererText());
    column = get_column(append_column("Type", *renderer) - 1);
    view_to_model_[column] = &columns_.filetype;
    default_sort_order_[column] = Gtk::SORT_ASCENDING;
    column->set_cell_data_func(*renderer,
                               sigc::mem_fun(*this, &FilelistView::cell_data_func_filetype));

    renderer = Gtk::manage(new Gtk::CellRendererText());
    column = get_column(append_column("Size", *renderer) - 1);
    default_sort_order_[column] = Gtk::SORT_DESCENDING;
    view_to_model_[column] = &columns_.size;
    column->set_cell_data_func(*renderer, sigc::mem_fun(*this, &FilelistView::cell_data_func_size));

    std::vector<Gtk::TreeViewColumn *> columns = get_columns();
    for (unsigned int k = 0; k < columns.size(); ++k) {
        columns[k]->set_resizable();
        columns[k]->set_reorderable();
        columns[k]->set_clickable();
        columns[k]->signal_clicked().connect(sigc::bind<Gtk::TreeViewColumn *>(
            sigc::mem_fun(*this, &FilelistView::on_filelist_header_clicked), columns[k]));
    }

    Gtk::TreeViewColumn *fake_column = Gtk::manage(new Gtk::TreeViewColumn);
    fake_column->set_max_width(0);
    append_column(*fake_column);

    // manage selection properties
    Glib::RefPtr<Gtk::TreeSelection> selection = get_selection();
    selection->set_mode(Gtk::SELECTION_MULTIPLE);
    selection->signal_changed().connect(sigc::mem_fun(*this, &FilelistView::on_selection_changed));
}

FilelistView::~FilelistView()
{
}

void
FilelistView::cell_data_func_filetype(Gtk::CellRenderer *cell, const Gtk::TreeModel::iterator &iter)
{
    Gtk::CellRendererText *renderer = dynamic_cast<Gtk::CellRendererText *>(cell);

    switch ((*iter)[columns_.filetype]) {
    case Clusters::TYPE_FILE:
        renderer->property_text() = "File";
        break;
    case Clusters::TYPE_DIR:
        renderer->property_text() = "Directory";
        break;
    }
}

void
FilelistView::cell_data_func_size(Gtk::CellRenderer *cell, const Gtk::TreeModel::iterator &iter)
{
    Gtk::CellRendererText *renderer = dynamic_cast<Gtk::CellRendererText *>(cell);
    renderer->property_text() = util::format_filesize((*iter)[columns_.size]);
}

void
FilelistView::on_filelist_header_clicked(Gtk::TreeViewColumn *column)
{
    static Gtk::TreeViewColumn *last_column = 0;
    assert(column);
    Gtk::TreeModelColumnBase *model_column = view_to_model_[column];
    assert(model_column);

    if (column == last_column) {
        if (Gtk::SORT_ASCENDING == column->get_sort_order()) {
            column->set_sort_order(Gtk::SORT_DESCENDING);
            liststore_->set_sort_column(*model_column, Gtk::SORT_DESCENDING);
        } else {
            column->set_sort_order(Gtk::SORT_ASCENDING);
            liststore_->set_sort_column(*model_column, Gtk::SORT_ASCENDING);
        }
    } else {
        // hide indicator on previous column
        if (last_column)
            last_column->set_sort_indicator(false);
        column->set_sort_indicator(true);
        Gtk::SortType sort_type = default_sort_order_[column];
        column->set_sort_order(sort_type);
        liststore_->set_sort_column(*model_column, sort_type);
    }

    last_column = column;
}

void
FilelistView::add_file_info(int id, int fragments, double severity, int filetype, uint64_t size,
                            const std::string &name, const std::string &dir)
{
    Gtk::TreeModel::Row row = *(liststore_->append());

    row[columns_.fileid] = id;
    row[columns_.fragments] = fragments;
    row[columns_.severity] = severity;
    row[columns_.filetype] = filetype;
    row[columns_.size] = size;
    row[columns_.name] = name;
    row[columns_.dir] = dir;
}

void
FilelistView::add_file_info(int id, int fragments, double severity, int filetype, uint64_t size,
                            const std::string &full_path)
{
    Gtk::TreeModel::Row row = *(liststore_->append());
    const size_t slash_pos = full_path.rfind('/');

    row[columns_.fileid] = id;
    row[columns_.fragments] = fragments;
    row[columns_.severity] = severity;
    row[columns_.filetype] = filetype;
    row[columns_.size] = size;
    row[columns_.name] = full_path.substr(slash_pos + 1, -1);
    row[columns_.dir] = full_path.substr(0, slash_pos);
}

void
FilelistView::clear()
{
    liststore_->clear();
}

void
FilelistView::on_selection_changed(void)
{
    assert(fragmap_);
    Glib::RefPtr<Gtk::TreeModel> model = get_model();
    fragmap_->file_begin();
    std::vector<Gtk::TreeModel::Path> items = get_selection()->get_selected_rows();
    for (unsigned int k = 0; k < items.size(); k++) {
        Gtk::TreeModel::Row row = *(model->get_iter(items[k]));
        fragmap_->file_add(row[columns_.fileid]);
    }
}
