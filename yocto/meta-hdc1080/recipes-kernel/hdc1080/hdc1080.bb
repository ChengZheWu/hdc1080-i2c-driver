SUMMARY = "HDC1080 temperature and humidity sensor kernel module"
DESCRIPTION = "Out-of-tree kernel module for TI HDC1080 I2C sensor, using Linux IIO subsystem"
LICENSE = "GPL-2.0-only"
LIC_FILES_CHKSUM = "file://${COMMON_LICENSE_DIR}/GPL-2.0-only;md5=801f80980d171dd6425610833a22dbe6"

inherit module

SRC_URI = "file://hdc1080.c;subdir=hdc1080 \
           file://Makefile;subdir=hdc1080 \
          "

S = "${WORKDIR}/hdc1080"

KERNEL_MODULE_AUTOLOAD += "hdc1080"

do_configure() {
    :
}

do_compile() {
    unset CFLAGS CPPFLAGS CXXFLAGS LDFLAGS
    oe_runmake -C ${STAGING_KERNEL_DIR} \
        M=${S} \
        O=${STAGING_KERNEL_BUILDDIR} \
        modules
}

do_install() {
    install -d ${D}/lib/modules/${KERNEL_VERSION}/extra
    find ${S} -name "*.ko" -exec install -m 0755 {} ${D}/lib/modules/${KERNEL_VERSION}/extra/ \;
}

FILES:${PN} += "/lib/modules"
