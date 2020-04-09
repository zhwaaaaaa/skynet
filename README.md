# skynet
skynet 是一个轻量级的rpc调用中间件，类似于service mesh，主要目的是降低跨语言rpc调用的复杂程度。目前实现了消息转发,zookeeper服务注册发现功能，后续陆续实现监控等功能。
## 架构

## build
目前暂时只测试过ubuntu编译 
### 安装依赖
`apt install cmake libgoogle-glog-dev libuv1-dev libgflags-dev libzookeeper-mt-dev libboost-all-dev`

### 可选依赖
 `apt install libtcmalloc-minimal4`

### 编译
`mkdir build && cd build && cmake .. && make skynet`

### 运行
`./skynet`
