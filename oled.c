#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/i2c.h>
#include <linux/types.h>
#include <linux/delay.h>
#include "font.h"

/* Constants */
#define DEV_NAME "oled"
#define OLED_COLS 128
#define OLED_ROWS 64
#define I2C_BUS   0x01
#define OLED_ADDR 0x3C
#define KERNEL_OUT(...) { printk(DEV_NAME": ", __VA_ARGS__); }

/* OLED 1306 COMMANDS */

/* Command macros */
#define CMD_SET_LCA_PAM(val) (val & 0x0F)
#define CMD_SET_HCA_PAM(val) (val & 0x0F) | 0x10
#define CMD_SET_STA_LNE(val) (val & 0xFF)
#define CMD_SET_PSA_PAM(val) (val & 0xB0)
#define CMD_SET_SEG_RMA(val) (val | 0xA0)
#define CMD_SET_COM_OSD(val) (val & 0x8) | 0xC0
/* End of macros*/
#define CMD_SET_ADD_MOD 0x20
#define CMD_SET_COL_ADD 0x21
#define CMD_SET_PAG_ADD 0x22
#define CMD_SET_CCT_BA0 0x81
#define CMD_COM_EDP_ONN 0xA4
#define CMD_COM_EDP_ONF 0xA5
#define CMD_SET_NRM_DPL 0xA6
#define CMD_SET_INV_DPL 0xA7
#define CMD_SET_MPX_RTO 0xA8
#define CMD_SET_DPL_DOF 0xAE
#define CMD_SET_DPL_DON 0xAF
#define CMD_SET_DPL_OFT 0xD3
#define CMD_SET_DPL_CDR 0xD5
#define CMD_SET_PRC_PER 0xD9
#define CMD_SET_COM_PHS 0xDA
#define CMD_VCO_DSL_LVL 0xDB
#define CMD_COM_OTH_NOP 0xE3
#define CMD_GAC_HRZ_SCS 0x26
#define CMD_GAC_CVE_SCS 0x29
#define CMD_GAC_DEA_SCR 0x2E
#define CMD_GAC_ACT_SCR 0x2F
#define CMD_GAC_SET_VSA 0xA3
#define CMD_CHN_PMP_SET 0x8D

static ssize_t oled_dev_write(struct file *file, const char *buff, size_t len, loff_t *offset);
static int32_t oled_drv_probe(struct i2c_client * client, const struct i2c_device_id * dev_id);
static int32_t oled_drv_remove(struct i2c_client * client);
static uint32_t oled_i2c_display(uint8_t);
static uint32_t oled_i2c_set_cursor(uint8_t, uint8_t);
static uint32_t oled_i2c_write_char(uint8_t);
static uint32_t oled_i2c_write_datablock(const char *, ssize_t);
static void oled_i2c_clear_display(uint8_t);

static char TEXT_BUFFER[1024] = { 0 };

static const uint8_t INIT_SEQ[] = {
    CMD_SET_MPX_RTO, 0x3F,
    CMD_SET_DPL_OFT, 0x00,
    CMD_SET_STA_LNE(0x40),
    CMD_SET_SEG_RMA(1),
    CMD_SET_COM_OSD(8),
    CMD_SET_COM_PHS, 0x12,
    CMD_SET_CCT_BA0, 0xFF,
    CMD_COM_EDP_ONN,
    CMD_SET_NRM_DPL,
    CMD_SET_DPL_CDR, 0x80,
    CMD_CHN_PMP_SET, 0x14,
    CMD_SET_DPL_DON,
    // CMD_SET_LCA_PAM(0),
    // CMD_SET_DPL_DOF,
    // CMD_SET_DPL_CDR, 0x80,
    // CMD_SET_MPX_RTO, 0x3F,
    // CMD_SET_DPL_OFT, 0x00,
    // CMD_SET_STA_LNE(0x40),
    // CMD_CHN_PMP_SET, 0x14,
    // CMD_SET_ADD_MOD, 0x00,
    // CMD_SET_SEG_RMA(1),
    // CMD_SET_COM_OSD(8),
    // CMD_SET_COM_PHS, 0x12,
    // CMD_SET_CCT_BA0, 0x80,
    // CMD_SET_PRC_PER, 0xF1,
    // CMD_VCO_DSL_LVL, 0x20,
    // CMD_COM_EDP_ONN,
    // CMD_SET_NRM_DPL,
    // CMD_GAC_DEA_SCR,
    // CMD_SET_DPL_DON,
};

