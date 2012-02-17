#ifndef __FRAGMAP_WIDGET_H__
#define __FRAGMAP_WIDGET_H__

#include <gtkmm/drawingarea.h>
#include <gtkmm/widget.h>
#include <gtkmm/scrollbar.h>
#include <gtkmm/adjustment.h>
#include <gtkmm/box.h>
#include <stdint.h>
#include <pthread.h>
#include "clusters.h"
#include "filelist-widget.h"

class Fragmap : public Gtk::HBox {
    public:
        // types
        enum FRAGMAP_MODE {
            FRAGMAP_MODE_SHOW_ALL = 0,
            FRAGMAP_MODE_CLUSTER,
            FRAGMAP_MODE_FILE
        };

    public:
        Fragmap ();
        ~Fragmap ();

        int get_cluster_count();
        void attach_clusters (Clusters& cl);
        void attach_filelist_widget (FilelistView& fl);
        void file_begin ();
        void file_add (int file_idx);
        void set_mode (enum FRAGMAP_MODE);
        bool highlight_cluster_at (double x, double y);

    protected:
        Clusters *clusters;
        FilelistView *filelist;

        Gtk::DrawingArea drawing_area;
        Gtk::VScrollbar scrollbar;

        int cluster_size_desired; // desired number of blocks each cluster contains
        int force_fill_clusters;
        int widget_size_changed;
        int cluster_count_changed;
        int box_size;
        int frag_limit;

        int target_cluster;
        int total_clusters;

        FRAGMAP_MODE display_mode;
        int selected_cluster;
        std::vector<int> selected_files;

        int shift_x;
        int shift_y;

        int cluster_map_width;
        int cluster_map_height;
        int cluster_map_full_height;

        double color_free[3];
        double color_free_selected[3];
        double color_frag[3];
        double color_nfrag[3];
        double color_back[3];

        double bleach_factor;
        double color_free_bleached[3];
        double color_frag_bleached[3];
        double color_nfrag_bleached[3];
        double color_back_bleached[3];

    protected:
        // signal handlers
        virtual bool on_drawarea_draw (const Cairo::RefPtr<Cairo::Context>& cr);
        virtual bool on_motion_notify_event (GdkEventMotion *event);
        virtual bool on_button_press_event (GdkEventButton* event);
        virtual bool on_drawarea_scroll_event (GdkEventScroll* event);
        virtual void on_size_allocate (Gtk::Allocation& allocation);
        virtual void on_scrollbar_value_changed (void);
    private:
        void cairo_set_source_rgbv (const Cairo::RefPtr<Cairo::Context>& cr, double const color[]);
};



#endif
