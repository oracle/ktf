4. Building and installing KTF
------------------------------

KTF's user land side depends on googletest.
The googletest project has seen some structural changes in moving from a
project specific gtest-config via no package management support at all to
recently introduce pkgconfig support. This version of KTF only supports
building against a googletest (gtest) with pkgconfig support, which means
that as of February 2018 you have to build googletest from source at
github.

Googletest has also recently been fairly in flux, and while we
try to keep up to date with the official googletest version on Github,
we have seen issues with changes that breaks KTF. We also have a small
queue of enhancements and fixes to Googletest based on our experience
and use of it a.o. with KTF. You can find the latest rebase of this
version in the ktf branch of knuto/googletest at Github, but expect it
to rebase as we move forward to keep it up-to-date.
This version will at any time have been tested with KTF by us, since
we use it internally. Let's assume for the rest of these instructions
that your source trees are below ``~/src`` and your build trees are
under ``~/build``::

	cd ~/src
	git clone https://github.com/knuto/googletest.git

or::

        cd ~/src
        git clone https://github.com/google/googletest.git

then::

	mkdir -p ~/build/$(uname -r)
	cd ~/build/$(uname -r)
	mkdir googletest
	cd googletest
	cmake ~/src/googletest
	make
	sudo make install

Default for googletest is to use static libraries.  If you want to use shared
libraries for googletest, you can specify ``-DBUILD_SHARED_LIBS=ON`` to
cmake. If you don't want to install googletest into /usr/local, you can
specify an alternate install path using ``-DCMAKE_INSTALL_PREFIX=<your path>``
to cmake for googletest, and similarly use ``--prefix=<your path>`` both for
KTF and your own test modules. Note that on some distros, cmake version
2 and 3 comes as different packages, make sure you get version 3, which may
require you to use ``cmake3`` as command instead of cmake above.

Building the standalone version of KTF
**************************************

To build KTF from the standalone KTF git project,
cd to your source tree then clone the ktf project::

	cd ~/src
	<clone ktf>
	cd ktf
	autoreconf

Create a build directory somewhere outside the source tree to allow the
same KTF source tree to be used against multiple versions of the
kernel. Note that the configure command needs to use an absolute path -
relative paths are not supported.
Assuming for simplicity that you want to build for the running
kernel but you can build for any installed ``kernel-*-devel``::

	cd ~/build/$(uname -r)
	mkdir ktf
	cd ktf
	~/src/ktf/configure KVER=$(uname -r)
	make

Now you should have got a ``kernel/ktf.ko`` that works with your test kernel
and modules for the ``examples`` and KTF ``selftest`` directories.
Kernel objects will be installed under ``kernel/``.

Setting up your own test suite based on KTF
*******************************************
You are now ready to create your own test modules based on KTF.
KTF provides a script for setting up KTF based modules in a similar
way as KTF, using the autotools measures implemented in KTF. The script
assumes for simplicity that you want to have your source tree beside the
ktf source::

	~/src/ktf/scripts/ktfnew mysuite

Now go ahead and write your own kernel tests in ``~/src/mysuite/kernel``, then
similarly to KTF, build your new project with::

	cd ~/build/$(uname -r)
	mkdir mysuite
	cd mysuite
	~/src/mysuite/configure KVER=$(uname -r)
	make

By default, dependencies of KTF configured this way will depend directly on the
KTF build tree, which is convenient during development.
KTF also supports the ``install`` target, to allow KTF and test modules to
be built and maintained more independently (dynamic linker support via library
paths instead of -rpath etc). To set up your environment this way do as
follows::

	cd ~/build/$(uname -r)/ktf
	sudo make install
	cd ~/build/$(uname -r)/mysuite
	~/src/mysuite/configure KVER=$(uname -r) --with-ktf=/usr/local
	make

Options for configuring against a kernel base
*********************************************
The default configuration approach using ``KVER`` as sketched above is used to
configure against a prebuilt kernel. KTF also supports configuring against a
development kernel build tree. This can be a full in-source kernel build tree,
or a separate built tree created using the kernel's ``make O=<build root>``
method of building of-of-source. Instead of using ``KVER``, instead let
``KDIR`` point to the root directory of the desired kernel build tree::

	cd ~/build/$(uname -r)/ktf
	~/src/ktf/configure KDIR=$HOME/build/kernel
	make
