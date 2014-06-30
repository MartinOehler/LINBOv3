#!/usr/bin/make -f
# Makefile for LINBO
# (C) Klaus Knopper <knoppix@knopper.net> 2013
# License: GPL V2

CMDLINE  = loglevel=4 nmi_watchdog=0 debug apm=power-off

VESAMODE = 0 # Don't activate VESA FB
# VESAMODE = 791 # 1024x768, 64k colors

# Configuration, these are evaluated by scripts and need export.

# Define which kernel to download and compile for LINBO
export KVERS    = 3.13.11

# Define which packages we need to install after debootstrap
export PACKAGES = apt-utils net-tools vim-tiny dbus dbus-x11 udev fuse ctorrent parted dosfstools e2fsprogs cifs-utils nfs-common xorg xserver-xorg-core xserver-xorg xserver-xorg-video-intel xserver-xorg-video-radeon xserver-xorg-video-nouveau firmware-linux xserver-xorg-video-all xserver-xorg-input-all libgl1-mesa-dri libgl1-mesa-glx libgl1-mesa-dri-experimental libdrm-intel1 libdrm-nouveau1a libdrm-radeon1 libdrm2 iceweasel/experimental iceweasel-l10n-de/experimental libnspr4 network-manager network-manager-gnome network-manager-openvpn network-manager-openvpn-gnome network-manager-pptp network-manager-pptp-gnome network-manager-vpnc network-manager-vpnc-gnome libnm-glib4 libnm-gtk0 libnm-util2 hdparm console-tools console-data inetutils-syslogd sudo virtualbox virtualbox-dkms kexec-tools xterm x11-xserver-utils xinit metacity fonts-dejavu xfonts-base less openssh-client udpcast reiserfsprogs qemu-utils x11vnc bridge-utils dialog lvm2 openbox grub-legacy lilo ipython libpython2.6 libpython2.7 python python-configobj python-crypto python-decorator python-django python-minimal python-mysqldb python-pexpect python-pip python-pkg-resources python-setuptools python-simplegeneric python-support python2.6 python2.6-minimal python2.7 python2.7-minimal apache2/stable libapache2-mod-wsgi/stable libapache2-mod-php5/stable wget gparted dmraid kpartx xfsprogs gpart acl attr iputils-ping

# Define kernel architecture. Currently, we support intel/amd 32 and 64bit
export ARCH     = i386
export ARCH64   = x86_64

# Define the CPU architecture for testing LINBO in kvm
# export CPU = kvm64
export CPU = qemu64
export MEM = 1800

# My packages
EXTRA_PACKAGES = Sources/ntfs-3g_2012.1.15AR.8-2knoppix0_i386.deb Sources/rsync_3.1.0-0ntfs0_i386.deb Sources/cloop-utils_2.0-1_i386.deb Sources/hwsetup_1.4-21_all.deb Sources/usleep-knoppix_0.5-2_i386.deb Sources/knoppix-smbmount_0.2-1_i386.deb

# My scripts
EXTRA_SCRIPTS = Sources/Linbo/linbo_cmd Sources/Linbo/grub.exe

help:
	@echo "[1mWELCOME TO THE LINBO BUILD SYSTEM"
	@echo ""
	@echo "make kernel		(Re-)Build Kernel packages"
	@echo "make kernel-install	(Re-)Install Kernel packages"
	@echo "make initrd		(Re-)Build Initramfs"
	@echo "make chroot		Work inside Filesystem"
	@echo "make chroot64		Work inside 64bit Filesystem dir"
	@echo "make iso		Make bootable iso (CD or DVD)"
	@echo "make filesystem  	Bootstrap and fill the LINBO ./Filesystem directory"
	@echo "make update      	Install or update required packages in LINBO ./Filesystem"
	@echo "make distclean   	Remove all resources that can be recreated."
	@echo "make test		Run ISO in kvm"
	@echo
	@echo "Don't worry about the sequence of build commands, this Makefile will tell you"
	@echo "what to do first, in case anything is missing."
	@echo
	@echo "Have a lot of fun. ;-)"
	@echo "[0m"

all-new:
	-rm -f Image.iso
#	-rm -f clean-stamp
	make knoppix cd de initrd compressed iso
	mv -f Image.iso Image-DVD-EN.iso

