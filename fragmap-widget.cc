#include "fragmap-widget.h"
#include <iostream>
#include <assert.h>
#include <sys/time.h>

Fragmap::Fragmap ()
{
    clusters = NULL;

    widget_size_changed = 0;
    cluster_count_changed = 1;
    total_clusters = 0;
    force_fill_clusters = 0;
    shift_x = 0;
    shift_y = 0;
    box_size = 7;
    frag_limit = 1;

    display_mode = FRAGMAP_MODE_SHOW_ALL;
    selected_cluster = -1;
    selected_files = NULL;

    target_cluster = 0;
    cluster_size_desired = 3500;

    file_list_view = NULL;
    update_file_list = NULL;

    set_events (Gdk::BUTTON_PRESS_MASK | Gdk::POINTER_MOTION_MASK | Gdk::SCROLL_MASK);

    // colors
    bleach_factor = 0.3;
    color_free[0] = 1.0;          color_free[1] = 1.0;          color_free[2] = 1.0;
    color_free_selected[0] = 1.0; color_free_selected[1] = 1.0; color_free_selected[2] = 0.0;
    color_frag[0] = 0.8;          color_frag[1] = 0.0;          color_frag[2] = 0.0;
    color_nfrag[0] = 0.0;         color_nfrag[1] = 0.0;         color_nfrag[2] = 0.8;
    color_back[0] = 0.25;         color_back[1] = 0.25;         color_back[2] = 0.25;

    for (int k = 0; k < 3; k ++) {
        color_free_bleached[k] = 1.0 - (1.0 - color_free[k]) * bleach_factor;
        color_frag_bleached[k] = 1.0 - (1.0 - color_frag[k]) * bleach_factor;
        color_nfrag_bleached[k] = 1.0 - (1.0 - color_nfrag[k]) * bleach_factor;
        color_back_bleached[k] = 1.0 - (1.0 - color_back[k]) * bleach_factor;
    }
}

Fragmap::~Fragmap ()
{

}

bool
Fragmap::on_motion_notify_event (GdkEventMotion *event)
{
    if (event->state & Gdk::BUTTON1_MASK) {
        if (highlight_cluster_at (event->x, event->y)) {
            queue_draw ();
        }
    }

    return true;
}

bool
Fragmap::on_button_press_event (GdkEventButton* event)
{
    if (event->button == 1) { // left mouse button
        if (highlight_cluster_at (event->x, event->y)) {
            queue_draw ();
        }
    }

    return true;
}

bool
Fragmap::on_scroll_event (GdkEventScroll* event)
{
    if (event->state & Gdk::CONTROL_MASK) {
        // scroll with Ctrl key
        force_fill_clusters = 1;
        clusters->lock_clusters ();

        if (GDK_SCROLL_UP == event->direction) {
            cluster_size_desired = cluster_size_desired / 1.15;
            if (cluster_size_desired == 0) cluster_size_desired = 1;
        }

        if (GDK_SCROLL_DOWN == event->direction) {
            int old = cluster_size_desired;
            cluster_size_desired = cluster_size_desired * 1.15;
            if (old == cluster_size_desired) cluster_size_desired ++;

            // clamp
            if (cluster_size_desired > total_clusters) {
                cluster_size_desired = total_clusters;
            }
        }
        std::cout << "updated cluster_size_desired = " << cluster_size_desired << std::endl;

        // size_allocate (get_allocation ());
        queue_draw ();
        clusters->unlock_clusters ();
    } else {
        Glib::RefPtr<Gtk::Adjustment> adj = scrollbar.get_adjustment ();
        double value = adj->get_value ();
        double step = adj->get_step_increment ();
        if (GDK_SCROLL_UP == event->direction) value -= step;
        if (GDK_SCROLL_DOWN == event->direction) value += step;
        value = std::min (value, adj->get_upper () - adj->get_page_size ());
        // 'set_value' will clamp value on 'lower'
        adj->set_value (value);
    }

    return true;
}

