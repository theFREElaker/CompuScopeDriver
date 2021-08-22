%define module gage-driver
%define version 5.04.33
%define _libdir /usr/lib

Name:		%{module}
Version:	%{version}
Release:	1%{?dist}
Summary:	GaGe Applied Technologies drivers and utility

License:	GPL
URL:		http://www.gage-applied.com/
Source0:	gage-driver-%{version}.tar.gz

BuildRequires:	gcc make kernel-headers java bash
Requires:	bash gcc make kernel-devel ldconfig

%description
Gage driver package

%prep
%setup -q


%build
%configure --prefix=/usr --libdir=\${prefix}/lib/gage --libexecdir=\${prefix}/lib/gage
make %{?_smp_mflags}
make %{?_smp_mflags} dist


%install
rm -rf %{buildroot}
make install DESTDIR=%{buildroot}
mkdir -p $RPM_BUILD_ROOT%{_usrsrc}/%{module}-%{version}/
tar xzf %{module}-%{version}.tar.gz -C $RPM_BUILD_ROOT%{_usrsrc}/

%clean
rm -rf %{buildroot}


%files
%defattr(-,root,root,-)
%{_bindir}/csrmd
%{_bindir}/gagehp.sh
%{_libdir}/gage/libCs16xyy.la
%{_libdir}/gage/libCs16xyy.so
%{_libdir}/gage/libCs16xyy.so.0
%{_libdir}/gage/libCs16xyy.so.0.0.0
%{_libdir}/gage/libCs8xxx.la
%{_libdir}/gage/libCs8xxx.so
%{_libdir}/gage/libCs8xxx.so.0
%{_libdir}/gage/libCs8xxx.so.0.0.0
%{_libdir}/gage/libCsAppSupport.la
%{_libdir}/gage/libCsAppSupport.so
%{_libdir}/gage/libCsAppSupport.so.0
%{_libdir}/gage/libCsAppSupport.so.0.0.0
%{_libdir}/gage/libCsDisp.la
%{_libdir}/gage/libCsDisp.so
%{_libdir}/gage/libCsDisp.so.0
%{_libdir}/gage/libCsDisp.so.0.0.0
%{_libdir}/gage/libCsFs.la
%{_libdir}/gage/libCsFs.so
%{_libdir}/gage/libCsFs.so.0
%{_libdir}/gage/libCsFs.so.0.0.0
%{_libdir}/gage/libCsRmDll.la
%{_libdir}/gage/libCsRmDll.so
%{_libdir}/gage/libCsRmDll.so.0
%{_libdir}/gage/libCsRmDll.so.0.0.0
%{_libdir}/gage/libCsSsm.la
%{_libdir}/gage/libCsSsm.so
%{_libdir}/gage/libCsSsm.so.0
%{_libdir}/gage/libCsSsm.so.0.0.0
%{_libdir}/gage/libCscdG12.la
%{_libdir}/gage/libCscdG12.so
%{_libdir}/gage/libCscdG12.so.0
%{_libdir}/gage/libCscdG12.so.0.0.0
%{_libdir}/gage/libCsxyG8.la
%{_libdir}/gage/libCsxyG8.so
%{_libdir}/gage/libCsxyG8.so.0
%{_libdir}/gage/libCsxyG8.so.0.0.0
%{_libdir}/gage/libCsEcdG8.la
%{_libdir}/gage/libCsEcdG8.so
%{_libdir}/gage/libCsEcdG8.so.0
%{_libdir}/gage/libCsEcdG8.so.0.0.0
%{_libdir}/gage/libCsE12xGy.la
%{_libdir}/gage/libCsE12xGy.so
%{_libdir}/gage/libCsE12xGy.so.0
%{_libdir}/gage/libCsE12xGy.so.0.0.0
%{_libdir}/gage/libCsE16bcd.la
%{_libdir}/gage/libCsE16bcd.so
%{_libdir}/gage/libCsE16bcd.so.0
%{_libdir}/gage/libCsE16bcd.so.0.0.0
%{_usrsrc}/%{module}-%{version}/

