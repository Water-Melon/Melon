
#
# Copyright (C) Niklaus F.Schen.
#

#!/bin/sh
for dir in `ls modules`
do
	tmp=`echo $dir|cut -d '/' -f 2`
	test -e modules/$tmp/Makefile && make -f modules/$tmp/Makefile clean
	test -e modules/$tmp/configure && sh modules/$tmp/configure $install_path
	test -e modules/$tmp/Makefile && make -f modules/$tmp/Makefile
	test -e modules/$tmp/Makefile && make -f modules/$tmp/Makefile install
done