void
Fragmap::on_size_allocate (Gtk::Allocation& allocation)
{
    Glib::RefPtr<Gtk::Adjustment> scroll_adj = scrollbar.get_adjustment ();
    int pix_width;
    int pix_height;

    display_mode = FRAGMAP_MODE_SHOW_ALL;

    // estimate map size without scrollbar
    pix_width = get_allocation().get_width();
    pix_height = get_allocation().get_height();

    // to get full width one need determine, if scrollbar visible
    if (scrollbar.get_visible()) pix_width += scrollbar.get_allocation().get_width();

    cluster_map_width = (pix_width - 1) / box_size;
    cluster_map_height = (pix_height - 1) / box_size;

    assert(device_size_in_blocks > 0);
    total_clusters = (device_size_in_blocks - 1) / cluster_size_desired + 1;
    cluster_map_full_height = (total_clusters - 1) / cluster_map_width + 1;

    if (cluster_map_full_height > cluster_map_height) {
        // map does not fit, show scrollbar
        scrollbar.show();
        // and then recalculate sizes
        pix_width = get_allocation().get_width();
        cluster_map_width = (pix_width - 1) / box_size;
        cluster_map_full_height = (total_clusters - 1) / cluster_map_width + 1;
    } else {
        // we do not need scrollbar, hide it
        scrollbar.hide();
    }


    scrollbar.set_range (0.0, cluster_map_full_height);
    scrollbar.set_increments (1.0, cluster_map_height);

    // upper limit for scroll bar is one page shorter, so we must recalculate page size
    scroll_adj->set_page_size ((double)cluster_map_height);

    widget_size_changed = 1;
}

void
Fragmap::cairo_set_source_rgbv (const Cairo::RefPtr<Cairo::Context>& cr, double const color[])
{
    cr->set_source_rgb (color[0], color[1], color[2]);
}


