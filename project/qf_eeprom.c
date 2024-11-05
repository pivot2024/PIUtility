
#include "quantum.h"
#include "dynamic_keymap.h"
#include "hal_pal.h"
#include "outputselect.h"
#include "report.h"
#include "bytequeue.h"
#include "interrupt_setting.h"
#include "main_master.h"


#define QF_CUSTOM_ARR_LENGTH 256
#define QF_REPORT_SIZE 8
uint8_t macroRecMode = MACAO_MODE_OFF;
uint8_t dataArrp[QF_CUSTOM_ARR_LENGTH] ;
uint8_t dataArrp_read[QF_CUSTOM_ARR_LENGTH] ;
byteQueue_t my_queue;

extern void qf_macro_read(uint8_t order,uint16_t size, uint8_t *data);
extern u16 get_offset_by_order(u8 order);

void dynamic_keymap_macro_get_buffer_qf(uint16_t offset, uint16_t size, uint8_t *data) {
    void *   source = (void *)(VIA_EEPROM_CUSTOM_CONFIG_ADDR + offset);
    uint8_t *target = data;
    for (uint16_t i = 0; i < size; i++) {
        if (offset + i < VIA_EEPROM_CUSTOM_CONFIG_SIZE) {
            *target = eeprom_read_byte(source);
        } else {
            *target = 0x00;
        }
        source++;
        target++;
    }
}

void qf_macro_read_common(u16 offset, u16 size, uint8_t *data) {

    void *   source = (void *)(VIA_EEPROM_CUSTOM_CONFIG_ADDR + offset);
    uint8_t *target = data;
    for (uint16_t i = 0; i < size; i++) {
        if (offset + i < VIA_EEPROM_CUSTOM_CONFIG_SIZE) {
            *target = eeprom_read_byte(source);
            if(*target == 0xff){
                break;
            }
        } else {
            *target = 0x00;
        }
        source++;
        target++;
    }
}

void qf_macro_read(u8 order, uint16_t size, uint8_t *data) {

    if(order > 0 && order < 99){
        u16 offset = get_offset_by_order(order);
          xprintf("qf_macro_read offset %d \n",offset);
        qf_macro_read_common(offset,size,data);
    }

}

void dynamic_keymap_macro_set_buffer_qf(uint16_t offset, uint16_t size, uint8_t *data) {

    if(size > 255){
        size = 255;
    }

    void *   target = (void *)(VIA_EEPROM_CUSTOM_CONFIG_ADDR + offset);

     xprintf(" VIA_EEPROM_CUSTOM_CONFIG_ADDR %d \n",VIA_EEPROM_CUSTOM_CONFIG_SIZE);
     xprintf(" offset %d \n",offset);
     xprintf(" size %d \n",size);


    uint8_t *source = data;
    for (uint16_t i = 0; i < size; i++) {
        if (offset + i < VIA_EEPROM_CUSTOM_CONFIG_SIZE) {
            eeprom_update_byte(target, *source);
        }
        source++;
        target++;
    }
}



void write_macro_to_eeprom(u8 order, uint8_t *data, uint8_t size) {

    if(order > 0){
        u16 offset = get_offset_by_order(order);
         xprintf(" soffset size %d \n",offset);

        dynamic_keymap_macro_set_buffer_qf(offset, size, data);
    }

}


void qf_queue_init(void){
    bytequeue_init(&my_queue, dataArrp, QF_CUSTOM_ARR_LENGTH-1);
    //  uint8_t length_q = bytequeue_length(&my_queue);
    //  xprintf(" Queue length is %d ",length_q);
}

void write_to_queue(report_keyboard_t *report){

        bytequeue_enqueue(&my_queue, report->mods);
        bytequeue_enqueue(&my_queue, report->reserved);
        bytequeue_enqueue(&my_queue, report->keys[0]);
        bytequeue_enqueue(&my_queue, report->keys[1]);
        bytequeue_enqueue(&my_queue, report->keys[2]);
        bytequeue_enqueue(&my_queue, report->keys[3]);
        bytequeue_enqueue(&my_queue, report->keys[4]);
        bytequeue_enqueue(&my_queue, report->keys[5]);

}
void print_queue_qf(byteQueue_t* queue);
void print_queue_eeprom_qf(u8* dataArrp_read);
void print_queue(){
   print_queue_qf(&my_queue);
}

void print_queue_eeprom(u8 seq){

      qf_macro_read(seq,QF_CUSTOM_ARR_LENGTH,dataArrp_read);
      xprintf(" read to dataArrp_read array for use \n");
      xprintf("\n *************start to print fetched from eeprom **********\n");

     print_queue_eeprom_qf(dataArrp_read);
}


void print_array(uint8_t *arr, int len) {
    for (int i = 0; i < len; i++) {
        xprintf("0x%x ", arr[i]);
    }
    xprintf("\n");
}

static report_keyboard_t report;
void send_macro_eeprom(uint8_t order){

    qf_macro_read(order,QF_CUSTOM_ARR_LENGTH,dataArrp_read);
    // xprintf("\n *************send_macro_eeprom **********\n");
    for (int i = 0; i < QF_CUSTOM_ARR_LENGTH/QF_REPORT_SIZE; i++) {
        uint8_t ele = dataArrp_read[QF_REPORT_SIZE*i];

        if (ele == 0xff) {
            //  xprintf(" end with 0xff %d\n",(i+1)*8);
             xprintf("\n");
            break;
        }
        // report->raw[i%8] = ele;
         // 通过数组赋值设置键盘报告
        //  report.report_id = 1;
        report.mods = dataArrp_read[QF_REPORT_SIZE*i];
        report.reserved = dataArrp_read[i*QF_REPORT_SIZE+1];
        report.keys[0] = dataArrp_read[i*QF_REPORT_SIZE+2];
        report.keys[1] = dataArrp_read[i*QF_REPORT_SIZE+3];
        report.keys[2] = dataArrp_read[i*QF_REPORT_SIZE+4];
        report.keys[3] = dataArrp_read[i*QF_REPORT_SIZE+5];
        report.keys[4] = dataArrp_read[i*QF_REPORT_SIZE+6];
        report.keys[5] = dataArrp_read[i*QF_REPORT_SIZE+7];


        // xprintf(" 0x%x ",report.mods);
        // xprintf(" 0x%x ",report.reserved);

        // print_array(report.keys, 6);
        extern void keyboard_report_to_string(report_keyboard_t *report) ;
        keyboard_report_to_string(&report);

        send_keyboard_qf(&report);
        wait_ms(40);


    }

    // xprintf("\n *************end to send_macro_eeprom **********\n");
}

void set_macroRecordMode(uint8_t macroSeq){

    if(macroSeq > 0 && macroSeq < 99){
        xprintf(" set_macroRecordMode %d \n",macroSeq);

        bytequeue_enqueue(&my_queue, 0xff);
        uint8_t length_q = bytequeue_length(&my_queue);
        xprintf(" write_macro_to_eeprom_m%d , will save to eeprom \n",macroSeq);
        write_macro_to_eeprom(macroSeq, my_queue.data, length_q);
        xprintf(" saved \n");

        print_queue_eeprom(macroSeq);

    }else if (macroSeq == MACAO_MODE_RECORDING) {

        // Reinitialize the queue
        xprintf(" re init the queue....********** \n");
        qf_queue_init();

    }

    macroRecMode = macroSeq;
}

uint8_t get_macroRecordMode(void){
    return macroRecMode;
}

