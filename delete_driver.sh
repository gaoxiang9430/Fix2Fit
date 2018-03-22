#!/bin/bash

exec 6< "/home/edwin/all_issue_ids.txt"

for ((i=1; i<=200; i++))
do
	read line_issue_id <& 6

        if [[ $i -le 178 ]]
        then
             continue
        fi

	delete_cmd_1='rm -f projects/ffmpeg_'$line_issue_id'/driver_178'
	eval "$delete_cmd_1"
        delete_cmd_2='rm -f projects/ffmpeg_'$line_issue_id'/ffmpeg_178'
        eval "$delete_cmd_2"
done 
