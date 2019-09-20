#FROM gcr.io/oss-fuzz-base/base-builder
FROM gaoxiang9430/fix2fit

COPY scripts /src/scripts
COPY testcase /testcase
COPY driver /driver
COPY [PROJECT_NAME] /src/[PROJECT_NAME]
COPY project_build.sh /src/proj4/project_build.sh
COPY project_config.sh /src/proj4/project_config.sh
