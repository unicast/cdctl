%define name cdctl
%define version 0.16
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
URL: http://cdctl.sourceforge.net

%description 
cdctl controls your cdrom drive under Linux.  It is an user-level console
application which gives the user access to the ioctl() interfaces of
the Uniform CDROM Driver of the Linux kernel.  It can play audio CDs,
control the drive's tray motor, change the drive's volume levels, read
the current type of disc in the drive, and much more.

%prep

%setup -n cdctl

%build
CXXFLAGS="$RPM_OPT_FLAGS" %configure
uname -a|grep SMP && make -j 2 || make

%install
rm -rf $RPM_BUILD_ROOT
mkdir -p $RPM_BUILD_ROOT/usr/local/bin
mkdir -p $RPM_BUILD_ROOT/usr/local/man/man1

install -s -m 755 cdctl $RPM_BUILD_ROOT/usr/local/bin
install -m 644 cdctl.1 $RPM_BUILD_ROOT/usr/local/man/man1

%clean
rm -rf $RPM_BUILD_ROOT

%files
%defattr(-,root,root,775)
%doc README LICENSE NEWS NUTSANDBOLTS PUBLICKEY

/usr/local/bin/cdctl
/usr/local/man/man1/cdctl.1

%changelog
* Sun Apr 02 2000 Andy Piper <squiggle@ukgateway.net> 
- Added man page to RPM package.

* Sat Apr 01 2000 Andy Piper <squiggle@ukgateway.net> 
- Modified spec file to remove redundant line and follow standards from the
HOWTO.

* Sun Mar 26 2000 Jason Spence <thalakan@technologist.com> 
- First spec file for cdctl.

# end of file
