include $(TOPDIR)/rules.mk

PKG_NAME:=fxc
PKG_VERSION:=1.0
PKG_RELEASE:=0

include $(INCLUDE_DIR)/package.mk
 
define Package/$(PKG_NAME)
    SECTION:=iot
    CATEGORY:=Customer
    DEPENDS:=+libubox
    TITLE:=device client to iot server
endef

define Package/$(PKG_NAME)/description
   this is a cloud client to manage the ap
endef

define Build/Prepare
	mkdir -p $(PKG_BUILD_DIR)
	$(CP) ./src/* $(PKG_BUILD_DIR)/
endef

define Package/$(PKG_NAME)/install
	$(INSTALL_DIR) $(1)/usr/bin
	$(INSTALL_BIN)  $(PKG_BUILD_DIR)/fxc  $(1)/usr/bin/
endef
 
$(eval $(call BuildPackage,$(PKG_NAME)))
