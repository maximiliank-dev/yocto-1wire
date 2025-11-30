SUMMARY = "bitbake-layers recipe"
DESCRIPTION = "Recipe created by bitbake-layers"
SECTION = "raspberrypi-software"
LICENSE = "MIT"
LIC_FILES_CHKSUM = "file://${COREBASE}/meta/files/common-licenses/MIT;md5=0835ade698e0bcf8506ecda2f7b4f302"
PV = "1.1"

CXX += "-std=c++20"

SRC_URI += "file://main.cpp \
            file://logger.cpp \
            file://logger.h \
            file://constants.h \
            file://Makefile \
            file://tcp-server.service \
            "

S = "${WORKDIR}"

EXTRA_OEMAKE = "PREFIX=${prefix} CXX='${CXX}' CFLAGS='${CFLAGS}' DESTDIR=${D} LIBDIR=${libdir} INCLUDEDIR=${includedir} BUILD_STATIC=no"

inherit systemd
SYSTEMD_SERVICE:${PN} = "tcp-server.service"

do_compile() {
    oe_runmake
}

do_install() {
    #oe_runmake install DESTDIR=${D}

    install -d ${D}${bindir}
    install -m 0755 ${WORKDIR}/tcp-server ${D}${bindir}/tcp-server

    install -d ${D}${systemd_system_unitdir}
    install -m 0644 ${WORKDIR}/tcp-server.service ${D}${systemd_system_unitdir}/

}

FILES:${PN} += "${systemd_system_unitdir}/tcp-server.service"
