#include <gtk/gtk.h>
#include <gio/gdesktopappinfo.h>
#include <glib/gstdio.h>
#include <string.h>
#include <time.h>

typedef struct {
    GPtrArray *apps;
    GtkWidget *grid;
} AppList;

static GFileMonitor *registry_monitor;

static void free_command_data(gpointer data, GClosure *closure) {
    (void)closure;
    g_free(data);
}

static void apply_registry_theme(void) {
    GKeyFile *key_file = g_key_file_new();
    char *path = g_build_filename(g_get_user_config_dir(), "lanet",
                                  "registry.ini", NULL);
    GError *error = NULL;
    gboolean prefer_dark = TRUE;

    if (g_key_file_load_from_file(key_file, path, G_KEY_FILE_NONE, NULL)) {
        prefer_dark = g_key_file_get_boolean(key_file, "desktop",
                                             "prefer-dark", &error);
        if (error) {
            g_clear_error(&error);
            prefer_dark = TRUE;
        }
    }
    g_object_set(gtk_settings_get_default(),
                 "gtk-application-prefer-dark-theme", prefer_dark, NULL);
    g_key_file_unref(key_file);
    g_free(path);
}

static void on_registry_changed(GFileMonitor *monitor, GFile *file,
                                GFile *other_file, GFileMonitorEvent event,
                                gpointer data) {
    (void)monitor;
    (void)file;
    (void)other_file;
    (void)data;
    if (event == G_FILE_MONITOR_EVENT_CHANGED ||
        event == G_FILE_MONITOR_EVENT_CREATED ||
        event == G_FILE_MONITOR_EVENT_CHANGES_DONE_HINT ||
        event == G_FILE_MONITOR_EVENT_MOVED_IN ||
        event == G_FILE_MONITOR_EVENT_RENAMED) {
        apply_registry_theme();
    }
}

static void watch_registry(void) {
    char *directory = g_build_filename(g_get_user_config_dir(), "lanet", NULL);
    g_mkdir_with_parents(directory, 0700);
    GFile *file = g_file_new_for_path(directory);
    GError *error = NULL;
    registry_monitor = g_file_monitor_directory(file, G_FILE_MONITOR_NONE,
                                                 NULL, &error);
    if (registry_monitor) {
        g_signal_connect(registry_monitor, "changed",
                         G_CALLBACK(on_registry_changed), NULL);
    } else if (error) {
        g_warning("Could not monitor Lanet settings: %s", error->message);
        g_error_free(error);
    }
    g_object_unref(file);
    g_free(directory);
}

static gboolean update_clock(GtkWidget *label) {
    char time_text[32];
    time_t now = time(NULL);
    struct tm *local = localtime(&now);
    strftime(time_text, sizeof(time_text), "%H:%M", local);
    gtk_label_set_text(GTK_LABEL(label), time_text);
    return G_SOURCE_CONTINUE;
}

static void launch_app(GtkWidget *button, gpointer data) {
    (void)button;
    GDesktopAppInfo *info = G_DESKTOP_APP_INFO(data);
    GError *error = NULL;
    if (!g_app_info_launch(G_APP_INFO(info), NULL, NULL, &error)) {
        g_warning("Could not launch application: %s", error->message);
        g_error_free(error);
    }
}

static void run_command(GtkWidget *button, gpointer data) {
    (void)button;
    GError *error = NULL;
    if (!g_spawn_command_line_async((const char *)data, &error)) {
        g_warning("Could not run command: %s", error->message);
        g_error_free(error);
    }
}

static gint app_compare(gconstpointer left, gconstpointer right) {
    GDesktopAppInfo *a = *(GDesktopAppInfo **)left;
    GDesktopAppInfo *b = *(GDesktopAppInfo **)right;
    const char *name_a = g_app_info_get_name(G_APP_INFO(a));
    const char *name_b = g_app_info_get_name(G_APP_INFO(b));
    return g_ascii_strcasecmp(name_a, name_b);
}

static void load_applications(AppList *state) {
    GList *infos = g_app_info_get_all();
    for (GList *item = infos; item; item = item->next) {
        if (G_IS_DESKTOP_APP_INFO(item->data)) {
            GDesktopAppInfo *info = item->data;
            if (!g_desktop_app_info_get_show_in(info, NULL)) {
                g_object_unref(info);
                continue;
            }
            g_ptr_array_add(state->apps, info);
        } else {
            g_object_unref(item->data);
        }
    }
    g_list_free(infos);
    g_ptr_array_sort(state->apps, app_compare);
}

