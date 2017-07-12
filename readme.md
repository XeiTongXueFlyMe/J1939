﻿* 本程序是由XieTongXueFlyMe对现有的J1939协议文档分析，和对前辈的贡献总结和封装，写出的一套开源的J1939驱动。

# 本协议特点：
* 易移植（不针对特定的CAN硬件，只要满足CAN2.0B即可）
* 轻量级（可适应低端的MCU）
* 支持多任务调用接口（可用于嵌入式系统）
* 双模式（轮询或者中断，逻辑更加简单明了）
* 不掉帧（数据采用收发列队缓存）
		
# 协议参考文献：
	1. SAE J1939 J1939概述
	2. SAE J1939-01 卡车，大客车控制通信文档（大概的浏览J1939协议的用法）
	3. SAE J1939-11 物理层文档
	4. SAE J1939-13 物理层文档
	5. SAE J1939-15 物理层文档
	6. SAE J1939-21 数据链路层文档（定义信息帧的数据结构，编码规则）
	7. SAE J1939-31 网络层文档（定义网络层的链接协议）
	8. SAE J1939-71 应用层文档（定义常用物理参数格式）
	9. SAE J1939-73 应用层文档（用于故障诊断）
	10. SAE J1939-74 应用层文档（可配置信息）
	11. SAE J1939-75 应用层文档（发电机组和工业设备）
	12. SAE J1939-81 网络管理协议
		
# 协议格式：
* UTF-8
	   
# 源代码分析网址：
* <http://blog.csdn.net/xietongxueflyme/article/details/74908563>
  
# 源代码移植：
* <http://blog.csdn.net/xietongxueflyme/article/details/74923355>