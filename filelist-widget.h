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

    protected:
        ModelColumns columns;
        Glib::RefPtr<Gtk::ListStore> liststore;

};
