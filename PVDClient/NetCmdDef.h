#ifndef __NETCMDDEV_H__
#define __NETCMDDEV_H__

//网络命令
#define NET_SDVR_LOGIN				0x00		//登录
#define NET_SDVR_LOGOUT				0x01		//注销

#define NET_SDVR_REAL_PLAY			0x02		//打开实时视频
#define NET_SDVR_REAL_STOP			0x03		//停止实时视频

#define NET_SDVR_VEFF_GET			0x04		//获取视频参数
#define NET_SDVR_VEFF_SET			0x05		//设置视频参数

#define NET_SDVR_KEYFRAME			0x06		//请求关键帧

#define NET_SDVR_PTZ_CTRL			0x07		//云台控制
#define NET_SDVR_PTZ_TRANS			0x08		//透明云台
#define NET_SDVR_CTRL				0x15		//主机控制(包括重启,关机等)
#define NET_SDVR_UPDATE_START		0x16		//升级控制
#define NET_SDVR_FORMAT_DISK		0x17		//格式化主机硬盘
#define NET_SDVR_FORMAT_STATE       0x18        //获取格式化状态
#define NET_SDVR_ALARM_GET			0x19		//获取报警输出
#define NET_SDVR_ALARM_SET			0x1A		//设置报警输出

#define NET_SDVR_VOICE_START		0x1B		//启动对讲操作
#define NET_SDVR_VOICE_STOP			0x1C		//停止对讲操作

#define NET_SDVR_SERIAL_START		0x1D		//启动透明通道
#define NET_SDVR_SERIAL_STOP		0x1E		//停止透明通道

#define NET_SDVR_CLICK_KEY			0x1F		//网络键盘

#define NET_SDVR_MANURECORD_SET		0x20		//设置手动录像
#define NET_SDVR_MANURECORD_GET		0x21		//获取手动录像

#define NET_SDVR_WORK_STATE			0x22		//获取主机工作状态

#define NET_SDVR_LOG_QUERY			0x23		//主机日志查询

#define NET_SDVR_DEVICECFG_GET		0x26		//获取设备硬件信息
#define NET_SDVR_DEVICECFG_SET		0x27		//设置设备硬件信息

#define NET_SDVR_NETCFG_GET			0x28		//获取设备网络参数
#define NET_SDVR_NETCFG_SET			0x29		//设置设备网络参数

#define NET_SDVR_PICCFG_GET			0x2A		//获取通道参数(包括报警录像,移动录像等)
#define NET_SDVR_PICCFG_SET			0x2B		//设置通道参数(包括报警录像,移动录像等)

#define NET_SDVR_COMPRESSCFG_GET	0x2C		//获取压缩参数
#define NET_SDVR_COMPRESSCFG_SET	0x2D		//设置压缩参数

#define NET_SDVR_RECORDCFG_GET		0X2E		//获取定时录像参数
#define NET_SDVR_RECORDCFG_SET		0X2F		//设置定时录像参数

#define NET_SDVR_DECODERCFG_GET		0X30		//获取解码器参数(云台参数)
#define NET_SDVR_DECODERCFG_SET		0X31		//设置解码器参数(云台参数)

#define NET_SDVR_ALARMINCFG_GET		0X32		//获取报警输入参数(扩展)
#define NET_SDVR_ALARMINCFG_SET		0X33		//设置报警输入参数
#define NET_SDVR_ALARMOUTCFG_GET	0X34		//获取报警输出参数(扩展)
#define NET_SDVR_ALARMOUTCFG_SET	0X35		//设置报警输出参数

#define NET_SDVR_TIMECFG_GET		0X36		//获取主机时间
#define NET_SDVR_TIMECFG_SET		0X37		//设置主机时间

#define NET_SDVR_USERCFG_GET		0X38		//获取用户权限
#define NET_SDVR_USERCFG_SET		0X39		//设置用户权限

#define NET_SDVR_SERIAL_GET			0x3A		//获取串口参数
#define NET_SDVR_SERIAL_SET			0x3B		//设置串口参数

#define NET_SDVR_MD5ID_GET			0x40		//获取MD5特征码,登陆时使用

#define NET_SDVR_REFRESH_FLASH		0x41		//更新DVR FLASH

