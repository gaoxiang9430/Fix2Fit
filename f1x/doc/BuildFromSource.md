# Building f1x from source #

Install dependencies (GCC, G++, Make, Boost.Filesystem, Boost.Program_options, Boost.Log, Gcovr, diff, patch):

    # Ubuntu:
    sudo apt-get install build-essential gcovr zlib1g-dev libtinfo-dev
    sudo apt-get install libboost-filesystem-dev libboost-program-options-dev libboost-log-dev
    
Install a new version of CMake (3.4.3 or higher, version is important).

[Download](http://releases.llvm.org/download.html) LLVM and Clang 3.8.1 (version is important):

    # This link is for Ubuntu 14.04 x86_64:
    wget http://releases.llvm.org/3.8.1/clang+llvm-3.8.1-x86_64-linux-gnu-ubuntu-14.04.tar.xz
    tar xf clang+llvm-3.8.1-x86_64-linux-gnu-ubuntu-14.04.tar.xz
    
Alternatively, you can build LLVM and Clang from sources (e.g. if the binary is not available for your platform or it is built using a different compiler). We provide a script for downloading, building and installing it locally from sources (create a directory `<my-llvm-dir>` where it will be installed):

    cd <my-llvm-dir>
    /path/to/f1x/infra/download_build_install_llvm.sh
    
To compile f1x, execute the following (from the f1x root directory):
    
    mkdir build
    cd build
    cmake -DF1X_LLVM=<my-llvm-dir> ..
    make
    
To install f1x, add the `build/tools` directory into your `PATH`:

    export PATH=$PWD/build/tools:$PATH
    
To test f1x, execute `./tests/runall.sh`.
