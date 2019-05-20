#  This file is part of f1x-oss-fuzz.
#  Copyright (C) 2017 Edwin Lesmana Tjiong, Sergey Mechtaev
#
#  f1x-oss-fuzz is free software: you can redistribute it and/or modify
#  it under the terms of the GNU General Public License as published by
#  the Free Software Foundation, either version 3 of the License, or
#  (at your option) any later version.
#
#  This program is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU General Public License for more details.
#
#  You should have received a copy of the GNU General Public License
#  along with this program.  If not, see <http://www.gnu.org/licenses/>.

#FROM gcr.io/oss-fuzz-base/base-builder
FROM gcr.io/oss-fuzz-base/base-builder

MAINTAINER mechtaev@gmail.com
ENV DEBIAN_FRONTEND noninteractive

RUN apt-get update --fix-missing && apt-get autoremove -y

RUN apt-get install -y build-essential cmake zlib1g-dev libtinfo-dev python
RUN apt-get install -y libboost-filesystem-dev libboost-program-options-dev libboost-log-dev

WORKDIR $SRC

RUN mkdir f1x-oss-fuzz  

ADD docs 	f1x-oss-fuzz/docs
ADD f1x 	f1x-oss-fuzz/f1x
ADD infra	f1x-oss-fuzz/infra
ADD projects	f1x-oss-fuzz/projects
ADD f1x/demo	f1x-oss-fuzz/f1x/demo
ADD IntPTI	f1x-oss-fuzz/IntPTI
ADD scripts/build_aflgo.sh /src/build_aflgo.sh
#ADD scripts/afl_driver.cpp /src/libfuzzer/afl/afl_driver.cpp
ADD scripts/afl-fuzz.c /afl-fuzz.c
ADD scripts/config.h /config.h
#ADD scripts/SharedMemorySetter.h /usr/include/SharedMemorySetter.h

RUN ls $SRC/f1x-oss-fuzz/IntPTI/
RUN rm -rf /src/f1x-oss-fuzz/f1x/llvm

RUN mkdir f1x-oss-fuzz/f1x/build && cd f1x-oss-fuzz/f1x/build \
    && cmake .. -DF1X_LLVM=/llvm-3.8.1  \
    && make && make install

ENV PATH="$SRC/f1x-oss-fuzz/f1x/build/tools:${PATH}"

RUN chmod u+x $SRC/build_aflgo.sh
RUN $SRC/build_aflgo.sh
ENV PATH="$SRC/llvm-4.0.0/install/bin/:${PATH}"
ENV AFLGO="$SRC/aflgo"

#Installing IntPTI
RUN apt-get install -y ant software-properties-common python-software-properties

RUN add-apt-repository ppa:webupd8team/java -y
RUN apt-get update -y
RUN echo "oracle-java8-installer shared/accepted-oracle-license-v1-1 select true" | debconf-set-selections
RUN apt-get install oracle-java8-installer -y

RUN cd $SRC/f1x-oss-fuzz/IntPTI/ && ant && ant jar

RUN cd $SRC/f1x-oss-fuzz/IntPTI/src/org/sosy_lab/cpachecker/core/phase/fix/lib/ && make && make install 
