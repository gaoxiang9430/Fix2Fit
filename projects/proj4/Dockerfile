# Copyright 2016 Google Inc.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#
################################################################################

#FROM gcr.io/oss-fuzz-base/base-builder
FROM gaoxiang9430/fix2fit
MAINTAINER even.rouault@spatialys.com
RUN apt-get update && apt-get install -y make vim autoconf automake libtool g++
RUN git clone --depth 1 https://github.com/OSGeo/proj.4 proj4
WORKDIR proj4
COPY scripts $SRC/scripts
COPY build.sh $SRC/
COPY proj4_testcase /proj4_testcase
COPY driver /driver
COPY proj4 $SRC/proj4
COPY project_build.sh $SRC/proj4/project_build.sh
COPY project_config.sh $SRC/proj4/project_config.sh
