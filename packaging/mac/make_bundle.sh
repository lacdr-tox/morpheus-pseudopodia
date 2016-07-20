#!/bin/bash
#
# Creates an Application Bundle for use on MacOSX: Morpheus.app
# And a disk image: Morpheus.dmg
#
#
# Author: Walter de Back

getDependencies(){
	TARGET=$1
	if [ -f "$TARGET" ] && [ ! -d "$TARGET" ]
	then 
		otool -L $TARGET | awk 'NR>1{ print $1 }' 
	fi
}


# Read command line argument (if DMG is present, create disk image) 
CREATE_DMG=0
if [ -z "$1" ]
  then
    echo "No DMG"
  else
    if [ $1 == "dmg" ] || [ $1 == "DMG" ]
	then
	    echo "Create DMG"
	    CREATE_DMG=1
	else
		echo "Unknown argument: " $1 
	fi
fi


# Remove Morpheus.app if it exists
if [ -d Morpheus.app ]
then
	rm -rf Morpheus.app
fi

MORPHEUS_ROOT_DIR="../.."
DATA_DIR="./data"
MACPORTS_DIR="/opt/local"
# MacPorts on 10.11
QT_DIR="$MACPORTS_DIR/libexec/qt4/Library/Frameworks" 
QT_PLUGIN_DIR="$MACPORTS_DIR/libexec/qt4/share/plugins"
# MacPorts on 10.6.8
#QT_DIR="$MACPORTS_DIR/Library/Frameworks"
#QT_PLUGIN_DIR="$MACPORTS_DIR/share/qt4/plugins"

######
#
# Bundle folder structure
#
######

ROOT_DIR="Morpheus.app/Contents"
SCRIPT_DIR="$ROOT_DIR/MacOS"
BINARY_DIR="$ROOT_DIR/MacOS/bin"
LIBRARY_DIR="$ROOT_DIR/Frameworks"
RESOURCES_DIR="$ROOT_DIR/Resources"
SHARE_DIR="$ROOT_DIR/Resources/share"
DOC_DIR="$ROOT_DIR/Resources/doc"
PLUGIN_DIR="$ROOT_DIR/plugins/sqldrivers"

# Create bundle directory structure
mkdir -p $SCRIPT_DIR
mkdir -p $BINARY_DIR
mkdir -p $LIBRARY_DIR
mkdir -p $RESOURCES_DIR
mkdir -p $DOC_DIR
mkdir -p $SHARE_DIR
mkdir -p $PLUGIN_DIR

######
#
# Copy scripts/icons/executables/xsd etc to bundle
#
######

echo "Copying files"

# Copy pre-flight script to MacOS
cp -Rp ${DATA_DIR}/Morpheus.sh $SCRIPT_DIR/Morpheus

# Copy Info.plist and Icons to Bundle
cp -Rp ${DATA_DIR}/Info.plist $ROOT_DIR
cp -Rp ${DATA_DIR}/Morpheus.icns $RESOURCES_DIR/Morpheus.icns

# Copy executables to MacOS
cp -Rp $MORPHEUS_ROOT_DIR/build/morpheus/core/morpheus $BINARY_DIR
cp -Rp $MORPHEUS_ROOT_DIR/build/gui/morpheus-gui $BINARY_DIR

# Copy python scripts (will be put in a different morpheus.lab projecton GitLab)
#cp -Rp $MORPHEUS_ROOT_DIR/Scripts/morphImageTable.py $BINARY_DIR
#cp -Rp $MORPHEUS_ROOT_DIR/Scripts/morphSweepData.py $BINARY_DIR

# Copy XSD and Examples to Resources.
cp -Rp $MORPHEUS_ROOT_DIR/build/morpheus/morpheus.xsd $SHARE_DIR

# Examples are now included in GUI, thus no need to explicitly copy them into Bundle
#cp -LpR ../Examples/ $SHARE_DIR

