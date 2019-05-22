# f1x Docker image #

To pull our Docker image, run:

    docker pull mechtaev/f1x

Alternatively, you can build an image locally (note that it relies on `mechtaev/ubuntu-16.04-llvm-3.8.1`, which is built using `infra/Dockerfile`):

    cd /path/to/f1x
    docker build . -t mechtaev/f1x

To test the image, you can run f1x test suite in a temporary container:

    docker run --rm -ti mechtaev/f1x /f1x/tests/runall.sh
    
To create a new container, execute

    docker create -ti --name f1x_0 mechtaev/f1x /bin/bash
    
To start a container, execute

    docker start -ai f1x_0
    

