#ifndef __J1939_H
#define __J1939_H

#include <stdint.h>

/******************************类型声明******************************/

/** 统一类型定义
 *  不同的单片机的编译器，char，int，short，long 的位数可能不同
 *
 *  在移植J1939协议栈时，首先应该配置这里
 */
typedef uint32_t    j1939_uint32_t; /** < 32位无符号整形 */
typedef int32_t     j1939_int32_t;  /** < 32位有符号整形 */
typedef uint16_t    j1939_uint16_t; /** < 16位无符号整形 */
typedef uint8_t     j1939_uint8_t;  /** <  8位无符号整形 */
typedef int8_t      j1939_int8_t;   /** <  8位有符号整形 */

/**< 是否对TP协议的支持（是否支持长帧（大于8字节的数据）的发送与接受）*/
#define J1939_TP_RX_TX              J1939_TRUE
/**< TP协议的支持的最大接受发送消息长度(最大可配置为1785)*/
#define J1939_TP_MAX_MESSAGE_LENGTH 240

#define J1939_NULL     0

#define J1939_FALSE    0 /**< 代表函数错误返回 */
#define J1939_TRUE     1 /**< 代表函数正确返回 */

/**< 函数返回代码 */
typedef enum {
    RC_SUCCESS        = 0, /**< 成功*/
    RC_QUEUEEMPTY     = 1, /**< 队列为空*/
    RC_QUEUEFULL      = 1, /**< 队列满*/
    RC_CANNOTRECEIVE  = 2, /**< 不能接收*/
    RC_CANNOTTRANSMIT = 2, /**< 不能传输*/
    RC_PARAMERROR     = 3, /**< 参数错误*/
} J1939_Status_t;

/**< J1939 默认的优先级（参考J1939文档） */
typedef enum {
    J1939_PRIORITY_CONTROL     = 0x03, /**< J1939文档默认的优先级*/
    J1939_PRIORITY_INFO        = 0x06, /**< J1939文档默认的优先级*/
    J1939_PRIORITY_PROPRIETARY = 0x06, /**< J1939文档默认的优先级*/
    J1939_PRIORITY_REQUEST     = 0x06, /**< J1939文档默认的优先级*/
    J1939_PRIORITY_ACK         = 0x06, /**< J1939文档默认的优先级*/
    J1939_PRIORITY_TP_CM       = 0x07, /**< J1939文档默认的优先级*/
    J1939_PRIORITY_TP_DT       = 0x07, /**< J1939文档默认的优先级*/
} J1939_Priority_t;

/**< J1939协议栈的PNG请求响应，相关的定义 */
typedef enum {
    J1939_PF_REQUEST2             = 201, /**< J1939协议栈的请求 PF */
    J1939_PF_TRANSFER             = 202, /**< J1939协议栈的转移 PF */
    J1939_PF_ACKNOWLEDGMENT       = 232, /**< 确认请求 或 用于握手机制*/
    J1939_PF_REQUEST              = 234, /**< 请求 或 用于握手机制*/
    J1939_PF_TP_DT                = 235, /**< TP协议传输---数据传输 PF*/
    J1939_PF_TP_CM                = 236, /**< TP协议传输---链接管理 PF*/
    J1939_PF_ADDRESS_CLAIMED      = 238,
    J1939_PF_CANNOT_CLAIM_ADDRESS = 238,
    J1939_PF_PROPRIETARY_A        = 239, /**< 专用A*/
    J1939_PF_PROPRIETARY_B        = 255, /**< 专用B*/
} J1939_PDUFormat_t;

/**< PF（PDUFormat）小于 240 时，PS（PDUSpecific）为 DA（DestinationAddress） */
typedef enum {
    J1939_PS_DA_GLOBAL = 255, /**< 全局地址*/
    J1939_PS_DA_NULL   = 254, /**< 空地址*/
} J1939_PDUSpecific_DA_t;

