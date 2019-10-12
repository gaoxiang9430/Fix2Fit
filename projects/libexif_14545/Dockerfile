FROM gaoxiang9430/fix2fit

MAINTAINER paul.l.kehrer@gmail.com
RUN apt-get update && apt-get install -y make autoconf automake libtool gettext autopoint
RUN git clone --depth 1 https://github.com/libexif/libexif
RUN git clone --depth 1 https://github.com/ianare/exif-samples
WORKDIR libexif
COPY exif_loader_fuzzer.cc build.sh $SRC/

COPY testcase /testcase
COPY libexif /src/libexif
COPY project_build.sh /src/libexif/project_build.sh
COPY project_config.sh /src/libexif/project_config.sh