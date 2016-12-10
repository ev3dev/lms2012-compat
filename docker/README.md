Using Docker to Cross-Compile lms2012-compat
--------------------------------------------

This assumes that you have already `docker` installed and that you have cloned
the git repository and that the current working directory is the `lms2012-compat`
source code directory. To build for BeagleBone, replace all instances of `armel`
below with `armhf`.

1. Create the docker image.

        docker build --tag lms2012-armel --file docker/armel.dockerfile docker/

2. Create an empty build directory. (This can actually be anywhere you like.)

        mkdir $HOME/lms2012-armel

3.  Create a docker container with the source and build directories mounted.

        docker run \
        --volume $HOME/lms2012-armel/:/build \
        --volume $(pwd):/src \
        --workdir /build \
        --name lms2012_armel \
        --env "TERM=$TERM" \
        --env "DESTDIR=/build/dist" \
        --tty \
        --detach \
        lms2012-armel tail

    Some notes:

    *   If you are using something other than `$HOME/lms2012-armel`, be sure to
        use the absolute path.
    *   `--env "TERM=$TERM"` is used so we can get ansi color output later.
    *   `--env "DESTDIR=/build/dist" is the staging directory where `make install`
        will install the program.
    *   `--tty`, `--detach` and `tail` are a trick to keep the container running.
        `--tty` causes `docker` to use a tty to provide STDIN and `tail` waits
        for input from STDIN. `--detach` causes the container to detach from our
        terminal so we can do other things.

4.  Run `cmake` in the build directory to get things setup.

        docker exec lms2012_armel cmake /src -DCMAKE_BUILD_TYPE=Debug \
        -DCMAKE_TOOLCHAIN_FILE=/home/compiler/toolchain-armel.cmake

5.  Then actually build the code.

        docker exec --tty lms2012_armel make
        docker exec --tty lms2012_armel make install

6.  When you are done building, you can stop the container.

        docker stop --time 0 lms2012_armel

    `docker exec ...` will not work until you start the container again.

        docker start lms2012_armel

    And the container can be deleted when you don't need it anymore (don't
    forget to stop it first).

        docker rm lms2012_armel
