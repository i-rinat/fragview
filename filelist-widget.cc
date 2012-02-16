#include "filelist-widget.h"

FilelistView::FilelistView ()
{
    liststore = Gtk::ListStore::create (columns);
    set_model (liststore);
    append_column ("fileid", columns.col_fileid);
    append_column ("Fragments", columns.col_fragments);
    append_column ("Severity", columns.col_severity);
    append_column ("Name", columns.col_name);
    append_column ("Dir", columns.col_dir);

    set_headers_clickable (true);

    for (int k = 1; k < 15; k ++) {
        Gtk::TreeModel::Row row = *(liststore->append());
        row[columns.col_fileid] = k;
    }
}

FilelistView::~FilelistView ()
{

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
