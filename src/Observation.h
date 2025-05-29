#ifndef OBSERVATION_H_HEADER_INCLUDED_B2ECC1E1
#define OBSERVATION_H_HEADER_INCLUDED_B2ECC1E1
#include "Record.h"
#include <map>
#include "CPQDataList.h"

using namespace std;

//##ModelId=4C5A8671009C
class Observation : public Record
{
  public:
	  Observation();
	  virtual ~Observation();

    //##ModelId=4C5A8671009E
    bool AddData(
        //文件头
        PQFileHead *pFileHeader, 
        // 数据列表
        CPQDataList *pDataList, 		
        // 从第几个点开始
        int iFromPos, 
        // 多少个点
        int& iDataNum);

    //##ModelId=4C5A867100A2
    bool ParseFile(FILE* pFile, 
        // 返回的数据列表
        CPQDataList& DataList, PQFileHead* pFileHeader);

  protected:
    //##ModelId=4C7369BB01BD
    bool AcceptCollection(const GUID& tag, int level);

    //##ModelId=4C7369BB01C0
    bool AcceptScalar(const GUID& tag, int level, INT1 typePhysical, void* pdata);

    //##ModelId=4C7369BB01C5
    bool AcceptVector(const GUID& tag, int level, INT1 typePhysical, c_vector* pVector, void* pdata);

    //##ModelId=4C7369BB01CC
    int ParseOneRecord(FILE* pFile, c_record_mainheader& hdrRecord, UINT4 styleCompression, UINT4 algCompression, bool bHeaderHasRead);

	//取序列值
	void *GetSeriesData
		( 
		UINT4   idStorageMethod, //存储方式
		INT1    idPhysicalType,	 //数据物理类型
		double  rScale,			 //比例
		double  rOffset,		 //零漂
		long    countRawPoints,	 //原点数(vector结构中的count)
		void *  pvectSeriesArray,//指向数据
		long&   countPoints		 //返回实际点数
		);

	//取得序列实际点数
	long GetSeriesCount
		( 
		UINT4   idStorageMethod,//存储方式
		INT1    idPhysicalType, //数据物理类型
		long    countRawPoints, //原点数(vector结构中的count)
		void *  pvectSeriesArray //指向数据
		);

	//取简单vector的值
	bool GetSimpVectorArrayValue
		( 
		void *  pdata, //指向数据
		INT1    typePhysical, //数据物理类型
		int     idx, //索引
		double& value //返回的值
		);

	//生成时间序列
	char *GeneratTimeSeries(PQFileHead *pFileHeader, CPQDataList *pDataList, int iFromPos, 
									  int& iDataNum,INT1& typePhysical);

	//生成值序列
	char *GeneratValueSeries(
		PQFileHead *pFileHeader, //文件头信息
		CPQDataList *pDataList, //数据列表
		int iFromPos,			//第一个点
		int& iDataNum,			//点数
		int poistion,			//在元素数据结构中的第几个值，0base
		int iChaNo				//通道号
		);

	//##ModelId=4C7369790150
    int m_iChannelInstIdx;

    //##ModelId=4C7369790160
    int m_iSeriesInstIdx;

    // 保存通道的值，其中map的key是通道号，值是数据指针map。这个map的key是谐波次数，值是谐波值（只有一对，且key=0时，则为通道值）
    //##ModelId=4C7371780288
    map<int,map<int,float*>*> m_ChaValues;

    //##ModelId=4C7374B6008A
    unsigned int *m_nTimes;

    //##ModelId=4C7380880187
    float *m_fTimes;

    // 当前通道索引
    //##ModelId=4C7376C90025
    int m_iCurrentChaIdx;

    // 当前谐波（0为基波，依次类推）
    //##ModelId=4C737C38030C
    int m_iCurrentHarm;

    // 当前通道的值，其中map的key是谐波次数，值是谐波值（只有一对，且key=0时，则为通道值）
    //##ModelId=4C7383B1011E
    map<int,float*> *m_pCurrentChaValues;

    // 点的个数
    //##ModelId=4C73B1E102CB
    int m_iPointCount;

	//本段数据的开始时间
	struct tm tmDataStartTime;

	//存放比例系数，key：序列索引，value：比例系数
	map<int,double>	m_SeriesScale;

	//存放零漂，key：序列索引，value：零漂
	map<int,double>	m_SeriesOffset;

};



#endif /* OBSERVATION_H_HEADER_INCLUDED_B2ECC1E1 */
