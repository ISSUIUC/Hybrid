// #include "panel_control.h"
// #include <Arduino.h>
// #include <driver/spi_common.h>
// #include <driver/spi_master.h>
// #include <SPI.h>
// // #include <soc/dport_access.h>
// // #include <soc/dport_reg.h>
// // #include <hal/spi_hal.h>
// // #include <hal/spi_ll.h>
// // #include <hal/dma_types.h>
// // #include <stdatomic.h>

// constexpr uint8_t rgbPins[] = {13, 47, 3, 35, 8, 6};
// constexpr uint8_t addrPins[] = {38, 46, 40, 18};
// constexpr uint8_t clockPin = 21;
// constexpr uint8_t latchPin = 9;
// constexpr uint8_t oePin = 41;

// // #define DEV_NUM_MAX 6

// // // struct spi_device_t {
// // //     int id;
// // //     QueueHandle_t trans_queue;
// // //     QueueHandle_t ret_queue;
// // //     spi_device_interface_config_t cfg;
// // //     spi_hal_dev_config_t hal_dev;
// // //     spi_host_t *host;
// // //     spi_bus_lock_dev_handle_t dev_lock;
// // // };

// // // void s_spi_prepare_data(spi_device_t *dev, const spi_hal_trans_config_t *hal_trans);


// // /// struct to hold private transaction data (like tx and rx buffer for DMA).
// // typedef struct {
// //     spi_transaction_t   *trans;
// //     const uint32_t *buffer_to_send;    //equals to tx_data, if SPI_TRANS_USE_RXDATA is applied; otherwise if original buffer wasn't in DMA-capable memory, this gets the address of a temporary buffer that is;
// //     //otherwise sets to the original buffer or NULL if no buffer is assigned.
// //     uint32_t *buffer_to_rcv;           //similar to buffer_to_send
// // #if SOC_SPI_SCT_SUPPORTED
// //     uint32_t reserved[2];              //As we create the queue when in init, to use sct mode private descriptor as a queue item (when in sct mode), we need to add a dummy member here to keep the same size with `spi_sct_trans_priv_t`.
// // #endif
// // } spi_trans_priv_t;


// // struct spi_bus_lock_t;
// // struct spi_bus_lock_dev_t;
// // /// Handle to the lock of an SPI bus
// // typedef struct spi_bus_lock_t* spi_bus_lock_handle_t;
// // /// Handle to lock of one of the device on an SPI bus
// // typedef struct spi_bus_lock_dev_t* spi_bus_lock_dev_handle_t;
// // /// Background operation control function
// // typedef void (*bg_ctrl_func_t)(void*);


// // struct spi_bus_lock_dev_t {
// //     SemaphoreHandle_t   semphr;     ///< Binary semaphore to notify the device it claimed the bus
// //     spi_bus_lock_t*     parent;     ///< Pointer to parent spi_bus_lock_t
// //     uint32_t            mask;       ///< Bitwise OR-ed mask of the REQ, PEND, LOCK bits of this device
// // };

// // struct spi_bus_lock_t {
// //     /**
// //      * The core of the lock. These bits are status of the lock, which should be always available.
// //      * No intermediate status is allowed. This is realized by atomic operations, mainly
// //      * `atomic_fetch_and`, `atomic_fetch_or`, which atomically read the status, and bitwise write
// //      * status value ORed / ANDed with given masks.
// //      *
// //      * The request bits together pending bits represent the actual bg request state of one device.
// //      * Either one of them being active indicates the device has pending bg requests.
// //      *
// //      * Whenever a bit is written to the status, it means the a device on a task is trying to
// //      * acquire the lock. But this will succeed only when no LOCK or BG bits active.
// //      *
// //      * The acquiring processor is responsible to call the scheduler to pass its role to other tasks
// //      * or the BG, unless it clear the last bit in the status register.
// //      */
// //     //// Critical resources, they are only writable by acquiring processor, and stable only when read by the acquiring processor.
// //     atomic_uint_fast32_t    status;
// //     spi_bus_lock_dev_t* volatile acquiring_dev;   ///< The acquiring device
// //     bool                volatile acq_dev_bg_active;    ///< BG is the acquiring processor serving the acquiring device, used for the wait_bg to skip waiting quickly.
// //     bool                volatile in_isr;         ///< ISR is touching HW
// //     //// End of critical resources

