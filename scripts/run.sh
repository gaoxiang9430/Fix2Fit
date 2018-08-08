#!/usr/bin/env bash
#
# Usage: run.sh [project_name] [bug_number] [core_id]
#

SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

pushd $SCRIPT_DIR/.. > /dev/null

# Function definitions

make_ffmpeg_project() {
    local LINE
    local BUG_NUMBER
    local FILED_BUG_NUMBER
    local FFMPEG_HASH
    local X264_HASH
    local INITIAL_DIR

    INITIAL_DIR=`pwd`
    
    BUG_NUMBER=$1
    FFMPEG_HASH=
    X264_HASH=
    while read LINE ;  do
	FILED_BUG_NUMBER=`echo -n $LINE | cut --delimiter=, --fields=1`
	if [ x$BUG_NUMBER = x$FILED_BUG_NUMBER ] ; then
	    FFMPEG_HASH=`echo -n $LINE | cut --delimiter=, --fields=2`
	    X264_HASH=`echo -n $LINE | cut --delimiter=, --fields=3`
	    SANITIZER_TYPE=`echo -n $LINE | cut --delimiter=, --fields=4`
	    break;
	fi
    done < $SCRIPT_DIR/../projects/all_issue_ids_ffmpeg.txt

    if [ x$FFMPEG_HASH != x ] ; then
	cd $SCRIPT_DIR/../projects/ffmpeg_$BUG_NUMBER
	git clone https://github.com/FFmpeg/FFmpeg.git ffmpeg
	cd ffmpeg
	git reset --hard $FFMPEG_HASH
	cd $SCRIPT_DIR/../projects/ffmpeg_$BUG_NUMBER
	git clone https://github.com/mirror/x264.git x264_prev
	cd x264_prev
	git reset --hard $X264_HASH
	pushd $SCRIPT_DIR/..
	#zip -r projects/ffmpeg_$BUG_NUMBER/f1x.zip f1x
	popd
    fi

    # Restore the supposedly original state
    cd $INITIAL_DIR
}

make_libarchive_project() {
    local LINE
    local BUG_NUMBER
    local FILED_BUG_NUMBER
    local LIBARCHIVE_HASH
    local INITIAL_DIR

    INITIAL_DIR=`pwd`
    
    BUG_NUMBER=$1
    LIBARCHIVE_HASH=
    while read LINE ;  do
	FILED_BUG_NUMBER=`echo -n $LINE | cut --delimiter=, --fields=1`
	if [ x$BUG_NUMBER = x$FILED_BUG_NUMBER ] ; then
	    LIBARCHIVE_HASH=`echo -n $LINE | cut --delimiter=, --fields=2`
	    SANITIZER_TYPE=`echo -n $LINE | cut --delimiter=, --fields=3`
	    break;
	fi
    done < $SCRIPT_DIR/../projects/all_issue_ids_libarchive.txt

    if [ x$LIBARCHIVE_HASH != x ] ; then
        cd $SCRIPT_DIR/../projects/libarchive_$BUG_NUMBER
        git clone https://github.com/libarchive/libarchive.git libarchive
        cd libarchive
        git reset --hard $LIBARCHIVE_HASH
	#pushd $SCRIPT_DIR/..
	#zip -r projects/libarchive_$BUG_NUMBER/f1x.zip f1x
	#popd
    fi

    # Restore the supposedly original state
    cd $INITIAL_DIR
}

make_openjpeg_project() {
    local LINE
    local BUG_NUMBER
    local FILED_BUG_NUMBER
    local OPENJPEG_HASH
    local INITIAL_DIR

    INITIAL_DIR=`pwd`
    
    BUG_NUMBER=$1
    OPENJPEG_HASH=
    while read LINE ;  do
	FILED_BUG_NUMBER=`echo -n $LINE | cut --delimiter=, --fields=1`
	if [ x$BUG_NUMBER = x$FILED_BUG_NUMBER ] ; then
	    OPENJPEG_HASH=`echo -n $LINE | cut --delimiter=, --fields=2`
	    SANITIZER_TYPE=`echo -n $LINE | cut --delimiter=, --fields=3`
	    break;
	fi
    done < $SCRIPT_DIR/../projects/all_issue_ids_openjpeg.txt

    if [ x$OPENJPEG_HASH != x ] ; then
	cd $SCRIPT_DIR/../projects/openjpeg_$BUG_NUMBER
	git clone https://github.com/uclouvain/openjpeg.git openjpeg_$BUG_NUMBER
	cd openjpeg_$BUG_NUMBER
	git reset --hard $OPENJPEG_HASH
	pushd $SCRIPT_DIR/..
	#zip -r projects/openjpeg_$BUG_NUMBER/f1x.zip f1x
	popd
    fi

    # Restore the supposedly original state
    cd $INITIAL_DIR
}

make_proj4_project() {
    local LINE
    local BUG_NUMBER
    local FILED_BUG_NUMBER
    local PROJ4_HASH
    local INITIAL_DIR

    INITIAL_DIR=`pwd`
    
    BUG_NUMBER=$1
    PROJ4_HASH=
    while read LINE ;  do
	FILED_BUG_NUMBER=`echo -n $LINE | cut --delimiter=, --fields=1`
	if [ x$BUG_NUMBER = x$FILED_BUG_NUMBER ] ; then
	    PROJ4_HASH=`echo -n $LINE | cut --delimiter=, --fields=2`
	    SANITIZER_TYPE=`echo -n $LINE | cut --delimiter=, --fields=3`
	    break;
	fi
    done < $SCRIPT_DIR/../projects/all_issue_ids_proj4.txt

    if [ x$PROJ4_HASH != x ] ; then
	cd $SCRIPT_DIR/../projects/proj4_$BUG_NUMBER
	git clone https://github.com/OSGeo/proj.4.git proj4
	cd proj4
	git reset --hard $PROJ4_HASH
	pushd $SCRIPT_DIR/..
	#zip -r projects/proj4_$BUG_NUMBER/f1x.zip f1x
	popd
    fi

    # Restore the supposedly original state
    cd $INITIAL_DIR
}

