
#include <stdint.h>
#include <string.h>

#include "nrf.h"
#include <stdbool.h>
#include "app_error.h"
#include "nordic_common.h"
#define NRF_LOG_MODULE_NAME "APP"
#include "nrf_log.h"
#include "nrf_log_ctrl.h"
#include "nrf_delay.h"
#include "eeprom.h"

#include "nrf_soc.h"
#include "power_manage.h"

uint8_t    patwr;
uint8_t    patrd;
uint8_t    patold;
uint32_t   pg_size;
uint32_t   pg_num;

uint32_t err_code;

void flash_page_erase(uint32_t * page_address)
{
    // Turn on flash erase enable and wait until the NVMC is ready:
    NRF_NVMC->CONFIG = (NVMC_CONFIG_WEN_Een << NVMC_CONFIG_WEN_Pos);

    while (NRF_NVMC->READY == NVMC_READY_READY_Busy)
    {
        // Do nothing.
    }
    // Erase page:
    NRF_NVMC->ERASEPAGE = (uint32_t)page_address;

    while (NRF_NVMC->READY == NVMC_READY_READY_Busy)
    {
        // Do nothing.
    }

    // Turn off flash erase enable and wait until the NVMC is ready:
    NRF_NVMC->CONFIG = (NVMC_CONFIG_WEN_Ren << NVMC_CONFIG_WEN_Pos);

    while (NRF_NVMC->READY == NVMC_READY_READY_Busy)
    {
        // Do nothing.
    }
}


/** @brief Function for filling a page in flash with a value.
 *
 * @param[in] address Address of the first word in the page to be filled.
 * @param[in] value Value to be written to flash.
 */
 void flash_word_write(uint32_t * address, uint32_t value)
{
    // Turn on flash write enable and wait until the NVMC is ready:
    NRF_NVMC->CONFIG = (NVMC_CONFIG_WEN_Wen << NVMC_CONFIG_WEN_Pos);

    while (NRF_NVMC->READY == NVMC_READY_READY_Busy)
    {
        // Do nothing.
    }

    *address = value;

    while (NRF_NVMC->READY == NVMC_READY_READY_Busy)
    {
        // Do nothing.
    }

    // Turn off flash write enable and wait until the NVMC is ready:
    NRF_NVMC->CONFIG = (NVMC_CONFIG_WEN_Ren << NVMC_CONFIG_WEN_Pos);

    while (NRF_NVMC->READY == NVMC_READY_READY_Busy)
    {
        // Do nothing.
    }
}

void eeprom_write(uint32_t channel_value)
{
	
    uint32_t * addr;

    err_code = NRF_LOG_INIT(NULL);
    APP_ERROR_CHECK(err_code);

    NRF_LOG_INFO("Flashwrite example, %d \r\n", channel_value);
    patold  = 0;
    pg_size = NRF_FICR->CODEPAGESIZE;
    pg_num  = NRF_FICR->CODESIZE - 2;  // Use last page in flash
	 // Start address:
        addr = (uint32_t *)(pg_size * pg_num);
        // Erase page:
	 NRF_LOG_INFO("32333333, %d \r\n", channel_value);
		flash_page_erase(addr);
	 NRF_LOG_INFO("1111111, %d \r\n", channel_value);
		flash_word_write(addr, channel_value);
		 NRF_LOG_INFO("22222222, %d \r\n", channel_value);
}

uint32_t eeprom_read(void)
{
	
    uint32_t * addr;

    err_code = NRF_LOG_INIT(NULL);
    APP_ERROR_CHECK(err_code);

    patold  = 0;
    pg_size = NRF_FICR->CODEPAGESIZE;
    pg_num  = NRF_FICR->CODESIZE - 2;  // Use last page in flash
	
	 NRF_LOG_INFO("pg_size is ************, %d \r\n", pg_size);
	 NRF_LOG_INFO("pg_num is ************, %d \r\n", pg_num);
	 // Start address:
    addr = (uint32_t *)(pg_size * pg_num);
        
		uint32_t channel_value=0; 
		channel_value=*(uint32_t*)addr;
		
    NRF_LOG_INFO("Flash Read 51822 is  %d \r",channel_value);
    NRF_LOG_INFO("Flash Read 51822 addr is  %x \r",(pg_size * pg_num));
	
		
		if(channel_value != 4 && channel_value !=1 && channel_value !=0 && channel_value !=2){
				#if RADIO_MODE_QFLY != 0 
				  channel_value = RADIO_MODE;
				#endif
				#if BLUE_MODE_QFLY != 0 
				  channel_value = BLE_MODE;
				#endif
        NRF_LOG_INFO("The value is init status , put to default mode   %d \r",channel_value);
		}
	
		
	  if(channel_value == 4){
				#if RADIO_MODE_QFLY == 0 
				  channel_value = 0;
				#endif
			
        NRF_LOG_INFO("change to BLE mode 0   %d \r",channel_value);
		}
		
		if(channel_value != 4){
				#if BLUE_MODE_QFLY == 0 
				  channel_value = 4;
					NRF_LOG_INFO("change to RADIO mode 4   %d \r",channel_value);
				#endif
			
        
		}
	
	 NRF_LOG_INFO("return value is  from eeprom   %d \r",channel_value);
		return channel_value;

}

