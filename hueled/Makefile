include $(TOPDIR)/rules.mk

PKG_NAME:=hueled
PKG_VERSION:=0.1
PKG_BUILD_DIR:=$(BUILD_DIR)/$(PKG_NAME)


include $(INCLUDE_DIR)/package.mk

define Package/$(PKG_NAME)
  TITLE:=hue led adapter
  SECTION:=iot
  CATEGORY:=Customer
  DEPENDS:=alljoyn +alljoyn-services_common \
	+alljoyn-about +alljoyn-config +alljoyn-controlpanel +alljoyn-notification \
	+libsqlite3 +libubus +libblobmsg-json +libubox +libpthread \
	+mxml
endef

define Package/$(PKG_NAME)/description
hue led adapter for non-alljoyn device
endef


PKG_INSTALL_DIR:=$(PKG_BUILD_DIR)/$(PKG_NAME)-$(PKG_VERSION)

define Build/Prepare
	mkdir -p $(PKG_BUILD_DIR)
	$(CP) ./src/* $(PKG_BUILD_DIR)/ -r
endef

define Package/hueled/postinst
#!/bin/sh
/etc/init.d/hueled enable
/usr/bin/hueled &
endef

define Package/hueled/postrm
#!/bin/sh
killall -kill hueled
endef

define Build/Compile
	$(MAKE) $(PKG_JOBS) -C $(PKG_BUILD_DIR)
endef

define Package/$(PKG_NAME)/install
	$(INSTALL_DIR) $(1)/usr/bin
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/hueled $(1)/usr/bin
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/ConfigService.conf $(1)/usr/bin/Philipsled.conf
	$(INSTALL_DIR) $(1)/usr/lib
	$(CP) ./files/* $(1)/
endef

$(eval $(call BuildPackage,$(PKG_NAME)))