static GtkWidget *create_app_button(GDesktopAppInfo *info) {
    GtkWidget *button = gtk_button_new();
    GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 7);
    GtkWidget *icon = gtk_image_new_from_gicon(
        g_app_info_get_icon(G_APP_INFO(info)), GTK_ICON_SIZE_DIALOG);
    GtkWidget *label = gtk_label_new(g_app_info_get_name(G_APP_INFO(info)));

    gtk_widget_set_size_request(button, 104, 104);
    gtk_widget_set_name(button, "app-item");
    gtk_widget_set_tooltip_text(button, g_app_info_get_description(G_APP_INFO(info)));
    gtk_widget_set_focus_on_click(button, FALSE);
    gtk_label_set_ellipsize(GTK_LABEL(label), PANGO_ELLIPSIZE_END);
    gtk_label_set_max_width_chars(GTK_LABEL(label), 12);

    gtk_box_pack_start(GTK_BOX(box), icon, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(box), label, FALSE, FALSE, 0);
    gtk_container_add(GTK_CONTAINER(button), box);
    g_signal_connect(button, "clicked", G_CALLBACK(launch_app), info);
    g_object_set_data_full(G_OBJECT(button), "lanet-app-name",
                           g_strdup(g_app_info_get_name(G_APP_INFO(info))), g_free);
    return button;
}

static void rebuild_grid(AppList *state, const char *query) {
    GList *children = gtk_container_get_children(GTK_CONTAINER(state->grid));
    for (GList *item = children; item; item = item->next) {
        gtk_widget_destroy(GTK_WIDGET(item->data));
    }
    g_list_free(children);

    guint index = 0;
    char folded_query[256];
    g_strlcpy(folded_query, query ? query : "", sizeof(folded_query));
    char *folded = g_utf8_casefold(folded_query, -1);

    for (guint i = 0; i < state->apps->len; ++i) {
        GDesktopAppInfo *info = g_ptr_array_index(state->apps, i);
        const char *name = g_app_info_get_name(G_APP_INFO(info));
        char *folded_name = g_utf8_casefold(name, -1);
        if (*folded == '\0' || strstr(folded_name, folded)) {
            GtkWidget *button = create_app_button(info);
            gtk_grid_attach(GTK_GRID(state->grid), button, index % 5, index / 5, 1, 1);
            index++;
        }
        g_free(folded_name);
    }
    g_free(folded);
    gtk_widget_show_all(state->grid);
}

static void on_search_changed(GtkSearchEntry *entry, gpointer data) {
    AppList *state = data;
    rebuild_grid(state, gtk_entry_get_text(GTK_ENTRY(entry)));
}

static GtkWidget *create_dock_item(const char *icon_name, const char *label,
                                   const char *command) {
    GtkWidget *button = gtk_button_new();
    GtkWidget *icon = gtk_image_new_from_icon_name(icon_name, GTK_ICON_SIZE_BUTTON);
    GtkWidget *box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 7);
    GtkWidget *text = gtk_label_new(label);

    gtk_widget_set_name(button, "dock-item");
    gtk_widget_set_tooltip_text(button, label);
    gtk_box_pack_start(GTK_BOX(box), icon, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(box), text, FALSE, FALSE, 0);
    gtk_container_add(GTK_CONTAINER(button), box);

    if (command) {
        g_signal_connect_data(button, "clicked", G_CALLBACK(run_command),
                              g_strdup(command), free_command_data,
                              G_CONNECT_DEFAULT);
    }
    return button;
}

static GtkWidget *create_topbar(void) {
    GtkWidget *bar = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
    GtkWidget *brand = gtk_label_new("LANET");
    GtkWidget *clock = gtk_label_new("");

    gtk_widget_set_name(bar, "topbar");
    gtk_widget_set_hexpand(bar, TRUE);
    gtk_widget_set_name(brand, "brand");
    gtk_widget_set_name(clock, "clock");
    gtk_box_pack_start(GTK_BOX(bar), brand, FALSE, FALSE, 12);
    gtk_box_pack_end(GTK_BOX(bar), clock, FALSE, FALSE, 12);
    update_clock(clock);
    g_timeout_add_seconds(1, (GSourceFunc)update_clock, clock);
    return bar;
}

