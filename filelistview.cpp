// filelistview.cpp
#include "clusters.h"
#include "filelistview.h"
#include "gtk_fragmap.h"

GtkWidget *flv_fm = NULL;

static gboolean file_list_view_cursor_changed (GtkTreeView *tv, gpointer user_data);

static GtkSortType inverse_sort_order (GtkSortType order) {
    switch (order) {
        case GTK_SORT_ASCENDING:
            order = GTK_SORT_DESCENDING;
            break;
        case GTK_SORT_DESCENDING:
            order = GTK_SORT_ASCENDING;
            break;
    }
    return order;
}

static gboolean file_list_view_header_clicked (GtkTreeViewColumn *col, gpointer name) {

    printf("++++ clicked '%s'\n", gtk_tree_view_column_get_title (col) );

    gboolean was_selected = gtk_tree_view_column_get_sort_indicator (col);
    gtk_tree_view_column_set_sort_indicator (col, TRUE);

    if (was_selected) {
        GtkSortType order = gtk_tree_view_column_get_sort_order (col);
        gtk_tree_view_column_set_sort_order (col, inverse_sort_order(order));
    }

    // disable others
    GList *columns, *p;
    GtkTreeView *tv = GTK_TREE_VIEW (gtk_tree_view_column_get_tree_view (col));

    p = columns = gtk_tree_view_get_columns (tv);
    while (p) {
        GtkTreeViewColumn *other_col = (GtkTreeViewColumn *) p->data;
        if (col != other_col) {
            gtk_tree_view_column_set_sort_indicator (other_col, FALSE);
        }
        p = g_list_next (p);
    }
    g_list_free (columns);

    return TRUE;
}

GtkWidget *file_list_view_new () {
    GtkWidget *file_list_view;
    GtkTreeViewColumn *column;
    GtkTreeView *tv;
    GList *p, *column_list;
    int k;

    file_list_view = gtk_tree_view_new ();
    tv = GTK_TREE_VIEW (file_list_view);

    column = gtk_tree_view_column_new ();
    gtk_tree_view_column_set_visible (column, false); // pointer
    gtk_tree_view_insert_column (tv, column, -1);

    gtk_tree_view_insert_column_with_attributes( tv, -1, "Fragments",
        gtk_cell_renderer_text_new(), "text", FILELISTVIEW_COL_FRAG,
        NULL);
    gtk_tree_view_insert_column_with_attributes( tv, -1, "Name",
        gtk_cell_renderer_text_new(), "text", FILELISTVIEW_COL_NAME,
        NULL);
    gtk_tree_view_insert_column_with_attributes( tv, -1, "Directory",
        gtk_cell_renderer_text_new(), "text", FILELISTVIEW_COL_DIR,
        NULL);

    // set column properties
    k = 0;
    while ((column = gtk_tree_view_get_column (tv, k))) {
        gtk_tree_view_column_set_sort_column_id (column, k);
        gtk_tree_view_column_set_resizable (column, true);
        gtk_tree_view_column_set_reorderable (column, true);
        // header click signal
        g_signal_connect (GTK_OBJECT(column), "clicked",
                GTK_SIGNAL_FUNC (file_list_view_header_clicked), NULL);
        k++;
    }

    // prepare GtkListStore
    GtkListStore  *store = file_list_model_new ();
    gtk_tree_view_set_model (GTK_TREE_VIEW(file_list_view), GTK_TREE_MODEL(store));
    g_object_unref (store);

    gtk_tree_view_set_headers_clickable(GTK_TREE_VIEW(file_list_view), TRUE);
    gtk_tree_view_set_reorderable(GTK_TREE_VIEW(file_list_view), TRUE);

    // other signals
    g_signal_connect (GTK_OBJECT (file_list_view), "cursor-changed",
            GTK_SIGNAL_FUNC (file_list_view_cursor_changed), NULL);

    return file_list_view;
}

GtkListStore *file_list_model_new() {
    GtkListStore  *store;
    store = gtk_list_store_new (
        FILELISTVIEW_NUM_COLS,
        G_TYPE_INT,             // pointer
        G_TYPE_UINT,            // fragments
        G_TYPE_STRING,          // name
        G_TYPE_STRING           // dir
    );
    return store;
}

static gboolean file_list_view_cursor_changed (GtkTreeView *tv, gpointer user_data)
{
    GtkTreeSelection *sel;
    GtkTreeModel *model;
    GtkTreeIter iter;
    GtkFragmap *fm = GTK_FRAGMAP (flv_fm);
    sel = gtk_tree_view_get_selection (tv);

    if (gtk_tree_selection_get_selected (sel, &model, &iter)) {
        GValue value = {0, };

        gtk_tree_model_get_value (model, &iter, FILELISTVIEW_COL_POINTER, &value);
        int file_idx = g_value_get_int(&value);

        gtk_fragmap_file_begin (fm);
        gtk_fragmap_file_add (fm, file_idx);
        gtk_fragmap_set_mode (fm, FRAGMAP_MODE_FILE);
    }

    return TRUE;
}

void file_list_view_attach_fragmap(GtkWidget *w) {
    flv_fm = w;
}