# Copy documentation and MathJax and overwrite paths
cp -LpR $MORPHEUS_ROOT_DIR/build/gui/doc $RESOURCES_DIR
cp -Rp ${DATA_DIR}/morpheus.qhcp $DOC_DIR

# Copy qt conf to Resources to find SQL plugin directory
cp -Rp ${DATA_DIR}/qt.conf $RESOURCES_DIR

# Copy qt gui builder to Resources
cp -LpR $QT_DIR/QtGui.framework/Versions/4/Resources/qt_menu.nib $RESOURCES_DIR

# Copy SQL (sqlite) drivers and update QtCore and QtSql to it (http://www.archivum.info/qt-interest@trolltech.com/2007-08/00457/Re-OSX-Deployment--SQLite--Driver-Not-Loaded.html)
cp -Lp $QT_PLUGIN_DIR/sqldrivers/libqsqlite.dylib $PLUGIN_DIR
cp -Lp $MACPORTS_DIR/lib/libsqlite3.0.dylib $LIBRARY_DIR


######
#
# Copy Qt Frameworks into bundle
#
######
echo 
echo "== Copying Qt Frameworks into bundle and Update IDs =="
echo 

QT_FRAMEWORKS="QtCore QtGui QtSql QtXml QtNetwork QtHelp QtWebKit QtCLucene QtXmlPatterns QtSvg"

for QT_FW in $QT_FRAMEWORKS
do
	echo "cp -R $QT_DIR/${QT_FW}.framework $LIBRARY_DIR"
	cp -R $QT_DIR/${QT_FW}.framework $LIBRARY_DIR
	QT_FRAMEWORK_LIBS="$QT_FRAMEWORK_LIBS $QT_DIR/${QT_FW}.framework/Versions/4/${QT_FW}"
	
	echo "Update ID $LIB"
    	install_name_tool \
        	-id @executable_path/../../Frameworks/${QT_FW}/$QT_FW \
    		$LIBRARY_DIR/${QT_FW}.framework/${QT_FW}
        	#-id @executable_path/../../Frameworks/${QT_FW}/Versions/4/$QT_FW \
    		#$LIBRARY_DIR/${QT_FW}.framework/Versions/4/${QT_FW}
		# used to be the wrong path: $QT_DIR/${QT_FW}.framework/Versions/4/${QT_FW}
 

done



######
#
# Copy Libraries into bundle
#
######
echo
echo "== Copying libraries into bundle and Update IDs =="
echo

POSSIBLE_PATHS=" $MACPORTS_DIR/lib $MACPORTS_DIR/lib/libgcc /usr/local/lib /usr/lib"

LIBRARIES=" libz.1.dylib libtiff.5.dylib libjpeg.9.dylib libpng16.16.dylib libssl.1.0.0.dylib 
		   libcrypto.1.0.0.dylib libssh.dylib liblzma.5.dylib libbz2.1.0.dylib libexpat.1.dylib
			libgvc.6.dylib libcgraph.6.dylib libstdc++.6.dylib libgcc_s.1.dylib libgomp.1.dylib libxdot.4.dylib libcdt.5.dylib libpathplan.4.dylib" # GraphViz


for LIB in $LIBRARIES
do
	echo "-- $(basename $LIB) --"

	for PPATH in $POSSIBLE_PATHS
	do
		if [ -f $PPATH/$LIB ]; then
			echo "cp -Lp $PPATH/$LIB $LIBRARY_DIR"
			cp -Lp $PPATH/$LIB $LIBRARY_DIR
	
			echo "Update ID $LIB"
			install_name_tool \
				-id @executable_path/../../Frameworks/$LIB \
    				$LIBRARY_DIR/$LIB
			break
		else
			echo "Not found: $LIB is not in $PPATH"
		fi
		echo $LIB $PPATH
	done
done

echo "== Copying and updating IDs of libssl and libcrypto requires sudo =="

