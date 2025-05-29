#ifndef PQDIFINTERFACE_H_HEADER_INCLUDED_B2ECBAB7
#define PQDIFINTERFACE_H_HEADER_INCLUDED_B2ECBAB7

#ifdef WIN32

#ifdef PQDIFTRAN_EXPORTS
#define PQDIFTRAN_API __declspec(dllexport)
#else
#define PQDIFTRAN_API __declspec(dllimport)
#endif

#else

#define PQDIFTRAN_API

#endif

#include "CPQDataList.h"
//#include "CPQDataType.h"
#include <vector>

using namespace std;

namespace SHPQDIF
{

	//PQDIF格式的文件头
	typedef struct PQFileHead
	{
		// 标记,"SHPQ"
		char Flag[4];

		// 版本，高:主版号, 低:副版号
		unsigned short Ver;

		//整个文件头大小
		unsigned short HeadSize;

		// 文件类型：
		//1<<0：频率偏差趋势
		//1<<1：电压偏差趋势
		//1<<2：三相不平衡度
		//1<<3：谐波趋势
		//1<<4：间谐波数据
		//1<<5：电压波动
		//1<<6：闪变
		//1<<7：谐波功率
		//1<<8：电能
		//1<<9：合格率
		unsigned int FileType;

		// 文件标题，100char，不足补零
		char FileTitle[100];

		// 压缩类型（文件头不压缩，仅压缩数据部分）：
		// 0：不压缩
		// 1：zlib压缩格式
		char CompressType;

		// 文件数据起始时间，通常为某天零点时间
		struct tm DataStartTime;

		// 文件数据结束时间
		struct tm DataEndTime;

		// 数据时间类型：
		// 1：相对时间
		// 2：绝对时间
		char DataTimeType;

		// 数据时间单位（DataTimeType=1时有效）：
		// 1：秒
		// 2：分钟
		// 3：小时
		// 4：天
		// 5：月
		char DataTimeUnit;

		// 有效线路个数，<=36,0表示无线路数据
		unsigned char ActiveLNum;

		// 有效通道个数，<=96,0表示无通道数据
		unsigned char ActiveChNum;

		// 每个点数据大小,在仅存储单种数据时有效
		unsigned int PerPointSize;

		// 点总数
		unsigned int TotalPoint;

		// 统计数据在文件种的偏移地址
		unsigned int StatDataOffset;

		// 统计数据长度，为0表示无统计数据
		unsigned int StatDataLen;

		//数据元素个数
		unsigned char DataEleNum;
	}PQFileHead,*PPQFileHead;

	struct ChannelInf
	{
		unsigned char Index;
		int LineNo;
		int ChannelNo;
		unsigned char ChannelType;
		char LineName[64];
		char ChannelName[64];
		float HiLimt;
		float LoLimt;
		float OddTrgLev;
		float EvenTrgLev;
		float HaTrgLev[24];
		float Ratio;
		float Rating;
	};

	//通道配置信息
	struct ChaCfgInf
	{
		unsigned char Index;
		int LineNo;
		int ChannelNo;
		unsigned char ChannelType;
		char LineName[64];
		char ChannelName[64];
	};

	//一般定值配置信息
	struct PQNormalSetInf
	{
		unsigned char ChanIndex;
		float HiLimt;
		float LoLimt;
		float Ratio;
		float Rating;
	};

	//谐波定值配置信息
	struct PQHSetInf
	{
		unsigned char ChanIndex;
		float THDTrgLev;
		float OddTrgLev;
		float EvenTrgLev;
		float HaTrgLev[24];
		float Ratio;
		float Rating;
	};

}

//##ModelId=4C5A867100A8
class PQDIFTRAN_API PQDIFInterFace
{
  public:
	  //定义单位
	  typedef enum valUnit {NONE/*无*/,VOLT/*伏特(V)*/,AMP/*安培(A)*/,HERTZ/*赫兹(hz)*/,DEGRESS/*度*/,
		  PERCENT/*百分比(%)*/,WATTS/*瓦特*/} VAL_UNIT;

    // PQDIF格式转换到双合格式电能质量格式
    //##ModelId=4C5A867100B1
    bool PQDIF2SHPQ(
        // 被转换的文件名
		const char* pSrcFile, SHPQDIF::PQFileHead* pFileHeader, vector<SHPQDIF::ChannelInf> **ChannelInfs, 
		vector<VAL_UNIT>  **Units,/*通道每个数值对应的单位*/
        // 返回的数据列表
        CPQDataList **pDataList);

    // PQDIF格式转换到双合格式电能质量格式
    //##ModelId=4C5A867100B4
    bool PQDIF2SHPQ(
        // 被转换的文件名
        const char* pSrcFile, 
        // 转换目标文件名
        const char* pDestFile);

    // 双合格式电能质量转换到PQDIF格式文件
    //##ModelId=4C5A867100B7
    bool SHPQ2PQDIF(
        // 被转换的文件名
        const char* pSrcFile, 
        // 转换目标文件名
        const char* pDestFile);

    // 双合格式电能质量转换到PQDIF格式文件
    //##ModelId=4C5A867100BA
    bool SHPQ2PQDIF(SHPQDIF::PQFileHead* pFileHeader, vector<SHPQDIF::ChannelInf> *pChannelInfs, 
        // 被转换的数据列表
        CPQDataList *pDataList, 
        // 转换目标文件名
        const char* pDestFile);

	void ClearData( CPQDataList **DataList );

	void ClearChanInf( vector<SHPQDIF::ChannelInf> **ChannelInfs );

	void ClearUnits( vector<VAL_UNIT> **Units );

};



#endif /* PQDIFINTERFACE_H_HEADER_INCLUDED_B2ECBAB7 */
