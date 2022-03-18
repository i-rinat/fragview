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

#include "fragmap-widget.hh"
#include "util.hh"
#include <assert.h>
#include <gtkmm/box.h>
#include <iostream>
#include <sys/time.h>

namespace
{

const int kBoxSize = 7;
const double kBleachFactor = 0.3;

class color3
{
public:
    color3(double r, double g, double b)
        : c_{r, g, b}
        , c_bleached_{1.0 - (1.0 - c_[0]) * kBleachFactor, 1.0 - (1.0 - c_[1]) * kBleachFactor,
                      1.0 - (1.0 - c_[2]) * kBleachFactor}
    {
    }

    operator const double *() const { return c_; }

    const double *
    bleached() const
    {
        return c_bleached_;
    }

private:
    const double c_[3];
    const double c_bleached_[3];
};

const color3 kColorFree{1.0, 1.0, 1.0};
const color3 kColorFreeSelected{1.0, 1.0, 0.0};
const color3 kColorFrag{0.8, 0.0, 0.0};
const color3 kColorNFrag{0.0, 0.0, 0.8};
const color3 kColorBack{0.25, 0.25, 0.25};

void
cairo_set_source_rgbv(const Cairo::RefPtr<Cairo::Context> &cr, double const color[])
{
    cr->set_source_rgb(color[0], color[1], color[2]);
}

}  // unnamed namespace

Fragmap::Fragmap()
    : clusters_(nullptr)
    , filelist_(nullptr)
    , statusbar_(nullptr)
    , statusbar_context_(0)
    , display_mode_(FRAGMAP_MODE_SHOW_ALL)
    , selected_cluster_(0)
    , target_block_(0)
    , shift_x_(0)
    , shift_y_(0)
{
    drawing_area_.set_events(Gdk::BUTTON_PRESS_MASK | Gdk::POINTER_MOTION_MASK | Gdk::SCROLL_MASK);

    scrollbar_.set_orientation(Gtk::ORIENTATION_VERTICAL);
    pack_start(drawing_area_, true, true);
    pack_start(scrollbar_, false, true);

    // signals
    drawing_area_.signal_draw().connect(sigc::mem_fun(*this, &Fragmap::on_drawarea_draw));
    drawing_area_.signal_scroll_event().connect(
        sigc::mem_fun(*this, &Fragmap::on_drawarea_scroll_event));
    scrollbar_.signal_value_changed().connect(
        sigc::mem_fun(*this, &Fragmap::on_scrollbar_value_changed));

    show_all_children();
    set_size_request(400, 50);
}

Fragmap::~Fragmap()
{
}

bool
Fragmap::on_motion_notify_event(GdkEventMotion *event)
{
    if (event->state & Gdk::BUTTON1_MASK) {
        if (highlight_cluster_at(event->x, event->y)) {
            queue_draw();
        }
    }

    return true;
}

bool
Fragmap::on_button_press_event(GdkEventButton *event)
{
    if (event->button == 1) {  // left mouse button
        if (highlight_cluster_at(event->x, event->y)) {
            queue_draw();
        }
    }

    return true;
}

bool
Fragmap::on_drawarea_scroll_event(GdkEventScroll *event)
{
    if (event->state & Gdk::CONTROL_MASK) {
        // scroll with Ctrl key
        clusters_->lock_clusters();

        uint64_t old_size = clusters_->get_actual_cluster_size();
        uint64_t new_size = old_size;

        if (event->direction == GDK_SCROLL_UP) {
            new_size /= 1.15;
            if (new_size == 0)
                new_size = 1;
        }

        if (event->direction == GDK_SCROLL_DOWN) {
            new_size *= 1.15;
            if (old_size == new_size)
                new_size++;
        }

        clusters_->set_desired_cluster_size(new_size);
        update_statusbar();

        recalculate_sizes();
        drawing_area_.queue_draw();
        clusters_->unlock_clusters();
    } else {
        Glib::RefPtr<Gtk::Adjustment> adj = scrollbar_.get_adjustment();
        double value = adj->get_value();
        double step = adj->get_step_increment();
        if (event->direction == GDK_SCROLL_UP)
            value -= step;
        if (event->direction == GDK_SCROLL_DOWN)
            value += step;
        value = std::min(value, adj->get_upper() - adj->get_page_size());
        // 'set_value' will clamp value on 'lower'
        adj->set_value(value);
    }

    return true;
}

void
Fragmap::recalculate_sizes()
{
    recalculate_sizes(get_allocation().get_width(), get_allocation().get_height());
}

