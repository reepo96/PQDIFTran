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
        //�ļ�ͷ
        PQFileHead *pFileHeader, 
        // �����б�
        CPQDataList *pDataList, 		
        // �ӵڼ����㿪ʼ
        int iFromPos, 
        // ���ٸ���
        int& iDataNum);

    //##ModelId=4C5A867100A2
    bool ParseFile(FILE* pFile, 
        // ���ص������б�
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

	//ȡ����ֵ
	void *GetSeriesData
		( 
		UINT4   idStorageMethod, //�洢��ʽ
		INT1    idPhysicalType,	 //������������
		double  rScale,			 //����
		double  rOffset,		 //��Ư
		long    countRawPoints,	 //ԭ����(vector�ṹ�е�count)
		void *  pvectSeriesArray,//ָ������
		long&   countPoints		 //����ʵ�ʵ���
		);

	//ȡ������ʵ�ʵ���
	long GetSeriesCount
		( 
		UINT4   idStorageMethod,//�洢��ʽ
		INT1    idPhysicalType, //������������
		long    countRawPoints, //ԭ����(vector�ṹ�е�count)
		void *  pvectSeriesArray //ָ������
		);

	//ȡ��vector��ֵ
	bool GetSimpVectorArrayValue
		( 
		void *  pdata, //ָ������
		INT1    typePhysical, //������������
		int     idx, //����
		double& value //���ص�ֵ
		);

	//����ʱ������
	char *GeneratTimeSeries(PQFileHead *pFileHeader, CPQDataList *pDataList, int iFromPos, 
									  int& iDataNum,INT1& typePhysical);

	//����ֵ����
	char *GeneratValueSeries(
		PQFileHead *pFileHeader, //�ļ�ͷ��Ϣ
		CPQDataList *pDataList, //�����б�
		int iFromPos,			//��һ����
		int& iDataNum,			//����
		int poistion,			//��Ԫ�����ݽṹ�еĵڼ���ֵ��0base
		int iChaNo				//ͨ����
		);

	//##ModelId=4C7369790150
    int m_iChannelInstIdx;

    //##ModelId=4C7369790160
    int m_iSeriesInstIdx;

    // ����ͨ����ֵ������map��key��ͨ���ţ�ֵ������ָ��map�����map��key��г��������ֵ��г��ֵ��ֻ��һ�ԣ���key=0ʱ����Ϊͨ��ֵ��
    //##ModelId=4C7371780288
    map<int,map<int,float*>*> m_ChaValues;

    //##ModelId=4C7374B6008A
    unsigned int *m_nTimes;

    //##ModelId=4C7380880187
    float *m_fTimes;

    // ��ǰͨ������
    //##ModelId=4C7376C90025
    int m_iCurrentChaIdx;

    // ��ǰг����0Ϊ�������������ƣ�
    //##ModelId=4C737C38030C
    int m_iCurrentHarm;

    // ��ǰͨ����ֵ������map��key��г��������ֵ��г��ֵ��ֻ��һ�ԣ���key=0ʱ����Ϊͨ��ֵ��
    //##ModelId=4C7383B1011E
    map<int,float*> *m_pCurrentChaValues;

    // ��ĸ���
    //##ModelId=4C73B1E102CB
    int m_iPointCount;

	//�������ݵĿ�ʼʱ��
	struct tm tmDataStartTime;

	//��ű���ϵ����key������������value������ϵ��
	map<int,double>	m_SeriesScale;

	//�����Ư��key������������value����Ư
	map<int,double>	m_SeriesOffset;

};



#endif /* OBSERVATION_H_HEADER_INCLUDED_B2ECC1E1 */