// //     atomic_intptr_t     dev[DEV_NUM_MAX];     ///< Child locks.
// //     bg_ctrl_func_t      bg_enable;      ///< Function to enable background operations.
// //     bg_ctrl_func_t      bg_disable;     ///< Function to disable background operations
// //     void*               bg_arg;            ///< Argument for `bg_enable` and `bg_disable` functions.

// //     spi_bus_lock_dev_t* last_dev;       ///< Last used device, to decide whether to refresh all registers.
// //     int                 periph_cs_num;  ///< Number of the CS pins the HW has.

// //     //debug information
// //     int                 host_id;        ///< Host ID, for debug information printing
// //     uint32_t            new_req;        ///< Last int_req when `spi_bus_lock_bg_start` is called. Debug use.
// // };

// // /// Attributes of an SPI bus
// // typedef struct {
// //     spi_bus_config_t bus_cfg;           ///< Config used to initialize the bus
// //     uint32_t flags;                     ///< Flags (attributes) of the bus
// //     int max_transfer_sz;                ///< Maximum length of bytes available to send
// //     bool dma_enabled;                   ///< To enable DMA or not
// //     size_t internal_mem_align_size;     ///< Buffer align byte requirement for internal memory
// //     spi_bus_lock_handle_t lock;
// // #ifdef CONFIG_PM_ENABLE
// //     esp_pm_lock_handle_t pm_lock;       ///< Power management lock
// // #endif
// // } spi_bus_attr_t;

// // typedef dma_descriptor_t dma_descriptor_align4_t;
// // typedef dma_descriptor_align4_t spi_dma_desc_t;
// // typedef struct gdma_channel_t *gdma_channel_handle_t;

// // typedef struct {
// // #if SOC_GDMA_SUPPORTED
// //     gdma_channel_handle_t tx_dma_chan;  ///< GDMA tx channel
// //     gdma_channel_handle_t rx_dma_chan;  ///< GDMA rx channel
// // #else
// //     spi_dma_chan_handle_t tx_dma_chan;  ///< TX DMA channel, on ESP32 and ESP32S2, tx_dma_chan and rx_dma_chan are same
// //     spi_dma_chan_handle_t rx_dma_chan;  ///< RX DMA channel, on ESP32 and ESP32S2, tx_dma_chan and rx_dma_chan are same
// // #endif
// //     int dma_desc_num;               ///< DMA descriptor number of dmadesc_tx or dmadesc_rx.
// //     spi_dma_desc_t *dmadesc_tx;     ///< DMA descriptor array for TX
// //     spi_dma_desc_t *dmadesc_rx;     ///< DMA descriptor array for RX
// // } spi_dma_ctx_t;

// // typedef struct {
// //     int id;
// //     spi_device_t* device[DEV_NUM_MAX];
// //     intr_handle_t intr;
// //     spi_hal_context_t hal;
// //     spi_trans_priv_t cur_trans_buf;
// // #if SOC_SPI_SCT_SUPPORTED
// //     spi_sct_desc_ctx_t sct_desc_pool;
// //     spi_sct_trans_priv_t cur_sct_trans;
// // #endif
// //     int cur_cs;     //current device doing transaction
// //     const spi_bus_attr_t* bus_attr;
// //     const spi_dma_ctx_t *dma_ctx;
// //     bool sct_mode_enabled;

// //     /**
// //      * the bus is permanently controlled by a device until `spi_bus_release_bus`` is called. Otherwise
// //      * the acquiring of SPI bus will be freed when `spi_device_polling_end` is called.
// //      */
// //     spi_device_t* device_acquiring_lock;
// //     portMUX_TYPE spinlock;

