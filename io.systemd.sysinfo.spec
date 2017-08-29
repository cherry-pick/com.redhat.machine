Name:           io.systemd.sysinfo
Version:        1
Release:        1%{?dist}
Summary:        Systemd Device Interface
License:        ASL2.0
URL:            https://github.com/varlink/%{name}
Source0:        https://github.com/varlink/%{name}/archive/%{name}-%{version}.tar.gz
BuildRequires:  autoconf automake pkgconfig
BuildRequires:  libvarlink-devel
BuildRequires:  libudev-devel

%description
Sytem Information Interface.

%prep
%setup -q

%build
./autogen.sh
%configure
make %{?_smp_mflags}

%install
%make_install

%files
%license AUTHORS
%license COPYRIGHT
%license LICENSE
%{_bindir}/io.systemd.sysinfo

%changelog
* Tue Aug 29 2017 <info@varlink.org> 1-1
- io.systemd.sysinfo 1
