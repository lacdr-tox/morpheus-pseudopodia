#!/bin/bash
#
#
# Script to retire the internal links of Frameworks, after they have been messed up by make_bundle.sh
#
# Needs to be called as sudo
#


MACPORTS_DIR="/opt/local"
QT_DIR="$MACPORTS_DIR/libexec/qt4/Library/Frameworks"
LIB_DIR="$MACPORTS_DIR/lib"

QT_FRAMEWORKS="QtCore QtGui QtSql QtXml QtNetwork QtHelp QtWebKit QtCLucene QtXmlPatterns QtSvg"


getDependencies(){
	TARGET=$1
	if [ -f "$TARGET" ] && [ ! -d "$TARGET" ]
	then 
		otool -L $TARGET | awk 'NR>1{ print $1 }' 
	fi
}


for QT_FW in $QT_FRAMEWORKS
do
	echo 
	echo — - - $QT_FW - - -
	echo 
	QT_FW_LIB=$QT_DIR/${QT_FW}.framework/Versions/4/${QT_FW}
	
	if [ -f $QT_FW_LIB ]
	then
		#otool -L $QT_FW_LIB 

		OLDREFS=$(getDependencies $QT_FW_LIB)

		for OLDREF in $OLDREFS
		do
			echo 
			echo — - - $OLDREF - - -
			echo
 
			if [ ! -f $OLDREF ]
			then
				NEWREF=$QT_DIR/$(basename $OLDREF).framework/Versions/4/$(basename $OLDREF)

				if [ ! -f $NEWREF ]
				then 
					NEWREF=$LIB_DIR/$(basename $OLDREF)
					if [ ! -f $NEWREF ]
					then
						echo ERROR: could not find $(basename $OLDREF)
					fi
				fi

				echo Updating internal link:
				echo Old: $OLDREF
				echo New: $NEWREF
				install_name_tool -change $OLDREF $NEWREF -id  $NEWREF $QT_FW_LIB
			else
				echo Not updating, because OLDREF exists: $OLDREF
			fi
		done
	else
		echo Framework does not exist:  $QT_FW_LIB
	fi
done
