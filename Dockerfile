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

FROM base-builder-3.8.1

MAINTAINER mechtaev@gmail.com
ENV DEBIAN_FRONTEND noninteractive

RUN apt-get update && apt-get upgrade -y && apt-get autoremove -y

RUN apt-get install -y build-essential cmake zlib1g-dev libtinfo-dev python
RUN apt-get install -y libboost-filesystem-dev libboost-program-options-dev libboost-log-dev

ADD build 	/f1x-oss-fuzz/build
ADD docs 	/f1x-oss-fuzz/docs
ADD f1x 	/f1x-oss-fuzz/f1x
ADD infra	/f1x-oss-fuzz/infra
ADD projects	/f1x-oss-fuzz/projects

RUN mkdir /f1x-oss-fuzz/f1x/build && cd /f1x-oss-fuzz/f1x/build \
    && cmake .. -DF1X_LLVM=/llvm-3.8.1  \
    && make

ENV PATH="/f1x-oss-fuzz/f1x/build/tools:${PATH}"


