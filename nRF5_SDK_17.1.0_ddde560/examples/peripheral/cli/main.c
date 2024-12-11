/**
 * Copyright (c) 2016 - 2021, Nordic Semiconductor ASA
 *
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this
 *    list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form, except as embedded into a Nordic
 *    Semiconductor ASA integrated circuit in a product or a software update for
 *    such product, must reproduce the above copyright notice, this list of
 *    conditions and the following disclaimer in the documentation and/or other
 *    materials provided with the distribution.
 *
 * 3. Neither the name of Nordic Semiconductor ASA nor the names of its
 *    contributors may be used to endorse or promote products derived from this
 *    software without specific prior written permission.
 *
 * 4. This software, with or without modification, must only be used with a
 *    Nordic Semiconductor ASA integrated circuit.
 *
 * 5. Any software provided in binary form under this license must not be reverse
 *    engineered, decompiled, modified and/or disassembled.
 *
 * THIS SOFTWARE IS PROVIDED BY NORDIC SEMICONDUCTOR ASA "AS IS" AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY, NONINFRINGEMENT, AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL NORDIC SEMICONDUCTOR ASA OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 * GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */
#include <stdio.h>
#include <stdbool.h>
#include <stddef.h>

#include "nrf.h"
#include "nrf_drv_clock.h"
#include "nrf_gpio.h"
#include "nrf_delay.h"

#include "app_timer.h"
#include "fds.h"
#include "app_error.h"
#include "app_util.h"

#include "nrf_cli.h"
#include "nrf_cli_rtt.h"
#include "nrf_cli_types.h"

#include "boards.h"

#include "nrf_log.h"
#include "nrf_log_ctrl.h"
#include "nrf_log_default_backends.h"
#include "nrf_log_backend_flash.h"
#include "nrf_fstorage_nvmc.h"

#include "nrf_mpu_lib.h"
#include "nrf_stack_guard.h"

#define CLI_OVER_USB_CDC_ACM 1


#include "nrf_cli_cdc_acm.h"
#include "nrf_drv_usbd.h"
#include "app_usbd_core.h"
#include "app_usbd.h"
#include "app_usbd_string_desc.h"
#include "app_usbd_cdc_acm.h"



#define CLI_OVER_UART 0



/* If enabled then CYCCNT (high resolution) timestamp is used for the logger. */
#define USE_CYCCNT_TIMESTAMP_FOR_LOG 0

/**@file
 * @defgroup CLI_example main.c
 *
 * @{
 *
 */

#if NRF_LOG_BACKEND_FLASHLOG_ENABLED
NRF_LOG_BACKEND_FLASHLOG_DEF(m_flash_log_backend);
#endif

#if NRF_LOG_BACKEND_CRASHLOG_ENABLED
NRF_LOG_BACKEND_CRASHLOG_DEF(m_crash_log_backend);
#endif

/* Counter timer. */
APP_TIMER_DEF(m_timer_0);

/* Declared in demo_cli.c */
extern uint32_t m_counter;
extern bool m_counter_active;



/**
 * @brief Enable power USB detection
 *
 * Configure if example supports USB port connection
 */
#ifndef USBD_POWER_DETECTION
#define USBD_POWER_DETECTION true
#endif


static void usbd_user_ev_handler(app_usbd_event_type_t event)
{
    switch (event)
    {
        case APP_USBD_EVT_STOPPED:
            app_usbd_disable();
            break;
        case APP_USBD_EVT_POWER_DETECTED:
            if (!nrf_drv_usbd_is_enabled())
            {
                app_usbd_enable();
            }
            break;
        case APP_USBD_EVT_POWER_REMOVED:
            app_usbd_stop();
            break;
        case APP_USBD_EVT_POWER_READY:
            app_usbd_start();
            break;
        default:
            break;
    }
}



/**
 * @brief Command line interface instance
 * */
#define CLI_EXAMPLE_LOG_QUEUE_SIZE  (4)

