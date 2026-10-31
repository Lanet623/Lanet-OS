#include <gtk/gtk.h>
#include <glib/gstdio.h>
#include <string.h>

#define LANET_CONFIG_GROUP "desktop"
#define SERVICE_COUNT 5

static const char *services[SERVICE_COUNT] = {
    "dbus", "networking", "seatd", "sshd", "crond"
};

static char *registry_path(void) {
    return g_build_filename(g_get_user_config_dir(), "lanet", "registry.ini", NULL);
}

static GKeyFile *registry_load(void) {
    GKeyFile *key_file = g_key_file_new();
    char *path = registry_path();
    GError *error = NULL;

    if (!g_key_file_load_from_file(key_file, path, G_KEY_FILE_NONE, &error)) {
        g_clear_error(&error);
        g_key_file_set_boolean(key_file, LANET_CONFIG_GROUP, "prefer-dark", TRUE);
    }
    g_free(path);
    return key_file;
}

static gboolean registry_save(GKeyFile *key_file) {
    char *path = registry_path();
    char *directory = g_path_get_dirname(path);
    char *contents = g_key_file_to_data(key_file, NULL, NULL);
    GError *error = NULL;
    gboolean saved = FALSE;

    if (g_mkdir_with_parents(directory, 0700) == 0 &&
        g_file_set_contents(path, contents, -1, &error)) {
        saved = TRUE;
    }
    if (error) {
        g_warning("Could not save Lanet settings: %s", error->message);
        g_error_free(error);
    }
    g_free(contents);
    g_free(directory);
    g_free(path);
    return saved;
}

static gboolean valid_package_name(const char *name) {
    if (!name || !*name || strlen(name) > 128) {
        return FALSE;
    }
    for (const char *cursor = name; *cursor; ++cursor) {
        if (!(g_ascii_isalnum(*cursor) || *cursor == '+' || *cursor == '.' ||
              *cursor == '_' || *cursor == '-')) {
            return FALSE;
        }
    }
    return TRUE;
}

static gboolean valid_service(const char *service) {
    for (guint i = 0; i < SERVICE_COUNT; ++i) {
        if (g_strcmp0(service, services[i]) == 0) {
            return TRUE;
        }
    }
    return FALSE;
}

static gboolean open_admin_terminal(const char *const command[]) {
    const char *terminal = g_getenv("TERMINAL");
    if (!terminal || !*terminal) {
        terminal = "xfce4-terminal";
    }

    GPtrArray *argv = g_ptr_array_new_with_free_func(g_free);
    g_ptr_array_add(argv, g_strdup(terminal));
    g_ptr_array_add(argv, g_strdup("--title=Lanet system change"));
    g_ptr_array_add(argv, g_strdup("--execute"));
    g_ptr_array_add(argv, g_strdup("doas"));
    for (guint i = 0; command[i]; ++i) {
        g_ptr_array_add(argv, g_strdup(command[i]));
    }
    g_ptr_array_add(argv, NULL);

    GError *error = NULL;
    gboolean started = g_spawn_async(NULL, (char **)argv->pdata, NULL,
                                     G_SPAWN_SEARCH_PATH, NULL, NULL,
                                     NULL, &error);
    if (!started) {
        g_warning("Could not open privileged terminal: %s", error->message);
        g_error_free(error);
    }
    g_ptr_array_free(argv, TRUE);
    return started;
}

static void on_dark_changed(GtkSwitch *widget, GParamSpec *property,
                            gpointer data) {
    (void)property;
    GKeyFile *key_file = data;
    gboolean prefer_dark = gtk_switch_get_active(widget);
    g_key_file_set_boolean(key_file, LANET_CONFIG_GROUP, "prefer-dark", prefer_dark);
    if (registry_save(key_file)) {
        GtkSettings *settings = gtk_settings_get_default();
        g_object_set(settings, "gtk-application-prefer-dark-theme", prefer_dark, NULL);
    }
}

static void on_install_package(GtkButton *button, gpointer data) {
    (void)button;
    GtkWidget *entry = data;
    const char *package = gtk_entry_get_text(GTK_ENTRY(entry));
    if (!valid_package_name(package)) {
        gtk_entry_set_placeholder_text(GTK_ENTRY(entry), "Enter a valid APK package name");
        return;
    }
    const char *command[] = {"apk", "add", "--", package, NULL};
    open_admin_terminal(command);
}

static void on_remove_package(GtkButton *button, gpointer data) {
    (void)button;
    GtkWidget *entry = data;
    const char *package = gtk_entry_get_text(GTK_ENTRY(entry));
    if (!valid_package_name(package)) {
        gtk_entry_set_placeholder_text(GTK_ENTRY(entry), "Enter an installed APK package name");
        return;
    }
    const char *command[] = {"apk", "del", "--", package, NULL};
    open_admin_terminal(command);
}

static void on_service_clicked(GtkButton *button, gpointer data) {
    const char *service = g_object_get_data(G_OBJECT(button), "lanet-service");
    gboolean enable = GPOINTER_TO_INT(data);
    if (!valid_service(service)) {
        return;
    }
    const char *command[] = {
        "rc-update", enable ? "add" : "del", service, "default", NULL
    };
    open_admin_terminal(command);
}