LIBRARIES="libssl.1.0.0.dylib libcrypto.1.0.0.dylib"

for LIB in $LIBRARIES
do
	echo "-- $(basename $LIB) --"

	for PPATH in $POSSIBLE_PATHS
	do
		if [ -f $PPATH/$LIB ]; then
			echo "sudo cp -Lp $PPATH/$LIB $LIBRARY_DIR"
			sudo cp -Lp $PPATH/$LIB $LIBRARY_DIR
	
			echo "Update ID $LIB"
			sudo install_name_tool \
				-id @executable_path/../../Frameworks/$LIB \
    				$LIBRARY_DIR/$LIB
			break
		else
			echo "Not found: $LIB is not in $PPATH"
		fi
		echo $LIB $PPATH
	done
done


echo
echo "== Copying SBML libraries (from /usr/local/lib/, /usr/lib and /opt/local/lib/libgcc) to bundle and Update IDs =="
echo 

LIBSBML1="libsbml.5.dylib"

if [ ! -f /usr/local/lib/$LIBSBML1 ]; then
	echo "Skipping SBML support because libSBML not found."
else
	echo "cp -Lp /usr/local/lib/$LIBSBML1 $LIBRARY_DIR"
	cp -Lp /usr/local/lib/$LIBSBML1 $LIBRARY_DIR	
	echo "Update ID $LIBSBML1"
	install_name_tool \
		-id @executable_path/../../Frameworks/$LIBSBML1 \
    		$LIBRARY_DIR/$LIBSBML1

	LIBSBML3="libstdc++.6.dylib libgcc_s.1.dylib libgomp.1.dylib"

	for LIB in $LIBSBML3
	do
		echo "cp -Lp $MACPORTS_DIR/lib/libgcc/$LIB $LIBRARY_DIR"
		cp -Lp $MACPORTS_DIR/lib/libgcc/$LIB $LIBRARY_DIR	
		
		echo "Update ID $LIB"
		install_name_tool \
			-id @executable_path/../../Frameworks/$LIB \
    			$LIBRARY_DIR/$LIB
    	
	done

	LIBRARIES=$LIBRARIES" "$LIBSBML1" "$LIBSBML2" "$LIBSBML3
fi

######
#
# Update Framework references
#
######

echo
echo "== Updating Framework Refs =="
echo

for QT_FW in $QT_FRAMEWORK_LIBS
do
	echo "-- $(basename $QT_FW) --"

	OLDREFS=$(getDependencies $QT_FW)
	#echo $QT_FW
	#echo $OLDREFS
	for REF in $OLDREFS
	do
		echo $(basename $QT_FW) depends on $(basename $REF): $REF 
	
		if [ -d "$LIBRARY_DIR/$(basename $REF).framework" ]
		then
			echo "Updating internal link from $REF"
			echo "to @executable_path/../../Frameworks/$(basename $REF).framework/$(basename $REF)"
			#echo "Update reference: $(basename $QT_FW) -> $(basename $REF)"
			install_name_tool \
				-change $REF \
				@executable_path/../../Frameworks/$(basename $REF).framework/Versions/4/$(basename $REF) \
				$LIBRARY_DIR/$(basename $QT_FW).framework/Versions/4/$(basename $QT_FW)
				#used to be: $QT_FW
			echo install_name_tool -change $REF @executable_path/../../Frameworks/$(basename $REF).framework/Versions/4/$(basename $REF) $LIBRARY_DIR/$(basename $QT_FW).framework/Versions/4/$(basename $QT_FW)
		elif [ -f "$LIBRARY_DIR/$(basename $REF)" ]
		then
			echo "Update reference: $(basename $QT_FW) -> $(basename $REF)"
			install_name_tool \
				-change $REF \
				@executable_path/../../Frameworks/$(basename $REF) \
				$LIBRARY_DIR/$(basename $QT_FW).framework/Versions/4/$(basename $QT_FW)
				#$LIBRARY_DIR/$(basename $QT_FW)
				# used to be: $QT_FW
		else
			echo "Not updating reference: $(basename $QT_FW) -> $(basename $REF)"
		fi    
	done
	
