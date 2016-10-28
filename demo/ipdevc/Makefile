include $(TOPDIR)/rules.mk

PKG_NAME:=ipdevc
PKG_RELEASE:=1

include $(INCLUDE_DIR)/package.mk

define Package/$(PKG_NAME)
	SUBMENU:=demo
	SECTION:=iot
	CATEGORY:=Customer
	DEPENDS:=libcurl
	TITLE:=ip dev client sample for iot
endef

define Package/$(PKG_NAME)/description
	This is a app for ip dev client sample for iot
endef

define Build/Prepare
	mkdir -p $(PKG_BUILD_DIR)
	$(CP) ./src/* $(PKG_BUILD_DIR)/
endef

define Build/Compile
	$(call Build/Compile/Default)
endef

define Package/$(PKG_NAME)/install
	$(INSTALL_DIR) $(1)/usr/bin
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/ipdevc $(1)/usr/bin/
endef

$(eval $(call BuildPackage,$(PKG_NAME)))
