include $(TOPDIR)/rules.mk

PKG_NAME:=smart
PKG_VERSION:=1.0
PKG_RELEASE:=0

include $(INCLUDE_DIR)/package.mk
 
define Package/$(PKG_NAME)
    SECTION:=iot
    CATEGORY:=Customer
    DEPENDS:=+libuci +libubus +libblobmsg-json +libubox
    TITLE:=smart service for adapter and smart device
endef

define Package/$(PKG_NAME)/description
        this is a smart service for adapter and smart device
endef

 
define Build/Prepare
	mkdir -p $(PKG_BUILD_DIR)
	$(CP) ./src/* $(PKG_BUILD_DIR)/
endef

define Package/smart/postinst
#!/bin/sh
/etc/init.d/smart enable
/etc/init.d/smart restart
rm -rf /tmp/luci-*
endef

define Package/smart/postrm
#!/bin/sh
rm -rf /tmp/luci-*
killall -kill smart
endef

define Build/Compile
	$(call Build/Compile/Default)
endef

define Package/$(PKG_NAME)/install
	$(INSTALL_DIR) $(1)/usr/bin
	$(INSTALL_BIN)  $(PKG_BUILD_DIR)/smart  $(1)/usr/bin/
	$(CP)  ./files/*    $(1)/
endef
 
$(eval $(call BuildPackage,$(PKG_NAME)))
