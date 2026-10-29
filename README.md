<img src="sc-apps-howdy.svg" width="128"/>

### KDE5 configuration module for [Howdy](https://github.com/boltgolt/howdy).

#### Current features:

- list/add/remove face models for current user (without elevated privileges)
- add/remove face models for other users (with elevated privileges)
- select camera, adjust `certainty` and `dark_threshold` (with elevated privileges)
- preview with recognition as in `howdy test` command

## This fork

Based on [golubevan/kcm-howdy](https://gitlab.com/golubevan/kcm-howdy) 6.0.1 (commit `51eebd78`), for Howdy 3 on KDE Plasma 6. One commit per change:

- the root helper runs the howdy CLI by absolute path (`HOWDY_COMMAND`)
- the preview thresholds start at Howdy's defaults
- only accounts people log into are listed (UID range of `/etc/login.defs`, shell in `/etc/shells`), with their avatars
- a layout like the Users page of System Settings, with more settings; settings that weaken facial unlock are shown read only
- the preview overlay is drawn by QML and stays sharp; the preview uses about 40 % of one core
- an optional head nod after recognition (Howdy's `nod` rubber stamp)

### Building

```sh
cmake -S . -B build \
    -DCMAKE_INSTALL_PREFIX=/usr -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTING=ON \
    -DCMAKE_POSITION_INDEPENDENT_CODE=ON \
    -DHOWDY_CONFIG_FILE=/etc/howdy/config.ini \
    -DHOWDY_MODELS_PATH=/etc/howdy/models \
    -DDLIB_MODELS_PATH=/usr/share/dlib-data \
    -DHOWDY_COMMAND=/usr/bin/howdy
cmake --build build
ctest --test-dir build
sudo cmake --install build
```

Point the four Howdy paths at your install, for example `/usr/local/...` for Howdy built from source. The module and its root helper are only looked up under `/usr`.

On Ubuntu, `liblapack-dev` ships no CMake config: add `-Dlapack_DIR=$PWD/cmake/ubuntu`.

### Howdy patches

`howdy-patches/` holds two patches for Howdy 3 itself, applied with `git apply` in the Howdy source tree:

- `stamps-fail-closed.patch`: upstream Howdy skips a rubber stamp that crashes or a rule it cannot read, and lets the unlock through. With this patch every such error denies the unlock. Apply it before turning the head nod on.
- `one-blas-thread.patch`: keeps OpenBLAS to one thread during authentication, which otherwise keeps several cores busy.

The head nod only checks that the nose moves up and down in the image: it stops a photo held still, not one moved in front of the camera.
