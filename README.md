# F&S: Fast & Safe IO Memory Protection
Memory protection is a security property that prevents an errant network interface card (NIC) from reading/writing to regions of host memory not mapped for DMA. The Input-Output Memory Management Unit (IOMMU) provides this protection via address translation: the NIC is given an IO Virtual Address (IOVA) that must be translated to a physical address by the IOMMU. This translation is sped up by an IOTLB that maps virtual addresses to physical addresses. Upon an IOTLB miss, the IOMMU must walk an IO page table sitting in host memory to obtain the virtual address. In addition to the IOTLB, modern IOMMUs contain a series of caches that cache intermediate page table entries that can speed up the page table walk. 

The key insight of F&S is that instead of focusing on reducing the IOTLB misses (which cannot be avoided with the strictest memory protection), we should reduce the *cost* of each miss by leveraging the IOMMU page table caches. With this perspective shift, we implement a few simple changes inside the Linux kernel to achieve memory protection with near-zero overhead. The main design ideas of F&S are:

* Allocate contiguous IOVAs within each Tx/Rx ring buffer, ensuring an access pattern with good locality in the IOMMU
* Disable unnecessary invalidations of the IOMMU page table caches and only invalidate when the page table structure changes
* Within each descriptor, batch DMA unmaps (which depends on contiguous IOVAs), saving CPU cycles, especially for the costly IOTLB invalidations


