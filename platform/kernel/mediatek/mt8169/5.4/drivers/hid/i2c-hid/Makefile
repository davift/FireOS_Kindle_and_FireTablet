# SPDX-License-Identifier: GPL-2.0-only
#
# Makefile for the I2C input drivers
#
ccflags-y += -I$(srctree)/drivers/hid/hid-focal/
ccflags-y += -I$(srctree)/drivers/hid/ILITEK_HID_over_I2c/
ccflags-y += -I$(srctree)/drivers/hid/i2c_hid_of_nvt_ts/
ccflags-y += -I$(srctree)/drivers/hid/stylus/
ccflags-$(CONFIG_DRM_MEDIATEK_V2) += -I$(srctree)/drivers/gpu/drm/mediatek/mediatek_v2/

obj-$(CONFIG_I2C_HID_CORE)			+= i2c-hid.o

i2c-hid-objs					=  i2c-hid-core.o
i2c-hid-$(CONFIG_DMI)				+= i2c-hid-dmi-quirks.o

obj-$(CONFIG_I2C_HID_ACPI)			+= i2c-hid-acpi.o
obj-$(CONFIG_I2C_HID_OF)			+= i2c-hid-of.o
