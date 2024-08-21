# SOSP 2024 Artifact Evaluation

## Hardware Configuration
Our hardware configuration used in the paper is: 
* CPU: 4 socket Intel Xeon Gold 6234 3.3GHz with 8 cores per socket 
* RAM: 384GB
* NIC: 100Gbps NVIDIA Mellanox ConnectX-5

We run Ubuntu 20.04 (LTS) with Linux kernel v6.0.3. 

**[NOTE]:** We have provided our setup to use by artifact evaluators, so you may skip the instructions and jump right to the section below on running experiments. Otherwise, if you have not followed the [getting started instruction](https://github.com/host-architecture/Fast-and-Safe-IO-Memory-Protection?tab=readme-ov-file#2-getting-started-guide), please do so before continuing. 

## Running Experiments

Many experiments require the same scripts to be run with different kernels and IOMMU configurations: Linux, Linux + IOMMU On, Linux + IOMMU On + F&S. We have included which configurations are required for each figure (e.g. **[Linux, Linux + IOMMU On]** or **[Linux, Linux + IOMMU On, Linux + IOMMU On + F&S]**). Instructions for booting into the correct kernel configuration are below. To save time, we recommend running all the scripts before changing the configuration and then running them all again rather than changing the configuration for each figure. 

#### Loading different kernel configurations (12 reboot mins)
Assuming the kernels have already been compiled with the names in [Getting Started Guide](https://github.com/host-architecture/Fast-and-Safe-IO-Memory-Protection/tree/artifact_eval#2-getting-started-guide), the following instructions show how to setup each configuration.

To further simplify setup for artifact evaluators, we have provided pre-made grub files for each configuration.

**Linux (IOMMU Off):**

Copy the Linux with IOMMU Off grub file to the default grub file: 
```
sudo cp /etc/default/grub_linux_off /etc/default/grub
sudo update-grub2
```
Reboot the machine and confirm that the correct kernel (6.0.3+) is being used and that the IOMMU is disabled
```bash
uname -r
dmesg | grep -i "iommu"
```
**Linux + IOMMU On:**

Copy the Linux with IOMMU On grub file to the default grub file: 
```
sudo cp /etc/default/grub_linux_on /etc/default/grub
sudo update-grub2
```
Reboot the machine and confirm that the correct kernel (6.0.3+) is being used and that the IOMMU is disabled
```bash
uname -r
dmesg | grep -i "iommu"
```

**Linux + IOMMU On + F&S:**

Copy the Linux with IOMMU On grub file to the default grub file: 
```
sudo cp /etc/default/grub_linux_fs /etc/default/grub
sudo update-grub2
```
Reboot the machine and confirm that the correct kernel (6.0.3fands+) is being used and that the IOMMU is disabled
```bash
uname -r
dmesg | grep -i "iommu"
```

### Detailed Instructions 
Now we provide how to use our scripts to reproduce the results in the paper. All scripts assume the working directory is `Fast-and-Safe-IO-Memory-Protection/scripts/sosp24-experiments`. The compute time listed is per-configuration. 

#### Figure 2 - Varying Number of Flows (20 compute-mins)
**[Linux, Linux + IOMMU On]**
Make sure the current kernel configuration is Linux with IOMMU off. Follow the instructions to [load different kernel configurations](https://github.com/host-architecture/Fast-and-Safe-IO-Memory-Protection/tree/artifact_eval/scripts/sosp24-experiments#loading-different-kernel-configurations) to change between Linux and Linux + IOMMU On configurations.  

```bash
./flows_exp.sh
```

**[NOTE]:** This does not generate part (e) of figure 2. This also applies to part (e) of figures, 3, 5, 6. Please see section [Logging IOVA Locality](https://github.com/host-architecture/Fast-and-Safe-IO-Memory-Protection/tree/artifact_eval/scripts/sosp24-experiments#logging-iova-locality-figures-2-3-5-6-part-e) if you would like to generate this figure. 

####  Figure 3 - Varying Ring Buffer Size (20 compute-mins)
**[Linux, Linux + IOMMU On]**
```bash
./ringbuffer_exp.sh
```


#### Figure 5 - Varying Number of Flows (20 compute-mins)
**[Linux + IOMMU On + F&S]**

Follow [these instructions](https://github.com/host-architecture/Fast-and-Safe-IO-Memory-Protection/tree/artifact_eval/scripts/sosp24-experiments#loading-different-kernel-configurations) to change the kernel to F&S and enable the IOMMU before running the command below.
```bash
./flows_exp.sh
```
Please re-use the Figure 2 results for Linux and Linux + IOMMU On configurations in this figure. 

#### Figure 6 - Varying Ring Buffer Size (20 compute-mins)
**[Linux + IOMMU On + F&S]**

Please re-use the Figure 3 results for Linux and Linux + IOMMU On configurations. 
```bash
./ringbuffer_exp.sh
```

#### Figure 7 - Latency (6 compute-mins)
**[Linux, Linux + IOMMU On, Linux + IOMMU On + F&S]**
```bash
./latency_fig7.sh
```

### Figure 8 - hostCC
**IOMMU Off results** (20 compute-mins)

For figure 8a, run the following with **[Linux]** (IOMMU Off):
```bash
./no_hcc_fig8.sh
./hcc_fig8.sh
```
**IOMMU On results** (25 compute-mins)

For figure 8b run the following, with the correct kernel configuration: 
Linux (red bar) **[Linux + IOMMU On]**
```bash
./no_hcc_fig8.sh
```
Linux + hostCC (pink bar) **[Linux + IOMMU On]**
```
./hcc_fig8.sh
```
Linux + hostCC + F&S (green bar) **[Linux + IOMMU On + F&S]**
```
./hcc_fig8.sh
```

### Figure 9 - F&S Performance Breakdown (10 reboot-min + 20 compute-mins)
For this experiment, we provide an additional kernel that only implements one of the F&S key ideas: preserving page table caches during IOTLB invalidations. Please follow these instructions to boot into it:

Copy the Linux with IOMMU cache preservation grub file to the default grub file: 
```
sudo cp /etc/default/grub_linux_preserve /etc/default/grub
sudo update-grub2
```
Reboot the machine and confirm that the correct kernel (6.0.3preserve+) is being used and that the IOMMU is disabled
```bash
uname -r
dmesg | grep -i "iommu"
```

Run the experiment with the updated kernel:
```bash
./ringbuffer_exp.sh
```
please use the results from the figure 3 experiment for Linux + IOMMU On and Linux + IOMMU On + F&S configurations. 

### Logging IOVA Locality (Figures 2, 3, 5, 6 part (e))
Generating the IOVA locality results requires changing the kernel to print the IOVA allocations in the mellanox driver datapath, which we disable for performance purposes. Logging the allocations would require compiling and booting into a separate kernel that enables this logging for each of **[Linux + IOMMU Off]** and **[Linux + IOMMU Off + F&S]**, which would add a significant amount of time to the artifact evaluation. Thus, we have not included it. However, if you would like to generate these results, we can prepare the setup for you upon request. 