#!/bin/sh
#
#
# Pre-flight script for Morpheus bundle for MacOS
# 
# Author: Walter de Back (walter.deback@tu-dresden.de)

echo "Starting Morpheus..."

# Find the location of the resources 
# This includes morpheus executable and morpheus.xsd 
M_BUNDLE="`echo "$0" | sed -e 's/.command//' | sed -e 's/\/Contents\/MacOS\/Morpheus//'`"
M_BINARIES="$M_BUNDLE/Contents/MacOS/bin"
M_LIBRARIES="$M_BUNDLE/Contents/Frameworks"
M_RESOURCES="$M_BUNDLE/Contents/Resources"
M_PLUGINS="$M_BUNDLE/Contents/plugins"

echo "M_BUNDLE: $M_BUNDLE"
echo "M_RESOURCES: $M_RESOURCES"

# Get path to gnuplot
#M_GNUPLOT_PATH=`which gnuplot`;
#M_GNUPLOT="`echo "$M_GNUPLOT_PATH" | sed -e 's/gnuplot//'`"
#echo "M_GNUPLOT: $M_GNUPLOT"

M_GNUPLOT="/Applications/Gnuplot.app/Contents/MacOS:/Applications/Gnuplot.app/Contents/Resources/bin"

# Add Resources folder in system PATH
export PATH=$M_GNUPLOT:$M_RESOURCES:$M_RESOURCES/share:$PATH

# Make sure morpheus is executable
echo "chmod 755 $M_BINARIES/morpheus"
echo "chmod 755 $M_BINARIES/morpheus-gui"
chmod 755 "$M_BINARIES/morpheus"
chmod 755 "$M_BINARIES/morpheus-gui"

ls -la "$M_BINARIES"

# Make sure that exectuables can find the libraries
export DYLD_LIBRARY_PATH=$M_LIBRARIES:$M_PLUGINS
echo "DYLD_LIBRARY_PATH: " $DYLD_LIBRARY_PATH

#export DYLD_PRINT_LIBRARIES=1 #note: any value will enable printing (comment out to disable)

# Make sure that Qt can find the SQL plugin
export QT_PLUGIN_PATH="$M_PLUGINS"
echo "QT_PLUGIN_PATH=$QT_PLUGIN_PATH"

# Run Morpheus-GUI
exec "$M_BINARIES/morpheus-gui"

