# Setup for OpenLLM-XLS

## Step 0

First, make sure that you have at least **35 GB** memory space available on your system!  
&emsp;&emsp; FYR: The default hidden `~/.cache/bazel/_bazel_<username>/<temp_number>/` folder created by `bazel` contains `27.1 GB (1,208,354 files)`.

Then, run the following command to set up your OpenLLM-XLS:  
&emsp;&emsp; FYR: The bazel build command will take **at least 4 hours** on `WSL-Ubuntu 20.04.6`.

```
git clone git@github.com:Mars-Cat2023/OpenLLM-XLS.git
cd ./OpenLLM-XLS
bazel build -c opt xls/...
```

#### Friendly Notes:  
If you successfully build bazel, you will get message as follows:
```
INFO: Found 4641 targets...
INFO: Elapsed time: 36687.845s, Critical Path: 1042.00s
INFO: 52944 processes: 19000 internal, 33928 linux-sandbox, 16 worker.
INFO: Build completed successfully, 52944 total actions
```
However, if you stuck with any error when you running within 52,944 actions of `bazel build`,  
please double check that you have all the dependencies ready and run this `bazel build` command again!  
For example:
```
sudo apt update
sudo apt install -y build-essential g++ gcc clang llvm libz3-dev python3
bazel build -c opt xls/...
```

------