#define NET_SDVR_RECOVER_DEFAULT	0x42		//恢复DVR缺省参数
#define NETCOM_QUERY_RECFILE        0x45		//PC向DVR发送的查询录像文件段信息请求
#define NETCOM_QUERY_RECFILE_EX     0x46		//PC向DVR发送的查询录像文件段信息请求(按卡号查询)

#define NET_SDVR_GET_HEARTBEAT      0x48		//获取网络客户端的耗时请求

#define NETCOM_VOD_RECFILE_REQ      0x49		//PC向DVR发送的点播录像文件请求
#define NETCOM_VOD_RECFILE_REQ_EX	0x4A		//PC向DVR发送的点播按时间段播放录像文件请求  add by cui 08.12.19
#define NETCOM_VOD_RECFILE_CTL	    0x4C		//tcp方式的设置点播速度；
#define NETCOM_VOD_RECFILE_END      0x4F		//PC向DVR发送的点播结束
#define NETCOM_BACKUP_RECFILE_REQ   0x51		//PC向DVR发送的备份录像文件请求
#define NETCOM_BACKUP_FILEHEAD      0x5A		//DVR向PC发送的备份文件头标志(在备份的码流中)
#define NETCOM_BACKUP_END           0x5C		//DVR向PC发送的备份文件结束标志(在备份的码流中) 
#define NETCOM_BACKUP_RECFILE_OK    0x53		//DVR向DVR发送的备份录像文件请求成功
#define NETCOM_BACKUP_RECFILE_NOOK  0x55		//DVR向DVR发送的备份录像文件请求失败

#define NETVOD_START				0x56		//PC 向DVR 发送的点播开始
#define NETVOD_DATA					0x57		//PC 向DVR 发送的点播数据
#define NETVOD_PAUSE				0x58		//PC 向DVR 发送的点播暂停
#define NETVOD_STOP					0x59		//PC 向DVR 发送的点播结束

#define	NET_SDVR_SHAKEHAND			0xFF		//握手协议
#define NET_SDVR_DNS_GET			0x60		//获取DNS
#define NET_SDVR_DNS_SET			0x61		//设置DNS   sDNSState=1时候登陆   
#define NET_SDVR_PPPoE_GET			0x62		//获取PPPoE
#define NET_SDVR_PPPoE_SET			0x63		//设置PPPoE  sPPPoEState=1时候拨号
#define	NET_SDVR_SERVERCFG_GET		0x64		//获取平台服务器参数
#define	NET_SDVR_SERVERCFG_SET		0x65		//获设置平台服务器参数
#define	NET_SDVR_CLEARALARM			0x66		//报警清除
#define NET_SDVR_SET_SERIALID		0x67		//设置序列号
#define NET_SDVR_GET_SERIALID		0x68		//获取序列号
#define NET_SDVR_GET_VLostStatus	0x69		//获取视频丢失
#define NET_SDVR_GET_PHOTO			0x70		//拍照
#define NET_SDVR_GET_DVRTYPE		0x71		//设备类型
#define NET_SDVR_SET_DVRTYPE		0x72		//设备类型(超宇版本)

#define NET_SDVR_SET_PRESETPOLL		0x73		//设置多预置点轮巡
#define NET_SDVR_GET_PRESETPOLL		0x74		//获取多预置点轮巡

#define NET_SDVR_SET_VIDEOSYS       0x75		//设置视频制式(N/P切换)
#define NET_SDVR_GET_VIDEOSYS       0x76		//获取视频制式(N/P切换)

#define NET_SDVR_GET_REAL_DEFENCE	0x7A		//获取实时布防状态
#define NET_SDVR_SET_REAL_DEFENCE	0x7B		//实时布防(即是从设防开始就启动布防，并延时一个时间段(比如60s))
#define NET_SDVR_GET_DELAY_PHOTO	0x7C		//抓延时图片
#define NET_SDVR_GET_UPLOAD_ALARMTYPE  0x7D  // 获取支持设置上传参数的报警类型 
#define NET_SDVR_GET_UPLOAD_ALARMINFO  0x7E  //获取主机报警上传参数 
#define NET_SDVR_SET_UPLOAD_ALARMINFO  0x7F  //设置主机报警上传参数 