// // //debug information
// //     bool polling;   //in process of a polling, avoid of queue new transactions into ISR
// // } spi_host_t;

// // struct spi_device_t {
// //     int id;
// //     QueueHandle_t trans_queue;
// //     QueueHandle_t ret_queue;
// //     spi_device_interface_config_t cfg;
// //     spi_hal_dev_config_t hal_dev;
// //     spi_host_t *host;
// //     spi_bus_lock_dev_handle_t dev_lock;
// // };

// // void uninstall_priv_desc(spi_trans_priv_t* trans_buf)
// // {
// //     spi_transaction_t *trans_desc = trans_buf->trans;
// //     if ((void *)trans_buf->buffer_to_send != &trans_desc->tx_data[0] &&
// //             trans_buf->buffer_to_send != trans_desc->tx_buffer) {
// //         free((void *)trans_buf->buffer_to_send); //force free, ignore const
// //     }
// //     // copy data from temporary DMA-capable buffer back to IRAM buffer and free the temporary one.
// //     if (trans_buf->buffer_to_rcv && (void *)trans_buf->buffer_to_rcv != &trans_desc->rx_data[0] && trans_buf->buffer_to_rcv != trans_desc->rx_buffer) { // NOLINT(clang-analyzer-unix.Malloc)
// //         if (trans_desc->flags & SPI_TRANS_USE_RXDATA) {
// //             memcpy((uint8_t *) & trans_desc->rx_data[0], trans_buf->buffer_to_rcv, (trans_desc->rxlength + 7) / 8);
// //         } else {
// //             memcpy(trans_desc->rx_buffer, trans_buf->buffer_to_rcv, (trans_desc->rxlength + 7) / 8);
// //         }
// //         free(trans_buf->buffer_to_rcv);
// //     }
// // }

// // static esp_err_t setup_priv_desc(spi_host_t *host, spi_trans_priv_t* priv_desc)
// // {
// //     spi_transaction_t *trans_desc = priv_desc->trans;
// //     const spi_bus_attr_t *bus_attr = host->bus_attr;
// //     uint16_t alignment = bus_attr->internal_mem_align_size;

// //     // rx memory assign
// //     uint32_t* rcv_ptr;
// //     if (trans_desc->flags & SPI_TRANS_USE_RXDATA) {
// //         rcv_ptr = (uint32_t *)&trans_desc->rx_data[0];
// //     } else {
// //         //if not use RXDATA neither rx_buffer, buffer_to_rcv assigned to NULL
// //         rcv_ptr = (uint32_t*)trans_desc->rx_buffer;
// //     }

// //     // tx memory assign
// //     const uint32_t *send_ptr;
// //     if (trans_desc->flags & SPI_TRANS_USE_TXDATA) {
// //         send_ptr = (uint32_t *)&trans_desc->tx_data[0];
// //     } else {
// //         //if not use TXDATA neither tx_buffer, tx data assigned to NULL
// //         send_ptr = (uint32_t*)trans_desc->tx_buffer ;
// //     }

// //     uint32_t tx_byte_len = (trans_desc->length + 7) / 8;
// //     uint32_t rx_byte_len = (trans_desc->rxlength + 7) / 8;
// // #if SOC_CACHE_INTERNAL_MEM_VIA_L1CACHE
// //     bool tx_unaligned = ((((uint32_t)send_ptr) | tx_byte_len) & (alignment - 1));
// //     bool rx_unaligned = ((((uint32_t)rcv_ptr) | rx_byte_len) & (alignment - 1));
// // #else
// //     bool tx_unaligned = false;   //tx don't need align on addr or length, for other chips
// //     bool rx_unaligned = (((uint32_t)rcv_ptr) & (alignment - 1));
// // #endif

// //     priv_desc->buffer_to_send = send_ptr;
// //     priv_desc->buffer_to_rcv = rcv_ptr;
// //     return ESP_OK;

