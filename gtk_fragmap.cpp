#include <gtk/gtk.h>
#include <math.h>
#include "gtk_fragmap.h"
#include "filelistview.h"

#include <sys/time.h>
#include <iostream>

static double color_free[3] = {1.0, 1.0, 1.0};
static double color_free_selected[3] = {1.0, 1.0, 0.0};
static double color_frag[3] = {0.8, 0.0, 0.0};
static double color_nfrag[3] = {0.0, 0.0, 0.8};
static double color_back[3] = {0.25, 0.25, 0.25};

static double bleach_factor = 0.3;
static double color_free_bleached[3];
static double color_frag_bleached[3];
static double color_nfrag_bleached[3];
static double color_back_bleached[3];

static void bleach_colors (void) {
    for (int k = 0; k < 3; k ++) {
        color_free_bleached[k] = 1.0 - (1.0 - color_free[k]) * bleach_factor;
        color_frag_bleached[k] = 1.0 - (1.0 - color_frag[k]) * bleach_factor;
        color_nfrag_bleached[k] = 1.0 - (1.0 - color_nfrag[k]) * bleach_factor;
        color_back_bleached[k] = 1.0 - (1.0 - color_back[k]) * bleach_factor;
    }
}

static void gtk_fragmap_class_init (GtkFragmapClass *klass) {
    GtkWidgetClass *widget_class;
    widget_class = GTK_WIDGET_CLASS (klass);
    widget_class->expose_event = gtk_fragmap_expose;
}

static gboolean gtk_fragmap_size_allocate (GtkWidget *widget, GdkRectangle *allocation) {
    GtkFragmap *fm = GTK_FRAGMAP(widget);
    fm->display_mode = FRAGMAP_MODE_SHOW_ALL;
    return TRUE;
}

static gboolean gtk_fragmap_highligh_cluster_at (GtkWidget *widget, gdouble x, gdouble y) {

    GtkFragmap *fm = GTK_FRAGMAP (widget);

    pthread_mutex_lock (fm->clusters_mutex);
    pthread_mutex_lock (fm->files_mutex);

    gboolean flag_update = FALSE;
    int cl_x = (int) (x - fm->shift_x) / fm->box_size;
    int cl_y = (int) (y - fm->shift_y) / fm->box_size;
    int clusters_per_row = widget->allocation.width / fm->box_size;

    int cl_raw = cl_y * clusters_per_row + cl_x;

    if (cl_raw >= fm->clusters->size())
        cl_raw = fm->clusters->size() - 1;

    // printf("clicked cluster # %d\n", cl_raw);
    cluster_info *ci = &(fm->clusters->at(cl_raw));

    if ( fm->display_mode != FRAGMAP_MODE_CLUSTER || fm->selected_cluster != cl_raw ) {
        fm->display_mode = FRAGMAP_MODE_CLUSTER;
        fm->selected_cluster = cl_raw;

        // printf("clusters total = %lu\n", fm->clusters->size());

        int ss = ci->files.size();

        GtkListStore  *store;
        GtkTreeIter    iter;
        store = file_list_model_new();

        for (int k = 0; k < ci->files.size(); ++k) {
            std::cout << fm->files->at(ci->files[k]).name << "\n";

            size_t q = fm->files->at(ci->files[k]).name.rfind('/');
            std::string filename = fm->files->at(ci->files[k]).name.substr(q+1, -1);
            std::string dirname = fm->files->at(ci->files[k]).name.substr(0, q);

            gtk_list_store_append (store, &iter);
            gtk_list_store_set (store, &iter,
                            FILELISTVIEW_COL_FRAG, fm->files->at(ci->files[k]).extents.size(),
                            FILELISTVIEW_COL_NAME, filename.c_str(),
                            FILELISTVIEW_COL_SEVERITY, fm->files->at(ci->files[k]).severity,
                            FILELISTVIEW_COL_DIR, dirname.c_str(),
                            FILELISTVIEW_COL_POINTER, ci->files[k],
                            -1);
        }

        if (fm->update_file_list) {
            fm->update_file_list (fm->file_list_view, GTK_TREE_MODEL(store));
        }
        g_object_unref (store);
        flag_update = TRUE;
    }

    pthread_mutex_unlock (fm->files_mutex);
    pthread_mutex_unlock (fm->clusters_mutex);
    return flag_update;
}

