#!/bin/bash

export XPCF_MODULE_ROOT=~/.remaken/packages/linux-gcc
echo "XPCF_MODULE_ROOT=$XPCF_MODULE_ROOT"

# Update configuration files by replacing win-cl-1.1 by linux in module paths
if [ -f "$PWD/$1_conf.xml" ]; then
   sed -i 's/win-cl-14.1/linux-gcc/' $PWD/$1_conf.xml
fi

if [ -f "$PWD/$1_Processing_conf.xml" ]; then
   sed -i 's/win-cl-14.1/linux-gcc/' $PWD/$1_Processing_conf.xml
fi

if [ -f "$PWD/$1_Producer_conf.xml" ]; then
   sed -i 's/win-cl-14.1/linux-gcc/' $PWD/$1_Producer_conf.xml
fi

if [ -f "$PWD/$1_Viewer_conf.xml" ]; then
   sed -i 's/win-cl-14.1/linux-gcc/' $PWD/$1_Viewer_conf.xml
fi

# include dependencies path to ld_library_path
ld_library_path="./"
if [ -f "$PWD/$1_conf.xml" ]; then
	for modulePath in $(grep -o "\$XPCF_MODULE_ROOT.*lib" $1_conf.xml)
	do
	   modulePath=${modulePath/"\$XPCF_MODULE_ROOT"/${XPCF_MODULE_ROOT}}
	   if ! [[ $ld_library_path =~ "$modulePath/x86_64/shared/release" ]]
	   then
		  ld_library_path=$ld_library_path:$modulePath/x86_64/shared/release
	   fi 
	done
fi

if [ -f "$PWD/$1_Processing_conf.xml" ]; then
	for modulePath in $(grep -o "\$XPCF_MODULE_ROOT.*lib" $1_Processing_conf.xml)
	do
	   modulePath=${modulePath/"\$XPCF_MODULE_ROOT"/${XPCF_MODULE_ROOT}}
	   if ! [[ $ld_library_path =~ "$modulePath/x86_64/shared/release" ]]
	   then
		  ld_library_path=$ld_library_path:$modulePath/x86_64/shared/release
	   fi 
	done
fi

if [ -f "$PWD/$1_Producer_conf.xml" ]; then
	for modulePath in $(grep -o "\$XPCF_MODULE_ROOT.*lib" $1_Producer_conf.xml)
	do
	   modulePath=${modulePath/"\$XPCF_MODULE_ROOT"/${XPCF_MODULE_ROOT}}
	   if ! [[ $ld_library_path =~ "$modulePath/x86_64/shared/release" ]]
	   then
		  ld_library_path=$ld_library_path:$modulePath/x86_64/shared/release
	   fi 
	done
fi

if [ -f "$PWD/$1_Viewer_conf.xml" ]; then
	for modulePath in $(grep -o "\$XPCF_MODULE_ROOT.*lib" $1_Viewer_conf.xml)
	do
	   modulePath=${modulePath/"\$XPCF_MODULE_ROOT"/${XPCF_MODULE_ROOT}}
	   if ! [[ $ld_library_path =~ "$modulePath/x86_64/shared/release" ]]
	   then
		  ld_library_path=$ld_library_path:$modulePath/x86_64/shared/release
	   fi 
	done
fi


echo "LD_LIBRARY_PATH=$ld_library_path $1 $2"
LD_LIBRARY_PATH=$ld_library_path $1 $2