#define NET_SDVR_PTZTYPE_GET		0x79		//获取云台协议

#define NET_SDVR_IPCWIRELESS_SET    0x87        //设置IPC无线参数配置
#define NET_SDVR_IPCWIRELESS_GET    0x88		//获取IPC无线参数配置

#define NET_SDVR_NTPCONFIG_SET	    0x90        //设置NTP设置
#define NET_SDVR_NTPCONFIG_GET	    0x91        //获取NTP设置

#define NET_SDVR_SMTPCONFIG_SET	    0x92        //设置SMTP设置
#define NET_SDVR_SMTPCONFIG_GET	    0x93        //获取SMTP设置

#define NET_SDVR_POLLCONFIG_SET	    0x94        //设置轮巡配置参数
#define NET_SDVR_POLLCONFIG_GET	    0x95        //获取轮巡配置参数

#define NET_SDVR_VIDEOMATRIX_SET	0x96        //设置视频矩阵参数
#define NET_SDVR_VIDEOMATRIX_GET	0x97        //获取视频矩阵参数

#define NET_SDVR_GET_VCOVER_DETECT  0x98		//获取遮挡报警设置 
#define NET_SDVR_SET_VCOVER_DETECT  0x99		//设置遮挡报警设置

#define NET_SDVR_IPCWORKPARAM_GET   0xB0        //获取IPC工作参数
#define NET_SDVR_IPCWORKPARAM_SET   0xB1		//设置IPC工作参数	

#define NET_SDVR_GET_CHLPARAM_SUPPORT     0xE6             //获取主机通道参数支持范围
//#define NET_SDVR_GET_USER_PERSION_VER     0XB3             //获取主机用户权限模式
#define NET_SDVR_USERCFG_GET_EXT1	      0XE8			   //获取用户信息扩展1
#define NET_SDVR_USERCFG_SET_EXT1		  0XE9			   //设置用户信息扩展1

#define NET_SDVR_GET_PLATFORM_PARAM	0xF0	    //获取平台参数
#define NET_SDVR_SET_PLATFORM_PARAM	0xF1	    //设置平台参数




#define NET_SDVR_INITIATIVE_LOGIN		0xF2    //主机主动连接平台服务器
#define NET_SDVR_REAL_PLAY_EX			0xF3	//主动连接模式打开视频请求扩展
#define NETCOM_VOD_RECFILE_REQ_EX1		0xF4	//主动连接模式远程录像点播
#define NETCOM_BACKUP_RECFILE_REQ_EX1	0xF5	//主动连接模式远程录像备份
#define NET_SDVR_VOICE_START_EX			0xF6	//主动连接模式打开语音
#define NET_SDVR_GET_PHOTO_EX			0xF7	//主动连接模式抓图

#define NET_SDVR_UPGRADE_REQ_EX			0xF9    //主动连接远程升级请求扩展

#define NET_SDVR_SET_VOICE_DECODE      0xDE   //设置对讲接收数据格式 

#define SDVR_SHUTDOWN				0			//关闭设备
#define SDVR_REBOOT					1			//重启设备
#define SDVR_RESTORE				2			//恢复出厂置


#define NET_SDVR_GET_ZERO_VENCCONF					0xA8	//获取复合通道视频参数
#define NET_SDVR_SET_ZERO_VENCCONF					0xA9	//设置复合通道视频参数
#define NET_SDVR_DST_TIME_GET						0xAA	//获取夏令时时间设置信息
#define NET_SDVR_DST_TIME_SET						0xAB	//设置夏令时时间设置信息

#define NETCOM_QUERY_RECFILE_EXIST  0X44		//查询给定天数内是否每天都有录像文件  2012-3-19

#define NET_SDVR_WORK_STATE_EX		0xC1		//获取主机设备工作状态(扩展)NVR
#define NET_SDVR_LOG_QUERY_EX		0xC3		//NVR日志信息查询

#define NET_SDVR_DEVICECFG_GET_EX	0xC6		//获取设备信息(扩展)NVR
#define NET_SDVR_DEVICECFG_SET_EX	0xC7		//设置设备信息(扩展)NVR

#define NET_SDVR_PTZLIST_GET_NVR	0xC8		//获取NVR云台协议列表

