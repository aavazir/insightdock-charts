#
# Sample OpenWrt feed package for InsightDock Charts for C++.
#
# This file is a starting point for packaging InsightDock Charts into an OpenWrt feed —
# it is not built as part of this repository's own CI. Copy it into your feed
# (e.g. `package/libs/insightdock/Makefile`), then:
#   - point PKG_SOURCE_URL at a real tagged release once one exists, and
#     replace PKG_HASH:=skip with the tarball's real sha256sum
#   - adjust INSIGHTDOCK_CHART_TYPES/EXPORT_FORMATS/FONT_MODE to taste (they
#     map directly onto the upstream Makefile's own feature-trim variables —
#     see CLAUDE.md's "Make (core only — OpenWrt/minimal)" section)
#
# Builds via the project's plain Makefile (not CMake): that build is the one
# with CHART_TYPES/EXPORT_FORMATS/FONT_MODE feature trimming for size-
# constrained targets, which is what most router/embedded packaging wants.
#
include $(TOPDIR)/rules.mk

PKG_NAME:=insightdock
PKG_VERSION:=1.0.0
PKG_RELEASE:=1

PKG_SOURCE:=$(PKG_NAME)-$(PKG_VERSION).tar.gz
PKG_SOURCE_URL:=https://github.com/aavazir/insightdock-charts/archive/refs/tags/v$(PKG_VERSION).tar.gz
PKG_HASH:=skip
PKG_BUILD_DIR:=$(BUILD_DIR)/insightdock-charts-$(PKG_VERSION)

PKG_MAINTAINER:=InsightDock Charts contributors
PKG_LICENSE:=MIT
PKG_LICENSE_FILES:=LICENSE

include $(INCLUDE_DIR)/package.mk

define Package/insightdock
  SECTION:=libs
  CATEGORY:=Libraries
  TITLE:=Headless C++17 charting library (PNG/JPEG/SVG/PDF)
  URL:=https://github.com/aavazir/insightdock-charts
  DEPENDS:=+libstdcpp
endef

define Package/insightdock/description
  InsightDock Charts renders pie, bar, line, area, combo, and scatter/bubble charts
  for reports and embedded UIs, with zero mandatory link-time dependencies.
  This package builds the project's size-trimmed core (static library) via
  its plain Makefile, intended for routers and other constrained targets.
endef

# Feature-trim knobs — mirror the upstream Makefile's CHART_TYPES/
# EXPORT_FORMATS/FONT_MODE variables. Defaults below match the README's own
# "Minimal OpenWrt build" example (pie only, PNG only, runtime-loaded font).
INSIGHTDOCK_CHART_TYPES?=pie
INSIGHTDOCK_EXPORT_FORMATS?=png
INSIGHTDOCK_FONT_MODE?=system

define Build/Compile
	+$(MAKE) -C $(PKG_BUILD_DIR) \
		CXX="$(TARGET_CXX)" \
		AR="$(TARGET_AR)" \
		CXXFLAGS="$(TARGET_CXXFLAGS) -std=c++17 -Os -ffunction-sections -fdata-sections" \
		CHART_TYPES="$(INSIGHTDOCK_CHART_TYPES)" \
		EXPORT_FORMATS="$(INSIGHTDOCK_EXPORT_FORMATS)" \
		FONT_MODE="$(INSIGHTDOCK_FONT_MODE)" \
		lib
endef

define Package/insightdock/install
	$(INSTALL_DIR) $(1)/usr/lib
	$(INSTALL_DATA) $(PKG_BUILD_DIR)/libinsightdock.a $(1)/usr/lib/
endef

define Package/insightdock-dev
$(Package/insightdock)
  TITLE:=Headless C++17 charting library (development headers)
  DEPENDS:=insightdock
endef

define Package/insightdock-dev/description
  Public headers for building against InsightDock Charts' static library
  (include/insightdock/, umbrella header insightdock/charts.h).
endef

define Package/insightdock-dev/install
	$(INSTALL_DIR) $(1)/usr/include
	$(CP) $(PKG_BUILD_DIR)/include/insightdock $(1)/usr/include/
endef

$(eval $(call BuildPackage,insightdock))
$(eval $(call BuildPackage,insightdock-dev))
