// filelistview.h

#ifndef __FILELISTVIEW_H__
#define __FILELISTVIEW_H__

#include <gtk/gtk.h>

G_BEGIN_DECLS

enum {
    FILELISTVIEW_COL_POINTER = 0,
    FILELISTVIEW_COL_FRAG,
    FILELISTVIEW_COL_NAME,
    FILELISTVIEW_COL_DIR,
    FILELISTVIEW_NUM_COLS
};


GtkWidget *file_list_view_new ();
GtkListStore *file_list_model_new ();
void file_list_view_attach_fragmap(GtkWidget *w);

G_END_DECLS

#endif
