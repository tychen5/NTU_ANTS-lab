# Ubuntu 18.04, Cuda 10, CudaNN 7, Pytorch

## Spec
- Ubuntu 18.04 LTS Server
- RTX 2080 Ti * 3
- NVIDIA DRIVER 418.56
- CUDA 10.1
- CudaNN 7
- Anaconda 3.7
- Pytorch

## Update MSI BIOS
See: [微星主機板必看BIOS更新教學詳解](https://forum-tc.msi.com/index.php?topic=46982.0)

## Install Ubuntu 18.04 Server
- 下載iso檔燒至USB後boot
- 在network config 階段如果網路線無法正常連到switch，可以考慮下載不是live版的iso檔

See: [如何安裝Ubuntu Server 18.04來架設伺服器？](https://magiclen.org/ubuntu-server-18-04/)

### Static IP Configuration
See: [Configure Static IP Addresses On Ubuntu 18.04 LTS Server](https://websiteforstudents.com/configure-static-ip-addresses-on-ubuntu-18-04-beta/)


## Install NVIDIA Driver 418.56 (runfile)
先去官網找顯卡最新的Driver ->
[Driver](https://www.nvidia.com/Download/index.aspx?lang=en-us)

```
wget "http://us.download.nvidia.com/XFree86/Linux-x86_64/418.56/NVIDIA-Linux-x86_64-418.56.run"

chmod +x NVIDIA-Linux-x86_64-418.56.run
sudo ./NVIDIA-Linux-x86_64-418.56.run

sudo reboot
```

### Checking
```
nvidia-smi # should list the GPU device
```

## Install CUDA 10.1 (runfile)
```
wget https://developer.nvidia.com/compute/cuda/10.1/Prod/local_installers/cuda_10.1.105_418.39_linux.run

# install dependency
sudo apt-get install gcc
sudo apt-get install make

chmod +x cuda_10.1.105_418.39_linux.run
sudo ./cuda_10.1.105_418.39_linux.run
```

### Set environment variable
```
# vim /home/user/.profile
export PATH="/usr/local/cuda/bin:$PATH"
export LD_LIBRARY_PATH="/usr/local/cuda/lib64:$LD_LIBRARY_PATH"

source .profile
```

### Version Check
```
nvcc -V
```


See: [CUDA Download](https://developer.nvidia.com/cuda-downloads?target_os=Linux&target_arch=x86_64&target_distro=Ubuntu&target_version=1710&target_type=runfilelocal)
, [CUDA Installation Guide](https://docs.nvidia.com/cuda/cuda-installation-guide-linux/index.html)

## Install CudaNN 7
```
wget "https://developer.nvidia.com/compute/machine-learning/cudnn/secure/v7.5.0.56/prod/10.1_20190225/cudnn-10.1-linux-x64-v7.5.0.56.tgz"

tar -xzvf cudnn-10.1-linux-x64-v7.5.0.56.tgz

sudo cp cuda/lib64/* /usr/local/cuda-10.0/lib64/
sudo cp cuda/include/* /usr/local/cuda-10.0/include
sudo chmod a+r /usr/local/cuda-10.0/include/cudnn.h

```

### Version Check
```
cat /usr/local/cuda/include/cudnn.h | grep CUDNN_MAJOR -A 2
```

See: [Deep Learning Software Setup: CUDA 10 + Ubuntu 18.04](https://hackernoon.com/deep-learning-software-setup-cuda-10-ubuntu-18-04-15548cefa30)

## Install Anaconda 3.7
```
wget https://repo.anaconda.com/archive/Anaconda3-2019.03-Linux-x86_64.sh

bash Anaconda3-2019.03-Linux-x86_64.sh
```
See: [Anaconda Install](https://www.anaconda.com/distribution/#download-section)

## Install Pytorch using pip
```
pip3 install https://download.pytorch.org/whl/cu100/torch-1.0.1.post2-cp37-cp37m-linux_x86_64.whl
pip3 install torchvision
```
See: [Pytorch Install](https://pytorch.org/get-started/locally/)


## Check if Pytorch can access CUDA & CudaNN
```python
In [1]: import torch

In [2]: torch.cuda.current_device()
Out[2]: 0

In [3]: torch.cuda.device(0)
Out[3]: <torch.cuda.device at 0x7efce0b03be0>

In [4]: torch.cuda.device_count()
Out[4]: 1

In [5]: torch.cuda.get_device_name(0)
Out[5]: 'GeForce RTX 2080 Ti'

In [6]: torch.backends.cudnn.version()
Out[6]: 7402
```
See: [Pytorch + CUDA](https://stackoverflow.com/questions/48152674/how-to-check-if-pytorch-is-using-the-gpu)
