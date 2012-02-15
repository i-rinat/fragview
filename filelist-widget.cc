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