## 1. Overview
### Repository overview
- *fands.patch* Linux kernel patch for F&S.
- *utils/* contains tools to re-create IOMMU congestion scenarios and to log IOMMU, CPU and app level performance
- *scripts/* contains scripts to run the experiments in the SOSP'24 paper. 

### System overview
For simplicity, we assume that users have two physical servers (Receiver and Sender) connected with each other over a network. Receiver server enables IOMMU address translation. Then Sender server runs high throughput applications using the standard Linux kernel TCP stack. F&S enables the server to receive data at high throughput by reducing the address translation overhead.

## Artifact Evaluation Instructions
To keep things convenient for the artifact evaluators of SOSP'24, we have provided a setup on our servers with the F&S Kernel as well as utilies for running experiments and logging results already installed. Thus, artifact evaluators can skip the [Getting Started Guide](https://github.com/host-architecture/Fast-and-Safe-IO-Memory-Protection/tree/artifact_eval#2-getting-started-guide). Please refer to "[scripts/sosp24-experiments](https://github.com/host-architecture/Fast-and-Safe-IO-Memory-Protection/tree/artifact_eval/scripts/sosp24-experiments)" for instructions on running the experiments to re-create the results in the paper.  

## 2. Getting Started Guide
The following sections provide instructions for:
 * Building the F&S kernel
 * Booting into the kernel 
 * Installing and configuring the utilities to run benchmarks


The detailed instructions to reproduce all individual results presented in our SOSP'24 paper is provided in the "[sosp24-experiments](https://github.com/host-architecture/Fast-and-Safe-IO-Memory-Protection/tree/artifact_eval/scripts/sosp24-experiments)" directory.

## Build F&S Kernel (with root) 
F&S has been successfully tested on Ubuntu 20.04 LTS with kernel 6.0.3. Building the F&S kernel should be done on the server and is not necessary for the traffic generating client.

**(Don't forget to be root)**
1. Download Linux kernel source tree:
   ```
   sudo -s
   cd ~
   wget https://mirrors.edge.kernel.org/pub/linux/kernel/v6.x/linux-6.0.3.tar.gz
   tar xzvf linux-6.0.3.tar.gz
   ```

2. Apply the F&S patch 

   ```
   git clone -b artifact_eval --single-branch git@github.com:host-architecture/Fast-and-Safe-IO-Memory-Protection.git
   cp ~/Fast-and-Safe-IO-Memory-Protection/fands.patch ~/linux-6.0.3/
   cd linux-6.0.3
   patch -p1 --ignore-whitespace < ~/Fast-and-Safe-IO-Memory-Protection/fands.patch
   ```

3. Update kernel configuration:

   ```
   cp /boot/config-x.x.x .config
   make olddefconfig
   ```
   "x.x.x" is a kernel version. It can be your current kernel version or latest version your system has. Type "uname -r" to see your current kernel version.  
 
   Edit ".config" file to include your name in the kernel version.
   ```
   vim .config
   (in the file)
   ...
   CONFIG_LOCALVERSION="-(your name)"
   ...
   ```
   Save the .config file and exit.   

4. Compile and install:

   ```
   (See NOTE below for '-32')
   make -j32 bzImage
   make -j32 modules
   make -j32 modules_install
   make -j32 install
   ```
   NOTE: The number 32 means the number of threads created for compilation. Set it to be the total number of cores of your system to reduce the compilation time. Type "lscpu | grep 'CPU(s)'" to see the total number of cores:
   
   ```
   CPU(s):                32
   On-line CPU(s) list:   0-31
   ```


### Running F&S Kernel (with root)

One can run F&S by booting into the kernel module and enabling the IOMMU. 
Since the kernel was built from a modified source, the name might have a '+' appended to it. You can check the image name by running:
```
ls /boot | grep 6.0.3-(your name)
``` 

Edit `/etc/default/grub` to boot with **[Linux 6.0.3 + F&S]** as default. Comment all `GRUB_DEFAULT=` lines except for the 6.0.3-(your name) version. For example:
```bash
#GRUB_DEFAULT="1>Ubuntu, with Linux 3.4.5"
GRUB_DEFAULT="1>Ubuntu, with Linux 6.0.3-(your name)+"
#GRUB_DEFAULT="1>Ubuntu, with Linux 5.4.3"
```
At the bottom of the `/etc/default/grub` file, make sure the kernel command line enables the IOMMU in strict mode:
```bash
GRUB_CMDLINE_LINUX_DEFAULT="intel_iommu=on iommu.strict=1"
#GRUB_CMDLINE_LINUX_DEFAULT="intel_iommu=off"
```
Update the grub configuration and reboot into the new kernel.

   ```
   update-grub && reboot
   ```

When the system is rebooted confirm that it is using the correct kernel (should be 6.0.3-(your name)) and that the IOMMU is enabled in strict mode.
```bash
uname -r
dmesg | grep -i "iommu"
```

### Installing required utilities

Instructions to install the set of benchmarking applications and measurement tools (for running similar experiments as in our SOSP'24 paper) is provided in the README in the *utils/* directory. 
+ Benchmarking applications: We use **iperf3** as network app generating throughput-bound traffic, **netperf** as network app generating latency-sensitive traffic, and **mlc** as the CPU app generating memory-intensive traffic.
+ Measurement tools: We use **Intel PCM** for measuring the host-level metrics and **Intel Memory Bandwidth Allocation** tool for performing host-local response. We also use **sar** utility to measure CPU utilization.

### Specifying desired experimental settings

Desired experiment settings, for eg., enabling DDIO, configuring MTU size, number of clients/servers used by the network-bound app, enabling TCP optimizations like TSO/GRO/aRFS (currently TCP optimizations can be configured using the provided script in this repo only for Mellanox CX5 NICs), etc can tuned using the script *utils/setup-envir.sh*. Run the script with -h flag to get list of all parameters, and their default values.  
```
sudo bash utils/setup-envir.sh -h
```



### Running benchmarking experiments

To help run experiments similar to those in SOSP'24 paper, we provide following scripts in the *scripts/* directory:

+ *run-hostcc-tput-experiment.sh*
+ *run-hostcc-latency-experiment.sh*

Run these scripts with -h flag to get list of all possible input parameters, including specifying the home directory, client/server IP addresses, experimental settings as discussed above. The output results are generated in *utils/reports/* directory, and measurement logs are stored in *utils/logs/*.


## 3. Reproducing results in the SOSP '24 paper. 

For ease of use, we also provide wrapper scripts inside the "[scripts/sosp24-experiments](https://github.com/host-architecture/Fast-and-Safe-IO-Memory-Protection/tree/artifact_eval/scripts/sosp24-experiments)" directory which run identical experiments as in the SOSP'24 paper in order to reproduce our key evaluation results. Refer to the README for details on how to run the scripts. 

