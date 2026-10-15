* `cd` here
* `docker compose up`
* wait
* terminal seems to get stuck at `Attaching to generic-linux-build-container-1`, but Docker Desktop says the container is alive.
* VS Code dev containers extension is able to connect.
* Make `/build/openmw` the active directory.
* `CI/linux/before_install_for_vcpkg.sh`
* `CI/before_script.linux.sh -B -generic -C`
* `cd build-generic`
* `cmake --build . --parallel` (but maybe explicitly add a thread count after parallel that's lower than your hardware thread count if you're using WSL2)
* resulting binaries should run fine in the container (potentially with `LD_LIBRARY_PATH` to pick up Qt plugins)
* `cpack` or `cmake --install .` should tweak things to produce generic packages that run on any distro

### Bonus

* Use `-B literally-any-suffix-you-want` to make more build directories
* `export DEPENDENCIES_ROOT_PATH=some/dir/here` to use different dependencies, e.g. a local build from openmw-deps-build
* Debug symbols for dependencies are available via https://openmw-sym.rgw.ctrl-c.liu.se.
  `export DEBUGINFOD_URLS="https://openmw-sym.rgw.ctrl-c.liu.se $DEBUGINFOD_URLS"` to use them.
* You don't *have* to do any of this in a container:
  - AlmaLinux 9 running on distrobox, a regular VM or bare metal should also produce generic Linux builds.
  - Running on other distros should produce a build that works just fine on that machine, but potentially won't produce something especially usable on other distros.