void
Fragmap::recalculate_sizes(int pix_width, int pix_height)
{
    // estimate map size without scrollbar
    cluster_map_width_ = std::max(1, (pix_width - 1) / kBoxSize);
    cluster_map_height_ = (pix_height - 1) / kBoxSize + 1;

    assert(clusters_ != nullptr);
    assert(clusters_->get_device_size() > 0);
    cluster_map_full_height_ = (clusters_->get_count() - 1) / cluster_map_width_ + 1;

    if (cluster_map_full_height_ > cluster_map_height_) {
        // map does not fit, show scrollbar
        scrollbar_.show();
        // and then recalculate sizes
        pix_width = pix_width - scrollbar_.get_allocation().get_width();
        cluster_map_width_ = std::max(1, (pix_width - 1) / kBoxSize);
        cluster_map_full_height_ = (clusters_->get_count() - 1) / cluster_map_width_ + 1;
    } else {
        // we do not need scrollbar, hide it
        scrollbar_.hide();
    }

    scrollbar_.set_range(0.0, cluster_map_full_height_);
    scrollbar_.set_increments(1.0, cluster_map_height_);
    scrollbar_.set_value(target_block_ / clusters_->get_actual_cluster_size() / cluster_map_width_);

    // upper limit for scroll bar is one page shorter, so we must recalculate page size
    scrollbar_.get_adjustment()->set_page_size(cluster_map_height_);
}

void
Fragmap::on_size_allocate(Gtk::Allocation &allocation)
{
    Gtk::HBox::on_size_allocate(allocation);  // parent

    recalculate_sizes(allocation.get_width(), allocation.get_height());
}

bool
Fragmap::on_drawarea_draw(const Cairo::RefPtr<Cairo::Context> &cr)
{
    Gtk::Allocation allocation = drawing_area_.get_allocation();
    const int width = allocation.get_width();
    const int height = allocation.get_height();

    switch (display_mode_) {
    case FRAGMAP_MODE_SHOW_ALL:
    default:
        cairo_set_source_rgbv(cr, kColorBack);
        break;
    case FRAGMAP_MODE_CLUSTER:
    case FRAGMAP_MODE_FILE:
        cairo_set_source_rgbv(cr, kColorBack.bleached());
        break;
    }

    cr->rectangle(0, 0, width, height);
    cr->fill();

    const int target_line =
        target_block_ / clusters_->get_actual_cluster_size() / cluster_map_width_;
    const int target_offset = target_line * cluster_map_width_;

    clusters_->lock_clusters();
    clusters_->lock_files();
    clusters_->__fill_clusters(target_offset, cluster_map_width_ * cluster_map_height_);
    clusters_->unlock_clusters();
    clusters_->unlock_files();

    int ky, kx;

    clusters_->lock_clusters();

    switch (display_mode_) {
    case FRAGMAP_MODE_SHOW_ALL:
    default:
        cairo_set_source_rgbv(cr, kColorFree);
        break;
    case FRAGMAP_MODE_CLUSTER:
    case FRAGMAP_MODE_FILE:
        cairo_set_source_rgbv(cr, kColorFree.bleached());
        break;
    }

    for (ky = 0; ky < cluster_map_height_; ky++) {
        for (kx = 0; kx < cluster_map_width_; kx++) {
            uint64_t cluster_idx = ky * cluster_map_width_ + kx + target_offset;
            if (cluster_idx < clusters_->get_count() && clusters_->at(cluster_idx).free) {
                cr->rectangle(kx * kBoxSize, ky * kBoxSize, kBoxSize - 1, kBoxSize - 1);
            }
        }
    }

    cr->fill();

    switch (display_mode_) {
    case FRAGMAP_MODE_SHOW_ALL:
    default:
        cairo_set_source_rgbv(cr, kColorFrag);
        break;
    case FRAGMAP_MODE_CLUSTER:
    case FRAGMAP_MODE_FILE:
        cairo_set_source_rgbv(cr, kColorFrag.bleached());
        break;
    }

    for (ky = 0; ky < cluster_map_height_; ky++) {
        for (kx = 0; kx < cluster_map_width_; kx++) {
            uint64_t cluster_idx = ky * cluster_map_width_ + kx + target_offset;
            if (cluster_idx < clusters_->get_count() && !(clusters_->at(cluster_idx).free) &&
                clusters_->at(cluster_idx).fragmented)
            {
                cr->rectangle(kx * kBoxSize, ky * kBoxSize, kBoxSize - 1, kBoxSize - 1);
            }
        }
    }
    cr->fill();

    switch (display_mode_) {
    case FRAGMAP_MODE_SHOW_ALL:
    default:
        cairo_set_source_rgbv(cr, kColorNFrag);
        break;
    case FRAGMAP_MODE_CLUSTER:
    case FRAGMAP_MODE_FILE:
        cairo_set_source_rgbv(cr, kColorNFrag.bleached());
        break;
    }

    for (ky = 0; ky < cluster_map_height_; ky++) {
        for (kx = 0; kx < cluster_map_width_; kx++) {
            uint64_t cluster_idx = ky * cluster_map_width_ + kx + target_offset;
            if (cluster_idx < clusters_->get_count() && !(clusters_->at(cluster_idx).free) &&
                !(clusters_->at(cluster_idx).fragmented))
            {
                cr->rectangle(kx * kBoxSize, ky * kBoxSize, kBoxSize - 1, kBoxSize - 1);
            }
        }
    }
    cr->fill();

    if (selected_cluster_ > clusters_->get_count())
        selected_cluster_ = clusters_->get_count() - 1;

    if (display_mode_ == FRAGMAP_MODE_CLUSTER) {
        ky = selected_cluster_ / cluster_map_width_;
        kx = selected_cluster_ - ky * cluster_map_width_;
        ky = ky - target_line;  // to screen coordinates

        if (clusters_->at(selected_cluster_).free) {
            cairo_set_source_rgbv(cr, kColorFreeSelected);
        } else {
            if (clusters_->at(selected_cluster_).fragmented) {
                cairo_set_source_rgbv(cr, kColorFrag);
            } else {
                cairo_set_source_rgbv(cr, kColorNFrag);
            }
        }
        cr->rectangle(kx * kBoxSize, ky * kBoxSize, kBoxSize - 1, kBoxSize - 1);
        cr->fill();
    }

    if (display_mode_ == FRAGMAP_MODE_FILE) {
        assert(clusters_);
        Clusters::file_list &files = clusters_->get_files();
        assert(clusters_->get_device_size() > 0);
        uint64_t cluster_size = clusters_->get_actual_cluster_size();

        for (auto file_idx : selected_files_) {
            if (files.at(file_idx).fragmented) {
                cairo_set_source_rgbv(cr, kColorFrag);
            } else {
                cairo_set_source_rgbv(cr, kColorNFrag);
            }

            for (unsigned int k2 = 0; k2 < files.at(file_idx).extents.size(); k2++) {
                uint64_t estart_c, eend_c;

                estart_c = files.at(file_idx).extents[k2].start / cluster_size;
                eend_c = (files.at(file_idx).extents[k2].start +
                          files.at(file_idx).extents[k2].length - 1);
                eend_c = eend_c / cluster_size;

                for (uint64_t k3 = estart_c; k3 <= eend_c; k3++) {
                    ky = k3 / cluster_map_width_;
                    kx = k3 - ky * cluster_map_width_;
                    ky = ky - target_line;  // to screen coordinates

                    if (0 <= ky && ky < cluster_map_height_) {
                        cr->rectangle(kx * kBoxSize, ky * kBoxSize, kBoxSize - 1, kBoxSize - 1);
                    }
                }
            }
            cr->fill();
        }
    }

    clusters_->unlock_clusters();
    return true;
}

