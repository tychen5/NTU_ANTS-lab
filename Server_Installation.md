# SuperPC building
## BIOS
* Update to latest one
* ~~*Use CPU in-built GPU (IGFX-only for default)*~~
* Set automatic Overclock CPU and RAM (X.M.P)

## OS
* Ubuntu 18.04 LTS Server(latest version,without GUI)

## System Settings
* ssh change port
* swap file in nvme (/swap/swap)
* data dir in nvme (/data)

## GPU
* NVIDIA driver for RTX 2080TI
* CUDA 10.1 (latest version)
* Cudnn library (corresponding latest version)

## Packages
* LDAP client (.43 is server)
* NFS client (.43 /home、.45 /FTP)
* pygraphviz
* proftp
* denyhost

## GPU Ultimate Goal: Ubuntu18.04.X LTS+Cuda10.X+Cudnn7.X+Pytorch1.X
* Best Ref: https://www.imooc.com/article/282412

***

Bios更新到官網最新版本，設定系統自動化超頻cpu, ram，~~改成僅用cpu內顯開機（IGFX?）~~。

Ubuntu 18.04最新server版本，安裝在2TB nvme硬碟上，設定一個空的固定140.112.107.xxx IP。

設定ssh服務為3389 port

安裝Nvidia 最新的驅動（非short term版本），確保指令nvidia-smi可以抓到四張顯卡。ref: https://blog.csdn.net/zsf211/article/details/86603319

https://www.geforce.com/drivers

https://askubuntu.com/questions/1079852/how-do-i-configure-an-nvidia-rtx-2080-ti-with-ubuntu

Cuda 10.1 + 對應之最新版cudnn developer跟runtime library還有cudnn-10.1-linux-x64-v7.XXX.tgz。參考：https://www.cnblogs.com/zhengbiqing/p/10346212.html (&參考Best Ref怎麼放置檔案)

格式化一個nvme硬碟，自動掛載至根目錄新目錄的掛載點並寫入fstab，創建檔案將該硬碟全部做成swap，並寫入fstab。參考：
https://blog.gtwang.org/linux/linux-add-format-mount-harddisk/

https://www.digitalocean.com/community/tutorials/how-to-add-swap-space-on-ubuntu-16-04

安裝LDAP client去跟.43介接。參考：https://www.server-world.info/en/note?os=Ubuntu_18.04&p=openldap&f=3

安裝NFS client掛載.43 /home到自己的/home，並寫入fstab，請注意本地端帳號將被覆蓋記得備份轉移重建在nfs上面。 (掛載.45的/FTP)

安裝proftp (standalone)，並設定讓大家可以用ldap帳號登入自己的home目錄上傳下載資料。

安裝pygraphviz。參考：https://stackoverflow.com/questions/15661384/python-does-not-see-pygraphviz

https://quantitativenotes.wordpress.com/2016/05/21/pygraphviz-installation/

開sudo權限給r06725035、你自己跟學姊帳號

安裝denyhost，防止暴力嘗試登入（5次鎖IP），其他設定可以自由發揮（例如如何自動解鎖），但要不限IP位址都可登入

 ~~格式化另外一個nvme硬碟，~~ 掛載/DATA(.45的/SAMBA NFS)  ~~，chmod 777該目錄，並寫入fstab給大家存資料跟model~~


=========APC Powersuite Ubuntu
google: apc powerchute ubuntu server
[ubuntu] APC PowerChute Network Shutdown

## Deprecated
https://www.imooc.com/article/282412

===========
1. 拔顯卡安裝好ubuntu，如果BIOS當中有CSM legacy，那不要用force AHCI安裝。關閉secure boot (建議安裝server版本)
2. 開啟遠端SSH，進入SSH系統sudo身分編輯/etc/modprobe.d/blacklist.conf
在檔案尾端加入以下:
blacklist amd76x_edac
blacklist vga16gb
blacklist nouveau
blacklist rivafb
blacklist nvidiafb
blacklist rivatv
blacklist lbm-nouveau
options nouveau modeset=0
alias nouveau off
alias lbm-nouveau off
=以下指令=
echo options nouveau modeset=0 | sudo tee -a /etc/modprobe.d/nouveau-kms.conf
sudo update-initramfs -u
reboot
3. 解安裝舊的東西:
apt-get purge nvidia-*
apt autoremove 
apt-get remove --purge nvidia*
dpkg --configure -a

echo options nouveau modset=0
sudo update-initramfs -u
shutdown
接上顯卡重新開機

