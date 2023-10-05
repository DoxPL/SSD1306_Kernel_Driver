#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/i2c.h>
#include <linux/types.h>
#include <linux/delay.h>
#include "types.h"
#include "font.h"


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

static uint8_t verify_input_for_scrl(OledPageAddr start_addr, OledPageAddr end_addr, OledFrameFreq freq) {
    uint8_t rc = OLED_NO_ERRORS;
    if ((start_addr >= OLED_PAGE0 && start_addr <= OLED_PAGE7) &&
        (end_addr >= OLED_PAGE0 && end_addr <= OLED_PAGE7) &&
        (end_addr >= start_addr) && (freq >= OLED_TIME_INT_5_FRAMES &&
        freq <= OLED_TIME_INT_2_FRAMES)) {
        rc = OLED_INPUT_ERROR;
    }
    return rc;
}

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

static void cmd_set_contrast_ctrl(uint8_t value) {
    uint16_t command = (0x81 << 8) | value;
    oled_i2c_write_datablock(command, sizeof(command));
}

static void cmd_reset_contrast(void) {
    oled_i2c_write_char(0x7F);
}

static uint8_t cmd_set_entire_display(OledEntireDisplay ed) {
    uint8_t rc = 1;
    char command[1];
    switch(ed) {
        case OLED_RESUME_TO_RAM:
        case OLED_ENTIRE_DISPLAY_ON:
            command[0] = ed;
            rc = oled_i2c_write_datablock(command, sizeof(command));
            break;
        default:
            break;
    }
    return rc;
}

static void cmd_set_display_mode(OledMode mode) {
    uint8_t rc = 1;
    char command[1];
    switch(mode) {
        case OLED_MODE_NORMAL:
        case OLED_MODE_INVERSE:
            command[0] = mode;
            rc = oled_i2c_write_datablock(command, sizeof(command));
            break;
        default:
            break;
    }
    return rc;
}

static uint8_t cmd_set_display(OledPower power) {
    uint8_t rc = 1;
    char command[1];
    switch(power) {
        case OLED_POWER_OFF:
        case OLED_POWER_ON:
            command[0] = power;
            rc = oled_i2c_write_datablock(command, sizeof(command));
            break;
        default:
            break;
    }
    return rc;
}

static uint8_t cmd_setup_hrz_scrl(OledScrollHrz scroll_hrz, OledPageAddr start_addr, OledFrameFreq freq, OledPageAddr end_addr) {
    uint8_t rc = OLED_NO_ERRORS;
    uint8_t input_error = OLED_NO_ERRORS;

    switch(scroll_hrz) {
        case OLED_SCROLL_HOTIZONTAL_RIGHT:
        case OLED_SCROLL_HOTIZONTAL_LEFT:
            input_error = verify_input_for_scrl(start_addr, end_addr, freq);
            break;
        default:
            input_error = OLED_INPUT_ERROR;
            break;
    }

    if (input_error == OLED_NO_ERRORS) {
        uint8_t command = { scroll_hrz, 0x0, start_addr, freq, end_addr, 0x0, 0xFF};
        rc = oled_i2c_write_datablock(command, sizeof(command));
    }

    return rc;
}

static uint8_t cmd_setup_vh_scrl(OledScrollVH scroll_vh, OledPageAddr start_addr, OledFrameFreq freq, OledPageAddr end_addr) {
    uint8_t rc = OLED_NO_ERRORS;
    uint8_t input_error = OLED_NO_ERRORS;

    switch(scroll_vh) {
        case OLED_SCROLL_VERT_HRZ_RIGHT:
        case OLED_SCROLL_VERT_HRZ_LEFT:
            input_error = verify_input_for_scrl(start_addr, end_addr, freq);
            break;
        default:
            input_error = OLED_INPUT_ERROR;
            break;
    }

    if (input_error == OLED_NO_ERRORS) {
        uint8_t command = { scroll_vh, 0x0, start_addr, freq, end_addr, 0x0, 0xFF};
        rc = oled_i2c_write_datablock(command, sizeof(command));
    }

    return rc;
}

static uint8_t cmd_switch_scroll(OledScrollState state) {
    uint8_t rc;
    uint8_t command[1];
    switch(state) {
        case OLED_DEACT_SCROLL:
        case OLED_ACT_SCROLL:
            command[0] = state;
            rc = oled_i2c_write_datablock(command, sizeof(command));
            break;
        default:
            break;
    }

    return rc;
}

static uint8_t cmd_set_vert_scrl_area(uint8_t rows_tfa, uint8_t rows_sa) {
    uint8_t rc;
    rows_tfa &= 0x1F; /* Rows in top fixed area */
    rows_sa &= 0x3F; /* Rows in scroll area */

    uint8_t command[3] = { 0xA3, rows_tfa, rows_sa};
    rc = oled_i2c_write_datablock(command, sizeof(command));
    return rc;
}

