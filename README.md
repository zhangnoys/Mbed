# 基于nucleo的传感器MQTT上传平台
![nucleo]
https://image.baidu.com/search/detail?ct=503316480&z=0&ipn=d&word=nucleo%20f411&step_word=&hs=0&pn=0&spn=0&di=3530&pi=0&rn=1&tn=baiduimagedetail&is=0%2C0&istype=0&ie=utf-8&oe=utf-8&in=&cl=2&lm=-1&st=undefined&cs=1513382988%2C7248150&os=3011320968%2C2209835981&simid=3460300684%2C557432004&adpicid=0&lpn=0&ln=273&fr=&fmq=1593482060276_R&fm=&ic=undefined&s=undefined&hd=undefined&latest=undefined&copyright=undefined&se=&sme=&tab=0&width=undefined&height=undefined&face=undefined&ist=&jit=&cg=&bdtype=15&oriquery=&objurl=http%3A%2F%2Fwww.pianshen.com%2Fimages%2F419%2F24e391779594eb8b98c49f1d68ff5df3.JPEG&fromurl=ippr_z2C%24qAzdH3FAzdH3Fooo_z%26e3Brtwgfijg_z%26e3Bv54AzdH3Fw6ptvsjAzdH3F9nm89adbdmAzdH3F&gsm=1&rpstart=0&rpnum=0&islist=&querylist=&force=undefined
## 所用到的器件介绍
### F411RE：
#### 基于Cortex-M4内核的F4微控制器的stm32开发板。
### X-NUCLEO-IKS01A2：
#### nucleo出品的集加速计、陀螺仪、湿度、磁力仪、压力、温度为一体的开发版，可直接与nucleo的F411RE配合使用。

####
## 概述：
本平台基于stm32开发，通过wifi连接上阿里云的mqtt服务器，把stm32单片机内连接的传感器的数值push到服务器之中。通过node-red的后台，访问浏览器，可以看到各种数据的可视化表格。

## 硬件方案：
 Demo版本的硬件方案采用的Nucleo的F411RE作为核心板，连接上X-NUCLEO-IKS01A2来获取传感器的各种的数值，再连接上IDW01M1作为数据的收发。
### 关于Nucleo
Nucle的开发板仅仅用一个usb口就集成了串口、otg、烧录功能，不仅非常简洁，也不再依赖Windows平台下的烧录工具，大大减少了开发者的负担，以及提升桌面的简洁程度和出错的可能。

## 服务端的方案：
采用阿里云的server,系统用的是Ubuntu的64bit的版本，安装上red-node、mosqutto的套件，并在网口端开启1880和1883端口，分别作为red-node效果的展示端和数据的发送端。
### 关于node-red
node-red拥有非常多的插件，可以实现非常丰富的功能，这里仅仅采用了最基本的表格功能，这些插件需要在控件中手动安装。
### 关于阿里云服务器的配置
在购买阿里云的服务器之后还需要再额外购买公网ip服务，已实现对服务器的端口访问，同时需要对node-red作开机自启动配置，使得开机就能自动运行，省去手动命令运行这一步骤。
## 软件实现的思路
采用mbed开发，省去了对于库函数的配置，使用最少的代码量及可以实现功能，由于是采用云端的服务器，所以在网络连接畅通的情况下，编译速度费非常快，同时Mbed还可以把工程调出为keil的执行文件，以便方便在遇到问题的时候精准调试。
在代码的实现中，首先是在Mbed中先添加mqtt客户端的库，再是调用Wifi及传感器的模块函数，为保证软件稳定性，还需要做一个看门狗，在While循环中每十秒喂一次狗，以防止软件在获取传感器数据时时间过长而导致卡死的问题。
#### 写在最后
在Mbed这一版本中，导出keil的复杂工程还是会有一定的问题，使用keil上的debug还是会有非常多的bug。
