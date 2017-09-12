Name:           io.systemd.sysinfo
Version:        1
Release:        1%{?dist}
Summary:        Systemd Device Interface
License:        ASL2.0
URL:            https://github.com/varlink/%{name}
Source0:        https://github.com/varlink/%{name}/archive/%{name}-%{version}.tar.gz
BuildRequires:  meson
BuildRequires:  gcc
BuildRequires:  pkgconfig
BuildRequires:  libvarlink-devel

%description
Sytem Information Interface.

%prep
%setup -q

%build
%meson
%meson_build

%check
export LC_CTYPE=C.utf8
%meson_test

%install
%meson_install

%files
%license LICENSE
%{_bindir}/io.systemd.sysinfo

%changelog
* Tue Aug 29 2017 <info@varlink.org> 1-1
- io.systemd.sysinfo 1
