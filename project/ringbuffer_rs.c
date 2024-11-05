#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "rb.h"

#define ELEMENT_SIZE (128 / 6)
#define BUFFER_SIZE 4     // 日志缓冲区总共4个元素



typedef struct {
    OLEDLog data[BUFFER_SIZE]; // 日志数据缓冲区
    uint8_t    start;                 // 开始索引
    uint8_t    end;                   // 结束索引
    uint8_t    count;                  // 是否满
} LogRingBuffer_t;

// 创建一个全局单例实例
LogRingBuffer_t myBuffer;

void log_ringbuffer_init(void) {
    myBuffer.start = 0;
    myBuffer.end   = 0;
    myBuffer.count = 0;
}

// 入队函数
bool log_ringbuffer_enqueue(OLEDLog item) {

    bool replaced = false;

    if (myBuffer.count < BUFFER_SIZE) {
        // 插入新元素
        myBuffer.data[myBuffer.end] = item;
        myBuffer.end                = (myBuffer.end + 1) % BUFFER_SIZE; // 更新结束索引
    } else {
        // 队列已满，替换最早的元素
        myBuffer.data[myBuffer.start] = item;
        myBuffer.start                = (myBuffer.start + 1) % BUFFER_SIZE; // 更新开始索引
        myBuffer.end                  = (myBuffer.end + 1) % BUFFER_SIZE;   // 更新结束索引
        replaced                      = true;                                   // 标记为替换
    }

    myBuffer.count ++;
    return replaced; // 返回是否替换了旧元素
}

// 读取函数
bool log_ringbuffer_read(OLEDLog* items, int* count) {
    if (myBuffer.start == myBuffer.end && myBuffer.count < BUFFER_SIZE) {
        *count = 0; // 缓冲区为空
        return false;
    }

    int index = 0;
    int i     = myBuffer.start;

    while (i != myBuffer.end) {
        items[index++] = myBuffer.data[i];
        i              = (i + 1) % BUFFER_SIZE; // 循环访问
    }

    *count = index; // 返回读取的元素数量
    return true;    // 成功读取
}

// 根据索引获取元素
bool log_ringbuffer_get_by_index(int index, OLEDLog* item) {
    // 计算从 start 到 end 的有效元素数量
    int size_from_start_to_end = (myBuffer.end - myBuffer.start + BUFFER_SIZE) % BUFFER_SIZE;
    int valid_size             =  myBuffer.count >= BUFFER_SIZE ? BUFFER_SIZE : size_from_start_to_end;

    if (index < 0 || index >= valid_size) {
        return false; // 索引无效
    }

    int actual_index = (myBuffer.start + index) % BUFFER_SIZE; // 计算实际位置
    *item            = myBuffer.data[actual_index];            // 获取元素
    return true;                                               // 成功获取
}
// 根据索引获取元素
bool AddCharToNewElement(uint8_t newLogChar) {

    // 创建一个新的 OLEDLog 对象
    OLEDLog newElement;

    // 将 newLogChar 放入新的 OLEDLog
    newElement.data[0] = newLogChar; // 添加字符
    newElement.data[1] = '\0'; // 添加字符串结束符
    newElement.size = 1; // 更新大小
    // 添加到队列
    log_ringbuffer_enqueue(newElement);
    return true; // 成功添加新元素
}

// 根据索引获取元素
bool AddCharToEndElement(uint8_t newLogChar,OLEDLog *lastElement) {


    uint8_t currentLength =  lastElement->size;
    // 添加字符到最后一个元素
    lastElement->data[currentLength] = newLogChar; // 添加字符
    lastElement->data[currentLength + 1] = '\0'; // 添加字符串结束符
    lastElement->size++;
    return true; // 成功添加字符

}


// 根据索引获取元素
bool AddCharToQueue(uint8_t newLogChar) {

    // 检查缓冲区是否为空
    if (myBuffer.start == myBuffer.end &&  myBuffer.count < BUFFER_SIZE) {
        // 如果缓冲区为空，添加新元素
        AddCharToNewElement(newLogChar);
        return true;
    }

    // 计算最后一个有效元素的索引
    int lastIndex = (myBuffer.end - 1 + BUFFER_SIZE) % BUFFER_SIZE;

    // 从缓冲区中获取最后一个元素
    OLEDLog *lastElement = &myBuffer.data[lastIndex];

    // 检查上一个字符是否为 '\n'
    if (lastElement->size > 0 && lastElement->data[lastElement->size - 1] == '\n') {

        if(newLogChar != '\n' || newLogChar != '\r'){
            AddCharToNewElement(newLogChar);
        }

       return true;
    }

    // 检查最后一个元素的大小
    if (lastElement->size < 19) {
        AddCharToEndElement(newLogChar,lastElement);
    }else if (lastElement->size == 19) {
        //如果不是换行符,就添加换行标记,并且之后再插入当前字符到下一行
        if(newLogChar != '\n'){
            AddCharToEndElement(newLogChar,lastElement);
            AddCharToNewElement('\n');
        }else{
            //如果是换行符,就添加换行标记 newLogChar = '\n'
            AddCharToEndElement(newLogChar,lastElement);
        }
    }else{
        AddCharToNewElement(newLogChar);
    }

    return true;
}

int main_test(void) {

    // Enqueue the item
   AddCharToQueue(32);



        OLEDLog item;
        bool success = log_ringbuffer_get_by_index(myBuffer.start, &item);

        if (success) {
            printf("Item at index %d: ", myBuffer.start);
            for (int i = 0; i < item.size; i++) {
                uint8_t value = item.data[i];
                if (value >= 32 && value <= 126) {  // 可见字符范围
                    printf("%c", (char)value);
                } else {
                    printf("Non-printable: %d", value);  // 打印不可见字符的数值
                }
            }
            printf("END ...\n");
        }

    return 0;
}