// // clean_up:
// //     uninstall_priv_desc(priv_desc);
// //     return ESP_ERR_NO_MEM;
// // }


// // static void spi_format_hal_trans_struct(spi_device_t *dev, spi_trans_priv_t *trans_buf, spi_hal_trans_config_t *hal_trans)
// // {
// //     spi_host_t *host = dev->host;
// //     spi_transaction_t *trans = trans_buf->trans;
// //     hal_trans->tx_bitlen = trans->length;
// //     hal_trans->rx_bitlen = trans->rxlength;
// //     hal_trans->rcv_buffer = (uint8_t*)host->cur_trans_buf.buffer_to_rcv;
// //     hal_trans->send_buffer = (uint8_t*)host->cur_trans_buf.buffer_to_send;
// //     hal_trans->cmd = trans->cmd;
// //     hal_trans->addr = trans->addr;

// //     if (trans->flags & SPI_TRANS_VARIABLE_CMD) {
// //         hal_trans->cmd_bits = ((spi_transaction_ext_t *)trans)->command_bits;
// //     } else {
// //         hal_trans->cmd_bits = dev->cfg.command_bits;
// //     }
// //     if (trans->flags & SPI_TRANS_VARIABLE_ADDR) {
// //         hal_trans->addr_bits = ((spi_transaction_ext_t *)trans)->address_bits;
// //     } else {
// //         hal_trans->addr_bits = dev->cfg.address_bits;
// //     }
// //     if (trans->flags & SPI_TRANS_VARIABLE_DUMMY) {
// //         hal_trans->dummy_bits = ((spi_transaction_ext_t *)trans)->dummy_bits;
// //     } else {
// //         hal_trans->dummy_bits = dev->cfg.dummy_bits;
// //     }

// //     hal_trans->cs_keep_active = (trans->flags & SPI_TRANS_CS_KEEP_ACTIVE) ? 1 : 0;
// //     //Set up OIO/QIO/DIO if needed
// //     hal_trans->line_mode.data_lines = (trans->flags & SPI_TRANS_MODE_DIO) ? 2 : (trans->flags & SPI_TRANS_MODE_QIO) ? 4 : 1;
// // #if SOC_SPI_SUPPORT_OCT
// //     if (trans->flags & SPI_TRANS_MODE_OCT) {
// //         hal_trans->line_mode.data_lines = 8;
// //     }
// // #endif
// //     hal_trans->line_mode.addr_lines = (trans->flags & SPI_TRANS_MULTILINE_ADDR) ? hal_trans->line_mode.data_lines : 1;
// //     hal_trans->line_mode.cmd_lines = (trans->flags & SPI_TRANS_MULTILINE_CMD) ? hal_trans->line_mode.data_lines : 1;
// // }

// // void IRAM_ATTR spicommon_dma_desc_setup_link(spi_dma_desc_t *dmadesc, const void *data, int len, bool is_rx);

// // static void s_spi_dma_prepare_data(spi_host_t *host, spi_hal_context_t *hal, const spi_hal_dev_config_t *dev, const spi_hal_trans_config_t *trans)
// // {
// //     const spi_dma_ctx_t *dma_ctx = host->dma_ctx;

// //     if (trans->rcv_buffer) {
// //         spicommon_dma_desc_setup_link(dma_ctx->dmadesc_rx, trans->rcv_buffer, ((trans->rx_bitlen + 7) / 8), true);

// //         spi_dma_reset(dma_ctx->rx_dma_chan);
// //         spi_hal_hw_prepare_rx(hal->hw);
// //         spi_dma_start(dma_ctx->rx_dma_chan, dma_ctx->dmadesc_rx);
// //     }
// // #if CONFIG_IDF_TARGET_ESP32
// //     else if (!dev->half_duplex) {
// //         //DMA temporary workaround: let RX DMA work somehow to avoid the issue in ESP32 v0/v1 silicon
// //         spi_ll_dma_rx_enable(hal->hw, 1);
// //         spi_dma_start(dma_ctx->rx_dma_chan, NULL);
// //     }
// // #endif
// //     if (trans->send_buffer) {
// //         spicommon_dma_desc_setup_link(dma_ctx->dmadesc_tx, trans->send_buffer, (trans->tx_bitlen + 7) / 8, false);