static GtkWidget *create_appearance_page(GKeyFile *key_file) {
    GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 14);
    GtkWidget *row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 12);
    GtkWidget *label = gtk_label_new("Use dark appearance");
    GtkWidget *dark_switch = gtk_switch_new();
    GError *error = NULL;
    gboolean prefer_dark = g_key_file_get_boolean(
        key_file, LANET_CONFIG_GROUP, "prefer-dark", &error);

    if (error) {
        g_error_free(error);
        prefer_dark = TRUE;
    }
    gtk_widget_set_halign(label, GTK_ALIGN_START);
    gtk_widget_set_hexpand(label, TRUE);
    gtk_switch_set_active(GTK_SWITCH(dark_switch), prefer_dark);
    gtk_box_pack_start(GTK_BOX(row), label, TRUE, TRUE, 0);
    gtk_box_pack_end(GTK_BOX(row), dark_switch, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(box), row, FALSE, FALSE, 0);
    g_signal_connect(dark_switch, "notify::active",
                     G_CALLBACK(on_dark_changed), key_file);
    return box;
}

static GtkWidget *create_packages_page(void) {
    GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 12);
    GtkWidget *description = gtk_label_new(
        "Install or remove packages from the enabled Alpine repositories.");
    GtkWidget *entry = gtk_entry_new();
    GtkWidget *buttons = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
    GtkWidget *install = gtk_button_new_with_label("Install package");
    GtkWidget *remove = gtk_button_new_with_label("Remove package");

    gtk_label_set_xalign(GTK_LABEL(description), 0.0f);
    gtk_entry_set_placeholder_text(GTK_ENTRY(entry), "APK package name");
    gtk_box_pack_start(GTK_BOX(buttons), install, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(buttons), remove, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(box), description, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(box), entry, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(box), buttons, FALSE, FALSE, 0);
    g_signal_connect(install, "clicked", G_CALLBACK(on_install_package), entry);
    g_signal_connect(remove, "clicked", G_CALLBACK(on_remove_package), entry);
    return box;
}

static GtkWidget *create_services_page(void) {
    GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    GtkWidget *description = gtk_label_new(
        "Choose whether an OpenRC service starts at boot.");
    gtk_label_set_xalign(GTK_LABEL(description), 0.0f);
    gtk_box_pack_start(GTK_BOX(box), description, FALSE, FALSE, 0);

    for (guint i = 0; i < SERVICE_COUNT; ++i) {
        GtkWidget *row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
        GtkWidget *name = gtk_label_new(services[i]);
        GtkWidget *enable = gtk_button_new_with_label("Enable at boot");
        GtkWidget *disable = gtk_button_new_with_label("Disable at boot");

        gtk_widget_set_halign(name, GTK_ALIGN_START);
        gtk_widget_set_hexpand(name, TRUE);
        g_object_set_data(G_OBJECT(enable), "lanet-service", (gpointer)services[i]);
        g_object_set_data(G_OBJECT(disable), "lanet-service", (gpointer)services[i]);
        g_signal_connect(enable, "clicked", G_CALLBACK(on_service_clicked),
                         GINT_TO_POINTER(TRUE));
        g_signal_connect(disable, "clicked", G_CALLBACK(on_service_clicked),
                         GINT_TO_POINTER(FALSE));
        gtk_box_pack_start(GTK_BOX(row), name, TRUE, TRUE, 0);
        gtk_box_pack_start(GTK_BOX(row), enable, FALSE, FALSE, 0);
        gtk_box_pack_start(GTK_BOX(row), disable, FALSE, FALSE, 0);
        gtk_box_pack_start(GTK_BOX(box), row, FALSE, FALSE, 0);
    }
    return box;
}

int main(int argc, char **argv) {
    gtk_init(&argc, &argv);
    GKeyFile *key_file = registry_load();

    GtkWidget *window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(window), "Lanet Control Center");
    gtk_window_set_default_size(GTK_WINDOW(window), 680, 460);
    gtk_window_set_position(GTK_WINDOW(window), GTK_WIN_POS_CENTER);

    GtkWidget *notebook = gtk_notebook_new();
    gtk_container_set_border_width(GTK_CONTAINER(notebook), 18);
    gtk_container_add(GTK_CONTAINER(window), notebook);
    gtk_notebook_append_page(GTK_NOTEBOOK(notebook),
        create_appearance_page(key_file), gtk_label_new("Appearance"));
    gtk_notebook_append_page(GTK_NOTEBOOK(notebook),
        create_packages_page(), gtk_label_new("Packages"));
    gtk_notebook_append_page(GTK_NOTEBOOK(notebook),
        create_services_page(), gtk_label_new("Services"));

    g_signal_connect(window, "destroy", G_CALLBACK(gtk_main_quit), NULL);
    gtk_widget_show_all(window);
    gtk_main();

    g_key_file_unref(key_file);
    return 0;
}
