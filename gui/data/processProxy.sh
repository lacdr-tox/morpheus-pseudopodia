#!/bin/bash

# a = action which will be executed by the processProxy.sh
# m = xml-file which describes the model which will be simulated
# i = id of the process its state will be checked
# d = directory name which will be created by cpmsim and which will be copied to local-homedirectory after simulation finished
# f = output-folder in which the simulation directorys will be created

while getopts ':a:m:i:n:e:d:f:' OPTION;
do
	case $OPTION in
		a) ACTION=$OPTARG
		;;

		m) MODEL=$OPTARG
                   OUT=$MODEL.out
                   ERR=$MODEL.err
		;;

		i) ID=$OPTARG
		;;

		n) THREADS=$OPTARG
		;;

		e) EXECUTABLE=$OPTARG
		;;

		d) SIMDIR=$OPTARG
		;;

		f) BASEDIR=$OPTARG
		;;

		\?) echo "processProxy: Unknown option \"-$OPTARG\"."
		;;

		:) echo "processProxy: Option \"-$OPTARG\" requires an argument."
		;;

		*) echo "processProxy: Unknown option ...\"$OPTION\"... "
         	;;
	esac
done

case $ACTION in
        start)
                cd ${BASEDIR}/${SIMDIR}/
                touch $OUT $ERR
				export OMP_NUM_THREADS=$THREADS
                bsub -o $OUT -e $ERR -a openmp -n $THREADS -R "span[hosts=1]" $EXECUTABLE $MODEL
                cd -
                date >> processProxy.out
                echo -e "\tStart: " bsub -o $OUT -e $ERR -a openmp -n $THREADS -R "span[hosts=1]" $EXECUTABLE $MODEL >> processProxy.out
                echo "in dir: ${BASEDIR}/${SIMDIR}"
        ;;

        stop)
                bkill $ID
                date >> processProxy.out
                echo -e "\tStop: " bkill $ID >> processProxy.out
        ;;

        check)
                bjobs -a | grep $ID | awk '{print $3;}'
                date >> processProxy.out
                echo -e "\tCheck:" bjobs -a | grep $ID | awk '{print $3;}' >> processProxy.out
        ;;

        zip)
                cd ${BASEDIR}
                bsub tar czf ${SIMDIR}.tar.gz $SIMDIR
                cd -
                date >> processProxy.out
                echo -e "\tZip: " tar czf $SIMDIR.tar.gz $SIMDIR  >> processProxy.out
        ;;

        delete)
                rm ${BASEDIR}/$SIMDIR.tar.gz
                rm -rf ${BASEDIR}/$SIMDIR
                date >> processProxy.out
                echo -e "\tDelete: " rm ${BASEDIR}/$SIMDIR.tar.gz  >> processProxy.out
        ;;

        debug)
				cd ${BASEDIR}/${SIMDIR}/
				echo "set logging file gdb.log" >> gdb_cmd.txt
				echo "set logging on" >> gdb_cmd.txt
				echo "set logging overwrite" >> gdb_cmd.txt
				echo "run " $MODEL >> gdb_cmd.txt
				echo "backtrace full " >> gdb_cmd.txt
				echo "quit " >> gdb_cmd.txt
			
				gdb --command=gdb_cmd.txt  --exec $EXECUTABLE 
                date >> processProxy.out
                echo -e "\tDebug: " $EXECUTABLE " " $MODEL  >> processProxy.out
				rm gdb_cmd.txt

        ;;


        revision)
                 $EXECUTABLE --revision
                date >> processProxy.out
                echo -e "\tREVISION: " $EXECUTABLE >> processProxy.out
        ;;

        version)
                $EXECUTABLE --version
                date >> processProxy.out
                echo -e "\tVERSION " $EXECUTABLE >> processProxy.out
        ;;

        gnuplot-version)
				$EXECUTABLE --gnuplot-version
				data >> processProxy.out
				echo -e "\tGnuPlot-Version " $EXECUTABLE >> processProxy.out
		;;
        *)
                echo "No value given for parameter '-a'!\n"
                echo "No action can be executed!\n"
        ;;
esac

exit 0;
