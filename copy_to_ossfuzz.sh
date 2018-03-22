#!/bin/bash

exec 6< "/home/edwin/all_issue_id_openjpeg.txt"

for ((i=1; i<=21; i++))
do
	read line_issue_id <& 6

        #remove_cmd_1="rm -rf projects/ffmpeg_"$line_issue_id
        #eval "$remove_cmd_1"

	copy_cmd_1="cp -r projects/openjpeg/build_f1x.sh projects/openjpeg_"$line_issue_id"/build_f1x.sh"
        echo "$copy_cmd_1"
	eval "$copy_cmd_1"

	copy_cmd_2="cp -r projects/openjpeg/build_no_config.sh projects/openjpeg_"$line_issue_id"/build_no_config.sh"
        echo "$copy_cmd_2"
        eval "$copy_cmd_2"

        #copy_cmd_3="cp -r projects/openjpeg/project.yaml projects/openjpeg_"$line_issue_id"/project.yaml"
        #echo "$copy_cmd_3"
        #eval "$copy_cmd_3"

	#remove_cmd_2="rm -rf projects/openjpeg_"$line_issue_id"/openjpeg"
	#eval "$remove_cmd_2"

	#scp_cmd="cp -r ../Patches/wireshark_patches/wireshark_"$line_issue_id"_codes projects/wireshark_"$line_issue_id"/wireshark_"$line_issue_id"_codes"
	#eval "$scp_cmd"
done 
