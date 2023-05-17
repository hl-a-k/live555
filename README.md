For documentation and instructions for building this software,
see <http://www.live555.com/liveMedia/>

# 概述 
我们现有一个IPC工程，之所以引进live555,是为了：
* 支持TCP
* 方便后续扩展，特别是后续需要支持srtp

live555 是一个开源项目。针对项目本身，请参考官方文档。  
我重点记录下如何移植到ASI19, 如何创建SINK, 做了那些兼容性修改。


# 概述

* 创建 config.armlinux 
  重点是设置编译链，openssl库路径

* 生成Makefile  
    `./genMakefiles armlinux `

* 编译  
    `make`

* 将编译产物拷贝到设备
  我目前使用了 testRTSPClient, 所以将./testProgs/testRTSPClient拷到设备即可  
  使用以下命令即可执行  
  <code>
  export LD_LIBRARY_PATH=/usr/app/ipgw_lib:/usr/app/3rd_lib:/usr/app/lib:/usr/lib:/lib

  ./testRTSPClient "rtsp_uri"
  </code>  
  那个双引号是必须的，因为有些uri会有特殊字符。

# 创建一个SINK
live555可以将数据输出到终端，也可以生成mp4, avi等文件。
但如果要在panel上实时播放，则需要自定义SINK.  
请参考git上的commit： h264 sink  
以下是步骤：
* 创建 *sink.hh, *sink.cpp
  我是拷贝一个相似的sink
* 修改Makefile.tail 包含新创建的源文件。
  也是参考相关的sink
* 重新生成Makefile
`./config.armlinux `

* 修改testRTSPClient.cpp 使用相应的SINK
如果subsession的编码为"H264", 使用刚创建的sink处理，即通过udp将数据传给video进程，进行播放
如果subsession的编码为"PCMA", 使用自带的FileSink，输出至stdout, 再使用管道技术，实现声音的播放  
`./testRTSPClient "$1" | ffmpeg -i pipe: -f wav pipe:1 | aplay`

# 兼容性修改

* 解决了 SN-IPV57/41ADR/Z DESCRIBE 响应close 导致tcp重连，导致鉴权失败
* 用ipv4协议访问支持ipv6的摄像头

# TODO
* 针对不同的声音编码格式创建相应的SINK
* 创建一个新的H264 sink, 直接调用vpu+v4l2播放，不再使用video进程