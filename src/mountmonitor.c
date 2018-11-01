#include "mountmonitor.h"
#include <string.h>
#include <stdio.h>
#include <sys/stat.h>

static GList *device_info_list = NULL;

G_DEFINE_TYPE (MountMonitor, mount_monitor, G_TYPE_OBJECT)

static guint signals[LAST_SIGNAL] = { 0 };

MountMonitor *
mount_monitor_new (void)
{
    return MOUNT_MONITOR (g_object_new (MOUNT_MONITOR_TYPE, NULL));
}

static void
mount_monitor_finalize (GObject *object)
{
    MountMonitor *monitor = MOUNT_MONITOR (object);

    if (monitor->mounts_channel != NULL)
    g_io_channel_unref (monitor->mounts_channel);
    if (monitor->mounts_watch_source != NULL)
    g_source_destroy (monitor->mounts_watch_source);


    g_list_foreach (monitor->mounts, (GFunc) g_object_unref, NULL);
    g_list_free (monitor->mounts);

    if (G_OBJECT_CLASS (mount_monitor_parent_class)->finalize != NULL)
    G_OBJECT_CLASS (mount_monitor_parent_class)->finalize (object);
}

static gboolean
have_mount (MountMonitor *monitor,
            dev_t               dev,
            const gchar        *mount_point)
{
    GList *l;
    gboolean ret;

    ret = FALSE;

    for (l = monitor->mounts; l != NULL; l = l->next)
    {
        MountInfo *mount = MOUNT_INFO (l->data);
        if (mount_info_get_dev (mount) == dev &&
            g_strcmp0 (mount_info_get_mount_path (mount), mount_point) == 0)
        {
            ret = TRUE;
            break;
        }
    }

    return ret;
}

static gboolean
mount_monitor_get_mountinfo (MountMonitor  *monitor,
                                    GError             **error)
{
    gboolean ret;
    gchar *contents;
    gchar **lines;
    guint n;

    ret = FALSE;
    contents = NULL;
    lines = NULL;

    if (!g_file_get_contents ("/proc/self/mountinfo", &contents, NULL, error))
    {
        g_prefix_error (error, "Error reading /proc/self/mountinfo: ");
        goto out;
    }

    /* See Documentation/filesystems/proc.txt for the format of /proc/self/mountinfo
    *
    * Note that things like space are encoded as \020.
    */
    lines = g_strsplit (contents, "\n", 0);
    for (n = 0; lines[n] != NULL; n++)
    {
        guint mount_id;
        guint parent_id;
        guint major, minor;
        gchar encoded_root[4096];
        gchar encoded_mount_point[4096];
        gchar *mount_point;
        dev_t dev;

        if (strlen (lines[n]) == 0)
            continue;

        if (sscanf (lines[n],
                    "%d %d %d:%d %4095s %4095s",
                    &mount_id,
                    &parent_id,
                    &major,
                    &minor,
                    encoded_root,
                    encoded_mount_point) != 6)
        {
            printf ("Error parsing line '%s'", lines[n]);
            continue;
        }
        encoded_root[sizeof encoded_root - 1] = '\0';
        encoded_mount_point[sizeof encoded_mount_point - 1] = '\0';

        /* Temporary work-around for btrfs, see
        *
        *  https://bugzilla.redhat.com/show_bug.cgi?id=495152#c31
        *  http://article.gmane.org/gmane.comp.file-systems.btrfs/2851
        *
        * for details.
        */
        if (major == 0)
        {
            const gchar *sep;
            sep = strstr (lines[n], " - ");
            if (sep != NULL)
            {
                gchar fstype[4096];
                gchar mount_source[4096];
                struct stat statbuf;

                if (sscanf (sep + 3, "%4095s %4095s", fstype, mount_source) != 2)
                {
                    printf ("Error parsing things past - for '%s'", lines[n]);
                    continue;
                }
                fstype[sizeof fstype - 1] = '\0';
                mount_source[sizeof mount_source - 1] = '\0';

                if (g_strcmp0 (fstype, "btrfs") != 0)
                continue;

                if (!g_str_has_prefix (mount_source, "/dev/"))
                continue;

                if (stat (mount_source, &statbuf) != 0)
                {
                    printf ("Error statting %s: %m", mount_source);
                    continue;
                }

                if (!S_ISBLK (statbuf.st_mode))
                {
                    printf ("%s is not a block device", mount_source);
                    continue;
                }

                dev = statbuf.st_rdev;
            }
            else
            {
                continue;
            }
        }
        else
        {
            dev = makedev (major, minor);
        }

        mount_point = g_strcompress (encoded_mount_point);

        /* TODO: we can probably use a hash table or something if this turns out to be slow */
        if (!have_mount (monitor, dev, mount_point))
        {
            MountInfo *mount;
            mount = _mount_info_new (dev, mount_point, MOUNT_TYPE_FILESYSTEM);
            monitor->mounts = g_list_prepend (monitor->mounts, mount);
        }

        g_free (mount_point);
    }

    ret = TRUE;

    out:
    g_free (contents);
    g_strfreev (lines);

    return ret;
}

