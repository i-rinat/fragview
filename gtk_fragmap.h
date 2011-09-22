#ifndef __GTK_FRAGMAP_H__
#define __GTK_FRAGMAP_H__
#include "clusters.h"

G_BEGIN_DECLS

typedef struct _GtkFragmap        GtkFragmap;
typedef struct _GtkFragmapClass   GtkFragmapClass;

enum FRAGMAP_MODE { 
    FRAGMAP_MODE_SHOW_ALL = 0,
    FRAGMAP_MODE_CLUSTER,
    FRAGMAP_MODE_FILE,
    FRAGMAP_MODE_FILEGROUP
};

struct _GtkFragmap {
    GtkDrawingArea parent;

    cluster_list *clusters;
    file_list *files;
    pthread_mutex_t *clusters_mutex;
    pthread_mutex_t *files_mutex;
    int cluster_count;
    int cluster_size_desired; // desired number of blocks each cluster contains
    __u64 device_size_in_blocks;
    int force_redraw;
    int box_size;
    int frag_limit;

    int target_cluster;

    FRAGMAP_MODE display_mode;
    int selected_cluster;
    GList *selected_files;

    int shift_x;
    int shift_y;

    GtkWidget *file_list_view;
    void (*update_file_list) ( GtkWidget *file_list, GtkTreeModel *model);
};

struct _GtkFragmapClass {
    GtkDrawingAreaClass parent_class;
};

#define GTK_FRAGMAP(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), gtk_fragmap_get_type(), GtkFragmap))
#define GTK_FRAGMAP_CLASS(obj)       (G_TYPE_CHECK_CLASS_CAST ((obj), gtk_fragmap_get_type(),  GtkFragmapClass))
#define GTK_IS_FRAGMAP(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), gtk_fragmap_get_type()))
#define GTK_IS_FRAGMAP_CLASS(obj)    (G_TYPE_CHECK_CLASS_TYPE ((obj), gtk_fragmap_get_type()))
#define GTK_FRAGMAP_GET_CLASS        (G_TYPE_INSTANCE_GET_CLASS ((obj), gtk_fragmap_get_type(), GtkFragmapClass))

GtkWidget *gtk_fragmap_new (void);
static gboolean gtk_fragmap_expose (GtkWidget *fragmap, GdkEventExpose *event);
static gboolean gtk_fragmap_size_allocate (GtkWidget *widget, GdkRectangle *allocation);
static void gtk_fragmap_class_init (GtkFragmapClass *klass);
GType gtk_fragmap_get_type (void) G_GNUC_CONST;

int gtk_fragmap_get_cluster_count(GtkFragmap *fragmap);
void gtk_fragmap_attach_cluster_list(GtkFragmap *fragmap, cluster_list *cl,
                                        pthread_mutex_t *cl_m);
void gtk_fragmap_attach_file_list(GtkFragmap *fragmap, file_list *fl,
                                        pthread_mutex_t *fl_m);

void gtk_fragmap_set_device_size(GtkFragmap *fragmap, __u64 sib);

void gtk_fragmap_attach_widget_file_list(GtkFragmap *fm, GtkWidget *w,
            void (*update)(GtkWidget *, GtkTreeModel *));

void gtk_fragmap_file_begin (GtkFragmap *fm);
void gtk_fragmap_file_add (GtkFragmap *fm, int file_idx);
void gtk_fragmap_set_mode (GtkFragmap *fm, enum FRAGMAP_MODE);


G_END_DECLS

#endif /* __GTK_FRAGMAP_H__ */