typedef enum {
    J1939_CONTROL_BYTE_ACK            = 0,    /**< 用于TP(长帧数据)，代表确认*/
    J1939_CONTROL_BYTE_NACK           = 1,    /**< 用于TP(长帧数据)，PNG不被支持。否定消息*/
    J1939_CONTROL_BYTE_ACCESS_DENIED  = 2,    /**< 拒绝访问，但是信息是被支持，暂时不能响应（需要再次发送请求）*/
    J1939_CONTROL_BYTE_CANNOT_RESPOND = 3,    /**< 不能做出反应，有空但是接受的缓存不够，或则发送资源被占领，暂时不能响应（需要再次发送请求）*/
    J1939_CONTROL_BYTE_RTS            = 16,   /**< TP.CM_RTS*/
    J1939_CONTROL_BYTE_CTS            = 17,   /**< TP.CM_CTS*/
    J1939_CONTROL_BYTE_EOMACK         = 19,   /**< 消息应答结束*/
    J1939_CONTROL_BYTE_BAM            = 32,   /**< 广播公告消息*/
    J1939_CONTROL_BYTE_CONNABORT      = 255,  /**< 连接中断控制字节（放弃连接）*/
    /* CONTROL BYTE RESERVED BELOW */
    J1939_CONTROL_BYTE_RESERVED       = 0xFF, /**< 变量的保留位的值*/
} J1939_CONTROL_BYTE_t;

/**< TP的超时时间，单位（ms） */
typedef enum {
    J1939_TP_Tr = 200,
    J1939_TP_Th = 500,
    J1939_TP_T1 = 750,
    J1939_TP_T2 = 1250,
    J1939_TP_T3 = 1250,
    J1939_TP_T4 = 1050,
} J1939_TP_TIMEOUT_t;

#define J1939_TP_TIMEOUT_NORMAL   0 /**< 未超时正常*/
#define J1939_TP_TIMEOUT_ABNORMAL 1 /**< 超时*/

/**< 与J1939网络层有关的定义 */
#define J1939_PGN0_REQ_ADDRESS_CLAIM    0x00
#define J1939_PGN1_REQ_ADDRESS_CLAIM    0xEA
#define J1939_PGN2_REQ_ADDRESS_CLAIM    0x00

#define J1939_PGN0_COMMANDED_ADDRESS    0xD8 /**< 参考J1939-81 地址命令配置*/
#define J1939_PGN1_COMMANDED_ADDRESS    0xFE /**< 命令地址消息*/
#define J1939_PGN2_COMMANDED_ADDRESS    0x00

/**CAN节点的选择枚举
 *
 * 默认支持最大4路CAN硬件\n
 */
typedef enum {
    Select_CAN_NODE_1,    /**< 选择CAN硬件 1*/
    Select_CAN_NODE_2,    /**< 选择CAN硬件 2*/
    Select_CAN_NODE_3,    /**< 选择CAN硬件 3*/
    Select_CAN_NODE_4,    /**< 选择CAN硬件 4*/
    Select_CAN_NODE_Null, /**< 不选择任何CAN硬件*/
} CAN_NODE_t;

#if J1939_TP_RX_TX
/**TP的状态描述枚举
 *
 */
typedef enum {
    J1939_TP_NULL,   /**< 长数据传输处于空闲，只有TP系统处于空闲，才能用处理下一个发送，和接受请求*/
    J1939_TP_RX,     /**< 长数据传输处于接收*/
    J1939_TP_TX,     /**< 长数据传输处于发送*/
    J1939_TP_OSBUSY, /**< 长数据传输处于繁忙，比如刚接受一整段长数据，但是CPU没来得处理，又一个长数据请求到来，为了数据不被覆盖，将状态设为本值*/
} J1939_TP_STATE_t;

/**TP的标志位结构体
 *
 * 本结构体记录了TP的状态，使用TP发送和接受的CAN硬件编号
 */
typedef struct {
    J1939_TP_STATE_t    state;          /**< TP的连接状态*/
    CAN_NODE_t          TP_RX_CAN_NODE; /**< TP接受请求产生的 CAN硬件编号*/
    CAN_NODE_t          TP_TX_CAN_NODE; /**< TP接受发送产生的 CAN硬件编号*/
} J1939_TP_FLAGS_t;

/**J1939消息对象的结构体
 *
 * 本结构体实现了 J1939的消息对象
 */