bool
Fragmap::on_draw(const Cairo::RefPtr<Cairo::Context>& cr)
{
    Gtk::Allocation allocation = get_allocation ();
    const int width = allocation.get_width ();
    const int height = allocation.get_height ();

    switch (display_mode) {
        case FRAGMAP_MODE_SHOW_ALL:
        default:
            cairo_set_source_rgbv (cr, color_back);
            break;
        case FRAGMAP_MODE_CLUSTER:
        case FRAGMAP_MODE_FILE:
            cairo_set_source_rgbv (cr, color_back_bleached);
            break;
    }

    cr->rectangle (0, 0, width, height);
    cr->fill ();

    int box_size = box_size;

    int target_line = target_cluster / cluster_map_width;
    int target_offset = target_line * cluster_map_width;

    struct timeval tv1, tv2;

    if (cluster_count_changed || force_fill_clusters )
    {
        gettimeofday(&tv1, NULL);
        clusters->lock_clusters ();
        clusters->lock_files ();
        clusters->__fill_clusters (device_size_in_blocks, total_clusters, frag_limit);
        force_fill_clusters = 0;
        cluster_count_changed = 0;
        clusters->unlock_clusters ();
        clusters->unlock_files ();
        gettimeofday(&tv2, NULL);
        printf("fill_clusters: %f sec\n", tv2.tv_sec-tv1.tv_sec+(tv2.tv_usec-tv1.tv_usec)/1000000.0);
    }

    gettimeofday(&tv1, NULL);
    int ky, kx;

    clusters->lock_clusters ();

    switch (display_mode) {
        case FRAGMAP_MODE_SHOW_ALL:
        default:
            cairo_set_source_rgbv (cr, color_free);
            break;
        case FRAGMAP_MODE_CLUSTER:
        case FRAGMAP_MODE_FILE:
            cairo_set_source_rgbv (cr, color_free_bleached);
            break;
    }

    for (ky = 0; ky < cluster_map_height; ++ky) {
        for (kx = 0; kx < cluster_map_width; ++kx) {
            int cluster_idx = ky*cluster_map_width + kx + target_offset;
            if (cluster_idx < total_clusters && clusters->at(cluster_idx).free) {
                cr->rectangle (kx * box_size, ky * box_size, box_size - 1, box_size - 1);
            }
        }
    }

    cr->fill ();

    switch (display_mode) {
        case FRAGMAP_MODE_SHOW_ALL:
        default:
            cairo_set_source_rgbv (cr, color_frag);
            break;
        case FRAGMAP_MODE_CLUSTER:
        case FRAGMAP_MODE_FILE:
            cairo_set_source_rgbv (cr, color_frag_bleached);
            break;
    }

    for (ky = 0; ky < cluster_map_height; ++ky) {
        for (kx = 0; kx < cluster_map_width; ++kx) {
            int cluster_idx = ky*cluster_map_width + kx + target_offset;
            if (cluster_idx < total_clusters && !(clusters->at(cluster_idx).free) &&
                clusters->at(cluster_idx).fragmented)
            {
                cr->rectangle (kx * box_size, ky * box_size, box_size - 1, box_size - 1);
            }
        }
    }
    cr->fill ();

    switch (display_mode) {
        case FRAGMAP_MODE_SHOW_ALL:
        default:
            cairo_set_source_rgbv (cr, color_nfrag);
            break;
        case FRAGMAP_MODE_CLUSTER:
        case FRAGMAP_MODE_FILE:
            cairo_set_source_rgbv (cr, color_nfrag_bleached);
            break;
    }

    for (ky = 0; ky < cluster_map_height; ++ky) {
        for (kx = 0; kx < cluster_map_width; ++kx) {
            int cluster_idx = ky*cluster_map_width + kx + target_offset;
            if (cluster_idx < total_clusters && !(clusters->at(cluster_idx).free) &&
                !(clusters->at(cluster_idx).fragmented))
            {
                cr->rectangle (kx * box_size, ky * box_size, box_size - 1, box_size - 1);
            }
        }
    }
    cr->fill ();

    int selected_cluster = selected_cluster;

    if (selected_cluster > clusters->size()) selected_cluster = clusters->size() - 1;

    if (FRAGMAP_MODE_CLUSTER == display_mode && selected_cluster >= 0 ) {
        ky = selected_cluster / cluster_map_width;
        kx = selected_cluster - ky * cluster_map_width;
        ky = ky - target_line;              // to screen coordinates

        if (clusters->at(selected_cluster).free) {
            cairo_set_source_rgbv (cr, color_free_selected);
        } else {
            if (clusters->at(selected_cluster).fragmented) {
                cairo_set_source_rgbv (cr, color_frag);
            } else {
                cairo_set_source_rgbv (cr, color_nfrag);
            }
        }
        cr->rectangle (kx * box_size, ky * box_size, box_size - 1, box_size - 1);
        cr->fill ();
    }

    if (FRAGMAP_MODE_FILE == display_mode) {
        int k2;
        GList *p = selected_files;
        Clusters::file_list& files = clusters->get_files ();
        uint64_t cluster_size = (device_size_in_blocks - 1) / total_clusters + 1;

        while (p) {
            int file_idx = GPOINTER_TO_INT (p->data);
            if (files.at(file_idx).fragmented) {
                cairo_set_source_rgbv (cr, color_frag);
            } else {
                cairo_set_source_rgbv (cr, color_nfrag);
            }

            for (k2 = 0; k2 < files.at(file_idx).extents.size(); k2 ++) {
                uint64_t estart_c, eend_c;
                uint64_t estart_b, eend_b;

                estart_c = files.at(file_idx).extents[k2].start / cluster_size;
                eend_c = (files.at(file_idx).extents[k2].start +
                          files.at(file_idx).extents[k2].length - 1);
                eend_c = eend_c / cluster_size;

                for (uint64_t k3 = estart_c; k3 <= eend_c; k3 ++) {
                    ky = k3 / cluster_map_width;
                    kx = k3 - ky * cluster_map_width;
                    ky = ky - target_line;              // to screen coordinates

                    if (0 <= ky && ky < cluster_map_height) {
                        cr->rectangle (kx * box_size, ky * box_size, box_size - 1, box_size - 1);
                    }
                }
            }
            cr->fill ();
            p = g_list_next (p);
        }
    }

    clusters->unlock_clusters ();

    gettimeofday(&tv2, NULL);
    std::cout << "cairo draw: " << (tv2.tv_sec-tv1.tv_sec+(tv2.tv_usec-tv1.tv_usec)/1000000.0) << " sec" << std::endl;

    return true;
}

