# ConfROSE

Implementation of confidentiality (information flow control) in the ROSE compiler for C.

## Description

This project aims to build a secure compiler for C which focuses on data confidentiality in low-level code in the absence of memory safety by preventing explicit information flow under the assumption of an active adversary. It works with a lattice model for static and dependent security types to provide a rich and realistic system to work with.

It uses static dataflow analysis, runtime instrumentation and taint-aware control-flow integrity to implement source-to-source transformations in C++ which transform code written in a subset of C with minimal security-related annotations into verifiably safe code.

## Installation Instructions for ROSE

Warning: The whole installation process requires enough storage space for both Boost and ROSE, the latter of which is upwards of 30GB (please visit the respective websites to get a clearer idea of system requirements).

To install ROSE, we'll first need to install the latest version of Boost from https://www.boost.org/doc/libs/1_73_0/more/getting_started/unix-variants.html (you would need to follow instructions in sections 1 and 5.1 both). The latest version needs to be installed because packages on distros like Ubuntu are quite a few versions behind the version we need to use.

Some important notes to be noted while building boost: 

1. The directory which is mentioned in section 1 should be /usr/local

2. In section 5.1, it is better to not use --prefix=DIRECTORY (and write it into /usr/local instead, to make the directory structure the same as if libboost-all-dev (Ubuntu's Boost package) was installed instead)

Then we need to run the following to finally build ROSE (taken from https://github.com/rose-compiler/rose/wiki/Install-Rose-From-Source, please have a look there before installing as well):

Note that some of the directories here are dummy directories, replace them with the correct ones before execution.

    sudo apt-get update
    sudo apt-get upgrade
    
    sudo apt-get install git wget build-essential libtool flex bison python3-dev unzip perl-doc doxygen texlive gdb gcc-7 g++-7 gfortran-7 autoconf automake libxml2-dev libdwarf-dev graphviz openjdk-8-jdk lsb-core ghostscript perl-doc
    
    PREFIX=/path/to/my/rose/directory

    NUM_PROCESSORS=4  #the link mentioned above has 10 processors, it was changed because using that tends to freeze some systems
    
    BOOST_ROOT=/path/to/my/boost/installation/directory
    
    export LD_LIBRARY_PATH="${BOOST_ROOT}/lib:$LD_LIBRARY_PATH"
    
    git clone -b release https://github.com/rose-compiler/rose "${PREFIX}/src"
    
    cd "${PREFIX}/src"
    
    ./build
    
    mkdir ../build
    
    cd ../build
    
    ../src/configure --prefix="${PREFIX}/install" \
                     --enable-languages=c,c++ \
                     --with-boost="${BOOST_ROOT}"
    
    make core -j${NUM_PROCESSORS}
    
    make install-core -j${NUM_PROCESSORS}
    
    make check-core -j${NUM_PROCESSORS}

This should be enough for a working install of ROSE (please refer to rosecompiler.org/uploads/ROSE-UserManual.pdf if there are any issues while installing)

## Compilation instructions for ConfROSE

Running "make" (without the quotes) inside the src directory in this git repo would compile the tool, and running "make run" would run it on the file demo.c which is also in the same directory, with the security description file being security.txt.

In the Makefile, note that you would need to change the declaration of ROSE_HOME suitably to your own installation of ROSE. 
