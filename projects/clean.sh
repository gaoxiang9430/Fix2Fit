#!/bin/bash

clean_project()
{
    project=$1
    rm ${project}_*/project*
    rm ${project}_*/build*
    rm ${project}_*/driver
    rm ${project}_*/Docker*
    rm -rf ${project}_*/scripts
    rm -rf ${project}_*/${project}
    if [ ${project} = "libchewing" ]
    then
        rm ${project}_*/chewing*
    elif [ ${project} = "libssh" ]
    then
        rm ${project}_*/libssh_server_fuzzer.cc
    elif [ ${project} = "libarchive" ]
    then
        rm ${project}_*/libarchive_fuzzer.cc
    fi
}

clean_project $1
