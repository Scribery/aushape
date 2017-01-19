Name:       aushape
Version:    1
Release:    1%{?dist}
Summary:    Audit message format conversion library and utility
Group:      Applications/System

License:    GPLv2+ and LGPLv2+
URL:        https://github.com/Scribery/aushape
Source:     https://github.com/Scribery/%{name}/releases/download/v%{version}/%{name}-%{version}.tar.gz

BuildRequires:  audit-libs-devel

BuildRoot: %(mktemp -ud %{_tmppath}/%{name}-%{version}-%{release}-XXXXXX)

%description
Aushape is a library and a tool for converting audit messages to other
formats.

%global _hardened_build 1

%prep
%setup -q

%build
%configure --disable-rpath --disable-static
make %{?_smp_mflags}

%check
make %{?_smp_mflags} check

%install
make install DESTDIR=%{buildroot}
rm %{buildroot}/%{_libdir}/*.la
# Remove development files as we're not doing a devel package yet
rm %{buildroot}/%{_libdir}/*.so
rm -r %{buildroot}/usr/include/%{name}
rm -r %{buildroot}/usr/include/%{name}.h

%files
%{!?_licensedir:%global license %doc}
%license COPYING
%license COPYING.LESSER
%doc %{_defaultdocdir}/%{name}
%{_bindir}/%{name}
%{_libdir}/lib%{name}.so*

%changelog
