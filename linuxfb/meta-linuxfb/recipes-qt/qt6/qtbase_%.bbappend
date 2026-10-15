# Trim qtbase for the linuxfb image. The demo is a Widgets app with no
# networking, D-Bus or text-boundary needs, so drop the features that pull
# in big or slow-to-build dependencies:
#   icu     ~25-30 MB of data and a slow recipe; Qt falls back to its own
#           Unicode handling (no ICU collation or break iterators)
#   openssl no QtNetwork TLS
#   glib    no glib event dispatcher
#   dbus    no QtDBus
# Target only: qtbase-native keeps its defaults for the host tools.
PACKAGECONFIG:remove:class-target = "icu openssl glib dbus"
