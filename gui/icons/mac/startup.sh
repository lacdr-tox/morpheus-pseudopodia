#!/bin/sh
#
#
# Pre-flight script for Morpheus bundle for MacOS
# 
# Author: Walter de Back (walter.deback@tu-dresden.de)


# Find the location of the resources 
# This includes morpheus executable and morpheus.xsd 
M_BUNDLE="`echo "$0" | sed -e 's/\/Contents\/MacOS\/Morpheus//'`"
M_RESOURCES="$M_BUNDLE/Contents/Resources"

echo "Running $0"
echo "M_BUNDLE: $M_BUNDLE"
echo "M_RESOURCES: $M_RESOURCES"

# Get path to gnuplot
#M_GNUPLOT_PATH=`which gnuplot`;
#M_GNUPLOT="`echo "$M_GNUPLOT_PATH" | sed -e 's/gnuplot//'`"
#echo "M_GNUPLOT: $M_GNUPLOT"

# Add Resources folder in system PATH
export PATH=$M_RESOURCES:$M_RESOURCES/examples:$M_RESOURCES/share:$M_RESOURCES/bin:/opt/local/bin:$PATH
echo "PATH = $PATH"

# Make sure morpheus is executable
chmod 744 $M_RESOURCES/bin/morpheus

# Run Morpheus-GUI
exec "$M_RESOURCES/bin/morpheus-gui"