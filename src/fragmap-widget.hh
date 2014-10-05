/*
 * Copyright © 2011-2014  Rinat Ibragimov
 *
 * This file is part of "fragview" software suite.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#ifndef FRAGVIEW_FRAGMAP_WIDGET_HH
#define FRAGVIEW_FRAGMAP_WIDGET_HH

#include <gtkmm/drawingarea.h>
#include <gtkmm/widget.h>
#include <gtkmm/scrollbar.h>
#include <gtkmm/statusbar.h>
#include <gtkmm/adjustment.h>
#include <gtkmm/box.h>
#include <stdint.h>
#include <pthread.h>
#include "clusters.hh"
#include "filelist-widget.hh"
#include "color3.hh"


class Fragmap : public Gtk::HBox {
public:
    enum mode {
        FRAGMAP_MODE_SHOW_ALL = 0,
        FRAGMAP_MODE_CLUSTER,
        FRAGMAP_MODE_FILE
    };

    Fragmap();
    ~Fragmap();

    void
    attach_clusters(Clusters &cl);
    
    void
    attach_filelist_widget(FilelistView &fl);
    
    void
    attach_statusbar(Gtk::Statusbar *sb, unsigned int sb_context);
    
    void
    file_begin();
    
    void
    file_add(int file_idx);
    
    void
    set_mode(enum Fragmap::mode);
    
    bool
    highlight_cluster_at(double x, double y);
    
    void
    recalculate_sizes(int pix_width, int pix_height);
    
    void
    recalculate_sizes(void);

protected:
    Clusters           *clusters_;
    FilelistView       *filelist_;
    Gtk::Statusbar     *statusbar_;
    unsigned int        statusbar_context_;

    Gtk::DrawingArea    drawing_area_;
    Gtk::Scrollbar      scrollbar_;

    int                 box_size_;

    Fragmap::mode       display_mode_;
    uint64_t            selected_cluster_;
    uint64_t            target_block_;
    std::vector<int>    selected_files_;

    int                 shift_x_;
    int                 shift_y_;

    int                 cluster_map_width_;
    int                 cluster_map_height_;
    int                 cluster_map_full_height_;

    color3              color_free_;
    color3              color_free_selected_;
    color3              color_frag_;
    color3              color_nfrag_;
    color3              color_back_;

    double              bleach_factor_;
    color3              color_free_bleached_;
    color3              color_frag_bleached_;
    color3              color_nfrag_bleached_;
    color3              color_back_bleached_;

protected:
    // signal handlers
    virtual bool
    on_drawarea_draw(const Cairo::RefPtr<Cairo::Context> &cr);
    
    virtual bool
    on_motion_notify_event(GdkEventMotion *event);
    
    virtual bool
    on_button_press_event(GdkEventButton *event);
    
    virtual bool
    on_drawarea_scroll_event(GdkEventScroll *event);
    
    virtual void
    on_size_allocate(Gtk::Allocation &allocation);
    
    virtual void
    on_scrollbar_value_changed(void);

private:
    void
    cairo_set_source_rgbv(const Cairo::RefPtr<Cairo::Context> &cr, double const color[]);
};

#endif // FRAGVIEW_FRAGMAP_WIDGET_HH
