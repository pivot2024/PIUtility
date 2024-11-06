
#include "main.h"

#include "app_button.h"
#include "app_error.h"
#include "app_scheduler.h"
#include "app_timer_appsh.h"
#include "ble.h"
#include "ble_advdata.h"
#include "ble_advertising.h"
#include "ble_bas.h"
#include "ble_conn_params.h"
#include "ble_conn_state.h"
#include "ble_dis.h"
#include "ble_hci.h"
#include "ble_hids.h"
#include "ble_srv_common.h"
#include "bsp.h"
#include "bsp_btn_ble.h"
#include "fds.h"
#include "fstorage.h"
#include "nordic_common.h"
#include "nrf.h"
#include "nrf_assert.h"
#include "nrf_gpio.h"
#include "peer_manager.h"
#include "sensorsim.h"
#include "softdevice_handler_appsh.h"
#include <stdint.h>
#include <string.h>

#define NRF_LOG_MODULE_NAME "APP"
#include "nrf_log.h"
#include "nrf_log_ctrl.h"

#include "HIDReportData.h"
#include "app_uart.h"
#include "nrf_delay.h"
#include "nrf_nvic.h" //
#include "power_manage.h"


#define IS_SRVC_CHANGED_CHARACT_PRESENT                                        \
  1 /**< Include or not the service_changed characteristic. if not enabled,    \
       the server's database cannot be changed for the lifetime of the         \
       device*/

#define CENTRAL_LINK_COUNT                                                     \
  0 /**< Number of central links used by the application. When changing this   \
       number remember to adjust the RAM settings*/
#define PERIPHERAL_LINK_COUNT                                                  \
  1 /**< Number of peripheral links used by the application. When changing     \
       this number remember to adjust the RAM settings*/

#define DEAD_BEEF                                                              \
  0xDEADBEEF /**< Value used as error code on stack dump, can be used to       \
                identify stack location on stack unwind. */
#include "ble_bas_server.h"
#include "ble_hid_server.h"
#include "ble_server.h"
#include "eeprom_Driver.h"
#include "uartfly.h"


#include "app_util_platform.h"
#include "output_source_selection_mgr.h"
#include "radio.h"


#include "eeprom.h"

/**@brief Callback function for asserts in the SoftDevice.
 *
 * @details This function will be called in case of an assert in the SoftDevice.
 *
 * @warning This handler is an example only and does not fit a final product.
 * You need to analyze how your product is supposed to react in case of Assert.
 * @warning On assert from the SoftDevice, the system can only recover on reset.
 *
 * @param[in] line_num   Line number of the failing ASSERT call.
 * @param[in] file_name  File name of the failing ASSERT call.
 */
void assert_nrf_callback(uint16_t line_num, const uint8_t *p_file_name) {
  app_error_handler(DEAD_BEEF, line_num, p_file_name);
}

/**@brief Function for the Timer initialization.
 *
 * @details Initializes the timer module. This creates and starts application
 * timers.
 */
static void timers_init(void) {
  // Initialize timer module.
  APP_TIMER_INIT(APP_TIMER_PRESCALER, APP_TIMER_OP_QUEUE_SIZE, false);
}

static void services_init(void) {
  ble_bas_server_init();
  ble_hids_server_init();
}

/**@brief ��ʱ������
 */
static void application_timers_start(int channel) { auto_sleep_init(); }

static void bsp_event_handler(bsp_event_t event) { return; }

static void ble_evt_dispatch(ble_evt_t *p_ble_evt) {
  ble_conn_state_on_ble_evt(p_ble_evt);
  pm_on_ble_evt(p_ble_evt);
  ble_conn_params_on_ble_evt(p_ble_evt);
  // bsp_btn_ble_on_ble_evt(p_ble_evt);
  ble_advertising_on_ble_evt(p_ble_evt);

  ble_server_evt_dispatch(p_ble_evt); // ble_server.c
  ble_hid_evt_dispatch(p_ble_evt);    // ble_hid_server.c
  ble_bas_evt_dispatch(p_ble_evt);    // ble_bas_server.c
}

static void sys_evt_dispatch(uint32_t sys_evt) {
  // Dispatch the system event to the fstorage module, where it will be
  // dispatched to the Flash Data Storage (FDS) module.
  fs_sys_event_handler(sys_evt);

  // Dispatch to the Advertising module last, since it will check if there are
  // any pending flash operations in fstorage. Let fstorage process system
  // events first, so that it can report correctly to the Advertising module.
  ble_advertising_on_sys_evt(sys_evt);
}

static void ble_stack_init(void) {
  uint32_t err_code;

  nrf_clock_lf_cfg_t clock_lf_cfg = NRF_CLOCK_LFCLKSRC;

  // Initialize the SoftDevice handler module.
  SOFTDEVICE_HANDLER_INIT(&clock_lf_cfg, NULL);

  ble_enable_params_t ble_enable_params;
  err_code = softdevice_enable_get_default_config(
      CENTRAL_LINK_COUNT, PERIPHERAL_LINK_COUNT, &ble_enable_params);
  APP_ERROR_CHECK(err_code);

  // Check the ram settings against the used number of links
  CHECK_RAM_START_ADDR(CENTRAL_LINK_COUNT, PERIPHERAL_LINK_COUNT);

  err_code = softdevice_enable(&ble_enable_params);
  APP_ERROR_CHECK(err_code);

  // Register with the SoftDevice handler module for BLE events.
  err_code = softdevice_ble_evt_handler_set(ble_evt_dispatch);
  APP_ERROR_CHECK(err_code);

  // Register with the SoftDevice handler module for BLE events.
  err_code = softdevice_sys_evt_handler_set(sys_evt_dispatch);
  APP_ERROR_CHECK(err_code);
}