static uint8_t cmd_set_lcsa_pam(uint8_t lcsa) {
    uint8_t rc;
    uint8_t command[1];
    /* Set lower column start address for page addressing mode */
    command[0] = lcsa & 0xF;
    rc = oled_i2c_write_datablock(command, sizeof(command));
    return rc;
}

static uint8_t cmd_set_hcsa_pam(uint8_t hcsa) {
    uint8_t rc;
    uint8_t command[1] = { 0x10 | (hcsa & 0xF) };
    /* Set higher column start address for page addressing mode */
    rc = oled_i2c_write_datablock(command, sizeof(command));
    return rc;
}

static uint8_t cmd_set_mem_addr_mode(OledMemAddrMode mode) {
    uint8_t rc;
    uint8_t command[2] = { 0x20, mode & 0x3 };
    rc = oled_i2c_write_datablock(command, sizeof(command));
    return rc;
}

static uint8_t cmd_set_col_addr(uint8_t col_start_addr, uint8_t col_end_addr) {
    uint8_t rc;
    uint8_t command[3] = { 0x21, col_start_addr & 0x7F, col_end_addr & 0x7F }; 
    rc = oled_i2c_write_datablock(command, sizeof(command));
    return rc;
}

static uint8_t cmd_set_page_addr(OledPageAddr start_addr, OledPageAddr end_addr) {
    uint8_t rc;
    uint8_t command[3] = { 0x22, start_addr, end_addr };
    rc = oled_i2c_write_datablock(command, sizeof(command));
    return rc;
}

static uint8_t cmd_set_psa_pam(OledPageAddr start_addr) {
    /* Set page start address for page addressing mode */
    uint8_t rc;
    uint8_t command[1] = { 0xB0 | start_addr };
    rc = oled_i2c_write_datablock(command, sizeof(command));
    return rc;
}

static uint8_t cmd_set_display_sl(uint8_t display_sl) {
    /* Set display start line */
    uint8_t rc;
    uint8_t command[1] = { 0x40 | (display_sl & 0x3F) };
    rc = oled_i2c_write_datablock(command, sizeof(command));
    return rc;
}

static uint8_t cmd_set_seg_remap(uint8_t lsb) {
    uint8_t rc;
    uint8_t command[1] = { 0xA0 | (lsb & 0x1) };
    rc = oled_i2c_write_datablock(command, sizeof(command));
    return rc; 
}

static uint8_t cmd_set_mux_ratio(uint8_t mux_ratio) {
    uint8_t rc;
    if (mux_ratio < 0xF) {
        /* Invalid entry */
        return 1;
    }
    uint8_t command[2] = { 0xA8, mux_ratio & 0x1F };
    rc = oled_i2c_write_datablock(command, sizeof(command));
    return rc;
}

static uint8_t set_com_osd(OledOSDMode osd_mode) {
    /* Set COM output scan direction */
    uint8_t rc;
    uint8_t command[1] = { osd_mode };
    rc = oled_i2c_write_datablock(command, sizeof(command));
    return rc;
}

static uint8_t set_display_offset(uint8_t offset) {
    uint8_t rc;
    uint8_t command[2] = { 0xD3, offset & 0x3F };
    rc = oled_i2c_write_datablock(command, sizeof(command));
    return rc;
}

static uint8_t set_com_pins(uint8_t pin_conf, uint8_t lr_remap) {
    uint8_t rc;
    uint8_t pin_hw_conf = 0x2;
    pin_hw_conf |= (pin_conf & 0x1) << 4;
    pin_hw_conf |= (lr_remap & 0x1) << 5;
    uint8_t command[2] = { 0xDA, pin_hw_conf};
    rc = oled_i2c_write_datablock(command, sizeof(command));
    return rc;
}

static uint8_t set_display_clk(uint8_t divide_ratio, uint8_t osc_freq) {
    uint8_t rc;
    uint8_t clk_conf = divide_ratio & 0xF;
    clk_conf |= (osc_freq & 0xF) << 4;
    uint8_t command[2] = { 0xD5, clk_conf};
    rc = oled_i2c_write_datablock(command, sizeof(command));
    return rc;
}

static uint8_t set_pre_charge_per(uint8_t phase_1, uint8_t phase_2) {
    uint8_t rc;
    uint8_t pre_charge_per = phase_1 & 0xF;
    pre_charge_per |= (phase_2 & 0xF) << 4;
    uint8_t command[2] = { 0xD9, pre_charge_per};
    rc = oled_i2c_write_datablock(command, sizeof(command));
    return rc;
}

static uint8_t set_vcomh_deselect_lvl(uint8_t deselect_lvl) {
    uint8_t rc;
    uint8_t command[2] = { 0xDB, (deselect_lvl << 4) & 0x70 };
    rc = oled_i2c_write_datablock(command, sizeof(command));
    return rc;
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
    oled_i2c_clear_display(0);
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