done


######
#
# Update Libraries Refs
#
######

echo
echo "== Updating Libraries Refs =="
echo


for LIB in $LIBRARIES
do
	echo "-- $LIB --"
	OLDREFS=$(getDependencies $LIBRARY_DIR/$LIB)
	#echo $LIB: $OLDREFS
	for REF in $OLDREFS
	do
		#echo $(basename $LIB) depends on $(basename $REF): $REF 
		if [ -f "$LIBRARY_DIR/$(basename $REF)" ]
		then
			echo "Update reference: $(basename $LIB) -> $(basename $REF)"
			install_name_tool \
				-change $REF \
				@executable_path/../../Frameworks/$(basename $REF) \
				$LIBRARY_DIR/$LIB
		else
			echo "Not updating reference: $(basename $LIB) -> $(basename $REF)"
		fi
	done
done

######
#
# Update plugins
#
######
echo
echo "== Updating SQL plugin == "
echo
# Copy SQL (sqlite) drivers and update QtCore and QtSql to it (http://www.archivum.info/qt-interest@trolltech.com/2007-08/00457/Re-OSX-Deployment--SQLite--Driver-Not-Loaded.html)

install_name_tool \
	-id @executable_path/../../Frameworks/libsqlite3.0.dylib \
	$LIBRARY_DIR/libsqlite3.0.dylib

install_name_tool \
	-id @executable_path/../../plugins/sqldrivers/libqsqlite.dylib \
	$PLUGIN_DIR/libqsqlite.dylib
	

echo
echo "-- libqsqlite.dylib (SQLite plugin) --"
echo 

QT_FWS="QtCore QtSql"

for QT_FW in $QT_FWS
do
	echo "Update reference: libqsqlite.dylib -> $QT_FW"
	echo install_name_tool -change $QT_DIR/${QT_FW}.framework/Versions/4/$QT_FW @executable_path/../../Frameworks/${QT_FW}.framework/$QT_FW $PLUGIN_DIR/libqsqlite.dylib
	install_name_tool \
		-change $QT_DIR/${QT_FW}.framework/Versions/4/$QT_FW \
		@executable_path/../../Frameworks/${QT_FW}.framework/Versions/4/$QT_FW \
		$PLUGIN_DIR/libqsqlite.dylib
		#-change $QT_DIR/${QT_FW}.framework/Versions/4/$QT_FW \
		#@executable_path/../../Frameworks/${QT_FW}.framework/Versions/4/$QT_FW \
done

echo "Update reference: libqsqlite.dylib -> libsqlite3.0.dylib"
install_name_tool \
	-change $MACPORTS_DIR/lib/libsqlite3.0.dylib \
	@executable_path/../../Frameworks/libsqlite3.0.dylib \
	$PLUGIN_DIR/libqsqlite.dylib 

#echo "Update reference: libqsqlite.dylib -> libqsqlite.dylib"
install_name_tool \
	-change $PLUGIN_DIR/libqsqlite.dylib \
	@executable_path/../../plugins/sqldrivers/libqsqlite.dylib \
	$PLUGIN_DIR/libqsqlite.dylib

#install_name_tool \
#	-change
#	 @executable_path/../../plugins/sqldrivers/libqsqlite.dylib  \
#	libqsqlite.dylib 
#	$PLUGIN_DIR/libqsqlite.dylib

######
#
# Update executables
#
######
echo
echo "== Updating executables == "
echo

echo
echo "-- morpheus -- "
echo

DEPS_OLD=$(getDependencies $BINARY_DIR/morpheus)

