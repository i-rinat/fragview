#include "filelist-widget.h"
#include <iostream>

FilelistView::FilelistView ()
{
    liststore = Gtk::ListStore::create (columns);
    set_model (liststore);
    append_column ("fileid", columns.col_fileid);
    append_column ("Fragments", columns.col_fragments);
    append_column ("Severity", columns.col_severity);
    append_column ("Name", columns.col_name);
    append_column ("Dir", columns.col_dir);

    int k;
    std::vector<Gtk::TreeViewColumn *> columns = get_columns ();
    for (k = 0; k < columns.size(); ++k) {
        columns[k]->set_resizable ();
        columns[k]->set_reorderable ();
        columns[k]->set_sort_column (k);
        columns[k]->signal_clicked ().connect(
            sigc::bind<int>(sigc::mem_fun (*this, &FilelistView::on_filelist_header_clicked), k));
    }
}

FilelistView::~FilelistView ()
{

}

void
FilelistView::on_filelist_header_clicked (int column_id) {
    Gtk::TreeViewColumn *col = get_column (column_id);

    if (col->get_sort_indicator ()) {
        if (Gtk::SORT_ASCENDING == col->get_sort_order ())
            col->set_sort_order (Gtk::SORT_DESCENDING);
        else
            col->set_sort_order (Gtk::SORT_ASCENDING);
    } else {
        col->set_sort_indicator (true);
    }

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
