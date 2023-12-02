obj-$(CONFIG_AMZN_DSP_FRAMEWORK) += src/cli.o
obj-$(CONFIG_AMZN_DSP_FRAMEWORK) += src/log.o
obj-$(CONFIG_AMZN_DSP_FRAMEWORK) += src/ringbuf.o
obj-$(CONFIG_AMZN_DSP_FRAMEWORK)+= src/state.o
obj-$(CONFIG_AMZN_DSP_FRAMEWORK)+= src/load.o
obj-$(CONFIG_AMZN_DSP_FRAMEWORK)+= src/adf_debug.o

ccflags-$(CONFIG_AMZN_DSP_FRAMEWORK) += -I$(srctree)/$(src)/include

ccflags-y += -Wno-int-to-void-pointer-cast
