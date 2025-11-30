DESCRIPTION = "Update device tree"
SECTION = "base"
LICENSE = "CLOSED"

# Specify compatibility with Raspberry Pi 3
COMPATIBLE_MACHINE = "raspberrypi3"

DEPENDS:append = " linux-raspberrypi"
DEPENDS:append = " dtc-native"

# Where to find the device tree overlay source file
FILESEXTRAPATHS:prepend := "${THISDIR}/files:"
SRC_URI = "file://raspberry-overlay.dts"

inherit devicetree
inherit deploy

DTB_FILE = "raspberry-overlay.dtbo"
do_install() {
    install -d ${D}${datadir}/dts-overlay-src
    install -m 0644 ${WORKDIR}/raspberry-overlay.dts ${D}${datadir}/dts-overlay-src/
}
FILES:${PN} += " \
${datadir}/dts-overlay-src \
${datadir}/dts-overlay-src/raspberry-overlay.dts \
"

do_deploy() {
    install -m 0664 ${B}/raspberry-overlay.dtbo ${DEPLOYDIR}/raspberry-overlay.dtbo
}

addtask deploy after do_compile

# Define the destination directory for the compiled overlay in the image
# DEVICETREE_OVERLAY_DIR = "/boot/devicetree"

# KERNEL_DEVICETREE:append = " ${DEVICETREE_OVERLAY_DIR}/raspberry-overlay.dtbo"

# Add the compiled overlay to the kernel image for bootloader to find
# IMAGE_BOOT_FILES += " ${DEPLOYDIR}/gpio-led.dtbo"

# Add the following to local.conf
# IMAGE_BOOT_FILES:append = " gpio-led.dtbo;overlays/gpio-led.dtbo"
# RPI_EXTRA_CONFIG = ' \ndtoverlay=gpio-led,gpio=19,label=heart,trigger=heartbeat \n'
