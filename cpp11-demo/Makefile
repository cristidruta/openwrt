include $(TOPDIR)/rules.mk

PKG_NAME:=cpp11-demo
PKG_RELEASE:=1
PKG_BUILD_DIR:=$(BUILD_DIR)/$(PKG_NAME)

include $(INCLUDE_DIR)/package.mk

define Package/cpp11-demo
	SUBMENU:=test
	SECTION:=test
	CATEGORY:=Customer
	TITLE:=$(PKG_NAME)
	DEPENDS:=+libstdcpp
endef

define Build/Prepare
	$(MKDIR) -p $(PKG_BUILD_DIR)
	$(CP) ./src/* $(PKG_BUILD_DIR)
endef

define Package/cpp11-demo/install
	$(INSTALL_DIR) $(1)/usr/bin
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/$(PKG_NAME) $(1)/usr/bin
endef

$(eval $(call BuildPackage,cpp11-demo))
