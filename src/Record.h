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

extern PQFileHead	g_FileHeader;	//�ļ�ͷ

extern int StoragMeth[2];	//���ݴ洢��ʽ�������У�

typedef struct
{
	unsigned int nDataLen;
	char *pData;
}UN_COMP_SEG;	//��ѹ��

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

	/*ʹ��zlib�㷨ѹ������
	pSrc��ָ��ѹ������
	nSrcLen����ѹ�����ݳ���
	pDest[out]������ѹ���������
	pDestLen[in/out]��ѹ��������ݳ���
	nCheckSum:У���
	*/
	static int Compress_z(const char *pSrc,unsigned int nSrcLen,char *pDest,unsigned int& nDestLen,unsigned int& nCheckSum);

	/*ʹ��zlib�㷨��ѹ����
	pSrc��ָ��ѹ������
	nSrcLen����ѹ�����ݳ���
	SegList����ѹ�����Ķ��б�
	*/
	static int UnCompress_z(const char *pSrc,unsigned int nSrcLen,vector<UN_COMP_SEG> &SegList);

	//��ʼ��zip��
	int InitZip();

	/*ʹ��zlib�㷨��ѹ����,ִ��ǰҪ����InitZip������ִ�к�Ҫ����EndZip
	pSrcFile��ָ��ѹ�������ļ�
	pOutData����Ž�ѹ����
	iDataLen[in/out]����ѹ���ݳ���
	����ֵ��-1-ʧ�ܣ�0-�ɹ���1-������ѹ
	*/
	int UnCompress_z(FILE *pSrcFile,std::string& sOutData,int& iDataLen);

	//����zip��
	int EndZip();

    //##ModelId=4C7C80FF0056
    bool WritePQDIFFile(FILE *pFile,const GUID& RecordType,UINT4 styleCompression, UINT4 algCompression, 
        // �Ƿ������һ����¼
        bool IsLastRecord);

    // ���³�ʼ����¼
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
	  /*����һ����¼
	  ����ֵ��0���ɹ���-1��ʧ��
	  */
    virtual int ParseOneRecord(FILE* pFile, c_record_mainheader& hdrRecord, UINT4 styleCompression, UINT4 algCompression, bool bHeaderHasRead);

    //##ModelId=4C5A8671004E
    bool ParseCollection(char* pRecordBody, SIZE4 sizeOfBody, 
        // ��colloection��body��ƫ��λ��
        SIZE4 nOffSet);

    //##ModelId=4C71F37A012E
    virtual bool AcceptCollection(const GUID& tag, int level);

    //##ModelId=4C7234DE001C
    virtual bool AcceptScalar(const GUID& tag, int level, INT1 typePhysical, void* pdata);

    //##ModelId=4C72464D032C
    virtual bool AcceptVector(const GUID& tag, int level, INT1 typePhysical, c_vector* pVector, void* pdata);

    // ��Ӽ�¼���׸�����
    //##ModelId=4C5A8671004A
    c_collection_element  * AddCollection(
        // ����Ԫ�ظ���
        const int iElemCount);

    // ��Ӽ����µ�Ƕ�׼���
    //##ModelId=4C7DA141004E
    c_collection_element  * AddCollection(
        // ����Ԫ�ظ���
        const int iElemCount, const GUID &tag, c_collection_element &ce);

    // ��һ��Ԫ�ؼ�һ��scalar���ݣ�������NULLʱ����ʾ�������Ƿ���Ԫ���ڲ�
    //##ModelId=4C7B2AF100FA
    void *AddScalar(
        // ��Ҫ����Ԫ�ص�����
        const char *pData, 
        // ���ݵ���������
        INT1 typePhysical, const GUID &tag, c_collection_element &ce);

    // ��һ��Ԫ�ؼ�һ��Vector����
    //##ModelId=4C7B4A2801A3
    void *AddVector(
        // ��Ҫ����Ԫ�ص�����
        const char *pData, 
        // ��������Ԫ�صĸ���
        const unsigned int nDataCount, 
        // ���ݵ���������
        INT1 typePhysical, const GUID &tag, c_collection_element   &ce);

    // ����record�ڴ��С(δѹ��֮ǰ�Ĵ�С)
    //##ModelId=4C7787A2022C
    unsigned int m_nTotalMemSize;

	long GetNumBytesOfType(long idType);

	//����4�ֽڵ����������򲹵�4�ֽ�������
	SIZE4 padSizeTo4Bytes( SIZE4 sizeOrig );

	/*����tag��ǩ���ؼ�¼����
	tag����¼��ǩ
	���أ�1��Container��¼��2��DataSource��¼��3��MonitorSetting��¼��4��Observation��¼
	*/
	int GetRecordType( const GUID& tag);

	/*��PQDIF��ʽ��ʱ��ת����struct tm�ṹ��ʱ��
	pPQTime��QDIF��ʽ��ʱ��
	pTmTime��struct tm�ṹ��ʱ��
	���أ��ɹ���ʧ��
	*/
	bool PQTime2Tm( TIMESTAMPPQDIF *pPQTime,struct tm *pTmTime);

	/*��PQDIF��ʽ��ʱ��ת����time_t
	pPQTime��QDIF��ʽ��ʱ��
	ttTime��time_tʱ��
	���أ��ɹ���ʧ��
	*/
	bool PQTime2Time_t( TIMESTAMPPQDIF *pPQTime,time_t & ttTime);

	/*��time_t��ʽ��ʱ��ת����PQDIF
	ttTime��time_tʱ��
	pPQTime��QDIF��ʽ��ʱ��
	���أ��ɹ���ʧ��
	*/
	bool Time_t2PQTime( time_t & ttTime,TIMESTAMPPQDIF *pPQTime);

	bool AddMemObjToList(void *pMem,unsigned int nMemLen);

    //##ModelId=4C7B0DF40152
    MemNode* m_pMemObjList;

	MemNode* m_pMemObjTail;

	z_stream m_strm;//zipѹ����
};



#endif /* RECORD_H_HEADER_INCLUDED_B2EC96B3 */