4. 停止GUI，sudo :
service lightdm stop 
killall xinit 
cd ~/Downloads/
**=> https://www.nvidia.com.tw/object/unix-tw.html
sudo wget http://us.download.nvidia.com/XFree86/Linux-x86_64/${VERSION}/NVIDIA-Linux-x86_64-${VERSION}.run
chmod +x NVIDIAXXX  (version可以自行替換，google unix版本號http://www.nvidia.com/object/unix.html)
(要先跑維護指令，sudo apt-get install build-essential -y)
./NVIDIA-Linux-x86_64-XXX

sudo apt-get install freeglut3-dev libx11-dev libxmu-dev libxi-dev libgl1-mesa-glx libglu1-mesa libglu1-mesa-dev

shutdown -r now

5. CUDA 8 download local deb version:
http://docs.nvidia.com/cuda/cuda-installation-guide-linux/#axzz4VZnqTJ2A
sudo service lightdm stop
sudo systemctl set-default multi-user.target
sudo sh cuda_10.0.130_410.48_linux.run –no-opengl-files
`sudo dpkg -i cuda-repo-ubuntu1804-10-0-local-10.0.130-410.48_1.0-1_amd64.deb`
`sudo apt-key add /var/cuda-repo-<version>/7fa2af80.pub`
`sudo apt-get update`
`sudo apt-get install cuda-10.0`

runfile(local)方法版本:
`sudo sh cuda_10.0.130_410.48_linux.run`

export PATH=/usr/local/cuda-8.0/bin${PATH:+:${PATH}}
export LD_LIBRARY_PATH=/usr/local/cuda-8.0/lib64${LD_LIBRARY_PATH:+:${LD_LIBRARY_PATH}}

export CUDA_HOME=/usr/local/cuda
export PATH=$PATH:$CUDA_HOME/bin
export LD_LIBRARY_PATH=/usr/local/cuda-10.0/lib64${LD_LIBRARY_PATH:+:${LD_LIBRARY_PATH}}

6.cudnn 6 :
download runtime
sudo dpkg -i  XXX
 (in .bashrc file enter:)
  export PATH=/usr/local/cuda-8.0/bin:$PATH export LD_LIBRARY_PATH=/usr/local/cuda-8.0/lib64:$LD_LIBRARY_PATH

cudnn7.5:
下載四個東西放進去FTP以後
tar -zxvf cudnn-10.0-linux-x64-v7.5.0.56.tgz
sudo cp cuda/include/cudnn.h /usr/local/cuda/include
sudo cp cuda/lib64/libcudnn* /usr/local/cuda/lib64
sudo chmod a+r /usr/local/cuda/include/cudnn.h 
sudo chmod a+r /usr/local/cuda/lib64/libcudnn*

sudo dpkg -i libcudnn7_7.5.0.56-1+cuda10.0_amd64.deb
sudo dpkg -i libcudnn7-dev_7.5.0.56-1+cuda10.0_amd64.deb
sudo dpkg -i libcudnn7-doc_7.5.0.56-1+cuda10.0_amd64.deb


7. download anaconda
https://www.anaconda.com/download/#linux
https://www.digitalocean.com/community/tutorials/how-to-install-the-anaconda-python-distribution-on-ubuntu-16-04

conda update conda
conda update anaconda
conda install nb_conda
======可能指令=====
(usb mount :)
mount -v -t auto /dev/sdb /mnt/usb

ubuntu install ref: http://abhay.harpale.net/blog/linux/nvidia-gtx-1080-installation-on-ubuntu-16-04-lts/
tf ref: https://www.tensorflow.org/install/install_linux

=========TF 1.7===================
cuda-9.1:
依據官網下載時依照nvidia給的步驟 https://developer.nvidia.com/cuda-downloads?target_os=Linux&target_arch=x86_64&target_distro=Ubuntu&target_version=1604&target_type=debnetwork
下載完以後指令改成以下
`sudo dpkg -i cuda-repo-ubuntu1604_9.0.176-1_amd64.deb`
`sudo apt-key adv --fetch-keys http://developer.download.nvidia.com/compute/cuda/repos/ubuntu1604/x86_64/7fa2af80.pub`
`sudo apt-get update`
`sudo apt-get install cuda-9.1`

cudnn7.1.X:
官網下載時有pdf install guide可以參考 https://developer.nvidia.com/rdp/cudnn-download        
 linux library( Library for Linux) 跟 ubuntu runtime(Runtime Library for Ubuntu16.04 (Deb))

前者用解壓縮的方式 tar -xzvf cudnn-9.1-linux-x64-v7.OOXX.tgz ;  sudo cp cuda/include/cudnn.h /usr/local/cuda/include ; sudo cp cuda/lib64/libcudnn* /usr/local/cuda/lib64 ; sudo chmod a+r /usr/local/cuda/include/cudnn.h /usr/local/cuda/lib64/libcudnn*

後者用安裝的方式 sudo dpkg -i libcudnn7_7.1.3.11-1+cuda9.1_amd64.deb

(上面打完基本上就好了)
###如果你遇到/sbin/ldconfig.real: /usr/local/cuda/lib64/libcudnn.so.5 is not a symbolic link这样的问题，重新链接一下(根据版本自行调整)：

cd /usr/local/cuda/lib64

sudo ln -sf libcudnn.so.7.1.3 libcudnn.so.7

sudo ln -sf libcudnn.so.7 libcudnn.so


ANACONDA: https://www.anaconda.com/download/#linux

=====================
REF: http://www.python36.com/install-tensorflow141-gpu/
