#include <iostream>
#include <iomanip>
#include <cassert>
#include "filelist-widget.hh"
#include "fragmap-widget.hh"
#include "util.hh"

FilelistView::FilelistView()
{
    fragmap = 0;

    liststore = Gtk::ListStore::create(columns);
    set_model(liststore);

    Gtk::TreeViewColumn *column;
    Gtk::CellRendererText *renderer;

    column = get_column(append_column ("Fragments", columns.col_fragments) - 1);
    default_sort_order[column] = Gtk::SORT_DESCENDING;
    view_to_model[column] = &columns.col_fragments;

    column = get_column(append_column_numeric("Severity", columns.col_severity, "%.1f") - 1);
    default_sort_order[column] = Gtk::SORT_DESCENDING;
    view_to_model[column] = &columns.col_severity;

    column = get_column(append_column("Name", columns.col_name) - 1);
    set_search_column(columns.col_name);
    default_sort_order[column] = Gtk::SORT_ASCENDING;
    view_to_model[column] = &columns.col_name;

    column = get_column(append_column("Dir", columns.col_dir) - 1);
    default_sort_order[column] = Gtk::SORT_ASCENDING;
    view_to_model[column] = &columns.col_dir;

    renderer = Gtk::manage(new Gtk::CellRendererText());
    column = get_column(append_column("Type", *renderer) - 1);
    view_to_model[column] = &columns.col_filetype;
    default_sort_order[column] = Gtk::SORT_ASCENDING;
    column->set_cell_data_func(*renderer, sigc::mem_fun(*this, &FilelistView::cell_data_func_filetype));

    renderer = Gtk::manage(new Gtk::CellRendererText());
    column = get_column(append_column ("Size", *renderer) - 1);
    default_sort_order[column] = Gtk::SORT_DESCENDING;
    view_to_model[column] = &columns.col_size;
    column->set_cell_data_func(*renderer, sigc::mem_fun(*this, &FilelistView::cell_data_func_size));

    std::vector<Gtk::TreeViewColumn *> columns = get_columns();
    for (unsigned int k = 0; k < columns.size(); ++k) {
        columns[k]->set_resizable();
        columns[k]->set_reorderable();
        columns[k]->set_clickable();
        columns[k]->signal_clicked().connect(
            sigc::bind<Gtk::TreeViewColumn *>(sigc::mem_fun (*this, &FilelistView::on_filelist_header_clicked),
                columns[k]));
    }

    Gtk::TreeViewColumn *fake_column = Gtk::manage(new Gtk::TreeViewColumn);
    fake_column->set_max_width(0);
    append_column(*fake_column);

    // manage selection properties
    Glib::RefPtr<Gtk::TreeSelection> selection = get_selection();
    selection->set_mode(Gtk::SELECTION_MULTIPLE);
    selection->signal_changed().connect(sigc::mem_fun (*this, &FilelistView::on_selection_changed));
}

FilelistView::~FilelistView ()
{

}

void
FilelistView::cell_data_func_filetype(Gtk::CellRenderer *cell, const Gtk::TreeModel::iterator &iter)
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
FilelistView::cell_data_func_size(Gtk::CellRenderer *cell, const Gtk::TreeModel::iterator &iter)
{
    Gtk::CellRendererText *renderer = dynamic_cast<Gtk::CellRendererText *>(cell);
    std::string filesize_string;

    uint64_t size = (*iter)[columns.col_size];
    Util::format_filesize(size, filesize_string);
    renderer->property_text() = filesize_string;
}

void
FilelistView::on_filelist_header_clicked(Gtk::TreeViewColumn *column)
{
    static Gtk::TreeViewColumn *last_column = 0;
    assert(column != NULL);
    Gtk::TreeModelColumnBase *model_column = view_to_model[column];
    assert(model_column != NULL);

    if (column == last_column) {
        if (Gtk::SORT_ASCENDING == column->get_sort_order()) {
            column->set_sort_order(Gtk::SORT_DESCENDING);
            liststore->set_sort_column(*model_column, Gtk::SORT_DESCENDING);
        } else {
            column->set_sort_order(Gtk::SORT_ASCENDING);
            liststore->set_sort_column(*model_column, Gtk::SORT_ASCENDING);
        }
    } else {
        // hide indicator on previous column
        if (last_column)
            last_column->set_sort_indicator(false);
        column->set_sort_indicator(true);
        Gtk::SortType sort_type = default_sort_order[column];
        column->set_sort_order(sort_type);
        liststore->set_sort_column(*model_column, sort_type);
    }

    last_column = column;
}

void
FilelistView::add_file_info(int id, int fragments, double severity, int filetype, uint64_t size,
                            const std::string& name, const std::string& dir)
{
    Gtk::TreeModel::Row row = *(liststore->append());

    row [columns.col_fileid] = id;
    row [columns.col_fragments] = fragments;
    row [columns.col_severity] = severity;
    row [columns.col_filetype] = filetype;
    row [columns.col_size] = size;
    row [columns.col_name] = name;
    row [columns.col_dir] = dir;
}

void
FilelistView::add_file_info(int id, int fragments, double severity, int filetype, uint64_t size,
                            const std::string& full_path)
{
    Gtk::TreeModel::Row row = *(liststore->append());

    row [columns.col_fileid] = id;
    row [columns.col_fragments] = fragments;
    row [columns.col_severity] = severity;
    row [columns.col_filetype] = filetype;
    row [columns.col_size] = size;

    size_t slash_pos = full_path.rfind('/');
    std::string filename = full_path.substr(slash_pos + 1, -1);
    std::string dirname = full_path.substr(0, slash_pos);

    row[columns.col_name] = filename;
    row[columns.col_dir] = dirname;
}

void
FilelistView::clear()
{
    liststore->clear();
}

void
FilelistView::on_selection_changed(void)
{
    assert(fragmap != 0);
    Glib::RefPtr<Gtk::TreeModel> model = get_model();
    fragmap->file_begin();
    std::vector<Gtk::TreeModel::Path> items = get_selection()->get_selected_rows();
    for (unsigned int k = 0; k < items.size(); ++ k) {
        Gtk::TreeModel::Row row = *(model->get_iter(items[k]));
        fragmap->file_add(row[columns.col_fileid]);
    }
}