static void
mount_monitor_ensure (MountMonitor *monitor)
{
    GError *error;

    if (monitor->have_data)
        goto out;

    error = NULL;
    if (!mount_monitor_get_mountinfo (monitor, &error))
    {
        printf ("Error getting mounts: %s (%s, %d)",
                        error->message, g_quark_to_string (error->domain), error->code);
        g_error_free (error);
    }

    monitor->have_data = TRUE;

out:
    ;
}

static void
mount_monitor_invalidate (MountMonitor *monitor)
{
  monitor->have_data = FALSE;

  g_list_foreach (monitor->mounts, (GFunc) g_object_unref, NULL);
  g_list_free (monitor->mounts);
  monitor->mounts = NULL;
}

static void
diff_sorted_lists (GList *list1,
                   GList *list2,
                   GCompareFunc compare,
                   GList **added,
                   GList **removed)
{
    int order;

    *added = *removed = NULL;

    while (list1 != NULL && list2 != NULL)
    {
        order = (*compare) (list1->data, list2->data);
        if (order < 0)
        {
            *removed = g_list_prepend (*removed, list1->data);
            list1 = list1->next;
        }
        else if (order > 0)
        {
            *added = g_list_prepend (*added, list2->data);
            list2 = list2->next;
        }
        else
        { /* same item */
            list1 = list1->next;
            list2 = list2->next;
        }
    }

    while (list1 != NULL)
    {
        *removed = g_list_prepend (*removed, list1->data);
        list1 = list1->next;
    }
    while (list2 != NULL)
    {
        *added = g_list_prepend (*added, list2->data);
        list2 = list2->next;
    }
}

static UDisksObject *
lookup_object_for_block (UDisksClient  *client,
                         dev_t          block_device)
{
  UDisksObject *ret;
  GList *objects;
  GList *l;

  ret = NULL;

  objects = g_dbus_object_manager_get_objects (udisks_client_get_object_manager (client));
  for (l = objects; l != NULL; l = l->next)
    {
      UDisksObject *object = UDISKS_OBJECT (l->data);
      UDisksBlock *block;

      block = udisks_object_peek_block (object);
      if (block != NULL)
        {
          if (block_device == udisks_block_get_device_number (block))
            {
              ret = g_object_ref (object);
              goto out;
            }
        }
    }

 out:
  g_list_foreach (objects, (GFunc) g_object_unref, NULL);
  g_list_free (objects);

  return ret;
}

static UDisksObject *lookup_object_by_path(UDisksClient  *client, const gchar *path)
{
    UDisksObject *ret;
    GList *objects;
    GList *l;

    ret = NULL;

    objects = g_dbus_object_manager_get_objects (udisks_client_get_object_manager (client));
    for (l = objects; l != NULL; l = l->next)
    {
        UDisksObject *object = UDISKS_OBJECT (l->data);
        if (g_strcmp0(g_dbus_object_get_object_path(G_DBUS_OBJECT(object)), path) == 0) {
            ret = g_object_ref (object);
            goto out;
        }
    }

out:
    g_list_foreach (objects, (GFunc) g_object_unref, NULL);
    g_list_free (objects);
    return ret;
}

static void get_device_info(dev_t dev, DeviceInfo *df)
{
    UDisksClient *client = NULL;
    GError *error = NULL;
    UDisksObject *object_block, *object_drive;
    UDisksBlock *block;
    UDisksDrive *drive;

    client = udisks_client_new_sync (NULL, /* GCancellable */ &error);
    if (client == NULL) {
        printf("Error connecting to the udisks daemon: %s\n", error->message);
        g_error_free (error);
        goto out;
    }
    object_block = lookup_object_for_block (client, dev);
    if (object_block == NULL) {
        printf("Error finding object for block device %d:%d\n", major (dev), minor (dev));
        goto out;
    }

    block = udisks_object_peek_block (object_block);
    df->uuid = g_strdup(udisks_block_get_id_uuid(block));
    df->drive_path = g_strdup(udisks_block_get_drive(block));

    object_drive = lookup_object_by_path(client, df->drive_path);
    if (object_drive == NULL) {
        printf("Error finding object for drive %s\n", df->drive_path);
        goto out;
    }
    drive = udisks_object_peek_drive(object_drive);
    df->serial = g_strdup(udisks_drive_get_serial(drive));
    df->vendor = g_strdup(udisks_drive_get_vendor(drive));
    df->model = g_strdup(udisks_drive_get_model(drive));

out:
    if (object_block != NULL)
        g_object_unref (object_block);
    if (object_drive != NULL)
        g_object_unref (object_drive);
    if (client != NULL)
        g_object_unref (client);
}

static DeviceInfo *get_devinfo_by_mount_path(GList *list, const gchar *path)
{
    gchar *p;
    DeviceInfo *tmp;
    while (list) {
        GList *next = list->next;

        tmp = (DeviceInfo *)list->data;
        p = tmp->mount_path;
        if (g_strcmp0(p, path) == 0) {
            return list->data;
        }
        list = next;
    }
    return NULL;
}

