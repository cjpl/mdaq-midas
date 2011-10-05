配置 Debian 下的 MIDAS 环境
===========================

在核数据实验室内部可以按如下步骤配置 Debian 6.0 下的 MIDAS 环境::

    1. 添加 Debian 内网软件源

       deb http://192.0.31.201/apt/CNDL/   bin-amd64/

    2. 更新软件包数据库

       $ sudo aptitude update

    3. 安装 midas 包 `mdaq-root`

       $ sudo aptitude install mdaq-root

    4. 安装 roody 包 `mdaq-roody`

       $ sudo aptitude install mdaq-roody

    5. 安装 sis1100 的内核驱动

       $ sudo aptitude install sis1100-dkms

完成以上步骤，你就已经配置好一个完整的并且支持 SIS1100/SIS3100 VME 硬件的 MIDAS
环境。