%post
if [ `uname -r | grep -c "BOOT"` -eq 0 ] && [ -e /lib/modules/`uname -r`/build/include ]; then

	install -m 644 %{_usrsrc}/%{module}-%{version}/Package/README.user.txt %{_usrsrc}/%{module}-%{version}/
	install -m 644 %{_usrsrc}/%{module}-%{version}/Package/dkms.conf %{_usrsrc}/%{module}-%{version}/
	install -m 755 %{_usrsrc}/%{module}-%{version}/Package/build_modules.sh %{_usrsrc}/%{module}-%{version}/
	install -m 755 %{_usrsrc}/%{module}-%{version}/Package/clean_modules.sh %{_usrsrc}/%{module}-%{version}/
	install -m 755 %{_usrsrc}/%{module}-%{version}/Package/register_dkms.sh %{_usrsrc}/%{module}-%{version}/
	install -m 755 %{_usrsrc}/%{module}-%{version}/Package/driver.sh %{_usrsrc}/%{module}-%{version}/

	for MODULE_DIR in CsJohnDeere CsRabbit CsCobraMax CsSpider CsSplenda CsDecade12 CsHexagon
	do
        make -C %{_usrsrc}/%{module}-%{version}/Boards/KernelSpace/$MODULE_DIR/Linux install > /dev/null 2>&1
        make -C %{_usrsrc}/%{module}-%{version}/Boards/KernelSpace/$MODULE_DIR/Linux clean > /dev/null 2>&1
	done
    %{_usrsrc}/%{module}-%{version}/register_dkms.sh add %{version} --fromRPM  > /dev/null 2>&1
	
	mkdir -p /usr/local/share/Gage
	install -m 644 %{_usrsrc}/%{module}-%{version}/Boards/Csi/CsSplenda/* /usr/local/share/Gage/

	xmlfile="/usr/local/share/Gage/GageDriver.xml"
	[ ! -e "$xmlfile" ] && echo -e "<?xml version=\"1.0\"?>\n<gageDriver>\n</gageDriver>"|sudo tee "$xmlfile" >/dev/null

	for driver in CscdG12 CsEcdG8 CsxyG8 Cs8xxx Cs16xyy CsE12xGy CsE16bcd; do
		if ! grep -q $driver $xmlfile; then
			sed -i "/<\/gageDriver>/ s/.*/\t<driver name=\"$driver\"><\/driver>\n&/" $xmlfile
		fi
		modprobe $driver > /dev/null 2>&1
	done

	install -m 644 %{_usrsrc}/%{module}-%{version}/Package/gage.conf %{_sysconfdir}/ld.so.conf.d/
	install -m 644 %{_usrsrc}/%{module}-%{version}/Package/gage.sh %{_sysconfdir}/profile.d/
	install -m 644 %{_usrsrc}/%{module}-%{version}/Package/gage-driver.conf %{_sysconfdir}/modules-load.d/
    
	ldconfig
	source /etc/profile
	
	chmod -R a+w %{_usrsrc}/%{module}-%{version}/Sdk

    echo "You need to relog into your account to be able to use the Gage drivers properly."
    notify-send -i dialog-warning -a Gage "Logout required" "You need to relog into your account to be able to use the Gage drivers properly."
elif [ `uname -r | grep -c "BOOT"` -gt 0 ]; then
    echo -e ""
    echo -e "Module build for the currently running kernel was skipped since you"
    echo -e "are running a BOOT variant of the kernel."
else
    echo -e ""
    echo -e "Module build for the currently running kernel was skipped since the"
    echo -e "kernel source for this kernel does not seem to be installed."
fi


%preun
echo -e "Uninstall of %{module} module (version %{version}) beginning:"
if ps ax|grep -v grep|grep -i 'csrmd'; then
    killall csrmd
fi

for driver in CscdG12 CsEcdG8 CsxyG8 Cs8xxx Cs16xyy CsE12xGy CsE16bcd; do
    if lsmod | grep -q $driver; then
        rmmod $driver
    fi
done

%{_usrsrc}/%{module}-%{version}/register_dkms.sh remove %{version} --fromRPM
for MODULE_DIR in CsJohnDeere CsRabbit CsCobraMax CsSpider CsSplenda CsDecade12 CsHexagon
do
    make -C %{_usrsrc}/%{module}-%{version}/Boards/KernelSpace/$MODULE_DIR/Linux uninstall  > /dev/null
    make -C %{_usrsrc}/%{module}-%{version}/Boards/KernelSpace/$MODULE_DIR/Linux clean > /dev/null
done

rm -f %{_usrsrc}/%{module}-%{version}/build_modules.sh \
    %{_usrsrc}/%{module}-%{version}/clean_modules.sh \
    %{_usrsrc}/%{module}-%{version}/register_dkms.sh \
    %{_usrsrc}/%{module}-%{version}/dkms.conf \
    %{_usrsrc}/%{module}-%{version}/driver.sh \
	%{_usrsrc}/%{module}-%{version}/README.user.txt \
    %{_sysconfdir}/ld.so.conf.d/gage.conf \
    %{_sysconfdir}/profile.d/gage.sh \
    %{_sysconfdir}/modules-load.d/gage-driver.conf

    
sudo ldconfig
source /etc/profile
rm -rf /usr/local/share/Gage

%changelog
* Fri Jun 09 2017 - Pascal De Lisio <pdelisio@gage-applied.com> - 0.1.3-1
- Removed DKMS dependencies
- Added support for Decade and Hexagon boards
- Removed CsFastBall

* Wed Aug 20 2014 - Simon Piette <simon.piette@savoirfairelinux.com> - 0.1.2-1
- Fix Sdk/CsAppSupport
- Sdk/Gage* examples can be compiled
- Update to gage-driver 0.1.2

* Wed Aug 13 2014 - Tristan Matthews <tristan.matthews@savoirfairelinux.com> - 0.1.1-6
- Update to gage-driver 0.1.1 (with added directories)

* Mon Aug 11 2014 - Tristan Matthews <tristan.matthews@savoirfairelinux.com> - 0.1-5
- Fix typo for dkms build, module build now works

* Thu Aug 07 2014 - Simon Piette <simon.piette@savoirfairelinux.com> - 0.1-4
- Simplify build, and succeed to do so

* Thu Aug 07 2014 - Simon Piette <simon.piette@savoirfairelinux.com> - 0.1-3
- Include all drivers

* Fri Aug 01 2014 - Simon Piette <simon.piette@savoirfairelinux.com> - 0.1-2
- Add dkms support

* Tue Jul 29 2014 - Simon Piette <simon.piette@savoirfairelinux.com> - 0.1-1
- Initial spec file