bool
Fragmap::highlight_cluster_at (gdouble x, gdouble y)
{
    clusters->lock_clusters ();
    clusters->lock_files ();

    gboolean flag_update = FALSE;
    int cl_x = (int) (x - shift_x) / box_size;
    int cl_y = (int) (y - shift_y) / box_size;
    int clusters_per_row = get_allocation().get_width() / box_size;

    int target_line = target_cluster / cluster_map_width;
    int cl_raw = (cl_y + target_line) * clusters_per_row + cl_x;

    if (cl_raw >= clusters->size())
        cl_raw = clusters->size() - 1;

    Clusters::cluster_info& ci = clusters->at (cl_raw);
    Clusters::file_list files = clusters->get_files ();
//TODO: fix
/*
    if ( display_mode != FRAGMAP_MODE_CLUSTER || selected_cluster != cl_raw ) {
        display_mode = FRAGMAP_MODE_CLUSTER;
        selected_cluster = cl_raw;

        int ss = ci.files.size();

        GtkListStore  *store;
        GtkTreeIter    iter;
        store = file_list_model_new();

        for (int k = 0; k < ci.files.size(); ++k) {
            std::cout << files.at(ci.files[k]).name << "\n";

            size_t q = files.at(ci.files[k]).name.rfind('/');
            std::string filename = files.at(ci.files[k]).name.substr(q+1, -1);
            std::string dirname = files.at(ci.files[k]).name.substr(0, q);

            gtk_list_store_append (store, &iter);
            gtk_list_store_set (store, &iter,
                            FILELISTVIEW_COL_FRAG, files->at(ci->files[k]).extents.size(),
                            FILELISTVIEW_COL_NAME, filename.c_str(),
                            FILELISTVIEW_COL_SEVERITY, files->at(ci->files[k]).severity,
                            FILELISTVIEW_COL_DIR, dirname.c_str(),
                            FILELISTVIEW_COL_POINTER, ci->files[k],
                            -1);
        }

        if (update_file_list) {
            update_file_list (file_list_view, GTK_TREE_MODEL(store));
        }
        g_object_unref (store);
        flag_update = TRUE;
    }
*/
    clusters->unlock_files ();
    clusters->unlock_clusters ();
    return flag_update;
}

int
Fragmap::get_cluster_count()
{
    return total_clusters;
}

void
Fragmap::attach_clusters (Clusters& cl)
{
    clusters = &cl;
}


/*
#include <gtk/gtk.h>
#include <math.h>
#include "gtk_fragmap.h"
#include "filelistview.h"

#include <sys/time.h>
#include <iostream>
#include <assert.h>



void gtk_fragmap_set_device_size(GtkFragmap *fragmap, __u64 sib) {
    fragmap->device_size_in_blocks = sib;
}

void gtk_fragmap_attach_widget_file_list(GtkFragmap *fm, GtkWidget *w,
            void (*update)(GtkWidget *, GtkTreeModel *))
{
    update_file_list = update;
    file_list_view = w;
}

void gtk_fragmap_file_begin (GtkFragmap *fm) {
    if (selected_files) {
        g_list_free (selected_files);
        selected_files = NULL;
    }
    g_list_free (selected_files);
}

void gtk_fragmap_file_add (GtkFragmap *fm, int file_idx) {
    selected_files = g_list_append (selected_files, GINT_TO_POINTER (file_idx));
}

void gtk_fragmap_set_mode (GtkFragmap *fm, enum FRAGMAP_MODE mode) {
    display_mode = mode;
    gtk_widget_queue_draw (GTK_WIDGET (fm));
}

static void scroll_value_changed (GtkRange *range, gpointer user_data) {
    GtkFragmap *fm = (GtkFragmap *) user_data;

    target_cluster = cluster_map_width * round (gtk_range_get_value (range));
    gtk_widget_queue_draw (GTK_WIDGET (fm));
}

void gtk_fragmap_attach_scroll (GtkFragmap *fm, GtkWidget *scroll_widget) {
    scroll_widget = scroll_widget;
    gtk_range_set_slider_size_fixed (GTK_RANGE (scroll_widget), FALSE);
    g_signal_connect (scroll_widget, "value-changed", G_CALLBACK (scroll_value_changed), fm);
}

G_DEFINE_TYPE (GtkFragmap, gtk_fragmap, GTK_TYPE_DRAWING_AREA);
*/