for DEP in $DEPS_OLD
do
	if [ -f $LIBRARY_DIR/$(basename $DEP) ]
	then
		echo "Update reference: morpheus -> $(basename $DEP)"
		install_name_tool \
    	    		-change $DEP \
       	 		@executable_path/../../Frameworks/$(basename $DEP) \
       	 		$BINARY_DIR/morpheus
	else
		echo "Not updating reference: morpheus -> $(basename $DEP)"
	fi 
done



echo
echo "-- morpheus-gui --"
echo


for QT_FW in $QT_FRAMEWORKS
do
	#QT_FRAMEWORK_LIBS="$QT_FRAMEWORK_LIBS $LIBRARY_DIR/$FW.framework/Versions/4/$FW"
	if [ -d $LIBRARY_DIR/${QT_FW}.framework ]
	then 
		echo "Update reference: morpheus-gui -> $QT_FW"
		install_name_tool \
   	 		-change $QT_DIR/${QT_FW}.framework/Versions/4/$QT_FW \
   	 		@executable_path/../../Frameworks/${QT_FW}.framework/Versions/4/$QT_FW \
    		$BINARY_DIR/morpheus-gui
   	 		#-change $QT_DIR/${QT_FW}.framework/$QT_FW \
   	 		#@executable_path/../../Frameworks/${QT_FW}.framework/$QT_FW \
    		#$BINARY_DIR/morpheus-gui
    else
		echo "WARNING: Not updating reference: morpheus-gui -> $(basename $QT_FW)"
    fi
done

otool -L $BINARY_DIR/morpheus-gui


DEPS_OLD=$(getDependencies $BINARY_DIR/morpheus-gui)

for DEP in $DEPS_OLD
do
	echo "Dependency: $DEP"
	if [ -f $LIBRARY_DIR/$(basename $DEP) ]
	then
		echo "Update reference: morpheus-gui -> $(basename $DEP)"
		install_name_tool \
    	    -change $DEP \
       	 	@executable_path/../../Frameworks/$(basename $DEP) \
       	 	$BINARY_DIR/morpheus-gui
	else
		echo "Not updating reference: morpheus-gui -> $(basename $DEP)"
	fi 
done

otool -L $BINARY_DIR/morpheus-gui


######
#
# Set executable flag of binaries and scripts 
#
######


# Make binaries executable 
chmod 755 $SCRIPT_DIR/Morpheus
#chmod 755 $BINARY_DIR/morphSweepData.py
#chmod 755 $BINARY_DIR/morphImageTable.py
chmod 755 $BINARY_DIR/morpheus
chmod 755 $BINARY_DIR/morpheus-gui

# change owner to default user (not root) and group
chown -R $(whoami):$(id -g -n $(whoami)) Morpheus.app

echo
echo "== Structure of Morpheus.app == "
echo

tree -d Morpheus.app


######
#
# Create disk image
#
######
echo
echo "== Create disk image == "
echo


DATE=$(date +%y%m%d)
BUNDLE_NAME="Morpheus_${DATE}.dmg"

if [ $CREATE_DMG == 1 ]
then
	echo "Creating disk image"
	imagedir="/tmp/$Morpheus.$$"
	mkdir $imagedir
	cp -RP Morpheus.app $imagedir
	cp -RP Gnuplot.app $imagedir
	ln -s /Applications $imagedir/Applications
	#cp -RP Applications $imagedir

	# TODO: copy over additional files, if any
	hdiutil create -ov -srcfolder $imagedir -format UDBZ -volname "Morpheus" $BUNDLE_NAME
	hdiutil internet-enable -yes $BUNDLE_NAME
	chown -R $(whoami):$(id -g -n $(whoami)) $BUNDLE_NAME
	rm -rf $imagedir
	echo "$BUNDLE_NAME created"
	echo $(whoami)
else
	echo "NOTE: No DMG created!" 
	echo
	echo "To create a Disk Image bundle (DMG), type:" 
	echo "sudo ./make_bundle.sh dmg"
fi

exit

