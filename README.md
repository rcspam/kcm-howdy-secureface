<img src="sc-apps-howdy.svg" width="128"/>

### KDE Plasma 6 configuration module for [Howdy](https://github.com/boltgolt/howdy).

#### Current features:

- list/add/remove face models for current user (without elevated privileges)
- add/remove face models for other users (with elevated privileges)
- select camera, adjust `certainty` and `dark_threshold` (with elevated privileges)
- preview with recognition as in `howdy test` command

## This fork

Based on [golubevan/kcm-howdy](https://gitlab.com/golubevan/kcm-howdy) 6.0.1 (commit `51eebd78`). One commit per change.

Howdy 3 has no release yet: the latest one, 2.6.1 from 2020, is not supported. This fork is built and tested against the development version of Howdy, `master` at commit [`d3ab993`](https://github.com/boltgolt/howdy/commit/d3ab99382f88f043d15f15c1450ab69433892a1c) (2025-06-22, version `beta` in its `meson.build`), on Ubuntu 24.04 (TUXEDO OS) with KDE Plasma 6 and Qt 6.10.

Changes:

- the root helper runs the howdy CLI by absolute path (`HOWDY_COMMAND`)
- the preview thresholds start at Howdy's defaults
- only accounts people log into are listed (UID range of `/etc/login.defs`, shell in `/etc/shells`), with their avatars
- a layout like the Users page of System Settings, with more settings; settings that weaken facial unlock are shown read only
- the preview overlay is drawn by QML and stays sharp; the preview uses about 40 % of one core
- an optional head nod after recognition (Howdy's `nod` rubber stamp)
- a French translation

### Installing Howdy 3 from source

Howdy 3 runs on a Python that has dlib, OpenCV and numpy. On Ubuntu 24.04, OpenCV and numpy come from apt, and dlib goes into a venv that also sees them:

```sh
sudo apt install build-essential cmake meson ninja-build python3-dev python3-venv \
    libopenblas-dev liblapack-dev libpam0g-dev libinih-dev libevdev-dev python3-opencv pamtester
sudo python3 -m venv --system-site-packages /opt/howdy/venv
sudo /opt/howdy/venv/bin/pip install dlib==20.0.1    # compiled on the spot, takes a while
```

Build Howdy at the tested commit, with the patches of this repository (see below):

```sh
git clone https://github.com/boltgolt/howdy.git
cd howdy
git checkout d3ab99382f88f043d15f15c1450ab69433892a1c
git apply /path/to/kcm-howdy-secureface/howdy-patches/*.patch
meson setup build --prefix=/usr/local -Dpython_path=/opt/howdy/venv/bin/python
meson compile -C build
sudo meson install -C build
```

Download the dlib models, point Howdy at the camera and add a face model:

```sh
cd /usr/local/share/dlib-data && sudo ./install.sh
sudoedit /usr/local/etc/howdy/config.ini    # device_path, preferably a /dev/v4l/by-path/... link
sudo howdy add
```

An IR camera is strongly advised: with a plain webcam, a photo of you can be enough to get in.

#### Unlocking the Plasma lock screen

The lock screen runs the `kde-fingerprint` PAM service next to the password field. Copy `/usr/lib/pam.d/kde-fingerprint` to `/etc/pam.d/`, then in the copy:

- replace the `pam_fprintd.so` auth line with
  `auth [success=ok new_authtok_reqd=ok default=die] /usr/local/lib/x86_64-linux-gnu/security/pam_howdy.so`
- remove the `auth optional pam_kwallet5.so` line: with no password it waits for one and blocks the face unlock.

Keep `default=die`. With a plain `required`, pam_howdy can return PAM_IGNORE, and the `pam_succeed_if` line before it would then unlock the screen on its own. Test without locking with `pamtester kde-fingerprint $USER authenticate`.

### Building the module

```sh
cmake -S . -B build \
    -DCMAKE_INSTALL_PREFIX=/usr -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTING=ON \
    -DCMAKE_POSITION_INDEPENDENT_CODE=ON \
    -DHOWDY_CONFIG_FILE=/usr/local/etc/howdy/config.ini \
    -DHOWDY_MODELS_PATH=/usr/local/etc/howdy/models \
    -DDLIB_MODELS_PATH=/usr/local/share/dlib-data \
    -DHOWDY_COMMAND=/usr/local/bin/howdy
cmake --build build
ctest --test-dir build
sudo cmake --install build
```

The four Howdy paths match the install above; a packaged Howdy uses `/etc/howdy` and `/usr/share/dlib-data` instead. The module itself must go under `/usr`, the only place System Settings and KAuth look for it.

On Ubuntu, `liblapack-dev` ships no CMake config: add `-Dlapack_DIR=$PWD/cmake/ubuntu`.

### Howdy patches

`howdy-patches/` holds two patches for Howdy itself, made against commit `d3ab993` and applied in the steps above:

- `stamps-fail-closed.patch`: upstream Howdy skips a rubber stamp that crashes or a rule it cannot read, and lets the unlock through. With this patch every such error denies the unlock. Apply it before turning the head nod on.
- `one-blas-thread.patch`: keeps OpenBLAS to one thread during authentication, which otherwise keeps several cores busy.

The head nod only checks that the nose moves up and down in the image: it stops a photo held still, not one moved in front of the camera.