// //         spi_dma_reset(dma_ctx->tx_dma_chan);
// //         spi_hal_hw_prepare_tx(hal->hw);
// //         spi_dma_start(dma_ctx->tx_dma_chan, dma_ctx->dmadesc_tx);
// //     }
// // }

// // static void s_spi_prepare_data(spi_device_t *dev, const spi_hal_trans_config_t *hal_trans)
// // {
// //     spi_host_t *host = dev->host;
// //     spi_hal_dev_config_t *hal_dev = &(dev->hal_dev);
// //     spi_hal_context_t *hal = &(host->hal);

// //     if (host->bus_attr->dma_enabled) {
// //         s_spi_dma_prepare_data(host, hal, hal_dev, hal_trans);
// //     } else {
// //         //Need to copy data to registers manually
// //         spi_hal_push_tx_buffer(hal, hal_trans);
// //     }

// //     //in ESP32 these registers should be configured after the DMA is set
// //     spi_hal_enable_data_line(hal->hw, (!hal_dev->half_duplex && hal_trans->rcv_buffer) || hal_trans->send_buffer, !!hal_trans->rcv_buffer);
// // }


// // static void spi_new_trans(spi_device_t *dev, spi_trans_priv_t *trans_buf)
// // {
// //     spi_transaction_t *trans = trans_buf->trans;
// //     spi_hal_context_t *hal = &(dev->host->hal);
// //     spi_hal_dev_config_t *hal_dev = &(dev->hal_dev);

// //     dev->host->cur_cs = dev->id;

// //     //Reconfigure according to device settings, the function only has effect when the dev_id is changed.
// //     // spi_setup_device(dev);

// //     //set the transaction specific configuration each time before a transaction setup
// //     spi_hal_trans_config_t hal_trans = {};
// //     spi_format_hal_trans_struct(dev, trans_buf, &hal_trans);
// //     spi_hal_setup_trans(hal, hal_dev, &hal_trans);
// //     s_spi_prepare_data(dev, &hal_trans);

// //     //Call pre-transmission callback, if any
// //     if (dev->cfg.pre_cb) {
// //         dev->cfg.pre_cb(trans);
// //     }
// //     //Kick off transfer
// //     spi_hal_user_start(hal);
// // }

// // esp_err_t spi_device_polling_start2(spi_device_handle_t handle, spi_transaction_t *trans_desc, TickType_t ticks_to_wait)
// // {
// //     esp_err_t ret;
// //     // SPI_CHECK(ticks_to_wait == portMAX_DELAY, "currently timeout is not available for polling transactions", ESP_ERR_INVALID_ARG);
// //     // ret = check_trans_valid(handle, trans_desc);
// //     // if (ret != ESP_OK) {
// //         // return ret;
// //     // }
// //     // SPI_CHECK(!spi_bus_device_is_polling(handle), "Cannot send polling transaction while the previous polling transaction is not terminated.", ESP_ERR_INVALID_STATE);

// //     spi_host_t *host = handle->host;
// //     spi_trans_priv_t priv_polling_trans = { .trans = trans_desc, };
// //     ret = setup_priv_desc(host, &priv_polling_trans);
// //     if (ret != ESP_OK) {
// //         return ret;
// //     }

