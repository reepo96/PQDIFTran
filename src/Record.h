#ifndef RECORD_H_HEADER_INCLUDED_B2EC96B3
#define RECORD_H_HEADER_INCLUDED_B2EC96B3

#include <string.h>
#include <stdio.h>

#include "zlib.h"
#include "pqdif_ph.h"
#include "pqdif_lg.h"
#include "pqdif_id.h"
#include <vector>

#include "PQDIFInterFace.h"

using namespace std;
using namespace SHPQDIF;

extern PQFileHead	g_FileHeader;	//文件头

extern int StoragMeth[2];	//数据存储方式（两序列）

typedef struct
{
	unsigned int nDataLen;
	char *pData;
}UN_COMP_SEG;	//解压段

//##ModelId=4C5A86710049
class Record
{
  public:
    //##ModelId=4C7B0CF903B1
    struct MemNode
    {
        //##ModelId=4C7B0D130161
        void *obj;

        //##ModelId=4C7B0D230347
        size_t length;

        //##ModelId=4C7B0D97024D
        MemNode* pNext;

    };

	Record();

	virtual ~Record();

	/*使用zlib算法压缩数据
	pSrc：指向被压缩数据
	nSrcLen：被压缩数据长度
	pDest[out]：保存压缩后的数据
	pDestLen[in/out]：压缩后的数据长度
	nCheckSum:校验和
	*/
	static int Compress_z(const char *pSrc,unsigned int nSrcLen,char *pDest,unsigned int& nDestLen,unsigned int& nCheckSum);

	/*使用zlib算法解压数据
	pSrc：指向被压缩数据
	nSrcLen：被压缩数据长度
	SegList：解压出来的段列表
	*/
	static int UnCompress_z(const char *pSrc,unsigned int nSrcLen,vector<UN_COMP_SEG> &SegList);

	//初始化zip库
	int InitZip();

	/*使用zlib算法解压数据,执行前要调用InitZip（），执行后要调用EndZip
	pSrcFile：指向被压缩数据文件
	pOutData：存放解压数据
	iDataLen[in/out]：解压数据长度
	返回值：-1-失败，0-成功，1-继续解压
	*/
	int UnCompress_z(FILE *pSrcFile,std::string& sOutData,int& iDataLen);

	//结束zip库
	int EndZip();

    //##ModelId=4C7C80FF0056
    bool WritePQDIFFile(FILE *pFile,const GUID& RecordType,UINT4 styleCompression, UINT4 algCompression, 
        // 是否是最后一个记录
        bool IsLastRecord);

    // 重新初始化记录
    //##ModelId=4C7CA96E00E7
    void ReInit();

    //##ModelId=4C5A8671004F
    c_record_mainheader header;

    //##ModelId=4C5A86710050
    char* pBody;

    //##ModelId=4C5A86710051
    Record* pNext;

  protected:
    //##ModelId=4C5A8671004C
	  /*分析一条记录
	  返回值：0：成功，-1：失败
	  */
    virtual int ParseOneRecord(FILE* pFile, c_record_mainheader& hdrRecord, UINT4 styleCompression, UINT4 algCompression, bool bHeaderHasRead);

    //##ModelId=4C5A8671004E
    bool ParseCollection(char* pRecordBody, SIZE4 sizeOfBody, 
        // 本colloection在body的偏移位置
        SIZE4 nOffSet);

    //##ModelId=4C71F37A012E
    virtual bool AcceptCollection(const GUID& tag, int level);

    //##ModelId=4C7234DE001C
    virtual bool AcceptScalar(const GUID& tag, int level, INT1 typePhysical, void* pdata);

    //##ModelId=4C72464D032C
    virtual bool AcceptVector(const GUID& tag, int level, INT1 typePhysical, c_vector* pVector, void* pdata);

    // 添加记录的首个集合
    //##ModelId=4C5A8671004A
    c_collection_element  * AddCollection(
        // 集合元素个数
        const int iElemCount);

    // 添加集合下的嵌套集合
    //##ModelId=4C7DA141004E
    c_collection_element  * AddCollection(
        // 集合元素个数
        const int iElemCount, const GUID &tag, c_collection_element &ce);

    // 往一个元素加一个scalar数据，当返回NULL时，表示此数据是放在元素内部
    //##ModelId=4C7B2AF100FA
    void *AddScalar(
        // 需要放入元素的数据
        const char *pData, 
        // 数据的物理类型
        INT1 typePhysical, const GUID &tag, c_collection_element &ce);

    // 往一个元素加一个Vector数据
    //##ModelId=4C7B4A2801A3
    void *AddVector(
        // 需要放入元素的数据
        const char *pData, 
        // 传入数据元素的个数
        const unsigned int nDataCount, 
        // 数据的物理类型
        INT1 typePhysical, const GUID &tag, c_collection_element   &ce);

    // 整个record内存大小(未压缩之前的大小)
    //##ModelId=4C7787A2022C
    unsigned int m_nTotalMemSize;

	long GetNumBytesOfType(long idType);

	//不是4字节的整数倍，则补到4字节整数倍
	SIZE4 padSizeTo4Bytes( SIZE4 sizeOrig );

	/*根据tag标签返回记录类型
	tag：记录标签
	返回：1：Container记录；2：DataSource记录；3：MonitorSetting记录；4：Observation记录
	*/
	int GetRecordType( const GUID& tag);

	/*将PQDIF格式的时间转换成struct tm结构的时间
	pPQTime：QDIF格式的时间
	pTmTime：struct tm结构的时间
	返回：成功，失败
	*/
	bool PQTime2Tm( TIMESTAMPPQDIF *pPQTime,struct tm *pTmTime);

	/*将PQDIF格式的时间转换成time_t
	pPQTime：QDIF格式的时间
	ttTime：time_t时间
	返回：成功，失败
	*/
	bool PQTime2Time_t( TIMESTAMPPQDIF *pPQTime,time_t & ttTime);

	/*将time_t格式的时间转换成PQDIF
	ttTime：time_t时间
	pPQTime：QDIF格式的时间
	返回：成功，失败
	*/
	bool Time_t2PQTime( time_t & ttTime,TIMESTAMPPQDIF *pPQTime);

	bool AddMemObjToList(void *pMem,unsigned int nMemLen);

    //##ModelId=4C7B0DF40152
    MemNode* m_pMemObjList;

	MemNode* m_pMemObjTail;

	z_stream m_strm;//zip压缩流
};



#endif /* RECORD_H_HEADER_INCLUDED_B2EC96B3 */
