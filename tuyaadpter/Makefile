#
# Copyright (C) 2011 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

include $(TOPDIR)/rules.mk

PKG_NAME:=tuyaled_a
PKG_RELEASE:=1

PKG_BUILD_DIR := $(BUILD_DIR)/$(PKG_NAME)

include $(INCLUDE_DIR)/package.mk

define Package/tuyaled_a
  SECTION:=iot
  CATEGORY:=Customer
  TITLE:=tuya led adpter
endef

define Package/tuyaled_a/description
 This package contains a MAC address manipulation utility.
endef

define Build/Prepare
	mkdir -p $(PKG_BUILD_DIR)
	$(CP) ./src/* $(PKG_BUILD_DIR)/
endef

define Build/Configure
endef

define Build/Compile
	$(MAKE) -C $(PKG_BUILD_DIR) \
		CC="$(TARGET_CC)" \
		CFLAGS="$(TARGET_CFLAGS) -Wall" \
		LDFLAGS="$(TARGET_LDFLAGS)"
endef

define Package/tuyaled_a/install
	$(INSTALL_DIR) $(1)/usr/sbin
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/tuyaled_a $(1)/usr/sbin/
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/tuyaled_cmd $(1)/usr/sbin/
endef

$(eval $(call BuildPackage,tuyaled_a))
