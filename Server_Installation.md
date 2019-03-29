# SuperPC building
## BIOS
* Update to latest one
* Use CPU in-built GPU (IGFX-only for default)
* Set automatic Overclock CPU and RAM

## OS
* Ubuntu 18.04 LTS Server(latest version,without GUI)

## System Settings
* ssh change port
* swap file in nvme (/swap/swap)
* data dir in nvme (/data)

## GPU
* NVIDIA driver for RTX 2080TI
* CUDA 10.1
* Cudnn library (corresponding latest version)

## Packages
* LDAP client (.43)
* NFS client (.43 /home、.45 /FTP)
* pygraphviz
* proftp
* denyhost

## GPU Ultimate Goal: Ubuntu18.04.LTS+Cuda10.+Cudnn7.+Pytorch1.
* Best Ref: https://www.imooc.com/article/282412

***

Bios更新到官網最新版本，設定系統自動化超頻cpu, ram，改成僅用cpu內顯開機（IGFX?）。

Ubuntu 18.04最新server版本，安裝在2TB nvme硬碟上，設定一個空的固定140.112.107.xxx IP。

設定ssh服務為3389 port

安裝Nvidia 最新的驅動（非short term版本），確保指令nvidia-smi可以抓到四張顯卡。ref: https://blog.csdn.net/zsf211/article/details/86603319

https://www.geforce.com/drivers

https://askubuntu.com/questions/1079852/how-do-i-configure-an-nvidia-rtx-2080-ti-with-ubuntu

Cuda 10.1 + 對應之最新版cudnn developer跟runtime library。參考：https://www.cnblogs.com/zhengbiqing/p/10346212.html

格式化一個nvme硬碟，自動掛載至根目錄新目錄的掛載點並寫入fstab，創建檔案將該硬碟全部做成swap，並寫入fstab。參考：
https://blog.gtwang.org/linux/linux-add-format-mount-harddisk/

https://www.digitalocean.com/community/tutorials/how-to-add-swap-space-on-ubuntu-16-04

安裝LDAP client去跟.43介接。參考：https://www.server-world.info/en/note?os=Ubuntu_18.04&p=openldap&f=3

安裝NFS client掛載.43 /home到自己的/home，並寫入fstab，請注意本地端帳號將被覆蓋記得備份轉移重建在nfs上面。

安裝proftp (standalone)，並設定讓大家可以用ldap帳號登入自己的home目錄上傳下載資料。

安裝pygraphviz。參考：https://stackoverflow.com/questions/15661384/python-does-not-see-pygraphviz

https://quantitativenotes.wordpress.com/2016/05/21/pygraphviz-installation/

開sudo權限給r06725035、你自己跟學姊帳號

安裝denyhost，防止暴力嘗試登入（5次鎖IP），其他設定可以自由發揮（例如如何自動解鎖），但要不限IP位址都可登入

格式化另外一個nvme硬碟，掛載在/DATA，chmod 777該目錄，並寫入fstab給大家存資料跟model


=========APC Powersuite Ubuntu
google: apc powerchute ubuntu server
[ubuntu] APC PowerChute Network Shutdown