#define NET_SDVR_ALRMIN_GET_NVR		0xCA		//获取NVR报警输入参数
#define NET_SDVR_ALRMIN_SET_NVR		0xCB		//设置NVR报警输入参数
#define NET_SDVR_ALRMOUT_GET_NVR	0xCC		//获取NVR报警输出参数
#define NET_SDVR_ALRMOUT_SET_NVR	0xCD		//设置NVR报警输出参数

#define NET_SDVR_PICCFG_GET_EX_NVR	0xD2		//获取NVR通道参数
#define NET_SDVR_PICCFG_SET_EX_NVR	0xD3		//设置NVR通道参数
#define NET_SDVR_RECORD_GET_EX_NVR	0xD4		//获取NVR录像参数
#define NET_SDVR_RECORD_SET_EX_NVR	0xD5		//设置NVR录像参数
#define NET_SDVR_MOTION_DETECT_GET_NVR	0xD6	//获取NVR移动侦测参数
#define NET_SDVR_MOTION_DETECT_SET_NVR	0xD7	//设置NVR移动侦测参数

#define NET_SDVR_STORDEVCFG_GET         0x09    //获取存储设备类型
#define NET_SDVR_FORMAT_STORDEV         0x0A    //格式化主机存储设备

#define NET_SDVR_OSDCFG_GET				0x3C	//获取通道字符叠加信息
#define NET_SDVR_OSDCFG_SET				0x3D	//设置通道字符叠加信息

#define NET_SDVR_QUERY_MONTH_RECFILE	0x47		//远程查询标记某月的录像记录

#define NET_SDVR_GET_TEMPERATURE			0x6A		//获取主机温度

#define NET_SDVR_GET_RECORDSTATISTICS    0x6B		//获取录像统计
#define NET_SDVR_RECORDNOTENOUGHDATE_ALARM	0x6C		//录像天数不足报警

#define NET_SDVR_ALARM_REQ           0xF8   //报警主动上传

#define NET_SDVR_SPECIFIC_DDNS						0x5E    //获取指定DDNS服务器参数
#define NET_SDVR_DDNSSERVERTLIST_GET         0x5F    //获取主机端支持的DDNS列表

#define NET_SDVR_PARAM_FILE_EXPORT           0xDA   //主机参数文件导出 
#define NET_SDVR_PARAM_FILE_IMPORT           0xDB   //主机参数文件导入 

#define NETCOM_BACKUP_RECFILE_REQ_EX       0x52   //PC向DVR发送的备份录像文件请求,可以实现断点续传
#define NET_SDVR_SUPPORT_FUNC					   0x5D   //获取主机端支持的功能

#define NET_SDVR_CHAN_CARDCOR_SET             0x13    //设置通道卡号录像状态 
#define NET_SDVR_CHAN_CARDCOR_GET             0x14   //获取通道卡号录像状态 

#define NET_SDVR_AUTO_TEST_DEVICE				 0xF000	//生产自动化测试
#define NET_SDVR_AUTOTEST_HARDDISK_VOD    0xF001 //硬盘识别、格式化、录像、数据分析等自动化测试

#define NET_SDVR_GET_CH_CLIENT_IP		0xC2		//获取连接到此通道的客户端IP列表
#define NET_SDVR_SERIAL_START_NVR	0xC4		//建立NVR透明通道
#define NET_SDVR_SERIAL_STOP_NVR		0xC5		//关闭NVR透明通道
#define NET_SDVR_ALRM_ATTR_NVR		0xC9		//获取NVR报警输入输出端口属性
#define NET_SDVR_ALRMIN_STATUS_GET_NVR			0xCE		//获取NVR报警输入状态
#define NET_SDVR_ALRMOUT_STATUS_GET_NVR		0xCF		//获取NVR报警输出状态
#define NET_SDVR_ALRMOUT_STATUS_SET_NVR		0xD1		//设置NVR报警输出状态
#define NET_SDVR_ABNORMAL_ALRM_GET_NVR		0xD8		//获取NVR异常报警参数
#define NET_SDVR_ABNORMAL_ALRM_SET_NVR		0xD9		//设置NVR异常报警参数
#define NET_SDVR_RESOLUTION_GET_NVR		0xDC		//获取NVR主机分辨率
#define NET_SDVR_RESOLUTION_SET_NVR		0xDD		//设置NVR主机分辨率
#define NET_SDVR_GET_REMOTE_RESOLUTION       0xDF   //获取NVR 前端设备分辨率列表

