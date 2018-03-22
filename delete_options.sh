#!/bin/bash

exec 6< "/home/edwin/all_issue_id_libarchive.txt"

for ((i=1; i<=20; i++))
do
     read line_issue_id <& 6
      
     sed -i 's/$SRC\/libarchive_'$line_issue_id'_testcase/libarchive_'$line_issue_id'_testcase/g' projects/libarchive_$line_issue_id/Dockerfile
     #sed -i 's/$SRC\/build_no_config.sh/\$SRC\/openjpeg\/build_no_config.sh/g' projects/openjpeg_$line_issue_id/Dockerfile
     #sed  -i 's/FROM gcr.io\/oss-fuzz-base\/base-builder/FROM f1x-oss-fuzz/g' projects/libarchive_$line_issue_id/Dockerfile
     #sed -i 's/COPY build_f1x.sh/COPY build_f1x.sh $SRC\//g' projects/ffmpeg_$line_issue_id/Dockerfile
     #sed -i '/export CFLAGS/d' projects/ffmpeg_$line_issue_id/build.sh
     #sed -i 's/autoreconf -fiv/.\/autogen.sh/g' projects/ffmpeg_$line_issue_id/build_f1x.sh
     # sed -i '1s/^/export CXXFLAGS="$CXXFLAGS -fno-sanitize=vptr" \n /' projects/ffmpeg_$line_issue_id/build.sh
     #sed -i '1s/^/export CFLAGS="$CFLAGS -fno-sanitize=vptr" \n /' projects/ffmpeg_$line_issue_id/build.sh
     
done
 


