#! /usr/bin/python

import dbus
import gobject
import dbus.mainloop.glib

def MountAdded(serial, vendor, model, uuid):
    print("MountAdded serial:%s vendor:%s model:%s uuid:%s" % (serial, vendor, model, uuid))

def MountRemoved(serial, vendor, model, uuid):
    print("MountRemoved serial:%s vendor:%s model:%s uuid:%s" % (serial, vendor, model, uuid))

dbus.mainloop.glib.DBusGMainLoop(set_as_default=True)
bus = dbus.SessionBus()
obj = bus.get_object("org.freedesktop.MountMonitor", "/org/freedesktop/MountMonitor")
interface = dbus.Interface(obj, "org.freedesktop.MountMonitor.Base")
interface.connect_to_signal("MountAdded", MountAdded)
interface.connect_to_signal("MountRemoved", MountRemoved)
gobject.MainLoop().run()
