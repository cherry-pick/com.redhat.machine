Name:           com.redhat.machine
Version:        1
Release:        1%{?dist}
Summary:        Machine Interface
License:        ASL2.0
URL:            https://github.com/varlink/%{name}
Source0:        https://github.com/varlink/%{name}/archive/%{name}-%{version}.tar.gz
BuildRequires:  meson
BuildRequires:  gcc
BuildRequires:  pkgconfig
BuildRequires:  libvarlink-devel

%description
Machine Interface.

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
%{_bindir}/com.redhat.machine

%changelog
* Tue Aug 29 2017 <info@varlink.org> 1-1
- com.redhat.machine 1
