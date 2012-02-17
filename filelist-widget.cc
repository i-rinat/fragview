#include "filelist-widget.h"
#include <iostream>
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

    int k;
    std::vector<Gtk::TreeViewColumn *> columns = get_columns ();
    for (k = 0; k < columns.size(); ++k) {
        columns[k]->set_resizable ();
        columns[k]->set_reorderable ();
        columns[k]->set_clickable ();
        columns[k]->signal_clicked ().connect(
            sigc::bind<int>(sigc::mem_fun (*this, &FilelistView::on_filelist_header_clicked), k));
    }
}

FilelistView::~FilelistView ()
{

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
FilelistView::add_file_info (int id, int fragments, double severity, const std::string& name, const std::string& dir)
{
    Gtk::TreeModel::Row row = *(liststore->append ());

    row [columns.col_fileid] = id;
    row [columns.col_fragments] = fragments;
    row [columns.col_severity] = severity;
    row [columns.col_name] = name;
    row [columns.col_dir] = dir;
}

void
FilelistView::add_file_info (int id, int fragments, double severity, const std::string& full_path)
{
    Gtk::TreeModel::Row row = *(liststore->append ());

    row [columns.col_fileid] = id;
    row [columns.col_fragments] = fragments;
    row [columns.col_severity] = severity;

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
