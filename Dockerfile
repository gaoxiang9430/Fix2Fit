#########################################################################
# This file is part of Fix2Fit.
# Copyright (C) 2016  Xiang Gao, Sergey Mechtaev, Shin Hwei Tan, Abhik Roychoudhury

# f1x is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.

# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.

# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.
#########################################################################


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
#ADD infra	f1x-oss-fuzz/infra
ADD projects	f1x-oss-fuzz/projects
ADD f1x/demo	f1x-oss-fuzz/f1x/demo
ADD aflgo	$SRC/aflgo
ADD scripts/build_aflgo.sh /src/build_aflgo.sh

RUN mkdir f1x-oss-fuzz/f1x/build && cd f1x-oss-fuzz/f1x/build \
    && cmake .. -DF1X_LLVM=/llvm-3.8.1  \
    && make && make install

ENV PATH="$SRC/f1x-oss-fuzz/f1x/build/tools:${PATH}"

RUN chmod u+x $SRC/build_aflgo.sh
RUN $SRC/build_aflgo.sh
ENV PATH="$SRC/llvm-4.0.0/install/bin/:${PATH}"
ENV AFLGO="$SRC/aflgo"