static gboolean gtk_fragmap_motion_event (GtkWidget *widget, GdkEventMotion *event, gboolean user_data) {

    if (event->state & GDK_BUTTON1_MASK) {
        if (gtk_fragmap_highligh_cluster_at(widget, event->x, event->y)) {
            gtk_widget_queue_draw (widget);
        }
    }

    return TRUE;
}


static gboolean gtk_fragmap_button_press_event (GtkWidget *widget, GdkEventButton *event, gboolean user_data) {

    if (event->button == 1) { // left mouse button
        if (gtk_fragmap_highligh_cluster_at(widget, event->x, event->y)) {
            gtk_widget_queue_draw (widget);
        }
    }

    return TRUE;
}

static void gtk_fragmap_init (GtkFragmap *fm) {
    fm->cluster_count = 0;
    fm->clusters = NULL;
    fm->force_redraw = 0;
    fm->shift_x = 0;
    fm->shift_y = 0;
    fm->box_size = 7;
    fm->frag_limit = 1;

    fm->display_mode = FRAGMAP_MODE_SHOW_ALL;
    fm->selected_cluster = -1;
    fm->selected_files = NULL;

    fm->file_list_view = NULL;
    fm->update_file_list = NULL;

    gtk_widget_set_events(GTK_WIDGET(fm),
        GDK_BUTTON_PRESS_MASK | GDK_POINTER_MOTION_MASK);

    g_signal_connect (GTK_OBJECT(fm), "motion-notify-event",
        GTK_SIGNAL_FUNC (gtk_fragmap_motion_event), NULL);

    g_signal_connect (GTK_OBJECT(fm), "button_press_event",
        GTK_SIGNAL_FUNC (gtk_fragmap_button_press_event), NULL);

    g_signal_connect (GTK_OBJECT(fm), "size_allocate",
        GTK_SIGNAL_FUNC (gtk_fragmap_size_allocate), NULL);

    bleach_colors ();
}

GtkWidget *gtk_fragmap_new (void) {
    return (GtkWidget *)g_object_new (gtk_fragmap_get_type(), NULL);
}

static void cairo_set_source_rgbv (cairo_t *cr, double const color[]) {
    cairo_set_source_rgb (cr, color[0], color[1], color[2]);
}

