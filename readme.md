
# 历史版本变更 


> 版本说明：V a,b,c
 1. a 代表版本号
 2. b 代表稳定的版本号
 3. c 代表基于稳定版本号上功能添加，新的功能不一定稳定

> 如果是工程使用，建议使用 **V x.x.0**  例如 ***V 1.1.0***

## J1939Socket API Version 2 

Version  | date |Description
------------- | ------------- | -------------
V2.0.1  |2017/12/8| 地址竞争，动态地址分配等J1939网络功能不能使用，本版本为V2.1.0发布前的测试版本。


## J1939Socket API Version 1   

Version  | date | Description
------------- | ------------- | -------------
[V1.1.0]  | 2017/11/22 | Version 1 稳定发布版。\n * 实现了J1939-21文档规定的功能（数据链路层）。\n * 轻量级（可适应低端的MCU）建议低端的MCU采用本版本移植开发。\n * 使用示例参考附带的readme.md和<http://blog.csdn.net/xietongxueflyme> \n * 移植示例参考 <http://blog.csdn.net/xietongxueflyme> 
[V1.0.1]  | 2017/08/04 | 完善功能,增加对TP(长帧，多组)传输的支持,\n 1.增加非阻塞API调用接口 \n * 使用示例参考附带的readme.md和<http://blog.csdn.net/xietongxueflyme> \n * 移植示例参考 <http://blog.csdn.net/xietongxueflyme> \n * 本文档不对Version 1 进行阐述。
V1.0.0  | 2017/06/04 | 首个开源版本\n 1.增加双模式（轮询或者中断，逻辑更加简单明了）\n 2.可适应低端的MCU \n 3.支持多任务调用接口（可用于嵌入式系统）
V0.0.1  | 2017/05/04 | 初建工程\n * 易移植(不针对特定的CAN硬件，只要满足CAN2.0B即可)

[V1.1.0]: https://github.com/XeiTongXueFlyMe/J1939/releases/tag/v1.1.0  "V1.1.0下载地址"
[V1.0.1]: https://github.com/XeiTongXueFlyMe/J1939/releases/tag/V1.01  "V1.0.1下载地址" 
[V2.0.1]: https://github.com/XeiTongXueFlyMe/J1939/releases  "V2.0.1下载地址"


