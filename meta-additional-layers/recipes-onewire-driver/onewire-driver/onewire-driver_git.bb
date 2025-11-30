DESCRIPTION = "My Device Character Driver"
SECTION = "base"
LICENSE = "CLOSED"

SRC_URI = "file://onewire_dev.c \
           file://Makefile \
           "

S = "${WORKDIR}"
 
inherit module

EXTRA_OEMAKE += 'KERNEL_SRC="${STAGING_KERNEL_BUILDDIR}"'

RPROVIDES:${PN} += "onewire_dev"