// //     /* If device_acquiring_lock is set to handle, it means that the user has already
// //      * acquired the bus thanks to the function `spi_device_acquire_bus()`.
// //      * In that case, we don't need to take the lock again. */
// //     // if (host->device_acquiring_lock != handle) {
// //     //     /* The user cannot ask for the CS to keep active has the bus is not locked/acquired. */
// //     //     if ((trans_desc->flags & SPI_TRANS_CS_KEEP_ACTIVE) != 0) {
// //     //         ret = ESP_ERR_INVALID_ARG;
// //     //     } else {
// //     //         ret = spi_bus_lock_acquire_start(handle->dev_lock, ticks_to_wait);
// //     //     }
// //     // } else {
// //     //     ret = spi_bus_lock_wait_bg_done(handle->dev_lock, ticks_to_wait);
// //     // }
// //     if (ret != ESP_OK) {
// //         uninstall_priv_desc(&priv_polling_trans);
// //         // ESP_LOGE(SPI_TAG, "polling can't get buslock");
// //         return ret;
// //     }
// //     //After holding the buslock, common resource can be accessed !!

// //     //Polling, no interrupt is used.
// //     host->polling = true;
// //     host->cur_trans_buf = priv_polling_trans;

// //     ESP_LOGV(SPI_TAG, "polling trans");
// //     spi_new_trans(handle, &host->cur_trans_buf);

// //     return ESP_OK;
// // }

// void test() {
//     // pinMode(latchPin, OUTPUT);
//     // pinMode(oePin, OUTPUT);
//     // pinMode(clockPin, OUTPUT);
//     pinMode(addrPins[0], OUTPUT);
//     pinMode(addrPins[1], OUTPUT);
//     pinMode(addrPins[2], OUTPUT);

//     spi_bus_config_t bus_config {
//         .data0_io_num = rgbPins[0],
//         .data1_io_num = rgbPins[1],
//         .sclk_io_num = clockPin,
//         .data2_io_num = rgbPins[2],
//         .data3_io_num = rgbPins[3],
//         .data4_io_num = rgbPins[4],
//         .data5_io_num = rgbPins[5],
//         .data6_io_num = oePin,
//         .data7_io_num = latchPin,
//         .flags = SPICOMMON_BUSFLAG_MASTER | SPICOMMON_BUSFLAG_OCTAL,
//         // .flags = SPICOMMON_BUSFLAG_MASTER | SPICOMMON_BUSFLAG_MOSI | SPICOMMON_BUSFLAG_SCLK
//     };
//     spi_device_interface_config_t dev_config {
//         .mode = 0,
//         .clock_speed_hz = 40000000,
//         .flags = SPI_DEVICE_HALFDUPLEX,
//         .queue_size = 8,
//     };
//     spi_device_handle_t spi_device{};

//     esp_err_t ret;
//     ret = spi_bus_initialize(SPI2_HOST, &bus_config, SPI_DMA_CH_AUTO);
//     ESP_ERROR_CHECK(ret);
//     ret = spi_bus_add_device(SPI2_HOST, &dev_config, &spi_device);
//     ESP_ERROR_CHECK(ret);

//     uint8_t* screen_data = (uint8_t*)heap_caps_aligned_calloc(16, 1, 40*8, MALLOC_CAP_DMA | MALLOC_CAP_INTERNAL);
//     for(int i = 0; i < 32*8; i++) {
//         // screen_data[i] = i & 0x3f;
//         if((i+i/32)&1) screen_data[i] = 0x3f;
//         // if((i%32)==31) screen_data[i] |= 0xb0;
//     }
    
//     spi_device_acquire_bus(spi_device, portMAX_DELAY);
//     while(true){
//         for(int row = 0; row < 8; row++){
//             spi_transaction_t transaction {
//                 .flags = SPI_TRANS_MODE_OCT,
//                 .length = 32*8,
//                 .tx_buffer = screen_data+row*32,
//             };
            
//             spi_device_polling_start(spi_device, &transaction, portMAX_DELAY);
//             spi_device_polling_end(spi_device, portMAX_DELAY);
//             uint32_t set{}, clear{};
//             for(int addr = 0; addr < 3; addr++){
//                 if(row & (1<<addr)){
//                     set |= 1<<(addrPins[addr]-32);
//                 } else {
//                     clear |= 1<<(addrPins[addr]-32);
//                 }
//             }
//             GPIO.out1_w1ts.val = set;
//             GPIO.out1_w1tc.val = clear;
//         }
//     }
// }
