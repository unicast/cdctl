%define name cdctl
%define version 0.15
%define release 1

Summary: Controls your cdrom drive.
Name: %{name}
Version: %{version}
Release: %{release}
Source0: %{name}-%{version}.tar.gz
Copyright: Free for non-commercial use
Group: Applications/System
BuildRoot: %{_tmppath}/%{name}-buildroot
Prefix: %{_prefix}
Requires:

%description 
cdctl controls your cdrom drive under Linux.  It is an user-level console
application which gives the user access to the ioctl() interfaces of
the Uniform CDROM Driver of the Linux kernel.  It can play audio CDs,
control the drive's tray motor, change the drive's volume levels, read
the current type of disc in the drive, and much more.

%prep

%setup

%build
CXXFLAGS="$RPM_OPT_FLAGS" %configure
uname -a|grep SMP && make -j 2 || make

%install
rm -rf $RPM_BUILD_ROOT
mkdir -p "$RPM_BUILD_ROOT/usr/local/bin"
mkdir -p "$RPM_BUILD_ROOT/usr/doc/cdctl-$VERSION"

install -s -m 755 cdctl "$RPM_BUILD_ROOT/usr/local/bin"
install -m 755 LICENSE NEWS NUTSANDBOLTS PUBLICKEY README "$RPM_BUILD_ROOT/usr/doc/cdctl-$VERSION"

%clean
rm -rf "$RPM_BUILD_ROOT"

%files
%defattr(-,root,root,775)
%doc README LICENSE NEWS NUTSANDBOLTS PUBLICKEY

/usr/local/bin/cdctl


%changelog
* Sun Mar 26 2000 Jason Spence <thalakan@technologist.com> 
- First spec file for cdctl.

# end of file