NRF_CLI_CDC_ACM_DEF(m_cli_cdc_acm_transport);
NRF_CLI_DEF(m_cli_cdc_acm,
            "usb_cli:~$ ",
            &m_cli_cdc_acm_transport.transport,
            '\r',
            CLI_EXAMPLE_LOG_QUEUE_SIZE);


//NRF_CLI_RTT_DEF(m_cli_rtt_transport);
//NRF_CLI_DEF(m_cli_rtt,
//            "rtt_cli:~$ ",
//            &m_cli_rtt_transport.transport,
//            '\n',
//            CLI_EXAMPLE_LOG_QUEUE_SIZE);

static void timer_handle(void * p_context)
{
    UNUSED_PARAMETER(p_context);

    if (m_counter_active)
    {
        m_counter++;
        NRF_LOG_RAW_INFO("counter = %d\n", m_counter);
    }
}

static void cli_start(void)
{
    ret_code_t ret;

    ret = nrf_cli_start(&m_cli_cdc_acm);
    APP_ERROR_CHECK(ret);

    //ret = nrf_cli_start(&m_cli_rtt);
    //APP_ERROR_CHECK(ret);
}

static void cli_init(void)
{
    ret_code_t ret;


    ret = nrf_cli_init(&m_cli_cdc_acm, NULL, true, true, NRF_LOG_SEVERITY_INFO);
    APP_ERROR_CHECK(ret);


    //ret = nrf_cli_init(&m_cli_rtt, NULL, true, true, NRF_LOG_SEVERITY_INFO);
    //APP_ERROR_CHECK(ret);
}


static void usbd_init(void)
{

    ret_code_t ret;
    static const app_usbd_config_t usbd_config = {
        .ev_handler = app_usbd_event_execute,
        .ev_state_proc = usbd_user_ev_handler
    };
    ret = app_usbd_init(&usbd_config);
    APP_ERROR_CHECK(ret);

    app_usbd_class_inst_t const * class_cdc_acm =
            app_usbd_cdc_acm_class_inst_get(&nrf_cli_cdc_acm);
    ret = app_usbd_class_append(class_cdc_acm);
    APP_ERROR_CHECK(ret);

    if (USBD_POWER_DETECTION)
    {
        ret = app_usbd_power_events_enable();
        APP_ERROR_CHECK(ret);
    }
    else
    {
        NRF_LOG_INFO("No USB power detection enabled\nStarting USB now");

        app_usbd_enable();
        app_usbd_start();
    }

    /* Give some time for the host to enumerate and connect to the USB CDC port */
    nrf_delay_ms(1000);

}


static void cli_process(void)
{

    nrf_cli_process(&m_cli_cdc_acm);

    //nrf_cli_process(&m_cli_rtt);
}


uint32_t cyccnt_get(void)
{
    return DWT->CYCCNT;
}

#include "nrf_cli.h"
int main(void)
{
    ret_code_t ret;


        APP_ERROR_CHECK(NRF_LOG_INIT(app_timer_cnt_get));
 

    ret = nrf_drv_clock_init();
    APP_ERROR_CHECK(ret);
    nrf_drv_clock_lfclk_request(NULL);

    ret = app_timer_init();
    APP_ERROR_CHECK(ret);

    ret = app_timer_create(&m_timer_0, APP_TIMER_MODE_REPEATED, timer_handle);
    APP_ERROR_CHECK(ret);

    ret = app_timer_start(m_timer_0, APP_TIMER_TICKS(1000), NULL);
    APP_ERROR_CHECK(ret);

    cli_init();

    usbd_init();

    //ret = fds_init();
    //APP_ERROR_CHECK(ret);


    //UNUSED_RETURN_VALUE(nrf_log_config_load());

    cli_start();

    //flashlog_init();

    //stack_guard_init();
    //NRF_CLI_CMD_REGISTER(nordic, NULL, "Print Nordic Semiconductor logo.", cmd_nordic);
    NRF_LOG_RAW_INFO("Command Line Interface example started.\n");
    NRF_LOG_RAW_INFO("Please press the Tab key to see all available commands.\n");

    while (true)
    {
        UNUSED_RETURN_VALUE(NRF_LOG_PROCESS());

        cli_process();
    }
}

/** @} */