static const uint8_t ENDL[] = { 0x22, 0x00, 0x10 };

static bool is_busy = FALSE;
static uint8_t lines_written = 1U;
static uint8_t chars_per_line = 0U;
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
    static int8_t index;
    static int8_t ascii_char;
    if (buff != NULL) {
        memcpy(TEXT_BUFFER, buff, len);
    }
    for (index = 0; index < len; index++) {
        ascii_char = TEXT_BUFFER[index];
        oled_i2c_write_char(ascii_char);

        if (ascii_char == 'x') {
            //oled_i2c_set_cursor(5, 0);
        }

        //     oled_i2c_write_datablock(ENDL, sizeof(ENDL));
        //     oled_i2c_clear_display(21 - chars_per_line);
        //     chars_per_line = 0U;
        // }

        // if (chars_per_line++ >= 21) {
        //     chars_per_line = 0U;
        //     lines_written++;
        // }

        // if (lines_written >= 8) {
        //     lines_written = 0U;
        //     oled_i2c_clear_display(0);
        // }
    }

    return len;
}

static int32_t oled_drv_probe(struct i2c_client * client, const struct i2c_device_id * dev_id) {
    return 0;
}

static int32_t oled_drv_remove(struct i2c_client * client) {
    return 0;
}

static uint32_t oled_i2c_display(uint8_t value) {
    char msg[2] = { 0x40, value };
    uint32_t rc = i2c_master_send(client, msg, 2);
    if (rc < 0) {
        KERNEL_OUT("Failed to send command");
    }
    return rc;
}

static uint32_t oled_i2c_set_cursor(uint8_t x, uint8_t y) {
    uint8_t command[2] = { 0x22, x };
    oled_i2c_write_datablock(command, sizeof(command));
    command[0] = 0x21;
    command[1] = y;
    oled_i2c_write_datablock(command, sizeof(command));
    return NULL;
}

// static uint32_t oled_i2c_display_block(const char *buff, ssize_t len) {
//     uint32_t rc = i2c_smbus_write_block_data(client, 0x40, 6 * len);
//     if (rc < 0) {
//         KERNEL_OUT("Failed to send command");
//     }
//     return rc;
// }

static uint32_t oled_i2c_write_char(uint8_t value) {
    uint32_t rc = i2c_smbus_write_block_data(client, 0x40, 6,
        oled_font_8x6 + (value * CHAR_WIDTH));
    if (rc < 0) {
        KERNEL_OUT("Failed to send command");
    }
    return rc;
}

static uint32_t oled_i2c_clear_screen(void) {
    uint32_t rc = i2c_smbus_write_block_data(client, 0x40, 6,
        oled_font_8x6);
    if (rc < 0) {
        KERNEL_OUT("Failed to send command");
    }
    return rc;
}

static uint32_t oled_i2c_write_datablock(const char *buff, ssize_t len) {
    uint32_t rc = i2c_smbus_write_block_data(client, 0x00, len, buff);
    if (rc < 0) {
        KERNEL_OUT("Failed to send datablock")
    }
    return rc;
}

static void oled_i2c_clear_display(uint8_t segments) {
    uint16_t i;
    uint16_t seg_max = segments ? segments : 8192;
    for (i = 0; i < seg_max; i++) {
        oled_i2c_display(0x00);
    }
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
    
    oled_i2c_write_datablock(INIT_SEQ, sizeof(INIT_SEQ));
    (void) is_busy;
    return 0;
}

static void __exit oled_exit(void) {
    //oled_i2c_clear_display(0);
    //oled_i2c_clear_screen();
    oled_i2c_display(0xE4);
    unregister_chrdev(major, DEV_NAME);
    i2c_unregister_device(client);
    i2c_del_driver(&oled_drv);
    KERNEL_OUT("Driver removed");
}

module_init(oled_init);
module_exit(oled_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Dominik G.");
MODULE_DESCRIPTION("OLED driver");
MODULE_VERSION("1.0");