make_wireshark_project() {
    local LINE
    local BUG_NUMBER
    local FILED_BUG_NUMBER
    local WIRESHARK_HASH
    local INITIAL_DIR

    INITIAL_DIR=`pwd`
    
    BUG_NUMBER=$1
    WIRESHARK_HASH=
    while read LINE ;  do
	FILED_BUG_NUMBER=`echo -n $LINE | cut --delimiter=, --fields=1`
	if [ x$BUG_NUMBER = x$FILED_BUG_NUMBER ] ; then
	    WIRESHARK_HASH=`echo -n $LINE | cut --delimiter=, --fields=2`
	    SANITIZER_TYPE=`echo -n $LINE | cut --delimiter=, --fields=3`
	    break;
	fi
    done < $SCRIPT_DIR/../projects/all_issue_ids_wireshark.txt

    if [ x$WIRESHARK_HASH != x ] ; then
	cd $SCRIPT_DIR/../projects/wireshark_$BUG_NUMBER
	git clone https://github.com/wireshark/wireshark.git wireshark_$BUG_NUMBER
	cd wireshark_$BUG_NUMBER
	git reset --hard $WIRESHARK_HASH
	pushd $SCRIPT_DIR/..
	#zip -r projects/wireshark_$BUG_NUMBER/f1x.zip f1x
	popd
    fi

    # Restore the supposedly original state
    cd $INITIAL_DIR
}

build_project() {
    local PROJECT_NAME
    local BUG_NUMBER
    local INITIAL_DIR
    
    PROJECT_NAME=$1
    BUG_NUMBER=$2
    ASSIGNED_CORE=$3
    INITIAL_DIR=`pwd`
    SANITIZER_TYPE=address
   
    if [ x$PROJECT_NAME = xffmpeg ] ; then
	make_ffmpeg_project $BUG_NUMBER
    elif [ x$PROJECT_NAME = xlibarchive ] ; then
	make_libarchive_project $BUG_NUMBER
    elif [ x$PROJECT_NAME = xopenjpeg ] ; then
	make_openjpeg_project $BUG_NUMBER
    elif [ x$PROJECT_NAME = xproj4 ] ; then
        # In case the 4 in proj4 gets misinterpreted as the bug id
	if [ x$BUG_NUMBER = x4 ] ; then
            return
	fi
	make_proj4_project $BUG_NUMBER
    elif [ x$PROJECT_NAME = xwireshark ] ; then
	make_wireshark_project $BUG_NUMBER
    fi
    python $SCRIPT_DIR/../infra/helper.py build_image $PROJECT_NAME $BUG_NUMBER
    python $SCRIPT_DIR/../infra/helper.py build_fuzzers --engine afl --core $ASSIGNED_CORE --sanitizer $SANITIZER_TYPE $PROJECT_NAME $BUG_NUMBER
    # For running the fuzzer
    # python $SCRIPT_DIR/../infra/helper.py run_fuzzer "$PROJECT_NAME"_$BUG_NUMBER "$PROJECT_NAME"_fuzzer

    cd $INITIAL_DIR
}

# The main procedure

PROJECT_NAME=$1

BUG_NUMBER=$2

ASSIGNED_CORE=$3

if [ x$PROJECT_NAME != x ] ; then
    PROJECT_DIR_LIST=`ls "projects/$PROJECT_NAME"_*`
    if [ "$PROJECT_DIR_LIST" = "" ] ; then
	echo Project $PROJECT_NAME not found.
	exit 1
    else
	if [ x$BUG_NUMBER != x ] ; then
	    PROJECT_DIR=`ls -d "projects/$PROJECT_NAME"_$BUG_NUMBER`
	    if [ x$PROJECT_DIR = x ] ; then
		echo Project $PROJECT_NAME with bug number $BUG_NUMBER not found.
		exit 1
	    else
		build_project $PROJECT_NAME $BUG_NUMBER $ASSIGNED_CORE
	    fi
	else
	    for BUG_NUMBER in `ls -d "projects/$PROJECT_NAME"_* | sed s/[^[:digit:]]/\ /g`
	    do
		build_project $PROJECT_NAME $BUG_NUMBER $ASSIGNED_CORE
	    done
	fi
    fi
else
    for PROJECT_NAME in ffmpeg libarchive openjpeg proj4 wireshark
    do
	for BUG_NUMBER in `ls -d "projects/$PROJECT_NAME"_* | sed s/[^[:digit:]]/\ /g`
	do
	    build_project $PROJECT_NAME $BUG_NUMBER $ASSIGNED_CORE
	done
    done
fi

popd

container_name=gcr.io/oss-fuzz/${PROJECT_NAME}_${BUG_NUMBER}
container_id=`docker ps --all | grep ${PROJECT_NAME}_${BUG_NUMBER} | awk '{print $1}'`
output_dir=$SCRIPT_DIR/../output/${PROJECT_NAME}_${BUG_NUMBER}
rm -rf $output_dir
mkdir $output_dir
docker cp $container_id:/src/scripts/original.txt $output_dir
docker cp $container_id:/src/scripts/afl.txt $output_dir
docker cp $container_id:/src/scripts/aflgo.txt $output_dir
docker cp $container_id:/src/scripts/aflgo_pat.txt $output_dir
docker cp $container_id:/src/scripts/aflgo_part.txt $output_dir
docker cp $container_id:/src/scripts/patch $output_dir
