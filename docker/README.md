Using Docker to Cross-Compile lms2012-compat
--------------------------------------------

This assumes that you have already `docker` installed and that you have cloned
the git repository and that the current working directory is the `lms2012-compat`
source code directory. To build for BeagleBone, replace all instances of `armel`
below with `armhf`.

1. Create the docker image and a docker container.

        ./docker/setup.sh armel

2.  Then build the code.

        docker exec -t lms2012_armel make
        docker exec -t lms2012_armel make install

### Tips

* To get an interactive shell to the container, run 

        docker exec -it lms2012_armel bash

* When you are done building, you can stop the container.

        docker stop --time 0 lms2012_armel

    `docker exec ...` will not work until you start the container again.

        docker start lms2012_armel

    And the container can be deleted when you don't need it anymore (don't
    forget to stop it first).

        docker rm lms2012_armel