typedef struct {
    j1939_uint32_t  PGN;                                /**< J1939的消息对象的 PGN*/
    j1939_uint8_t   data[J1939_TP_MAX_MESSAGE_LENGTH];  /**< J1939的消息对象的 数据*/
    j1939_uint16_t  byte_count;                         /**< J1939的消息对象的 数据大小*/
    j1939_uint8_t   SA;                                 /**< J1939的消息对象的 目标地址（发送目的地  或  接受来源地）*/
} J1939_TP_MESSAGE_t;

/**J1939消息对象的结构体
 *
 * 本结构体实现了 J1939的多帧消息对象
 */
typedef struct {
    j1939_uint8_t   *data;      /**< 缓存区指针*/
    j1939_uint16_t  data_num;   /**< 缓存区大小*/
    j1939_uint8_t   SA;         /**< J1939的消息对象的 数据 源地址*/
    j1939_uint16_t  byte_count; /**< J1939的消息对象的 数据大小*/
    j1939_uint32_t  PGN;        /**< J1939的消息对象的 PGN*/
} TP_RX_MESSAGE_t;

/**J1939_TP_TX_STEP_t 枚举
 *
 * 实现了记录长帧（多帧）传输的TX 的步骤
 */
typedef enum {
    J1939_TP_TX_WAIT,
    J1939_TP_TX_CM_START,
    J1939_TP_TX_CM_WAIT,
    J1939_TP_TX_DT,
    J1939_TP_WAIT_ACK,
    J1939_TP_TX_ERROR,
    J1939_TX_DONE,
} J1939_TP_TX_STEP_t; /* 协议的发送步骤 */

/**J1939_TRANSPORT_TX_INFO_t 结构体
 *
 * 实现了长帧传输中产生的临时数据，和一些传输交换数据
 */
typedef struct {
    J1939_TP_MESSAGE_t  tp_tx_msg;              /**< J1939的消息对象*/
    j1939_uint16_t      time;                   /**< 时间*/
    j1939_uint8_t       packet_offset_p;        /**< 数据包偏移指针*/
    j1939_uint8_t       packets_total;          /**< 总共有多少个数据包*/
    j1939_uint8_t       packets_request_num;    /**< 请求发送的数据包数（接受方准备接受的数据包数）*/
    J1939_TP_TX_STEP_t  state;                  /**< 协议的发送步骤*/
} J1939_TRANSPORT_TX_INFO_t;

/**J1939_TP_RX_STEP_t 枚举
 *
 * 实现了记录长帧（多帧）传输的RX 的步骤
 */
typedef enum {
    J1939_TP_RX_WAIT,
    J1939_TP_RX_READ_DATA,
    J1939_TP_RX_DATA_WAIT,
    J1939_TP_RX_ERROR,
    J1939_RX_DONE,
} J1939_TP_RX_STEP_t; /* 协议的接收步骤 */

/**J1939_TRANSPORT_RX_INFO_t 结构体
 *
 * 实现了长帧传输中产生的临时数据，和一些传输交换数据
 */
typedef struct {
    J1939_TP_MESSAGE_t  tp_rx_msg;      /**< J1939的消息对象*/
    j1939_uint8_t       osbusy;         /**< 此位置1，代表系统繁忙，cpu需要处理其他的事物，直接拒绝一切的链接请求\n 如果正在接受中，此位置1，则会发出链接保持消息帧。*/
    j1939_uint16_t      time;           /**< 时间*/
    j1939_uint8_t       packets_total;  /**< 总共有多少个数据包*/
    j1939_uint8_t       packets_ok_num; /**< 已经接受的数据包数*/
    J1939_TP_RX_STEP_t  state;          /**< 协议的接受步骤*/
} J1939_TRANSPORT_RX_INFO_t;

#endif /* J1939_TP_RX_TX */

/**
 * @note 实现Request_PGN 的响应
 */
struct RequestList {
    j1939_uint8_t       *data;
    j1939_uint16_t      length;
    j1939_uint32_t      PGN;
    CAN_NODE_t          Can_Node;
    void                (*update)();    /**< 在函数里需要对data更新，如果不用更新data赋值为J1939_NULL*/
    struct RequestList  *next;          /**< 链表末尾，需要一直保持J1939_NULL*/
};

