MYFW_CTL_VERSION = 1.0
MYFW_CTL_SITE = $(MYFW_CTL_PKGDIR)/src
MYFW_CTL_SITE_METHOD = local
MYFW_CTL_DEPENDENCIES = libnl

define MYFW_CTL_BUILD_CMDS
	$(TARGET_MAKE_ENV) $(MAKE) $(TARGET_CONFIGURE_OPTS) -C $(@D) all
endef

define MYFW_CTL_INSTALL_TARGET_CMDS
	$(INSTALL) -D -m 0755 $(@D)/myfw-ctl $(TARGET_DIR)/usr/bin/myfw-ctl
endef

$(eval $(generic-package))