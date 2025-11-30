SUMMARY = "Config Receipe"
DESCRIPTION = "Config receipe. Set kayboard and onewire driver rw"
LICENSE = "CLOSED"
# c and c++ libs are not needed
INHIBIT_DEFAULT_DEPS = "1"
inherit allarch

# Use systemd
DISTRO_FEATURES:append = " systemd"
VIRTUAL-RUNTIME_init_manager ?= "systemd"
VIRTUAL-RUNTIME_initscripts ?= ""


do_install() {
    # Ensure target dirs exist
    install -d ${D}${sysconfdir}
    install -d ${D}${sysconfdir}/udev/rules.d
    install -d ${D}${sysconfdir}/sysconfig

    # Add keyboard config to key layout ot DE
    cat > ${D}${sysconfdir}/vconsole.conf << 'EOF'
KEYMAP=de
FONT=
FONT_MAP=
FONT_UNIMAP=
EOF

    cat > ${D}${sysconfdir}/sysconfig/keyboard << 'EOF'
KEYTABLE="de"
MODEL="pc"
LAYOUT="de"
VARIANT=""
OPTIONS=""
EOF

    # Set /dev/onewire_dev driver to +rw using udev
    cat > ${D}${sysconfdir}/udev/rules.d/99-onewire.rules << 'EOF'
KERNEL=="onewire_dev", MODE="0666"
EOF
}

FILES:${PN} += " \
    ${sysconfdir}/vconsole.conf \
    ${sysconfdir}/sysconfig/keyboard \
    ${sysconfdir}/udev/rules.d/99-onewire.rules \
"
