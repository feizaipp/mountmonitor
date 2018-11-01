#ifndef __MOUNT_MONITOR_H__
#define __MOUNT_MONITOR_H__
#include "mountinfo.h"
#include <udisks/udisks.h>

typedef struct _DeviceInfo DeviceInfo;
struct _DeviceInfo {
    dev_t dev;
    gchar *mount_path;
    gchar *drive_path;
    gchar *serial;
    gchar *uuid;
    gchar *model;
    gchar *vendor;
};

typedef struct _MountMonitor MountMonitor;
struct _MountMonitor
{
    GObject parent_instance;

    GIOChannel *mounts_channel;
    GSource *mounts_watch_source;

    GIOChannel *swaps_channel;
    GSource *swaps_watch_source;

    gboolean have_data;
    GList *mounts;
};

typedef struct _MountMonitorClass MountMonitorClass;

struct _MountMonitorClass
{
    GObjectClass parent_class;

    void (*mount_added)   (MountMonitor  *monitor,
                            MountInfo         *mount);
    void (*mount_removed) (MountMonitor  *monitor,
                            MountInfo         *mount);
};

enum
{
    MOUNT_ADDED_SIGNAL,
    MOUNT_REMOVED_SIGNAL,
    LAST_SIGNAL,
};


#define MOUNT_MONITOR_TYPE         (mount_monitor_get_type ())
#define MOUNT_MONITOR(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), MOUNT_MONITOR_TYPE, MountMonitor))
#define IS_MOUNT_MONITOR(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), MOUNT_MONITOR_TYPE))

GType                mount_monitor_get_type           (void) G_GNUC_CONST;
MountMonitor  *mount_monitor_new                (void);
GList               *mount_monitor_get_mounts_for_dev (MountMonitor  *monitor,
                                                              dev_t                dev);
gboolean             mount_monitor_is_dev_in_use      (MountMonitor  *monitor,
                                                              dev_t                dev,
                                                              MountType     *out_type);

#endif