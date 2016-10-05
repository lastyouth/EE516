#!/bin/bash

g_target=$HOME

drawline()
{
	remain=$1
	for (( c=$remain; c>0; c-- ))
	do
		if [ "$c" -eq "1" ]; then
			printf "|___"
		else
			printf "|   "
		fi
	done
}

search()
{
	local current=$1
	local tabcounter=$2
	for file in $(ls $current)
	do
		drawline $tabcounter
		echo $file
		local path=""
		if [ "$current" = "/" ];then
			path="$current$file"
		else
			path="$current/$file"
		fi
		#echo $path
		if [ -d $path ]; then
			search $path $tabcounter+1
		fi
	done
	
}

search $g_target 1

exit 0

