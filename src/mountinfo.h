#ifndef __MOUNT_H__
#define __MOUNT_H__
#include  <dbus/dbus-glib.h>

typedef enum
{
    MOUNT_TYPE_FILESYSTEM,
    MOUNT_TYPE_SWAP
} MountType;

typedef struct _MountInfo MountInfo;
struct _MountInfo
{
    GObject parent_instance;

    gchar *mount_path;
    dev_t dev;
    MountType type;
};

typedef struct _MountInfoClass MountInfoClass;

struct _MountInfoClass
{
    GObjectClass parent_class;
};

#define MOUNT_INFO_TYPE         (mount_info_get_type ())
#define MOUNT_INFO(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), MOUNT_INFO_TYPE, MountInfo))
#define IS_MOUNT_INFO(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), MOUNT_INFO_TYPE))

GType            mount_info_get_type       (void) G_GNUC_CONST;
MountType  mount_info_get_mount_type (MountInfo *mount);
const gchar     *mount_info_get_mount_path (MountInfo *mount);
dev_t            mount_info_get_dev        (MountInfo *mount);
gint             mount_info_compare        (MountInfo *mount,
                                              MountInfo *other_mount);
MountInfo *_mount_info_new (dev_t dev,
                   const gchar *mount_path,
                   MountType type);

#endif