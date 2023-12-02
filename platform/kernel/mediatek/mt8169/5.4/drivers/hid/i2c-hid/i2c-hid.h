/* SPDX-License-Identifier: GPL-2.0+ */

#ifndef I2C_HID_H
#define I2C_HID_H

#include <linux/i2c.h>
#include <linux/hid.h>
#include <linux/regulator/consumer.h>

#ifdef CONFIG_DMI
struct i2c_hid_desc *i2c_hid_get_dmi_i2c_hid_desc_override(uint8_t *i2c_name);
char *i2c_hid_get_dmi_hid_report_desc_override(uint8_t *i2c_name,
					       unsigned int *size);
#else
static inline struct i2c_hid_desc
		   *i2c_hid_get_dmi_i2c_hid_desc_override(uint8_t *i2c_name)
{ return NULL; }
static inline char *i2c_hid_get_dmi_hid_report_desc_override(uint8_t *i2c_name,
							     unsigned int *size)
{ return NULL; }
#endif

#define I2C_HID_CMD(opcode_) \
	.opcode = opcode_, .length = 4, \
	.registerIndex = offsetof(struct i2c_hid_desc, wCommandRegister)

struct i2c_hid_cmd {
	unsigned int registerIndex;
	__u8 opcode;
	unsigned int length;
	bool wait;
};

union command {
	u8 data[0];
	struct cmd {
		__le16 reg;
		__u8 reportTypeID;
		__u8 opcode;
	} __packed c;
};

/**
 * struct i2chid_ops - Ops provided to the core.
 *
 * @power_up: do sequencing to power up the device.
 * @power_down: do sequencing to power down the device.
 * @shutdown_tail: called at the end of shutdown.
 */
struct i2chid_ops {
	int (*power_up)(struct i2chid_ops *ops);
	void (*power_down)(struct i2chid_ops *ops);
	void (*shutdown_tail)(struct i2chid_ops *ops);
	int (*irq_callback)(struct hid_device *hid, int type, const u8 *data, u32 size);
	__u8 *(*report_desc_override)(struct hid_device *hdev, unsigned int *size);
};

struct i2c_hid_desc {
	__le16 wHIDDescLength;
	__le16 bcdVersion;
	__le16 wReportDescLength;
	__le16 wReportDescRegister;
	__le16 wInputRegister;
	__le16 wMaxInputLength;
	__le16 wOutputRegister;
	__le16 wMaxOutputLength;
	__le16 wCommandRegister;
	__le16 wDataRegister;
	__le16 wVendorID;
	__le16 wProductID;
	__le16 wVersionID;
	__le32 reserved;
} __packed;

/* The main device structure */
struct i2c_hid {
	struct i2c_client	*client;	/* i2c client */
	struct hid_device	*hid;	/* pointer to corresponding HID dev */
	union {
		__u8 hdesc_buffer[sizeof(struct i2c_hid_desc)];
		struct i2c_hid_desc hdesc;	/* the HID Descriptor */
	};
	__le16			wHIDDescRegister; /* location of the i2c
						   * register of the HID
						   * descriptor. */
	unsigned int		bufsize;	/* i2c buffer size */
	u8			*inbuf;		/* Input buffer */
	u8			*rawbuf;	/* Raw Input buffer */
	u8			*cmdbuf;	/* Command buffer */
	u8			*argsbuf;	/* Command arguments buffer */

	unsigned long		flags;		/* device flags */
	unsigned long		quirks;		/* Various quirks */

	wait_queue_head_t	wait;		/* For waiting the interrupt */

	bool			irq_wake_enabled;
	struct mutex		reset_lock;

	struct i2chid_ops	*ops;
};

struct i2c_hid_of {
	struct i2chid_ops ops;

	struct i2c_client *client;
	struct regulator_bulk_data supplies[2];
	int post_power_delay_ms;
};

int i2c_hid_core_probe(struct i2c_client *client, struct i2chid_ops *ops,
		       u16 hid_descriptor_address);
int i2c_hid_core_remove(struct i2c_client *client);

void i2c_hid_core_shutdown(struct i2c_client *client);

extern const struct dev_pm_ops i2c_hid_core_pm;

int i2c_hid_command(struct i2c_client *client, const struct i2c_hid_cmd *command,
		    unsigned char *buf_recv, int data_len);

#endif