bool
Fragmap::highlight_cluster_at(gdouble x, gdouble y)
{
    clusters_->lock_clusters();
    clusters_->lock_files();

    bool flag_update = FALSE;
    int cl_x = (int)(x - shift_x_) / kBoxSize;
    int cl_y = (int)(y - shift_y_) / kBoxSize;

    int target_line = target_block_ / clusters_->get_actual_cluster_size() / cluster_map_width_;
    uint64_t cl_raw = (cl_y + target_line) * cluster_map_width_ + cl_x;

    if (cl_raw >= clusters_->get_count())
        cl_raw = clusters_->get_count() - 1;

    Clusters::cluster_info &ci = clusters_->at(cl_raw);
    Clusters::file_list &files = clusters_->get_files();

    if (display_mode_ != FRAGMAP_MODE_CLUSTER || selected_cluster_ != cl_raw) {
        display_mode_ = FRAGMAP_MODE_CLUSTER;
        selected_cluster_ = cl_raw;

        if (filelist_) {
            filelist_->clear();

            for (auto const fid : ci.files) {
                Clusters::f_info &fi = files[fid];
                filelist_->add_file_info(fid, fi.extents.size(), fi.severity, fi.filetype, fi.size,
                                         fi.name);
            }
        }
        flag_update = TRUE;
    }

    clusters_->unlock_files();
    clusters_->unlock_clusters();

    update_statusbar();

    return flag_update;
}

void
Fragmap::update_statusbar()
{
    if (!statusbar_)
        return;

    const uint64_t blocks_in_cluster = clusters_->get_actual_cluster_size();
    const uint64_t bytes_in_cluster = blocks_in_cluster * clusters_->get_device_block_size();
    const std::string s = "Cluster " + std::to_string(selected_cluster_) + ", " +
                          std::to_string(blocks_in_cluster) + " block(s) in cluster (" +
                          util::format_filesize(bytes_in_cluster) + ")";

    statusbar_->remove_all_messages(statusbar_context_);
    statusbar_->push(s, statusbar_context_);
}

void
Fragmap::attach_clusters(Clusters &cl)
{
    clusters_ = &cl;
}

void
Fragmap::attach_statusbar(Gtk::Statusbar *sb, unsigned int sb_context)
{
    statusbar_ = sb;
    statusbar_context_ = sb_context;
}

void
Fragmap::on_scrollbar_value_changed()
{
    target_block_ =
        cluster_map_width_ * clusters_->get_actual_cluster_size() * round(scrollbar_.get_value());
    drawing_area_.queue_draw();
}

void
Fragmap::set_mode(enum Fragmap::mode dmode)
{
    display_mode_ = dmode;
    drawing_area_.queue_draw();
}

void
Fragmap::attach_filelist_widget(FilelistView &fl)
{
    filelist_ = &fl;
    filelist_->attach_fragmap(this);
}

void
Fragmap::file_begin()
{
    selected_files_.clear();
    set_mode(FRAGMAP_MODE_FILE);
}

void
Fragmap::file_add(int idx)
{
    selected_files_.push_back(idx);
}
