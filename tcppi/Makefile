include $(TOPDIR)/rules.mk

PKG_NAME:=tcppi
PKG_VERSION:=1.2
PKG_RELEASE:=1

include $(INCLUDE_DIR)/package.mk

define Package/tcppi
  SECTION:=iot
  CATEGORY:=Customer
  TITLE:=IPv4/IPv6 tcp connection proxy
  URL:=http://www.spreadspace.org/tcpproxy/
endef

define Package/tcppi/conffiles
/etc/config/tcpproxy
endef

define Package/tcppi/description
tcpproxy is a simple tcp connection proxy which combines the features of rinetd and 6tunnel.
 tcpproxy supports IPv4 and IPv6 and also supports connections from IPv6 to IPv4 endpoints and vice versa.
endef

define Package/tcppi/configure
endef

define Build/Prepare
	mkdir -p $(PKG_BUILD_DIR)/
	$(CP) ./src/* $(PKG_BUILD_DIR)/
	(cd $(PKG_BUILD_DIR)/src; \
    touch include.mk; \
    echo '#ifndef TCPPROXY_config_h_INCLUDED' > config.h; \
    echo '#define TCPPROXY_config_h_INCLUDED' >> config.h; \
    echo '' >> config.h; \
    echo '#define VERSION_STRING_0 "tcpproxy version '`cat $(PKG_BUILD_DIR)/version`'"' >> config.h; \
    echo '#define VERSION_STRING_1 "built on '`hostname`', '`date +"%d.%m.%Y %H:%M:%S %Z"`'"' >> config.h; \
    echo '' >> config.h; \
    echo '#define TARGET "linux"' >> config.h; \
    echo '#define PREFIX "/usr"' >> config.h; \
    echo '#define BINDIR "/usr/bin"' >> config.h; \
    echo '#define SYSTEMDDIR "/tmp"' >> config.h; \
    echo '#define ETCDIR "/etc"' >> config.h; \
    echo '#define CONFFILE "/etc/tcpproxy.conf"' >> config.h; \
    echo '' >> config.h; \
    echo '#endif' >> config.h \
  )
endef

define Build/Compile
	$(MAKE) -C $(PKG_BUILD_DIR)/src \
    $(TARGET_CONFIGURE_OPTS) \
    TARGET=Linux \
    CFLAGS="$(TARGET_CFLAGS)" \
    LDFLAGS="$(TARGET_LDFLAGS)"
	$(STRIP) $(PKG_BUILD_DIR)/src/tcpproxy
endef

define Package/tcppi/install
	$(INSTALL_DIR) $(1)/etc/config
	$(INSTALL_DATA) ./files/tcpproxy.config $(1)/etc/config/tcpproxy
	$(INSTALL_DIR) $(1)/usr/bin
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/$(2)/src/tcpproxy $(1)/usr/bin/tcpproxy
	$(INSTALL_DIR) $(1)/etc/init.d
	$(INSTALL_BIN) ./files/tcpproxy.init $(1)/etc/init.d/tcpproxy
endef

$(eval $(call BuildPackage,tcppi))
