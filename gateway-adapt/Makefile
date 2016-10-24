GW_MODULE:=adapt
GW_MODULE_NAME:=adapt

include $(TOPDIR)/rules.mk

PKG_NAME:=gateway-$(GW_MODULE)
PKG_BASE_VERSION:=0.1
PKG_PATCHLEVEL:=.00
PKG_RELEASE:=1
PKG_VERSION:=$(PKG_BASE_VERSION)$(PKG_PATCHLEVEL)
PKG_TARBALL_VERSION:=$(PKG_VERSION)
PKG_BUILD_DIR:=$(BUILD_DIR)/$(PKG_NAME)
PKG_BUILD_OBJ_DIR = $(PKG_BUILD_DIR)/obj


include $(INCLUDE_DIR)/package.mk

define Package/$(PKG_NAME)
  TITLE:=gateway adapt
  SECTION:=iot
  CATEGORY:=Customer
  DEPENDS:=alljoyn +alljoyn-services_common \
	+alljoyn-about +alljoyn-config +alljoyn-controlpanel +alljoyn-notification \
	+libsqlite3 +libubus +libblobmsg-json +libubox +libpthread \
	+mxml
endef

define Package/$(PKG_NAME)/description
gateway adapt for non-alljoyn device
endef



ifeq ($(CONFIG_DEBUG),y)
  GW_ADAPT_BUILD_VARIANT:=debug
else
  GW_ADAPT_BUILD_VARIANT:=release
endif


PKG_INSTALL_DIR:=$(PKG_BUILD_DIR)/$(GW_ADAPT_BUILD_VARIANT)/$(PKG_NAME)-$(PKG_VERSION)


define Build/Prepare
	mkdir -p $(PKG_BUILD_DIR) $(PKG_BUILD_OBJ_DIR)
	$(CP) ./src/* $(PKG_BUILD_DIR)/ -r
endef

define Build/Compile
	$(MAKE) $(PKG_JOBS) -C $(PKG_BUILD_DIR)
endef

define Package/$(PKG_NAME)/install
	$(INSTALL_DIR) $(1)/usr/bin
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/gateway-adapt $(1)/usr/bin
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/FactoryConfigService.conf $(1)/usr/bin/feixunFlight.conf
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/ConfigService.conf $(1)/usr/bin/FXled123456.conf
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/ConfigService.conf $(1)/usr/bin/TYled334455.conf
	$(INSTALL_DIR) $(1)/usr/lib
endef

$(eval $(call BuildPackage,$(PKG_NAME)))
