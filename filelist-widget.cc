#define __STDC_LIMIT_MACROS
#include "filelist-widget.h"
#include <iostream>
#include <iomanip>
#include <cassert>
#include "fragmap-widget.h"

FilelistView::FilelistView ()
{
    fragmap = 0;

    liststore = Gtk::ListStore::create (columns);
    set_model (liststore);
    default_sort_order [append_column ("fileid", columns.col_fileid) - 1] = Gtk::SORT_ASCENDING;
    default_sort_order [append_column ("Fragments", columns.col_fragments) - 1] = Gtk::SORT_DESCENDING;
    default_sort_order [append_column ("Severity", columns.col_severity) - 1] = Gtk::SORT_DESCENDING;
    default_sort_order [append_column ("Name", columns.col_name) - 1] = Gtk::SORT_ASCENDING;
    default_sort_order [append_column ("Dir", columns.col_dir) - 1] = Gtk::SORT_ASCENDING;

    int column_id;
    Gtk::CellRendererText *renderer;

    renderer = Gtk::manage (new Gtk::CellRendererText ());
    column_id = append_column ("Type", *renderer) - 1;
    default_sort_order [column_id] = Gtk::SORT_ASCENDING;
    get_column (column_id)->set_cell_data_func (*renderer, sigc::mem_fun (*this, &FilelistView::cell_data_func_filetype));

    renderer = Gtk::manage (new Gtk::CellRendererText ());
    column_id = append_column ("Size", *renderer) - 1;
    default_sort_order [column_id] = Gtk::SORT_DESCENDING;
    get_column (column_id)->set_cell_data_func (*renderer, sigc::mem_fun (*this, &FilelistView::cell_data_func_size));

    std::vector<Gtk::TreeViewColumn *> columns = get_columns ();
    for (unsigned int k = 0; k < columns.size(); ++k) {
        columns[k]->set_resizable ();
        columns[k]->set_reorderable ();
        columns[k]->set_clickable ();
        columns[k]->signal_clicked ().connect(
            sigc::bind<int>(sigc::mem_fun (*this, &FilelistView::on_filelist_header_clicked), k));
    }

    // manage selection properties
    Glib::RefPtr<Gtk::TreeSelection> selection = get_selection ();
    selection->set_mode (Gtk::SELECTION_MULTIPLE);
    selection->signal_changed ().connect (sigc::mem_fun (*this, &FilelistView::on_selection_changed));
}

FilelistView::~FilelistView ()
{

}

void
FilelistView::cell_data_func_filetype (Gtk::CellRenderer *cell, const Gtk::TreeModel::iterator &iter)
{
    Gtk::CellRendererText *renderer = dynamic_cast<Gtk::CellRendererText *>(cell);

    switch ((*iter)[columns.col_filetype]) {
        case Clusters::TYPE_FILE:
            renderer->property_text() = "File";
            break;
        case Clusters::TYPE_DIR:
            renderer->property_text() = "Directory";
            break;
    }
}

void
FilelistView::cell_data_func_size (Gtk::CellRenderer *cell, const Gtk::TreeModel::iterator &iter)
{
    Gtk::CellRendererText *renderer = dynamic_cast<Gtk::CellRendererText *>(cell);

    uint64_t size = (*iter)[columns.col_size];
    std::stringstream ss;

    if (size < 1024) {
        ss << size << " B";
        renderer->property_text() = ss.str();
        return;
    }

    ss << std::fixed << std::setprecision(1);

    if (size < __UINT64_C(1048576)) {
        ss << (double)size/__UINT64_C(1024) << " kiB";
    } else if (size < __UINT64_C(1073741824)) {
        ss << (double)size/__UINT64_C(1048576) << " MiB";
    } else if (size < __UINT64_C(1099511627776)) {
        ss << (double)size/__UINT64_C(1073741824) << " GiB";
    } else {
        ss << (double)size/__UINT64_C(1099511627776) << " TiB";
    }

    renderer->property_text() = ss.str();
}

void
FilelistView::on_filelist_header_clicked (int column_id)
{
    static int last_column_id = 0;

    Gtk::TreeViewColumn *col = get_column (column_id);
    if (column_id == last_column_id) {
        if (Gtk::SORT_ASCENDING == col->get_sort_order ()) {
            col->set_sort_order (Gtk::SORT_DESCENDING);
            liststore->set_sort_column (column_id, Gtk::SORT_DESCENDING);
        } else {
            col->set_sort_order (Gtk::SORT_ASCENDING);
            liststore->set_sort_column (column_id, Gtk::SORT_ASCENDING);
        }
    } else {
        // hide indicator on previous column
        get_column (last_column_id)->set_sort_indicator (false);

        col->set_sort_indicator (true);
        Gtk::SortType sort_type = default_sort_order [column_id];
        col->set_sort_order (sort_type);
        liststore->set_sort_column (column_id, sort_type);
    }

    last_column_id = column_id;
}

void
FilelistView::add_file_info (int id, int fragments, double severity, int filetype, uint64_t size, const std::string& name, const std::string& dir)
{
    Gtk::TreeModel::Row row = *(liststore->append ());

    row [columns.col_fileid] = id;
    row [columns.col_fragments] = fragments;
    row [columns.col_severity] = severity;
    row [columns.col_filetype] = filetype;
    row [columns.col_size] = size;
    row [columns.col_name] = name;
    row [columns.col_dir] = dir;
}

void
FilelistView::add_file_info (int id, int fragments, double severity, int filetype, uint64_t size, const std::string& full_path)
{
    Gtk::TreeModel::Row row = *(liststore->append ());

    row [columns.col_fileid] = id;
    row [columns.col_fragments] = fragments;
    row [columns.col_severity] = severity;
    row [columns.col_filetype] = filetype;
    row [columns.col_size] = size;

    size_t slash_pos = full_path.rfind ('/');
    std::string filename = full_path.substr (slash_pos + 1, -1);
    std::string dirname = full_path.substr(0, slash_pos);

    row [columns.col_name] = filename;
    row [columns.col_dir] = dirname;
}

void
FilelistView::clear ()
{
    liststore->clear ();
}

void
FilelistView::on_selection_changed (void)
{
    assert (fragmap != 0);
    Glib::RefPtr<Gtk::TreeModel> model = get_model ();
    fragmap->file_begin ();
    std::vector<Gtk::TreeModel::Path> items = get_selection ()->get_selected_rows ();
    for (unsigned int k = 0; k < items.size(); ++ k) {
        Gtk::TreeModel::Row row = *(model->get_iter (items[k]));
        fragmap->file_add (row [columns.col_fileid]);
    }
}
