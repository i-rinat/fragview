// filelistview.cpp
#include "clusters.h"
#include "filelistview.h"
#include "gtk_fragmap.h"

GtkWidget *flv_fm = NULL;

static gboolean file_list_view_cursor_changed (GtkTreeView *tv, gpointer user_data);

GtkWidget *file_list_view_new () {
	
	GtkWidget *file_list_view;
	GtkTreeViewColumn *column;
	file_list_view = gtk_tree_view_new ();

	column = gtk_tree_view_column_new ();
	gtk_tree_view_column_set_visible (column, false);
//	gtk_tree_view_column_set_title (column, "pointer");
	gtk_tree_view_insert_column (GTK_TREE_VIEW(file_list_view), column, -1);
	
	gtk_tree_view_insert_column_with_attributes( GTK_TREE_VIEW(file_list_view),
		-1,
		"Fragments",
		gtk_cell_renderer_text_new(),
		"text", FILELISTVIEW_COL_FRAG,
		NULL);
	gtk_tree_view_insert_column_with_attributes( GTK_TREE_VIEW(file_list_view),
		-1,
		"Name",
		gtk_cell_renderer_text_new(),
		"text", FILELISTVIEW_COL_NAME,
		NULL);
	gtk_tree_view_insert_column_with_attributes( GTK_TREE_VIEW(file_list_view),
		-1,
		"Directory",
		gtk_cell_renderer_text_new(),
		"text", FILELISTVIEW_COL_DIR,
		NULL);
		
	// set column properties
	column = gtk_tree_view_get_column (GTK_TREE_VIEW(file_list_view), FILELISTVIEW_COL_FRAG);
	gtk_tree_view_column_set_resizable (column, true);

	column = gtk_tree_view_get_column (GTK_TREE_VIEW(file_list_view), FILELISTVIEW_COL_NAME);
	gtk_tree_view_column_set_resizable (column, true);

	column = gtk_tree_view_get_column (GTK_TREE_VIEW(file_list_view), FILELISTVIEW_COL_DIR);
	gtk_tree_view_column_set_resizable (column, true);
	
	// prepare GtkListStore
	GtkListStore  *store = file_list_model_new ();
	gtk_tree_view_set_model (GTK_TREE_VIEW(file_list_view), GTK_TREE_MODEL(store));
	g_object_unref (store);
	
	gtk_tree_view_set_headers_clickable(GTK_TREE_VIEW(file_list_view), TRUE);
	gtk_tree_view_set_reorderable(GTK_TREE_VIEW(file_list_view), TRUE);

	// signals
	g_signal_connect (GTK_OBJECT (file_list_view), "cursor-changed",
			GTK_SIGNAL_FUNC (file_list_view_cursor_changed), NULL);
	
	return file_list_view;
}

GtkListStore *file_list_model_new() {
	GtkListStore  *store;
	store = gtk_list_store_new (
		FILELISTVIEW_NUM_COLS,
		G_TYPE_POINTER,			// pointer
		G_TYPE_UINT,			// fragments
		G_TYPE_STRING,			// name
		G_TYPE_STRING			// dir
	);
	return store;
}

static gboolean file_list_view_cursor_changed (GtkTreeView *tv, gpointer user_data)
{
//	printf("something activated.\n");
	GtkTreeSelection *sel;
	GtkTreeModel *model;
	GtkTreeIter iter;
	sel = gtk_tree_view_get_selection (tv);
	
	if (gtk_tree_selection_get_selected (sel, &model, &iter)) {
		printf("there is selected node\n");
		GValue value = {0, };
		//g_value_init(&value, G_TYPE_UINT);
		gtk_tree_model_get_value (model, &iter, FILELISTVIEW_COL_POINTER, &value);
		
		printf("value = %p, ", g_value_get_pointer(&value));
		f_info *fi = *(f_info **)g_value_get_pointer(&value);
		printf("name = %s\n", fi->name.c_str());
		
		gtk_fragmap_file_begin (GTK_FRAGMAP (flv_fm));
		gtk_fragmap_file_add (GTK_FRAGMAP (flv_fm), fi);
		gtk_fragmap_set_mode (GTK_FRAGMAP (flv_fm), FRAGMAP_MODE_FILE);
	}

	return TRUE;
}

void file_list_view_attach_fragmap(GtkWidget *w) {
	flv_fm = w;
}

