Using Docker to Cross-Compile lms2012-compat
--------------------------------------------

This assumes that you have already cloned the git repository and the current
working directory is the source code directory and that you have docker >= 1.12
installed.

1. Create the docker image.

        docker build -t lms2012-armhf -f docker/armhf.dockerfile docker/

2. Create an empty build directory. (This can be anywhere you like.)

        mkdir $HOME/lms2012-armhf

3.  Create a docker container with the source and build directories mounted.

        docker run -v $HOME/lms2012-armhf/:/build \
        -v $(pwd)/lms2012-compat:/src \
        -w /build \
        --name lms2012_armhf \
        -e "TERM=$TERM" \
        -td lms2012-armhf tail

    Some notes:

    *   If you are using something other than `$HOME/lms2012-armhf`, be sure to
        use the absolute path.
    *   `-e "TERM=$TERM"` is used so we can get ansi color output later.
    *   `-td` and `tail` are a trick to keep the container running. `-t` causes
        `docker` to use a tty to provide STDIN and `tail` waits for input from
        STDIN. `-d` causes the container to detach from our terminal so we can
        do other things.

4.  Run `cmake` in the build directory to get things setup.

        docker exec -t lms2012_armhf cmake -DCMAKE_BUILD_TYPE=Debug \
        -DCMAKE_TOOLCHAIN_FILE=/home/compiler/toolchain-armhf.cmake

5.  Then actually build the code.

        docker exec -t lms2012_armhf make

6.  When you are done building, you can stop the container.

        docker stop -t 0 lms2012_armhf

    `docker exec ...` will not work until you start the container again.

        docker start lms2012_armhf

    And the container can be deleted when you don't need it anymore.

        docker rm lms2012_armhf
