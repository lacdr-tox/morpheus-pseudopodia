#!/bin/bash

for s in 16 22 36 48 64 128 256 512
do
	mkdir ${s}x${s}
	echo "Creating icon of size $s x $s"
	convert -sample $sx$s icon.png  ${s}x${s}/morpheus.png
done
echo "done"
