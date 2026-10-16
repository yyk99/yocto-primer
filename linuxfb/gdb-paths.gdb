# Source path mappings for the debug builds in meta-linuxfb.
#
# Yocto compiles with -fdebug-prefix-map, so gdb looks for sources under
# /usr/src/debug/<recipe>/<version>/ (the recipe's S directory). Map those
# to the files in this checkout. Run gdb from linuxfb/:
#
#   gdb-multiarch -x gdb-paths.gdb ~/dbg/linuxfb/usr/bin/linuxfb-demo
#
# The version (1.0) is the recipe's PV; change it here if the recipe's does.
python
import os
layer = os.path.join(os.getcwd(), "meta-linuxfb")
for prefix, rel in (
    ("/usr/src/debug/linuxfb-demo/1.0", "recipes-qt/linuxfb-demo/files"),
    ("/usr/src/debug/debug-demo/1.0", "recipes-apps/debug-demo/files"),
):
    gdb.execute("set substitute-path %s %s" % (prefix, os.path.join(layer, rel)))
end