#define NET_SDVR_GET_LOGO_UPDATE_INFO						0xF002		//获取LOGO升级信息
#define NET_SDVR_AUTOTEST_HARDDISK_VOD_STOP			0xF003		//硬盘识别、格式化、录像、数据分析等自动化测试结束

#define NET_SDVR_ADD_IPDEV								0x10   //添加IP设备 
#define NET_SDVR_DEL_IPDEV								0x11   //删除IP设备 
#define NET_SDVR_GET_ALLIPCH_INFO					0x12   //获取主机所有添加的IP设备信息
#define NET_SDVR_GET_IPCH_INFO							0x4b   //获取主机添加的某个IP设备信息
#define NET_SDVR_IPDEV_REBOOT							0x6D   //远程重启IP设备
#define NET_SDVR_GET_IPDEV_NETPARAM				0x4d   //获取IP设备网络参数
#define NET_SDVR_SET_IPDEV_NETPARAM				0x4e   //设置IP设备网络参数
#define NET_SDVR_GET_IPDEV_TIME						0x50   //获取IP设备系统时间
#define NET_SDVR_SET_IPDEV_TIME						0x54   //设置IP设备系统时间
#define NET_SDVR_GET_LOCH_STATUE					0x6E   //获取本地通道启用状态 
#define NET_SDVR_SET_LOCH_STATUE						0x6F   //设置本地通道启用状态 

#define NET_SDVR_PREANDDELRECCFG_GET        0x24   //获取通道录像预录及延录时间 
#define NET_SDVR_PREANDDELRECCFG_SET        0x25   //设置通道录像预录及延录时间

#define NETCOM_QUERY_SNAPSHOT                0xE0   //远程图片查询请求 
#define NETCOM_BACKUP_SNAPSHOT_REQ           0xE1   //远程图片备份请求 
#define NETCOM_BACKUP_SNAPSHOT_FILEHEAD      0xE2   //远程图片文件头标志 
#define NETCOM_BACKUP_SNAPSHOT_END           0xE3   //远程图片备份结束 
#define NET_SDVR_SNAPSHOT_GET                0xE4   //获取远程图片设置信息 
#define NET_SDVR_SNAPSHOT_SET                0xE5   //设置远程图片设置信息

#define NET_SDVR_SEARCH_IPDEV                 0x8A    //搜索IP 设备 
#define NET_SDVR_NETINFO_6_GET                0x8B    //获取主机IPV6 网络参数 
#define NET_SDVR_NETINFO_6_SET                0x8C    //设置主机IPV6 网络参数 
#define NET_SDVR_OSD_CONFIG_GET               0x8D    //获取扩展OSD 
#define NET_SDVR_OSD_CONFIG_SET               0x8E    //设置扩展OSD 
#define NET_SDVR_FRAMERATE_GET                0x8F    //获取总帧率和剩余帧率

#define   NET_SDVR_HB_YDT_GET                 0x9A   //获取汉邦一点通参数 

#define   NET_SDVR_HB_YDT_SET                 0x9B   //设置汉邦一点通参数 
#define   NET_SDVR_TELEADJUSTING_CAMERA   0xED     //摄像机参数远程调节

#define   NET_SDVR_GET_TIMING_REBOOTINFO      0xEB    //获取主机定时重启参数
#define   NET_SDVR_SET_TIMING_REBOOTINFO      0xEC    //设置主机定时重启参数 

#define NET_SDVR_REAL_PLAY_MULTI  0xF004//开启单socket多通道实时流
#define NET_SDVR_REAL_STOP_MULTI  0xF005//关闭单socket多通道实时流

