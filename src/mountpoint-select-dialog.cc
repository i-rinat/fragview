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

#include <gtkmm/box.h>
#include <gtkmm/button.h>
#include <gtkmm/stock.h>
#include <gtkmm/treemodelcolumn.h>
#include <gtkmm/cellrendererprogress.h>
#include <iostream>
#include <fstream>
#include <sys/vfs.h>
#include "util.hh"
#include "mountpoint-select-dialog.hh"


MountpointSelectDialog::MountpointSelectDialog(void)
{
    set_title("Select mountpoint");
    set_size_request(400, 300);
    set_border_width(10);

    add_button(Gtk::Stock::CANCEL, Gtk::RESPONSE_CANCEL);
    add_button(Gtk::Stock::OK, Gtk::RESPONSE_OK);
    get_content_area()->pack_start(tv_, Gtk::PACK_EXPAND_WIDGET, 10);
    show_all_children();

    // prepare table
    liststore_ = Gtk::ListStore::create(columns_);
    tv_.set_model(liststore_);

    tv_.append_column("Mount point", columns_.mountpoint);
    tv_.set_search_column(columns_.mountpoint);

    {
        Gtk::CellRenderer *renderer = Gtk::manage(new Gtk::CellRendererText());
        int column_id = tv_.append_column("Size", *renderer) - 1;
        tv_.get_column(column_id)->set_cell_data_func(*renderer,
            sigc::bind(sigc::mem_fun(*this, &MountpointSelectDialog::cell_data_func_size),
                       &columns_.size));
    }

    {
        Gtk::CellRenderer *renderer = Gtk::manage(new Gtk::CellRendererText());
        int column_id = tv_.append_column("Used", *renderer) - 1;
        tv_.get_column(column_id)->set_cell_data_func(*renderer,
            sigc::bind(sigc::mem_fun(*this, &MountpointSelectDialog::cell_data_func_size),
                       &columns_.used));
    }

    {
        Gtk::CellRenderer *renderer = Gtk::manage(new Gtk::CellRendererText());
        int column_id = tv_.append_column("Available", *renderer) - 1;
        tv_.get_column(column_id)->set_cell_data_func(*renderer,
            sigc::bind(sigc::mem_fun(*this, &MountpointSelectDialog::cell_data_func_size),
                       &columns_.available));
    }

    tv_.append_column("Type", columns_.type);

    {
        Gtk::CellRendererProgress *renderer = Gtk::manage(new Gtk::CellRendererProgress());
        int column_id = tv_.append_column("Used %", *renderer) - 1;
        tv_.get_column(column_id)->add_attribute(renderer->property_value(),
                                                 columns_.used_percentage);
    }

    tv_.get_selection()->signal_changed().connect(
        sigc::mem_fun(*this, &MountpointSelectDialog::on_list_selection_changed));
    tv_.signal_row_activated().connect(sigc::mem_fun(*this,
                                       &MountpointSelectDialog::on_list_row_activated));

    // populate
    std::ifstream m_f;
    m_f.open("/proc/mounts");
    std::locale clocale("C");
    m_f.imbue(clocale);
    std::string m_device, m_mountpoint, m_type, m_options, m_freq, m_passno;

    while (!m_f.eof()) {
        m_f >> m_device >> m_mountpoint >> m_type >> m_options >> m_freq >> m_passno;
        struct statfs64 sfsb;
        struct stat64 sb;
        if (0 != statfs64(m_mountpoint.c_str(), &sfsb))
            continue;
        if (0 != lstat64(m_mountpoint.c_str(), &sb))
            continue;
        if (sfsb.f_blocks == 0)
            continue; // pseudo-fs's have zero size
        if ("tmpfs" == m_type)
            continue; // tmpfs has neither FIEMAP not FIBMAP
        if ("devtmpfs" == m_type)
            continue; // the same as tmpfs

        Gtk::TreeModel::Row row = *(liststore_->append());
        row[columns_.mountpoint] = m_mountpoint;
        row[columns_.type] = m_type;
        row[columns_.size] = sfsb.f_blocks * sb.st_blksize;
        row[columns_.used] = (sfsb.f_blocks - sfsb.f_bfree) * sb.st_blksize;
        row[columns_.available] = sfsb.f_bavail * sb.st_blksize;
        row[columns_.used_percentage] = 100 * (sfsb.f_blocks - sfsb.f_bfree) / sfsb.f_blocks;
    }

    m_f.close();
}

MountpointSelectDialog::~MountpointSelectDialog(void)
{
}

Glib::ustring&
MountpointSelectDialog::get_path(void)
{
    return selected_path_;
}

void
MountpointSelectDialog::on_list_selection_changed(void)
{
    auto iter = tv_.get_selection()->get_selected();
    if (iter)
        selected_path_ = (*iter)[columns_.mountpoint];
}

void
MountpointSelectDialog::on_list_row_activated(const Gtk::TreeModel::Path &path,
                                              Gtk::TreeViewColumn *column)
{
    auto iter = liststore_->get_iter(path);
    if (iter) {
        selected_path_ = (*iter)[columns_.mountpoint];
        response(Gtk::RESPONSE_OK);
    }
}

void
MountpointSelectDialog::cell_data_func_size(Gtk::CellRenderer *cell,
                                            const Gtk::TreeModel::iterator &iter,
                                            Gtk::TreeModelColumn<uint64_t> *column)
{
    auto *renderer = dynamic_cast<Gtk::CellRendererText *>(cell);
    std::string filesize_string;
    uint64_t size = (*iter)[*column];

    Util::format_filesize(size, filesize_string);
    renderer->property_text() = filesize_string;
}
