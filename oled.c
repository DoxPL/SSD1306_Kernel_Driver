#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/i2c.h>
#include <linux/types.h>
#include <linux/delay.h>
#include "font.h"

#define DEV_NAME "oled"
#define OLED_COLS 128
#define OLED_ROWS 64
#define I2C_BUS   0x01
#define OLED_ADDR 0x3C
#define KERNEL_OUT(msg) { printk("%s: %s\n", DEV_NAME, msg); }

static ssize_t oled_dev_write(struct file *, const char *, size_t, loff_t *);
static int32_t oled_drv_probe(struct i2c_client *, const struct i2c_device_id *);
static int32_t oled_drv_remove(struct i2c_client *);
static uint32_t oled_i2c_display(uint8_t);
static uint32_t oled_i2c_write_char(uint8_t); 

static const uint8_t INIT_FRAME[] = {
    0x00, 0xAE, 0xD5, 0x80, 0xA8, 0x3F, 0xD3, 0x00, 0x40,
    0x8D, 0x14, 0x20, 0x00, 0xA1, 0xC8, 0xDA, 0x12, 0x81, 
    0x80, 0xD9, 0xF1, 0xDB, 0x20, 0xA4, 0xA6, 0x2E, 0xAF
};

static char text_buffer[16];
static uint8_t is_busy = 0;
static int32_t major = 0;
static struct i2c_adapter * adapter;
static struct i2c_board_info board_info = {
    I2C_BOARD_INFO(DEV_NAME, OLED_ADDR)
};
static struct i2c_client * client;
static const struct i2c_device_id dev_id[] = {
    { DEV_NAME, 0 },
    {}
};
MODULE_DEVICE_TABLE(i2c, dev_id);

static struct i2c_driver oled_drv = {
    .driver = {
        .name = DEV_NAME,
        .owner = THIS_MODULE
    },
    .probe = oled_drv_probe,
    .remove = oled_drv_remove,
    .id_table = dev_id
};
static struct file_operations fops = {
    .write = oled_dev_write
};

static ssize_t oled_dev_write(struct file *file, const char *buff, size_t len, loff_t *offset) {
    static uint16_t i = 0;
    // ssize_t bytes_count = strlen(buff);
    // is_busy = 1;
    // for (i=0; i < bytes_count; i++) {
    //     //memcpy(text_buffer, buff, sizeof(text_buffer));
    //     printk("buff to write (%s)\n", buff);
    //     oled_i2c_write_char(0);
    // }

    oled_i2c_write_char(i);
    i += CHAR_WIDTH;
    return 1;
}

static int32_t oled_drv_probe(struct i2c_client * client, const struct i2c_device_id * dev_id) {
    return 0;
}

static int32_t oled_drv_remove(struct i2c_client * client) {
    return 0;
}

static uint32_t i2c_send(uint8_t value) {
    uint8_t msg[2];
    msg[0] = 0x00;
    msg[1] = value;
    uint32_t rc = i2c_master_send(client, msg, 2);
    if (rc < 0) {
        KERNEL_OUT("Failed to init the display");
    }
    return rc;
}

static uint32_t oled_i2c_display(uint8_t value) {
    char msg[2] = { 0x40, value };
    uint32_t rc = i2c_master_send(client, msg, 2);
    //uint32_t rc = i2c_smbus_write_byte_data(client, 0x40, value);
    if (rc < 0) {
        KERNEL_OUT("Failed to send command");
    }
    return rc;
}

static uint32_t oled_i2c_write_char(uint8_t value) {
    uint32_t rc = i2c_smbus_write_block_data(client, 0x40, 6, oled_font_8x7 + value);
    if (rc < 0) {
        KERNEL_OUT("Failed to send command");
    }
    return rc;
}

static int32_t __init oled_init(void) {
    major = register_chrdev(0, DEV_NAME, &fops);
    if (major < 0) {
        KERNEL_OUT("Error, char device cannot be registered");
        return -ECANCELED;
    }
    KERNEL_OUT("Driver initialized");
    adapter = i2c_get_adapter(I2C_BUS);
    if (adapter == NULL) {
        KERNEL_OUT("Cannot get i2c adapter");
        return -ECANCELED;
    }
    client = i2c_new_device(adapter, &board_info);
    if (client == NULL) {
        KERNEL_OUT("The i2c device cannot be instantiated");
        return -ECANCELED;
    }
    i2c_add_driver(&oled_drv);
    
    // if (oled_i2c_init()) {
    //     KERNEL_OUT("oled init err");
    //     return -ECANCELED;
    // }
    
    i2c_send(0xAE);
    i2c_send(0xD5);
    i2c_send(0x80);
    i2c_send(0xA8);
    i2c_send(0x3F);
    i2c_send(0xD3);
    i2c_send(0x00);
    i2c_send(0x40);
    i2c_send(0x8D);
    i2c_send(0x14);
    i2c_send(0x20);
    i2c_send(0x00);
    i2c_send(0xA1);
    i2c_send(0xC8);
    i2c_send(0xDA);
    i2c_send(0x12);
    i2c_send(0x81);
    i2c_send(0x80);
    i2c_send(0xD9);
    i2c_send(0xF1);
    i2c_send(0xDB);
    i2c_send(0x20);
    i2c_send(0xA4);
    i2c_send(0xA6);
    i2c_send(0x2E);
    i2c_send(0xAF);

    return 0;
}

static void __exit oled_exit(void) {
    uint32_t i;
    for (i = 0; i < 1024; i++) {
        oled_i2c_display(0x00);
    }

    unregister_chrdev(major, DEV_NAME);
    i2c_unregister_device(client);
    i2c_del_driver(&oled_drv);
    KERNEL_OUT("Driver removed");
}

module_init(oled_init);
module_exit(oled_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Dominik");
MODULE_DESCRIPTION("OLED driver");
MODULE_VERSION("1.0");