//云台控制命令
#define SDVR_PAN_AUTO				0x0001001c 	//自动扫描
#define SDVR_STOP_PTZ				0x00010028	//停止云台
#define SDVR_BRUSH					0x0001002e  //雨刷
#define SDVR_UP						0x0001000c	//向上旋转
#define SDVR_DOWN					0x0001000d	//向下旋转
#define SDVR_LEFT					0x0001000e 	//向左转
#define SDVR_RIGHT					0x0001000f	//向右转
#define SDVR_ZOOM_IN				0x00010016	//焦距变大(以速度SS倍率变大)
#define SDVR_ZOOM_OUT				0x00010017 	//焦距变小(以速度SS倍率变小)
#define SDVR_IRIS_OPEN				0x00010018 	//光圈扩大(以速度SS扩大)
#define SDVR_IRIS_CLOSE				0x00010019	//光圈缩小(以速度SS缩小)
#define SDVR_FOCUS_FAR				0x00010015 	//焦点后调(以速度SS后调)
#define SDVR_FOCUS_NEAR				0x00010014  //焦点前调(以速度SS前调)

#define SDVR_LIGHT_PWRON			0x00010024	//接通灯光电源
#define SDVR_WIPER_PWRON			0x00010025	//接通雨刷开关
#define SDVR_SET_PRESET				0x0001001a 	//设置预置点
#define SDVR_GOTO_PRESET			0x0001001b 	//快球转到预置点
#define SDVR_CLE_PRESET				18			//清除预置点

#define SDVR_FAN_PWRON				19			//接通风扇开关
#define SDVR_HEATER_PWRON			20			//接通加热器开关
#define SDVR_AUX_PWRON				21			//接通辅助设备开关

#define SDVR_FILL_PRE_SEQ			30			//将预置点加入巡航序列
#define SDVR_SET_SEQ_DWELL			31			//设置巡航点停顿时间
#define SDVR_SET_SEQ_SPEED			32			//设置巡航速度
#define SDVR_CLE_PRE_SEQ			33			//将预置点从巡航序列中删除
#define SDVR_STA_MEM_CRUISE			34			//开始记录轨迹
#define SDVR_STO_MEM_CRUISE			35			//停止记录轨迹
#define SDVR_RUN_CRUISE				36			//开始轨迹
#define SDVR_RUN_SEQ				37			//开始巡航
#define SDVR_STOP_SEQ				38			//停止巡航
#define SDVR_SYSTEM_RESET			40			//系统复位

//主动连接切换连接请求返回码
#define INITIATIVE_REPONSE_RESULT	9000		

///////////////////////////////////////////////////////////////
//设备类型
#define SDVR_TYPE			-1					//未知设备
#define SDVR_TYPE_IPC		103					//IPC(即:网络摄像机)
//#define SDVR_TYPE_7000T		7					//7000T(即:7000TDVR)
//#define SDVR_TYPE_8000		8					//8000(即:8000DVR)
//#define SDVR_TYPE_8000T		9					//8000T(即:8000TDVR)
//#define SDVR_TYPE_8024T		10					//8024T(即:8000T系列24路DVR)

#define SDVR_TYPE_7000T		0					//7000T(即:7000TDVR)
#define SDVR_TYPE_8000T		1					//8000T(即:8000TDVR)
#define SDVR_TYPE_8000		80					//8000(即:8000DVR)
#define SDVR_TYPE_8024T		81					//8024T(即:8000T系列24路DVR)


//设备类型2(保留字段)
#define DEVTYPE_7200H       9                   //7200H系列
#define DEVTYPE_7200L       10                  //7200L系列
#define DEVTYPE_7000M       12                  //7000M系列
#define DEVTYPE_7000L       15                  //7000L系列
#define DEVTYPE_2201TL		16					//2201设备
#define DEVTYPE_2600T       17                  //2600T设备  
#define DEVTYPE_2600B		18					//2600B设备  

#define DEVTYPE_7024M       70                  //7024M(在7000M的基础上根据通道数区分)
#define DEVTYPE_7024T       71                  //7024T(在7000T的基础上根据通道数区分)
#define DEVTYPE_7032X       72					//7032X(在7000M的基础上根据通道数区分)

#define SDVR_TYPE_9000		90					//9000系列高清设备
#define SDVR_TYPE_NVR		100					//NVR

#define NET_SDVR_UPNPCFG_GET        0x3C       //获取UPNP配置参数
#define NET_SDVR_UPNPCFG_SET        0x3D        //设置UPNP配置参数 


#endif