/* J1939 Data Structures */
/* J1939_MESSAGE_STRUCT旨在J1939消息块映射到设备的地址映射。 只有字段PDU格式不映射到 */
/* 结构应该简单地使用PDUFormat和忽视PDUFormat_Top。调整将立即接收和传输之前 */
/* 注：编译器创建结构从低一点的位置高一些位置，所以可能出现不匹配的设备寄存器 */
#define J1939_MSG_LENGTH    9  /* 消息长度 */
#define J1939_DATA_LENGTH   8  /* 数据长度 */

/** J1939_MESSAGE_UNION 结构体
 *
 * 实现了J1939消息对象
 */
typedef union J1939_MESSAGE_UNION {
    /** J1939 的 ID 组成结构体
     *
     */
    struct J1939_PID {
        j1939_uint8_t  DataPage                 : 1;  /**< 数据页*/
        j1939_uint8_t  Res                      : 1;  /**< Res位*/
        j1939_uint8_t  Priority                 : 3;  /**< 优先级*/
        j1939_uint8_t                           : 3;  /**< 空闲*/
        j1939_uint8_t  PDUFormat;                     /**< PF*/
        j1939_uint8_t  PDUSpecific;                   /**< PS*/
        j1939_uint8_t  SourceAddress;                 /**< SA*/
        j1939_uint8_t  DataLength               : 4;  /**< 数据长度*/
        j1939_uint8_t  RTR                      : 4;  /**< RTR位*/
        j1939_uint8_t  Data[J1939_DATA_LENGTH];       /**< 数据*/
        j1939_uint32_t PGN                      : 24; /**< 参数群编号*/
        j1939_uint32_t                          : 8;  /**< 空闲*/
    } Mxe;                                            /**< J1939 的 ID 组成结构体*/
    j1939_uint8_t Array[J1939_MSG_LENGTH + J1939_DATA_LENGTH]; /**< 联合体数组，方便快速处理结构体赋值*/
} J1939_MESSAGE_t;

/** 一个宏定义，具体变量名称作用命名
 *
 */
#define GroupExtension      PDUSpecific
#define DestinationAddress  PDUSpecific

typedef union J1939_FLAGS_UNION {
    struct {
        j1939_uint8_t TransmitMessagesdCover  : 1;  /* 发送数据时，J1939协议接受缓存有数据覆盖 */
        j1939_uint8_t ReceivedMessagesdCoverOrDroppedNode : 3;
        j1939_uint8_t ReceivedMessagesdCover  : 1;  /* 接受数据时，J1939协议接受缓存有数据覆盖 */
        j1939_uint8_t ReceivedMessagesDropped : 1;  /* 接受数据时，J1939协议接受缓存有数据溢出 */
    };
    j1939_uint8_t FlagVal;
} J1939_FLAG_t;

/******************************API******************************/

/* 初始化函数 */
extern void J1939_Initialization(void);
/* CAN驱动收发中断入口 */
extern void J1939_ISR(void);
/* 心跳函数，定时被调用 */
extern void J1939_Poll(void);
/* 读取单帧消息 */
extern J1939_Status_t J1939_Read_Message(J1939_MESSAGE_t *MsgPtr, CAN_NODE_t _Can_Node);
/* 发送单帧消息 */
extern J1939_Status_t J1939_Send_Message(J1939_MESSAGE_t *MsgPtr, CAN_NODE_t _Can_Node);
#if J1939_TP_RX_TX
/* 多帧（多组）消息发送函数  (RTS/CTS传输协议) */
extern J1939_Status_t J1939_TP_TX_Message(j1939_uint32_t PGN, j1939_uint8_t DA, j1939_uint8_t *data,
                                          j1939_uint16_t data_num, CAN_NODE_t _Can_Node);
/* 多帧（多组）消息接受函数  (RTS/CTS传输协议) */
extern J1939_Status_t J1939_TP_RX_Message(TP_RX_MESSAGE_t *msg, CAN_NODE_t _Can_Node);
/* 请求获去一个PGN */
extern void J1939_Request_PGN(j1939_uint32_t PGN, j1939_uint8_t DA, CAN_NODE_t _Can_Node);
/* 创建一个PGN响应 */
extern void J1939_Create_Response(j1939_uint8_t data[], j1939_uint16_t dataLength, j1939_uint32_t PGN,
                                  void (*dataUPFun)(), CAN_NODE_t _Can_Node);
#endif /* J1939_TP_RX_TX */

#endif /* __J1939_H */
