#!/bin/bash

# a = action which will be executed by the morpheusProxy.sh
# m = xml-file which describes the model which will be simulated
# i = id of the process its state will be checked
# d = directory name which will be created by cpmsim and which will be copied to local-homedirectory after simulation finished
# f = output-folder in which the simulation directorys will be created

while getopts ':a:m:i:n:e:d:f:' OPTION;
do
	case $OPTION in
		a)	ACTION=$OPTARG
		;;

		m)	MODEL=$OPTARG
			OUT=$MODEL.out
			ERR=$MODEL.err
		;;

		i)	ID=$OPTARG
		;;

		n)	THREADS=$OPTARG
		;;

		e)	EXECUTABLE=$OPTARG
		;;

		d)	SIMDIR=$OPTARG
		;;

		f)	BASEDIR=$OPTARG
		;;

		\?) echo "morpheusProxy: Unknown option \"-$OPTARG\"."
		;;

		:) echo "morpheusProxy: Option \"-$OPTARG\" requires an argument."
		;;

		*) echo "morpheusProxy: Unknown option ...\"$OPTION\"... "
         	;;
	esac
done

case $ACTION in
        start)
                cd ${BASEDIR}/${SIMDIR}/
                touch $OUT $ERR
                #create job submission script
				cat << EOF > job_${ID}.sh
#!/bin/sh
#SBATCH -o $OUT
#SBATCH -e $ERR
#SBATCH --nodes=1
#SBATCH --cpus-per-task=$THREADS
export OMP_NUM_THREADS=$THREADS
$EXECUTABLE $MODEL
EOF
				#make script executable
				chmod +x job_${ID}.sh
				# do job submission, and print job ID
				sbatch job_${ID}.sh | awk '{print $4}'
				cd -
				date >> morpheusProxy.out
				echo -e "\tStart: " sbatch job_${ID}.sh >> morpheusProxy.out
        ;;

        stop)
				scancel $ID
				date >> morpheusProxy.out
				echo -e "\tStop: " scancel $ID >> morpheusProxy.out
        ;;

        check)
				# check job status, returns: PENDING, RUNNING, COMPLETED, CANCELLED, FAILED 
				squeue -j $ID --states=PENDING,RUNNING,COMPLETED,CANCELLED,FAILED -h -o %T
				date >> morpheusProxy.out
				echo -e "\tCheck:" squeue -j $ID -t=PD,R,CD,CA,F -h -o %T >> morpheusProxy.out
        ;;

#        zip)
#				cd ${BASEDIR}
#				bsub tar czf ${SIMDIR}.tar.gz $SIMDIR
#				cd -
#				date >> morpheusProxy.out
#				echo -e "\tZip: " tar czf $SIMDIR.tar.gz $SIMDIR  >> morpheusProxy.out
#       ;;

#        delete)
#                rm ${BASEDIR}/$SIMDIR.tar.gz
#                rm -rf ${BASEDIR}/$SIMDIR
#                date >> morpheusProxy.out
#                echo -e "\tDelete: " rm ${BASEDIR}/$SIMDIR.tar.gz  >> morpheusProxy.out
#        ;;

#        debug)
#				cd ${BASEDIR}/${SIMDIR}/
#				echo "set logging file gdb.log" >> gdb_cmd.txt
#				echo "set logging on" >> gdb_cmd.txt
#				echo "set logging overwrite" >> gdb_cmd.txt
#				echo "run " $MODEL >> gdb_cmd.txt
#				echo "backtrace full " >> gdb_cmd.txt
#				echo "quit " >> gdb_cmd.txt
			
#				gdb --command=gdb_cmd.txt  --exec $EXECUTABLE 
#                date >> morpheusProxy.out
#                echo -e "\tDebug: " $EXECUTABLE " " $MODEL  >> morpheusProxy.out
#				rm gdb_cmd.txt
#
#        ;;


        revision)
				$EXECUTABLE --revision
				date >> morpheusProxy.out
				echo -e "\tREVISION: " $EXECUTABLE >> morpheusProxy.out
        ;;

        version)
				$EXECUTABLE --version
				date >> morpheusProxy.out
				echo -e "\tVERSION " $EXECUTABLE >> morpheusProxy.out
        ;;

        gnuplot-version)
				$EXECUTABLE --gnuplot-version
				data >> morpheusProxy.out
				echo -e "\tGnuPlot-Version " $EXECUTABLE >> morpheusProxy.out
		;;
        *)
                echo "No value given for parameter '-a'!\n"
                echo "No action can be executed!\n"
        ;;
esac

exit 0;
