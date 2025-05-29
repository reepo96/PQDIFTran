#ifndef PQDIFFILE_H_HEADER_INCLUDED_B2ECE1A0
#define PQDIFFILE_H_HEADER_INCLUDED_B2ECE1A0
#include "Container.h"
#include "DataSource.h"
#include "MonitorSetting.h"
#include "Observation.h"

//##ModelId=4C5A86710060
class PQDIFFile
{
  public:
    // PQDIF格式转换到双合格式电能质量格式
    //##ModelId=4C5B6A8303E7
    bool PQDIF2SHPQ(
        // 被转换的文件名
        const char* pSrcFile, PQFileHead* pFileHeader, vector<ChannelInf>& ChannelInfs, 
		vector<PQDIFInterFace::VAL_UNIT>  *Units,
        // 返回的数据列表
        CPQDataList& DataList);

    // PQDIF格式转换到双合格式电能质量格式
    //##ModelId=4C5B6A8303EC
    bool PQDIF2SHPQ(
        // 被转换的文件名
        const char* pSrcFile, 
        // 转换目标文件名
        const char* pDestFile);

    // 双合格式电能质量转换到PQDIF格式文件
    //##ModelId=4C5B6A84000E
    bool SHPQ2PQDIF(
        // 被转换的文件名
        const char* pSrcFile, 
        // 转换目标文件名
        const char* pDestFile);

	//根据文件头信息创建PQDIF格式的文件头，并保存到文件中
	bool CreateFileHeader(SHPQDIF::PQFileHead* pFileHeader,FILE *pFile);

    // 双合格式电能质量转换到PQDIF格式文件
    //##ModelId=4C5B6A840011
    bool SHPQ2PQDIF(SHPQDIF::PQFileHead* pFileHeader, vector<SHPQDIF::ChannelInf> *pChannelInfs, 
        // 被转换的数据列表
        CPQDataList *pDataList, 
        // 转换目标文件名
        FILE *pFile);

	void ClearData( CPQDataList& DataList );

  protected:
    //##ModelId=4C5A86710076
    Container HeaderRecord;

    //##ModelId=4C5A86710077
    DataSource ds;

    //##ModelId=4C5A86710078
    MonitorSetting ms;

    //##ModelId=4C5A86710079
    Observation DataRecord;

};



#endif /* PQDIFFILE_H_HEADER_INCLUDED_B2ECE1A0 */
