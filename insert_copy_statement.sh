#!/bin/bash

exec 6< "/home/edwin/all_issue_ids_wireshark.txt"

for ((i=1; i<=31; i++))
do
     read line_issue_id <& 6
       
     cpy_cmd_1="COPY f1xcmd_"$line_issue_id" \$SRC/wireshark/f1xcmd_"$line_issue_id
     echo $cpy_cmd_1 >> projects/wireshark_$line_issue_id/Dockerfile
     #cpy_cmd_2="COPY build_no_config.sh \$SRC/libarchive/build_no_config.sh"
     #echo $cpy_cmd_2 >> projects/libarchive_$line_issue_id/Dockerfile      
     #cpy_cmd_3="COPY libarchive_"$line_issue_id"_codes \$SRC/libarchive"    
     #echo $cpy_cmd_3 >> projects/libarchive_$line_issue_id/Dockerfile 
     #cpy_cmd_4="COPY libarchive_"$line_issue_id"_testcase \$SRC/libarchive_"$line_issue_id"_testcase"
     #echo $cpy_cmd_4 >> projects/libarchive_$line_issue_id/Dockerfile
     #cpy_cmd_5="COPY driver_"$line_issue_id" \$SRC/driver_"$line_issue_id
     #echo $cpy_cmd_5 >> projects/libarchive_$line_issue_id/Dockerfile

     #exec_bash_cmd="exec \"/bin/bash\""
     #echo $exec_bash_cmd >> projects/libarchive_$line_issue_id/build.sh
     #cpy_cmd_5="COPY build_no_config.sh \$SRC/"
     #echo $cpy_cmd_5 >> projects/wireshark_$line_issue_id/Dockerfile

     #chmod_cmd_1="sudo chmod 777 /home/edwin/f1x-oss-fuzz/projects/ffmpeg_"$line_issue_id"/f1xcmd_"$line_issue_id
     #eval $chmod_cmd_1

     #echo "COPY x264_prev x264" >> projects/ffmpeg_$line_issue_id/Dockerfile
     #echo "RUN apt-get install -y vim" >> projects/ffmpeg_$line_issue_id/Dockerfile

     #cpy_cmd_3="cp build_f1x.sh projects/ffmpeg_"$line_issue_id"/build_f.sh"
     #eval $cpy_cmd_3

     #chmod_cmd="chmod 777 projects/ffmpeg_"$line_issue_id"/build_f1x.sh"
     #eval $chmod_cmd

     #echo "COPY build_f1x.sh \$SRC/" >> projects/ffmpeg_$line_issue_id/Dockerfile
     #echo "COPY build-no-config.sh \$SRC/" >> projects/ffmpeg_$line_issue_id/Dockerfile
     
     #echo "export CFLAGS=\"-O1 -fno-omit-frame-pointer -gline-tables-only -DFUZZING_BUILD_MODE_UNSAFE_FOR_PRODUCTION -fsanitize=bool,array-bounds,float-divide-by-zero,function,integer-divide-by-zero,return,shift,signed-integer-overflow,vla-bound,vptr -fno-sanitize-recover=undefined -fsanitize-coverage=trace-cmp -fno-sanitize=vptr\"" >> projects/ffmpeg_$line_issue_id/build_f1x.sh
     #echo "export CXXFLAGS=\"-O1 -fno-omit-frame-pointer -gline-tables-only -DFUZZING_BUILD_MODE_UNSAFE_FOR_PRODUCTION -fsanitize=bool,array-bounds,float-divide-by-zero,function,integer-divide-by-zero,return,shift,signed-integer-overflow,vla-bound,vptr -fno-sanitize-recover=undefined -fsanitize-coverage=trace-cmp -stdlib=libc++ -fno-sanitize=vptr\"" >> projects/ffmpeg_$line_issue_id/build_f1x.sh
       
     echo "Command executed to issue no. "$line_issue_id  
done
 