#include "output_source_selection_mgr.h"


void eeprom_write_peerId_common(uint32_t channelIndex, uint32_t peerId) {

//if read_send_mode == 4, means it is in RADIO mode, do not write peerId to flash
  if(read_send_mode() == RADIO_MODE){
    return;
  }

  uint32_t currPeerId = eeprom_read_peerId(channelIndex);
	
		if(peerId == currPeerId){
			NRF_LOG_INFO("do nothing, before vs after has no change... \r");
			return ;
		}
    uint32_t * addr;

    err_code = NRF_LOG_INIT(NULL);
    APP_ERROR_CHECK(err_code);

    patold  = 0;
    pg_size = NRF_FICR->CODEPAGESIZE; // 1024 byte == 1 page == 1K
    pg_num  = NRF_FICR->CODESIZE - 10 + channelIndex; // Use last 3 pages(6K) in flash (6*1024=6144 byte)

	 addr = (uint32_t *)(pg_size * pg_num);
	 NRF_LOG_INFO("addrrrrr***  %x \r",pg_size * pg_num); 

		sd_flash_page_erase(pg_num); 

		uint32_t value = peerId;
		nrf_delay_ms(100);
    sd_flash_write(addr, (uint32_t *)&value, 4);
    nrf_delay_ms(100);

}

uint32_t eeprom_read_peerId(uint8_t channelIndex) {
	
	
	 NRF_LOG_INFO("xxxxxxx Flash Read channelIndex**********   is  %d \r",channelIndex);
		if(channelIndex > 3) return 99; // only 4 channels (0-3)
   

    uint32_t * addr;

    err_code = NRF_LOG_INIT(NULL);
    APP_ERROR_CHECK(err_code);

    patold  = 0;
    pg_size = NRF_FICR->CODEPAGESIZE;
    pg_num  = NRF_FICR->CODESIZE - 10 + channelIndex;
	 // Start address:
	uint32_t addr_raw = pg_size * pg_num;
    addr = (uint32_t *)addr_raw; // 2 byte per channel

    uint32_t peerId=0;
    peerId=*(uint32_t*)addr;
	//NRF_LOG_INFO("Flash Read ori..... eerId is  %d \r",peerId);
	//NRF_LOG_INFO("addr_raw Read ori..... eerId is  %x \r",addr_raw);
		if(peerId > 3){
			peerId = 99;
		}


	return peerId;
}



void eeconfig_update_by_MODE(uint8_t rMode,uint32_t val) { 
	
	
	if(4 == rMode) 
  {
      return;
  }
	
	if(99 == val) 
  {
     eeprom_write_peerId_common(rMode, 99); 
		return;
  }


  uint32_t existed = eeprom_read_peerId(rMode);
	
	if((0 != rMode) 
      && (eeprom_read_peerId(0) == val)){
      eeprom_write_peerId_common(0, 99); 
  }

  if((1 != rMode) 
      && (eeprom_read_peerId(1) == val)){
      eeprom_write_peerId_common(1, 99); 
  }

  if((2 != rMode) 
      && (eeprom_read_peerId(2) == val)){
      eeprom_write_peerId_common(2, 99); 
  }

  if(existed == val){
      NRF_LOG_INFO("existed rMode is same , skipped mode %d, val $d",rMode, val);
      return ;
  
  }

  switch(rMode){

    case 0:
        eeprom_write_peerId_common(0, val); 
				sleep_reset_count();
        break;
    case 1:
        eeprom_write_peerId_common(1, val); 
				sleep_reset_count();
        break;
    case 2:
        eeprom_write_peerId_common(2, val); 
				sleep_reset_count();
        break;
    default:
        break;
  }
	

	//Current Channel 
//	if(val == 1 || val == 0 || val == 2){
//		nrf_delay_ms(200);
//		NVIC_SystemReset();
//	}
 
}