static GtkWidget *create_dock(void) {
    GtkWidget *dock = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
    gtk_widget_set_name(dock, "dock");
    gtk_widget_set_halign(dock, GTK_ALIGN_CENTER);

    gtk_box_pack_start(GTK_BOX(dock),
        create_dock_item("system-file-manager", "Files", "thunar"), FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(dock),
        create_dock_item("utilities-terminal", "Terminal", "xfce4-terminal"), FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(dock),
        create_dock_item("preferences-system", "Settings", "lanet-settings"), FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(dock),
        create_dock_item("system-log-out", "Session", "swaymsg exit"), FALSE, FALSE, 0);
    return dock;
}

int main(int argc, char **argv) {
    gtk_init(&argc, &argv);
    apply_registry_theme();
    watch_registry();

    GtkWidget *window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(window), "Lanet OS Shell");
    gtk_window_set_decorated(GTK_WINDOW(window), FALSE);
    gtk_window_fullscreen(GTK_WINDOW(window));

    gtk_window_set_default_size(GTK_WINDOW(window), 1280, 720);
    gtk_widget_set_name(window, "desktop");

    GtkWidget *root = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_container_add(GTK_CONTAINER(window), root);
    gtk_box_pack_start(GTK_BOX(root), create_topbar(), FALSE, FALSE, 0);

    GtkWidget *content = gtk_box_new(GTK_ORIENTATION_VERTICAL, 14);
    gtk_widget_set_margin_top(content, 42);
    gtk_widget_set_margin_bottom(content, 20);
    gtk_widget_set_margin_start(content, 42);
    gtk_widget_set_margin_end(content, 42);
    gtk_widget_set_hexpand(content, TRUE);
    gtk_widget_set_vexpand(content, TRUE);
    gtk_box_pack_start(GTK_BOX(root), content, TRUE, TRUE, 0);

    GtkWidget *search = gtk_search_entry_new();
    gtk_widget_set_name(search, "launcher-search");
    gtk_widget_set_size_request(search, 360, 42);
    gtk_widget_set_halign(search, GTK_ALIGN_CENTER);
    gtk_entry_set_placeholder_text(GTK_ENTRY(search), "Search applications");
    gtk_box_pack_start(GTK_BOX(content), search, FALSE, FALSE, 0);

    AppList state = {0};
    state.apps = g_ptr_array_new_with_free_func(g_object_unref);
    load_applications(&state);

    state.grid = gtk_grid_new();
    gtk_grid_set_row_spacing(GTK_GRID(state.grid), 14);
    gtk_grid_set_column_spacing(GTK_GRID(state.grid), 14);
    gtk_widget_set_halign(state.grid, GTK_ALIGN_CENTER);
    gtk_widget_set_vexpand(state.grid, TRUE);
    gtk_box_pack_start(GTK_BOX(content), state.grid, TRUE, FALSE, 0);

    g_signal_connect(search, "search-changed", G_CALLBACK(on_search_changed), &state);
    rebuild_grid(&state, "");

    gtk_box_pack_start(GTK_BOX(root), create_dock(), FALSE, FALSE, 12);

    GtkCssProvider *css = gtk_css_provider_new();
    const char *css_paths[] = {
        "data/lanet-shell.css",
        "/usr/share/lanet-shell/lanet-shell.css",
        NULL
    };
    GError *error = NULL;
    gboolean css_loaded = FALSE;
    for (const char **path = css_paths; *path && !css_loaded; ++path) {
        if (g_file_test(*path, G_FILE_TEST_EXISTS)) {
            gtk_css_provider_load_from_path(css, *path, &error);
            css_loaded = TRUE;
        }
    }
    if (!css_loaded || error) {
        g_printerr("Could not load Lanet shell stylesheet: %s\n",
                   error ? error->message : "no stylesheet found");
        if (error) g_error_free(error);
    }
    gtk_style_context_add_provider_for_screen(
        gdk_screen_get_default(), GTK_STYLE_PROVIDER(css),
        GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);

    g_signal_connect(window, "destroy", G_CALLBACK(gtk_main_quit), NULL);
    gtk_widget_show_all(window);
    gtk_widget_grab_focus(search);

    gtk_main();
    g_ptr_array_free(state.apps, TRUE);
    g_object_unref(css);
    return 0;
}