distclean:
	sudo rm -rf Filesystem/* Image/boot/isolinux/linux* Image/boot/isolinux/minirt.gz Kernel/* *-stamp

# Add a timestamp to visible scripts, i.e. /init and /etc/init.d/knoppix-autoconfig
version:
	V="`date +'%Y-%m-%d %H:%M'`"; sudo sed -i -e 's/^VERSION=.*$$/VERSION="'"$$V"'"/g' Initrd/init Filesystem/etc/init.d/knoppix-autoconfig

filesystem-stamp: ./Scripts/LINBO.mkfilesystem
	-rm -f update-stamp knoppify-stamp clean-stamp
	./Scripts/LINBO.mkfilesystem
	touch filesystem-stamp

filesystem:
	make filesystem-stamp

knoppify: filesystem-stamp
	make knoppify-stamp

knoppify-stamp: ./Scripts/LINBO.chroot
	-rm -f clean-stamp
	sudo Scripts/LINBO.chroot Filesystem /bin/bash -c "ln -snf /bin/bash /bin/sh; apt-get update; apt-get install locales busybox sudo"
	[ -d Sources/Knoppix/lib/udev/devices ] || ( cd Sources; sudo tar zxvf Knoppix-devices.tar.gz )
	Scripts/LINBO.knoppify Sources/Knoppix Filesystem
	sudo Scripts/LINBO.chroot Filesystem /bin/bash < Scripts/LINBO.knoppify-chroot
	sudo Scripts/LINBO.chroot Filesystem /bin/bash < Scripts/LINBO.mkfilesystem64-chroot
	sudo Scripts/LINBO.chroot Filesystem/64 /bin/bash < Scripts/LINBO.knoppify-chroot
	sudo install -o root -m 4755 Sources/asroot/asroot Filesystem/usr/bin/
	touch knoppify-stamp
	
update-stamp: filesystem-stamp knoppify-stamp
	-rm -f clean-stamp
	sudo Scripts/LINBO.chroot Filesystem /bin/bash -c "apt-get update; apt-get install -t unstable --no-install-recommends $(PACKAGES)"
	touch update-stamp

update:
	make update-stamp
		
install-gui-stamp: filesystem-stamp knoppify-stamp update-stamp Sources/Linbo_GUI
	[ -d Filesystem/srv/django ] || sudo mkdir -p Filesystem/srv/django
	sudo rsync -HavS --partial --progress --stats --delete --exclude='.svn' Sources/Linbo_GUI/linboweb/* Filesystem/srv/django/
	[ -d Filesystem/srv/www/site_media ] || sudo mkdir -p Filesystem/srv/www/site_media
	sudo rsync -HavS --partial --progress --stats --delete --exclude='.svn' Sources/Linbo_GUI/linboweb-static/* Filesystem/srv/www/site_media/
	[ -d Filesystem/srv/django/static ] || sudo mkdir -p Filesystem/srv/django/static
	sudo cp Sources/Linbo_GUI/linboweb/static/dajaxice.core.js Filesystem/srv/django/static/
	sudo cp -dR Sources/Linbo_GUI/etcapache2/apache2/* Filesystem/etc/apache2/
	sudo Scripts/LINBO.chroot Filesystem chown -Rh www-data:www-data /srv/django /srv/www/site_media
	grep -q '^/etc/init.d/apache2' Filesystem/etc/rc.local || sudo sed -i 's|^exit 0$$||g;$$a /etc/init.d/apache2 start\nexit 0' Filesystem/etc/rc.local
	touch install-gui-stamp

install-gui:
	make install-gui-stamp

chroot64: Filesystem ./Scripts/LINBO.chroot
	rm -f clean-stamp
	sudo Scripts/LINBO.chroot Filesystem/64

chroot: Filesystem ./Scripts/LINBO.chroot
	rm -f clean-stamp
	sudo Scripts/LINBO.chroot Filesystem

clean-stamp: Filesystem ./Scripts/LINBO.clean
	sudo Scripts/LINBO.chroot Filesystem /bin/bash < Scripts/LINBO.clean
	sudo Scripts/LINBO.chroot Filesystem/64 /bin/bash < Scripts/LINBO.clean
	touch $@

compressed: filesystem-stamp knoppify-stamp update-stamp kernel-install-stamp extra-scripts-stamp clean-stamp Scripts/LINBO.mkcompressed Bin/create_compressed_fs version
	-mkdir -p Image/LINBO
	nice -10 ionice -c 3 sudo ./Scripts/LINBO.mkcompressed Filesystem Image/LINBO/LINBO

addons: Addons
	cd $< ; sudo genisoimage -l -R -U -v . | ../Bin/create_compressed_fs -L -2 -B 131072 -m - ../Image/LINBO/LINBO1

extra-packages:
	make extra-packages-stamp

extra-packages-stamp: filesystem-stamp Scripts/LINBO.chroot
	-rm -f clean-stamp
	@for i in $(EXTRA_PACKAGES); do [ -r "$$i" ] || { echo "Please build package $$i first." >&2; exit 1; } ; done
	ln -nf $(EXTRA_PACKAGES) Filesystem/tmp/
	sudo ./Scripts/LINBO.chroot Filesystem bash -c "apt-get update; apt-get install -t unstable libpopt0 pciutils"
	sudo ./Scripts/LINBO.chroot Filesystem bash -c "cd /tmp; dpkg -i $(notdir $(EXTRA_PACKAGES))"
	touch extra-packages-stamp
	
extra-scripts:
	make extra-scripts-stamp

extra-scripts-stamp:
	-rm -f clean-stamp
	@for i in $(EXTRA_SCRIPTS); do [ -r "$$i" ] || { echo "Please provide $$i first." >&2; exit 1; } ; done
	sudo install -m 755 $(EXTRA_SCRIPTS) Filesystem/usr/bin/
	touch extra-scripts-stamp

kernel:
	make kernel-stamp

kernel-stamp: ./Scripts/LINBO.kernel
	rm -f kernel-install-stamp
	./Scripts/LINBO.kernel
	touch kernel-stamp

env:
	Scripts/LINBO.env

kernel-install: filesystem-stamp update-stamp kernel-stamp
	make kernel-install-stamp

kernel-install-stamp: Scripts/LINBO.chroot
	-mkdir -p Image/boot/isolinux
	ln -nf Kernel/linux-image-$(KVERS)*.deb Kernel/linux-headers-$(KVERS)*.deb Filesystem/tmp/
	sudo Scripts/LINBO.chroot Filesystem bash -c "dpkg -l libncursesw5 | grep -q '^i' || { apt-get update; apt-get install libncursesw5; } ; apt-get --purge remove linux-headers-\* linux-image-\*; dpkg -i /tmp/linux-headers-$(KVERS)*.deb /tmp/linux-image-$(KVERS)*.deb; /etc/kernel/header_postinst.d/dkms $(KVERS) /boot/vmlinuz-$(KVERS); /etc/kernel/header_postinst.d/dkms $(KVERS)-64 /boot/vmlinuz-$(KVERS)-64"
	[ -r Filesystem/boot/vmlinuz-$(KVERS) ] && cp -uv  Filesystem/boot/vmlinuz-$(KVERS) Image/boot/isolinux/linux || true
	[ -r Filesystem/boot/vmlinuz-$(KVERS)-64 ] && cp -uv Filesystem/boot/vmlinuz-$(KVERS)-64 Image/boot/isolinux/linux64 || true
	touch kernel-install-stamp

initrd: Initrd/init version
	-mkdir -p Image/boot/isolinux
	( cd Initrd && find . | sudo cpio -H newc -ov | gzip -9v ) > Image/boot/isolinux/minirt.gz
	touch initrd-stamp

Initrd Initrd/init: Initrd.tar.gz
	sudo tar zxpPvf $<

iso: Image Image/boot/isolinux/isolinux.bin Scripts/LINBO.mkfinal
	-rm -f Image/LINBO/LINBO.tmp
	Scripts/LINBO.mkfinal Image Image.iso

Image/boot/isolinux/isolinux.bin: /usr/lib/syslinux/isolinux.bin
	cp -v $< $@

boot-iso: Image Image/boot/isolinux/isolinux.bin
	@echo "[1mCreating kernel-only boot iso image...[0m"
	genisoimage -input-charset ISO-8859-1 -pad -l -r -J \
          -V "LINBO_BOOT" -A "LINBO_BOOT" \
          -no-emul-boot -boot-load-size 4 -boot-info-table \
	  -b boot/isolinux/isolinux.bin -c boot/isolinux/boot.cat \
          -hide-rr-moved \
          -m LINBO -m \*.html -o Image-bootonly.iso Image || bailout 1
	@echo "[1mKernel-only boot iso image created.[0m"

de:
	sed -i -e 's/lang=../lang=$@/g;s/# *KBDMAP german.kbd/KBDMAP german.kbd/g' Image/boot/isolinux/isolinux.cfg

en:
	sed -i -e 's/lang=../lang=$@/g;s/^KBDMAP german.kbd/# KBDMAP german.kbd/g' Image/boot/isolinux/isolinux.cfg

Data/mkisofs.sort:
	Scripts/LINBO.mksortlist Filesystem

sda.img:
	qemu-img create -f qcow2 -o cluster_size=4k,preallocation=metadata $@ 50G

kvm:
	[ -d /sys/module/kvm_intel -o -d /sys/module/kvm_amd ] || for i in intel amd; do sudo modprobe kvm_$$i; done || true

test: kvm Image.iso sda.img
	@echo "[1mStarting LINBO in kvm...[0m"
	kvm -cpu $(CPU) $(VNCOPTS) -usb -m $(MEM) -vga vmware -monitor stdio -soundhw es1370 -boot d -cdrom Image.iso -hda sda.img

hdtest: kvm sda.img
	@echo "[1mStarting LINBO in kvm...[0m"
	kvm -cpu $(CPU) $(VNCOPTS) -usb -m $(MEM) -vga vmware -monitor stdio -soundhw es1370 -hda sda.img -boot c -net user,tftp=Tftpboot,bootfile=/pxelinux.0,smb=/LINBO,smbserver=10.0.2.4 -net nic

pxe-dependencies:
	@[ -r /usr/lib/syslinux/pxelinux.0 ] || { echo "Please install the syslinux package version 4.x for testing LINBO via PXE boot."; exit 1; }
	@[ -d Tftpboot -o -L Tftpboot ] || { echo "Please create or symlink directory \"Tftoboot\" to the directory containing LINBO kernel and diles."; exit 1; }

PXEBOOTFILES = /usr/lib/syslinux/pxelinux.0 Image/boot/isolinux/logo.16 Image/boot/isolinux/linux Image/boot/isolinux/linux64 Image/boot/isolinux/minirt.gz Image/boot/isolinux/minirt.gz Image/boot/isolinux/isolinux.cfg Image/boot/isolinux/grub.exe

pxetest: kvm pxe-dependencies $(PXEBOOTFILES)
	@echo "[1mStarting LINBO in kvm via PXE...[0m"
	rsync -HavSP $(PXEBOOTFILES) Tftpboot/
	[ -w /LINBO/. ] && rsync -HaSP Image/LINBO /LINBO/
	-mkdir -p Tftpboot/pxelinux.cfg
	sed -e 's|lang=de |lang=de server=10.0.2.2 smbdir=//10.0.2.4/qemu |g' Tftpboot/isolinux.cfg > Tftpboot/pxelinux.cfg/default
	kvm -cpu $(CPU) $(VNCOPTS) -usb -m $(MEM) -vga vmware -monitor stdio -soundhw es1370 -hda sda.img -boot n -net user,tftp=Tftpboot,bootfile=/pxelinux.0,smb=/LINBO,smbserver=10.0.2.4 -net nic

vnctest: kvm Image.iso
	$(MAKE) VNCOPTS="-vnc :10 -k de" test

flash:
	cd Image && flash-knoppix

burn: Image.iso
	sudo cdrecord -v -dao -eject speed=8 Image.iso

hardlinks:
	sudo Scripts/LINBO.chroot Filesystem /bin/bash < Scripts/LINBO.hardlinks | tee Filesystem/tmp/hardlinks.sh

findorphans: Filesystem ./Scripts/LINBO.findorphans
	sudo Scripts/LINBO.chroot Filesystem /bin/bash < Scripts/LINBO.findorphans
