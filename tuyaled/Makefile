include $(TOPDIR)/rules.mk

PKG_NAME:=tuyaled
PKG_VERSION:=0.1
PKG_BUILD_DIR:=$(BUILD_DIR)/$(PKG_NAME)


include $(INCLUDE_DIR)/package.mk

define Package/$(PKG_NAME)
  TITLE:=tuya led adapter
  SECTION:=iot
  CATEGORY:=Customer
  DEPENDS:=alljoyn +alljoyn-services_common \
	+alljoyn-about +alljoyn-config +alljoyn-controlpanel +alljoyn-notification \
	+libsqlite3 +libubus +libblobmsg-json +libubox +libpthread \
	+mxml
endef

define Package/$(PKG_NAME)/description
tuya led adapter for non-alljoyn device
endef


PKG_INSTALL_DIR:=$(PKG_BUILD_DIR)/$(PKG_NAME)-$(PKG_VERSION)

define Build/Prepare
	mkdir -p $(PKG_BUILD_DIR)
	$(CP) ./src/* $(PKG_BUILD_DIR)/ -r
endef

define Build/Compile
	$(MAKE) $(PKG_JOBS) -C $(PKG_BUILD_DIR)
endef

define Package/$(PKG_NAME)/install
	$(INSTALL_DIR) $(1)/usr/bin
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/tuyaled $(1)/usr/bin
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/ConfigService.conf $(1)/usr/bin/tuyaled.conf
	$(INSTALL_DIR) $(1)/usr/lib
endef

$(eval $(call BuildPackage,$(PKG_NAME)))