static void
reload_mounts (MountMonitor *monitor)
{
    GList *old_mounts;
    GList *cur_mounts;
    GList *added;
    GList *removed;
    GList *l;

    mount_monitor_ensure (monitor);

    old_mounts = g_list_copy (monitor->mounts);
    g_list_foreach (old_mounts, (GFunc) g_object_ref, NULL);

    mount_monitor_invalidate (monitor);
    mount_monitor_ensure (monitor);

    cur_mounts = g_list_copy (monitor->mounts);

    old_mounts = g_list_sort (old_mounts, (GCompareFunc) mount_info_compare);
    cur_mounts = g_list_sort (cur_mounts, (GCompareFunc) mount_info_compare);
    diff_sorted_lists (old_mounts, cur_mounts, (GCompareFunc) mount_info_compare, &added, &removed);

    for (l = removed; l != NULL; l = l->next)
    {
        DeviceInfo *df;
        MountInfo *mount = MOUNT_INFO (l->data);
        df = get_devinfo_by_mount_path(device_info_list, mount->mount_path);
        if (df) {
            g_signal_emit (monitor, signals[MOUNT_REMOVED_SIGNAL], 0, df->serial, df->vendor, df->model, df->uuid);
            // delete df from list
            device_info_list = g_list_remove(device_info_list, df);
            g_free(df);
        } else {
            printf("cant find device info.\n");
        }
    }

    for (l = added; l != NULL; l = l->next)
    {
        MountInfo *mount = MOUNT_INFO (l->data);
        DeviceInfo *df = g_malloc(sizeof(DeviceInfo));
        df->mount_path = g_strdup(mount->mount_path);
        df->dev = mount->dev;
        get_device_info(mount->dev, df);
        device_info_list = g_list_append(device_info_list, df);
        g_signal_emit (monitor, signals[MOUNT_ADDED_SIGNAL], 0, df->serial, df->vendor, df->model, df->uuid);
    }

    g_list_foreach (old_mounts, (GFunc) g_object_unref, NULL);
    g_list_free (old_mounts);
    g_list_free (cur_mounts);
    g_list_free (removed);
    g_list_free (added);
}

static gboolean
mounts_changed_event (GIOChannel *channel,
                      GIOCondition cond,
                      gpointer user_data)
{
    MountMonitor *monitor = MOUNT_MONITOR (user_data);
    if (cond & ~G_IO_ERR)
        goto out;
    reload_mounts (monitor);

out:
    return TRUE;
}

static void
mount_monitor_constructed (GObject *object)
{
    MountMonitor *monitor = MOUNT_MONITOR (object);
    GError *error;

    error = NULL;
    monitor->mounts_channel = g_io_channel_new_file ("/proc/self/mountinfo", "r", &error);
    if (monitor->mounts_channel != NULL)
    {
        monitor->mounts_watch_source = g_io_create_watch (monitor->mounts_channel, G_IO_ERR);
        g_source_set_callback (monitor->mounts_watch_source, (GSourceFunc) mounts_changed_event, monitor, NULL);
        g_source_attach (monitor->mounts_watch_source, g_main_context_get_thread_default ());
        g_source_unref (monitor->mounts_watch_source);
    }
    else
    {
        g_error ("No /proc/self/mountinfo file: %s", error->message);
        g_error_free (error);
    }

    if (G_OBJECT_CLASS (mount_monitor_parent_class)->constructed != NULL)
    (*G_OBJECT_CLASS (mount_monitor_parent_class)->constructed) (object);
}

static void
mount_monitor_init (MountMonitor *monitor)
{
    monitor->mounts = NULL;
}

static void
mount_monitor_class_init (MountMonitorClass *klass)
{
    GObjectClass *gobject_class = (GObjectClass *) klass;

    gobject_class->finalize    = mount_monitor_finalize;
    gobject_class->constructed = mount_monitor_constructed;

    signals[MOUNT_ADDED_SIGNAL] = g_signal_new ("mount-added",
                                                G_OBJECT_CLASS_TYPE (klass),
                                                G_SIGNAL_RUN_LAST | G_SIGNAL_DETAILED,
                                                0,
                                                NULL,
                                                NULL,
                                                g_cclosure_marshal_VOID__STRING,
                                                G_TYPE_NONE,
                                                4,
                                                G_TYPE_STRING, G_TYPE_STRING,
                                                G_TYPE_STRING, G_TYPE_STRING);

    signals[MOUNT_REMOVED_SIGNAL] = g_signal_new ("mount-removed",
                                                G_OBJECT_CLASS_TYPE (klass),
                                                G_SIGNAL_RUN_LAST | G_SIGNAL_DETAILED,
                                                0,
                                                NULL,
                                                NULL,
                                                g_cclosure_marshal_VOID__STRING,
                                                G_TYPE_NONE,
                                                4,
                                                G_TYPE_STRING, G_TYPE_STRING,
                                                G_TYPE_STRING, G_TYPE_STRING);
}