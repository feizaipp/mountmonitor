#include "mountinfo.h"

G_DEFINE_TYPE (MountInfo, mount_info, G_TYPE_OBJECT);

static void
mount_info_init (MountInfo *mount)
{
}

static void
mount_info_finalize (GObject *object)
{
    MountInfo *mount = MOUNT_INFO (object);

    g_free (mount->mount_path);

    if (G_OBJECT_CLASS (mount_info_parent_class)->finalize)
        G_OBJECT_CLASS (mount_info_parent_class)->finalize (object);
}

static void
mount_info_class_init (MountInfoClass *klass)
{
    GObjectClass *gobject_class;

    gobject_class = G_OBJECT_CLASS (klass);
    gobject_class->finalize = mount_info_finalize;
}

MountInfo *
_mount_info_new (dev_t dev,
                   const gchar *mount_path,
                   MountType type)
{
    MountInfo *mount;

    mount = MOUNT_INFO (g_object_new (MOUNT_INFO_TYPE, NULL));
    mount->dev = dev;
    mount->mount_path = g_strdup (mount_path);
    mount->type = type;

    return mount;
}

gint
mount_info_compare (MountInfo  *mount,
                      MountInfo  *other_mount)
{
    gint ret;

    g_return_val_if_fail (IS_MOUNT_INFO (mount), 0);
    g_return_val_if_fail (IS_MOUNT_INFO (other_mount), 0);

    ret = g_strcmp0 (other_mount->mount_path, mount->mount_path);
    if (ret != 0)
    goto out;

    ret = (mount->dev - other_mount->dev);
    if (ret != 0)
    goto out;

    ret = mount->type - other_mount->type;

    out:
    return ret;
}

const gchar *
mount_info_get_mount_path (MountInfo *mount)
{
    g_return_val_if_fail (IS_MOUNT_INFO (mount), NULL);
    g_return_val_if_fail (mount->type == MOUNT_TYPE_FILESYSTEM, NULL);
    return mount->mount_path;
}

dev_t
mount_info_get_dev (MountInfo *mount)
{
    g_return_val_if_fail (IS_MOUNT_INFO (mount), 0);
    return mount->dev;
}

