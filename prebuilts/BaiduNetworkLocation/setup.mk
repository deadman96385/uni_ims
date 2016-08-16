BAIDU_NETWORK_LOCATION_PATH := $(strip \
    $(wildcard vendor/sprd/proprietories-source/prebuilts/BaiduNetworkLocation) \
    $(wildcard vendor/sprd/proprietories/prebuilts/BaiduNetworkLocation) \
)

PRODUCT_PACKAGE_OVERLAYS += $(BAIDU_NETWORK_LOCATION_PATH)/overlay

PRODUCT_PACKAGES += BaiduNetworkLocation