static gboolean gtk_fragmap_expose (GtkWidget *widget, GdkEventExpose *event) {
    cairo_t *cr;

    GtkFragmap *fm = GTK_FRAGMAP(widget);

    cr = gdk_cairo_create (widget->window);
    cairo_rectangle (cr, event->area.x, event->area.y,
                        event->area.width, event->area.height);
    cairo_clip (cr);

    switch (fm->display_mode) {
        case FRAGMAP_MODE_SHOW_ALL:
        default:
            cairo_set_source_rgbv (cr, color_back);
            break;
        case FRAGMAP_MODE_CLUSTER:
        case FRAGMAP_MODE_FILE:
            cairo_set_source_rgbv (cr, color_back_bleached);
            break;
    }

    cairo_rectangle (cr, 0, 0, widget->allocation.width, widget->allocation.height);
    cairo_fill (cr);

    int box_size = fm->box_size;

    int pix_width = widget->allocation.width;
    int pix_height = widget->allocation.height;
    int cluster_width = (pix_width - 1) / (box_size);
    int cluster_height = (pix_height - 1) / (box_size);


    // center image
    fm->shift_x = (pix_width - cluster_width*box_size)/2;
    fm->shift_y = (pix_height - cluster_height*box_size)/2;
    fm->shift_y = 0;
    fm->shift_x = 0;
    cairo_translate(cr, fm->shift_x, fm->shift_y);

    printf("clusters_total = %d\n", cluster_width * cluster_height);

    struct timeval tv1, tv2;

    gettimeofday(&tv1, NULL);
    if (fm->cluster_count != cluster_width * cluster_height ||
        fm->force_redraw )
    {
        pthread_mutex_lock(fm->clusters_mutex);
        pthread_mutex_lock(fm->files_mutex);
        __fill_clusters(&files2,
                        fm->device_size_in_blocks,
                        fm->clusters,
                        cluster_width * cluster_height,
                        fm->frag_limit);
        fm->cluster_count = cluster_width * cluster_height;
        fm->force_redraw = 0;
        pthread_mutex_unlock(fm->clusters_mutex);
        pthread_mutex_unlock(fm->files_mutex);
    }
    gettimeofday(&tv2, NULL);
    printf("fill_clusters: %f sec\n", tv2.tv_sec-tv1.tv_sec+(tv2.tv_usec-tv1.tv_usec)/1000000.0);

    gettimeofday(&tv1, NULL);
    int ky, kx;

    pthread_mutex_lock(fm->clusters_mutex);

    cluster_list *cl = fm->clusters;

    switch (fm->display_mode) {
        case FRAGMAP_MODE_SHOW_ALL:
        default:
            cairo_set_source_rgbv (cr, color_free);
            break;
        case FRAGMAP_MODE_CLUSTER:
        case FRAGMAP_MODE_FILE:
            cairo_set_source_rgbv (cr, color_free_bleached);
            break;
    }

    for (ky = 0; ky < cluster_height; ++ky) {
        for (kx = 0; kx < cluster_width; ++kx) {
            if (cl->at(ky*cluster_width+kx).free) {
                cairo_rectangle (cr, kx * box_size, ky * box_size ,
                    box_size - 1, box_size - 1);
            }
        }
    }
    cairo_fill (cr);

    switch (fm->display_mode) {
        case FRAGMAP_MODE_SHOW_ALL:
        default:
            cairo_set_source_rgbv (cr, color_frag);
            break;
        case FRAGMAP_MODE_CLUSTER:
        case FRAGMAP_MODE_FILE:
            cairo_set_source_rgbv (cr, color_frag_bleached);
            break;
    }

    for (ky = 0; ky < cluster_height; ++ky) {
        for (kx = 0; kx < cluster_width; ++kx) {
            if (!(cl->at(ky*cluster_width+kx).free) &&
                cl->at(ky*cluster_width+kx).fragmented)
            {
                cairo_rectangle (cr, kx * box_size, ky * box_size ,
                    box_size - 1, box_size - 1);
            }
        }
    }
    cairo_fill (cr);

    switch (fm->display_mode) {
        case FRAGMAP_MODE_SHOW_ALL:
        default:
            cairo_set_source_rgbv (cr, color_nfrag);
            break;
        case FRAGMAP_MODE_CLUSTER:
        case FRAGMAP_MODE_FILE:
            cairo_set_source_rgbv (cr, color_nfrag_bleached);
            break;
    }

    for (ky = 0; ky < cluster_height; ++ky) {
        for (kx = 0; kx < cluster_width; ++kx) {
            if (!(cl->at(ky*cluster_width+kx).free) &&
                !(cl->at(ky*cluster_width+kx).fragmented))
            {
                cairo_rectangle (cr, kx * box_size, ky * box_size ,
                    box_size - 1, box_size - 1);
            }
        }
    }
    cairo_fill (cr);

    int selected_cluster = fm->selected_cluster;

    if (selected_cluster > cl->size()) selected_cluster = cl->size() - 1;

    if (FRAGMAP_MODE_CLUSTER == fm->display_mode &&    selected_cluster > 0 ) {
        ky = selected_cluster / cluster_width;
        kx = selected_cluster - ky * cluster_width;

        if (cl->at(selected_cluster).free) {
            cairo_set_source_rgbv (cr, color_free_selected);
        } else {
            if (cl->at(selected_cluster).fragmented) {
                cairo_set_source_rgbv (cr, color_frag);
            } else {
                cairo_set_source_rgbv (cr, color_nfrag);
            }
        }
        cairo_rectangle (cr, kx * box_size, ky * box_size ,
            box_size - 1, box_size - 1);
        cairo_fill (cr);
    }

    if (FRAGMAP_MODE_FILE == fm->display_mode) {
        int k2;
        GList *p = fm->selected_files;
        __u64 cluster_size = (fm->device_size_in_blocks-1) / fm->cluster_count + 1;

        while (p) {
            int file_idx = GPOINTER_TO_INT (p->data);
            if (fm->files->at(file_idx).fragmented) {
                cairo_set_source_rgbv (cr, color_frag);
            } else {
                cairo_set_source_rgbv (cr, color_nfrag);
            }

            for (k2 = 0; k2 < fm->files->at(file_idx).extents.size(); k2 ++) {
                __u64 estart_c, eend_c;
                __u64 estart_b, eend_b;

                estart_c = fm->files->at(file_idx).extents[k2].start / cluster_size;
                eend_c = (fm->files->at(file_idx).extents[k2].start +
                          fm->files->at(file_idx).extents[k2].length - 1);
                eend_c = eend_c / cluster_size;

                for (__u64 k3 = estart_c; k3 <= eend_c; k3 ++) {
                    ky = k3 / cluster_width;
                    kx = k3 - ky * cluster_width;

                    cairo_rectangle (cr, kx * box_size, ky * box_size ,
                        box_size - 1, box_size - 1);
                }
            }
            cairo_fill (cr);
            p = g_list_next (p);
        }
    }

    pthread_mutex_unlock(fm->clusters_mutex);
    cairo_destroy(cr);

    gettimeofday(&tv2, NULL);
    printf("cairo draw: %f sec\n", tv2.tv_sec-tv1.tv_sec+(tv2.tv_usec-tv1.tv_usec)/1000000.0);

    return FALSE;
}

