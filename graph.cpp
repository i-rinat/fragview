#define _XOPEN_SOURCE 500
#include <gtk/gtk.h>
#include <glib.h>
#include "gtk_fragmap.h"
#include "clusters.h"
#include "filelistview.h"
#include <pthread.h>
#include <unistd.h>
#include <assert.h>
#include <pwd.h>

static int collection_done = 0;

pthread_t wt, ut;

static void destroy( GtkWidget *widget, gpointer data) {

    printf("cancel worker thread ... ");
    pthread_cancel(wt);
    pthread_join(wt, NULL);
    printf("done.\n");

    printf("cancel updater thread ... ");
    pthread_cancel(ut);
    pthread_join(ut, NULL);
    printf("done.\n");

    gtk_main_quit();
}

static void *worker_thread(void *arg) {
    const char *initial_dir = (const char *) arg;

    collect_fragments(initial_dir, &files2, &files2_mutex);
    printf("===> collecting done <===\n");
    collection_done = 1;

    return NULL;
}

static void *updater_thread(void *arg) {
    static int files_count = 0;

    while (!collection_done) {
        usleep(1*1000*1000);
        if (files2.size() != files_count) {
            files_count = files2.size();
            GTK_FRAGMAP((GtkWidget *)arg)->force_redraw = 1;
            gtk_widget_queue_draw((GtkWidget *)arg);
            printf("queue draw\n");
        }
    }
    return NULL;
}

static void refill_model( GtkWidget *file_list, GtkTreeModel *model) {
    gtk_tree_view_set_model( GTK_TREE_VIEW (file_list),
                                GTK_TREE_MODEL(model));
}

int main(int argc, char *argv[]) {

    const char *initial_dir;
    if (argc > 1) {
        initial_dir = argv[1];
    } else {
        struct passwd *pw = getpwuid(getuid());
        assert(pw != NULL);
        initial_dir = pw->pw_dir;
    }

    GtkWidget *window;
    GtkWidget *fm;
    GtkWidget *file_list_view;
    GtkWidget *scroll_area1;
    GtkWidget *vpaned;

    gtk_init (&argc, &argv);
    window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
    gtk_widget_set_size_request (GTK_WIDGET (window), 200, 100);

    vpaned = gtk_vpaned_new();
    gtk_container_add (GTK_CONTAINER(window), vpaned);
    gtk_widget_show(vpaned);

    fm = gtk_fragmap_new();
    gtk_widget_set_size_request ( fm, 100, 50);
    gtk_paned_add1 ( GTK_PANED(vpaned), fm);

    file_list_view = file_list_view_new();

    scroll_area1 = gtk_scrolled_window_new(NULL, NULL);
    gtk_widget_show(scroll_area1);
    gtk_container_add( GTK_CONTAINER(scroll_area1), file_list_view);
    gtk_paned_add2( GTK_PANED(vpaned), scroll_area1);

    gtk_widget_show(file_list_view);

    g_signal_connect (window, "destroy", G_CALLBACK(destroy), window);

    gtk_fragmap_set_device_size(GTK_FRAGMAP(fm),
            get_device_size_in_blocks(initial_dir));
    gtk_fragmap_attach_cluster_list(GTK_FRAGMAP(fm), &clusters2, &clusters2_mutex);
    gtk_fragmap_attach_file_list(GTK_FRAGMAP(fm), &files2, &files2_mutex);

    gtk_fragmap_attach_widget_file_list(GTK_FRAGMAP(fm), file_list_view, &refill_model);
    file_list_view_attach_fragmap (fm);

    gtk_widget_show(fm);
    gtk_widget_show (window);

    pthread_create(&wt, NULL, &worker_thread, (void *) initial_dir);
    pthread_create(&ut, NULL, &updater_thread, (void *)fm);

    gtk_main ();

    return 0;
}
