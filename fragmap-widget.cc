#include "fragmap-widget.h"
#include <iostream>
#include <assert.h>
#include <sys/time.h>
#include <gtkmm/box.h>

Fragmap::Fragmap ()
{
    clusters = NULL;

    shift_x = 0;
    shift_y = 0;
    box_size = 7;

    display_mode = FRAGMAP_MODE_SHOW_ALL;

    target_cluster = 0;
    selected_cluster = 0;

    filelist = 0;
    statusbar = NULL;
    statusbar_context = 0;

    drawing_area.set_events (Gdk::BUTTON_PRESS_MASK | Gdk::POINTER_MOTION_MASK | Gdk::SCROLL_MASK);

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

    scrollbar.set_orientation (Gtk::ORIENTATION_VERTICAL);
    pack_start (drawing_area, true, true);
    pack_start (scrollbar, false, true);

    // signals
    drawing_area.signal_draw ().connect (sigc::mem_fun (*this, &Fragmap::on_drawarea_draw));
    drawing_area.signal_scroll_event ().connect (sigc::mem_fun (*this, &Fragmap::on_drawarea_scroll_event));
    scrollbar.signal_value_changed ().connect (sigc::mem_fun (*this, &Fragmap::on_scrollbar_value_changed));

    show_all_children ();

    this->set_size_request (400, 50);
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
Fragmap::on_drawarea_scroll_event (GdkEventScroll* event)
{
    if (event->state & Gdk::CONTROL_MASK) {
        // scroll with Ctrl key
        clusters->lock_clusters ();

        uint64_t old_size = clusters->get_actual_cluster_size ();
        uint64_t new_size = old_size;

        if (GDK_SCROLL_UP == event->direction) {
            new_size /= 1.15;
            if (new_size == 0) new_size = 1;
        }

        if (GDK_SCROLL_DOWN == event->direction) {
            new_size *= 1.15;
            if (old_size == new_size) new_size ++;
        }

        clusters->set_desired_cluster_size (new_size);
        if (statusbar) {
            std::stringstream ss;
            uint64_t acs = clusters->get_actual_cluster_size ();
            ss << acs << " block(s) in cluster";
            statusbar->push (ss.str(), statusbar_context);
        }

        recalculate_sizes ();
        drawing_area.queue_draw ();
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
Fragmap::recalculate_sizes (void)
{
    recalculate_sizes (get_allocation().get_width(), get_allocation().get_height());
}

void
Fragmap::recalculate_sizes (int pix_width, int pix_height)
{
    // estimate map size without scrollbar
    cluster_map_width = std::max (1, (pix_width - 1) / box_size);
    cluster_map_height = (pix_height - 1) / box_size;

    assert (clusters != NULL);
    uint64_t device_size = clusters->get_device_size ();
    assert (device_size > 0);
    cluster_map_full_height = (clusters->get_count() - 1) / cluster_map_width + 1;

    if (cluster_map_full_height > cluster_map_height) {
        // map does not fit, show scrollbar
        scrollbar.show();
        // and then recalculate sizes
        pix_width = pix_width - scrollbar.get_allocation().get_width();
        cluster_map_width = std::max (1, (pix_width - 1) / box_size);
        cluster_map_full_height = (clusters->get_count() - 1) / cluster_map_width + 1;
    } else {
        // we do not need scrollbar, hide it
        scrollbar.hide();
    }

    scrollbar.set_range (0.0, cluster_map_full_height);
    scrollbar.set_increments (1.0, cluster_map_height);

    // upper limit for scroll bar is one page shorter, so we must recalculate page size
    Glib::RefPtr<Gtk::Adjustment> scroll_adj = scrollbar.get_adjustment ();
    scroll_adj->set_page_size ((double)cluster_map_height);
}

void
Fragmap::on_size_allocate (Gtk::Allocation& allocation)
{
    Gtk::HBox::on_size_allocate (allocation); // parent

    recalculate_sizes (allocation.get_width(), allocation.get_height());
}

void
Fragmap::cairo_set_source_rgbv (const Cairo::RefPtr<Cairo::Context>& cr, double const color[])
{
    cr->set_source_rgb (color[0], color[1], color[2]);
}


bool
Fragmap::on_drawarea_draw(const Cairo::RefPtr<Cairo::Context>& cr)
{
    Gtk::Allocation allocation = drawing_area.get_allocation ();
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

    int target_line = target_cluster / cluster_map_width;
    int target_offset = target_line * cluster_map_width;

    clusters->lock_clusters ();
    clusters->lock_files ();
    clusters->__fill_clusters (target_offset, cluster_map_width * cluster_map_height);
    clusters->unlock_clusters ();
    clusters->unlock_files ();

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
            uint64_t cluster_idx = ky*cluster_map_width + kx + target_offset;
            if (cluster_idx < clusters->get_count() && clusters->at(cluster_idx).free) {
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
            uint64_t cluster_idx = ky*cluster_map_width + kx + target_offset;
            if (cluster_idx < clusters->get_count() && !(clusters->at(cluster_idx).free) &&
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
            uint64_t cluster_idx = ky*cluster_map_width + kx + target_offset;
            if (cluster_idx < clusters->get_count() && !(clusters->at(cluster_idx).free) &&
                !(clusters->at(cluster_idx).fragmented))
            {
                cr->rectangle (kx * box_size, ky * box_size, box_size - 1, box_size - 1);
            }
        }
    }
    cr->fill ();

    if (selected_cluster > clusters->get_count()) selected_cluster = clusters->get_count() - 1;

    if (FRAGMAP_MODE_CLUSTER == display_mode) {
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
        Clusters::file_list& files = clusters->get_files ();
        assert (clusters != NULL);
        uint64_t device_size = clusters->get_device_size ();
        assert (device_size > 0);
        uint64_t cluster_size = clusters->get_actual_cluster_size ();

        for (unsigned int k = 0; k < selected_files.size (); ++k) {
            int file_idx = selected_files[k];
            if (files.at(file_idx).fragmented) {
                cairo_set_source_rgbv (cr, color_frag);
            } else {
                cairo_set_source_rgbv (cr, color_nfrag);
            }

            for (unsigned int k2 = 0; k2 < files.at(file_idx).extents.size(); k2 ++) {
                uint64_t estart_c, eend_c;

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
        }
    }

    clusters->unlock_clusters ();

    return true;
}

bool
Fragmap::highlight_cluster_at (gdouble x, gdouble y)
{
    clusters->lock_clusters ();
    clusters->lock_files ();

    bool flag_update = FALSE;
    int cl_x = (int) (x - shift_x) / box_size;
    int cl_y = (int) (y - shift_y) / box_size;

    int target_line = target_cluster / cluster_map_width;
    uint64_t cl_raw = (cl_y + target_line) * cluster_map_width + cl_x;

    if (cl_raw >= clusters->get_count()) cl_raw = clusters->get_count() - 1;

    Clusters::cluster_info& ci = clusters->at (cl_raw);
    Clusters::file_list& files = clusters->get_files ();

    if ( display_mode != FRAGMAP_MODE_CLUSTER || selected_cluster != cl_raw ) {
        display_mode = FRAGMAP_MODE_CLUSTER;
        selected_cluster = cl_raw;

        if (filelist) {
            filelist->clear();

            for (Clusters::file_p_list::iterator iter = ci.files.begin(); iter != ci.files.end(); ++ iter) {
                unsigned int fid = *iter;
                Clusters::f_info &fi = files[fid];
                filelist->add_file_info (fid, fi.extents.size(), fi.severity, fi.filetype, fi.size, fi.name);
            }
        }
        flag_update = TRUE;
    }

    clusters->unlock_files ();
    clusters->unlock_clusters ();
    return flag_update;
}

void
Fragmap::attach_clusters (Clusters& cl)
{
    clusters = &cl;
}

void
Fragmap::attach_statusbar (Gtk::Statusbar *sb, unsigned int sb_context)
{
    statusbar = sb;
    statusbar_context = sb_context;
}

void
Fragmap::on_scrollbar_value_changed (void)
{
    target_cluster = cluster_map_width * round (scrollbar.get_value ());
    drawing_area.queue_draw ();
}

void
Fragmap::set_mode (enum FRAGMAP_MODE mode)
{
    display_mode = mode;
    drawing_area.queue_draw ();
}

void
Fragmap::attach_filelist_widget (FilelistView& fl)
{
    filelist = &fl;
    filelist->attach_fragmap (this);
}

void
Fragmap::file_begin ()
{
    selected_files.clear ();
    set_mode (FRAGMAP_MODE_FILE);
}

void
Fragmap::file_add (int idx)
{
    selected_files.push_back (idx);
}
