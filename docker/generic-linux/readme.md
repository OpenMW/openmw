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