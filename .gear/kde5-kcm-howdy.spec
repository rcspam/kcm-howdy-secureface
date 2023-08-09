%define _unpackaged_files_terminate_build 1

Name: kde5-kcm-howdy
Version: 0.0.1
Release: alt1

Summary: KDE Workspace 5 Howdy configuration module
License: GPLv3+
Group: Graphical desktop/KDE
Url: https://gitlab.com/golubevan/kcm-howdy

Source: %name-%version.tar

BuildRequires(pre): rpm-build-kf5
BuildRequires: cmake
BuildRequires: extra-cmake-modules
BuildRequires: gcc-c++
BuildRequires: kf5-kcoreaddons-devel
BuildRequires: kf5-kpackage-devel
BuildRequires: kf5-kcmutils-devel
BuildRequires: kf5-kdeclarative-devel
BuildRequires: kf5-ki18n-devel

%description
KDE Workspace 5 Howdy configuration module

%prep
%setup

%build
%K5build

%install
%K5install
%K5install_move data kcm_howdy kpackage

%files
%_K5xdgapp/kcm_howdy.desktop
%_K5plug/plasma/kcms/systemsettings/kcm_howdy.so
%_K5data/kpackage/kcms/kcm_howdy/
%_K5data/icons/hicolor/scalable/apps/howdy.svg

%changelog
* Wed Aug 09 2023 Anton Golubev <golubevan@altlinux.org> 0.0.1-alt1
- Initial build.