static void buttons_leds_init(bool *p_erase_bonds) {

  bsp_event_t startup_event;

  uint32_t err_code =
      bsp_init(BSP_INIT_LED, APP_TIMER_TICKS(100, APP_TIMER_PRESCALER),
               bsp_event_handler);

  APP_ERROR_CHECK(err_code);

  // err_code = bsp_btn_ble_init(NULL, &startup_event);
  // APP_ERROR_CHECK(err_code);

  *p_erase_bonds = (startup_event == BSP_EVENT_CLEAR_BONDING_DATA);
}

void app_error_fault_handler(uint32_t id, uint32_t pc, uint32_t info) {
  // NRF_LOG_ERROR("Fatal will happen %d %d %d   \r\n",id,pc,info);
  //  NRF_LOG_FINAL_FLUSH();
  // On assert, the system can only recover with a reset.
  // #ifndef DEBUG
  NVIC_SystemReset();
  // #else
  //  app_error_save_and_stop(id, pc, info);
  // #endif // DEBUG
}

extern volatile uint32_t uart_received_count;
extern volatile uint32_t uart_processed_count;
#include "board_support.h"
extern void timer_jump_init(void);
int main(void) {
  uint32_t err_code;
  bool erase_bonds;

  err_code = NRF_LOG_INIT(NULL);
  APP_ERROR_CHECK(err_code);
  URAT_init();

  timers_init();
  NRF_LOG_INFO(" timers_init************....\r\n");
  nrf_gpio_cfg_input(SLEEP_WK_PIN, NRF_GPIO_PIN_PULLUP);

  int channelC = eeprom_read();
  NRF_LOG_INFO("ChannelC *************is %d\r\n", channelC);

  setup_send_mode(channelC);

  if (channelC == RADIO_MODE) {
#if RADIO_MODE_QFLY != 0
    radio_init();
    // timer_jump_init();
#endif

    NRF_LOG_INFO(" 51822 instance started by RADIO Mode....\r\n");
  }
  if (channelC != RADIO_MODE) {
#if BLUE_MODE_QFLY != 0
    buttons_leds_init(&erase_bonds); //
#ifdef USE_23_PIN
    nrf_gpio_cfg_output(26);
    nrf_gpio_cfg_input(27, NRF_GPIO_PIN_PULLDOWN);
#endif
    ble_stack_init();
    if (erase_bonds == true) {
      NRF_LOG_INFO("Bonds erased!\r\n");
    }
    services_init();
    ble_server_init(erase_bonds);
#endif
  }

  application_timers_start(channelC); //Timer for power control

  uint32_t sleep_counter_h;
  uint32_t sleep_counter_h2;
//   bool led_qf_status = false;
  if (channelC != RADIO_MODE) {

#if BLUE_MODE_QFLY != 0
    NRF_LOG_INFO("51822 instance started by BLE Mode....\r\n");
    NRF_LOG_INFO("BLE mode *****!\r\n");

    ble_server_advertising_start();

    NRF_LOG_INFO("BLE mode *****2222222222!\r\n");
    APP_ERROR_CHECK(err_code);
    for (;;) {
      sleep_counter_h++;
	  sleep_counter_h2++;
      ble_data_send_with_queue();
		// bsp_board_led_on_fly();
		// NRF_LOG_INFO("bsp_board_led_on_fly is on ! %ld,%ld\r\n",sleep_counter_h,sleep_counter_h2);
		if (get_m_conn_handle() != BLE_CONN_HANDLE_INVALID) {
			if (sleep_counter_h2 >= 8000) {
				
				// NRF_LOG_INFO("led status ! ,%ld\r\n",sleep_counter_h2);
				sleep_counter_h2 = 0;
			
					nrf_gpio_pin_toggle(1); 
			
			}
		}
      NRF_LOG_FLUSH();
      uint32_t err_code;
      // Use SoftDevice API for power_management when in Bluetooth Mode.
      if (uart_received_count <= uart_processed_count && sleep_counter_h > 20000) {
		NRF_LOG_INFO("GO WFE %ld,%ld\r\n",sleep_counter_h,sleep_counter_h2);
		NRF_LOG_FLUSH();
		sleep_counter_h=0;
		set_all_leds_off();
        err_code = sd_app_evt_wait();
        APP_ERROR_CHECK(err_code);
      }
    }
#endif
  } else {
#if RADIO_MODE_QFLY != 0
    NRF_LOG_INFO("51822 instance started by RADIO Mode....\r\n");

    for (;;) {

      sleep_counter_h++;

      radio_data_send_with_queue();

      if (uart_received_count != uart_processed_count) {

        //							 NRF_LOG_INFO("count....
        //is running  %d \n",sleep_counter_h);
        //							 NRF_LOG_INFO("uart_received_count/uart_processed_count
        //%d ,%d \n",uart_received_count,uart_processed_count); 							  NRF_LOG_FLUSH();
      }
      NRF_LOG_FLUSH();
      // if there is no pending message ,then sleep
      if (uart_received_count <= uart_processed_count) {
        // NRF_LOG_INFO("**********WFE *****%d\n",sleep_counter_h);
        __WFE();

        // Clear Event Register.
        __SEV();
        __WFE();
      }
    }
#endif
  }
}

/**
 * @}
 */
