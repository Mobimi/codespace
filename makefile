THEOS_PACKAGE_SCHEME = rootless
TARGET := iphone:clang:latest:15.0

include $(THEOS)/makefiles/common.mk

TWEAK_NAME = MyTweak
MyTweak_FILES = Tweak.xm
MyTweak_FRAMEWORKS = UIKit QuartzCore Metal
MyTweak_CFLAGS = -fobjc-arc -Wno-deprecated-declarations -Wno-unused-variable -Wno-incomplete-implementation

include $(THEOS_MAKE_PATH)/tweak.mk