int gtk_fragmap_get_cluster_count(GtkFragmap *fragmap) {

    return fragmap->cluster_count;
}

void gtk_fragmap_attach_cluster_list(GtkFragmap *fragmap, cluster_list *cl,
                                        pthread_mutex_t *cl_m)
{
    if (NULL != cl && NULL != cl_m) {
        fragmap->clusters = cl;
        fragmap->clusters_mutex = cl_m;
    }
}

void gtk_fragmap_attach_file_list(GtkFragmap *fragmap, file_list *fl,
                                    pthread_mutex_t *fl_m)
{
    if (NULL != fl && NULL != fl_m) {
        fragmap->files = fl;
        fragmap->files_mutex = fl_m;
    }
}

void gtk_fragmap_set_device_size(GtkFragmap *fragmap, __u64 sib) {
    fragmap->device_size_in_blocks = sib;
}

void gtk_fragmap_attach_widget_file_list(GtkFragmap *fm, GtkWidget *w,
            void (*update)(GtkWidget *, GtkTreeModel *))
{
    fm->update_file_list = update;
    fm->file_list_view = w;
}

void gtk_fragmap_file_begin (GtkFragmap *fm) {
    if (fm->selected_files) {
        g_list_free (fm->selected_files);
        fm->selected_files = NULL;
    }
    g_list_free (fm->selected_files);
}

void gtk_fragmap_file_add (GtkFragmap *fm, int file_idx) {
    fm->selected_files = g_list_append (fm->selected_files, GINT_TO_POINTER (file_idx));
}

void gtk_fragmap_set_mode (GtkFragmap *fm, enum FRAGMAP_MODE mode) {
    fm->display_mode = mode;
    fm->force_redraw = 1;
    gtk_widget_queue_draw (GTK_WIDGET (fm));
}

G_DEFINE_TYPE (GtkFragmap, gtk_fragmap, GTK_TYPE_DRAWING_AREA);
