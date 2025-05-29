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

	//PQDIF��ʽ���ļ�ͷ
	typedef struct PQFileHead
	{
		// ���,"SHPQ"
		char Flag[4];

		// �汾����:�����, ��:�����
		unsigned short Ver;

		//�����ļ�ͷ��С
		unsigned short HeadSize;

		// �ļ����ͣ�
		//1<<0��Ƶ��ƫ������
		//1<<1����ѹƫ������
		//1<<2�����಻ƽ���
		//1<<3��г������
		//1<<4����г������
		//1<<5����ѹ����
		//1<<6������
		//1<<7��г������
		//1<<8������
		//1<<9���ϸ���
		unsigned int FileType;

		// �ļ����⣬100char�����㲹��
		char FileTitle[100];

		// ѹ�����ͣ��ļ�ͷ��ѹ������ѹ�����ݲ��֣���
		// 0����ѹ��
		// 1��zlibѹ����ʽ
		char CompressType;

		// �ļ�������ʼʱ�䣬ͨ��Ϊĳ�����ʱ��
		struct tm DataStartTime;

		// �ļ����ݽ���ʱ��
		struct tm DataEndTime;

		// ����ʱ�����ͣ�
		// 1�����ʱ��
		// 2������ʱ��
		char DataTimeType;

		// ����ʱ�䵥λ��DataTimeType=1ʱ��Ч����
		// 1����
		// 2������
		// 3��Сʱ
		// 4����
		// 5����
		char DataTimeUnit;

		// ��Ч��·������<=36,0��ʾ����·����
		unsigned char ActiveLNum;

		// ��Чͨ��������<=96,0��ʾ��ͨ������
		unsigned char ActiveChNum;

		// ÿ�������ݴ�С,�ڽ��洢��������ʱ��Ч
		unsigned int PerPointSize;

		// ������
		unsigned int TotalPoint;

		// ͳ���������ļ��ֵ�ƫ�Ƶ�ַ
		unsigned int StatDataOffset;

		// ͳ�����ݳ��ȣ�Ϊ0��ʾ��ͳ������
		unsigned int StatDataLen;

		//����Ԫ�ظ���
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

	//ͨ��������Ϣ
	struct ChaCfgInf
	{
		unsigned char Index;
		int LineNo;
		int ChannelNo;
		unsigned char ChannelType;
		char LineName[64];
		char ChannelName[64];
	};

	//һ�㶨ֵ������Ϣ
	struct PQNormalSetInf
	{
		unsigned char ChanIndex;
		float HiLimt;
		float LoLimt;
		float Ratio;
		float Rating;
	};

	//г����ֵ������Ϣ
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
	  //���嵥λ
	  typedef enum valUnit {NONE/*��*/,VOLT/*����(V)*/,AMP/*����(A)*/,HERTZ/*����(hz)*/,DEGRESS/*��*/,
		  PERCENT/*�ٷֱ�(%)*/,WATTS/*����*/} VAL_UNIT;

    // PQDIF��ʽת����˫�ϸ�ʽ����������ʽ
    //##ModelId=4C5A867100B1
    bool PQDIF2SHPQ(
        // ��ת�����ļ���
		const char* pSrcFile, SHPQDIF::PQFileHead* pFileHeader, vector<SHPQDIF::ChannelInf> **ChannelInfs, 
		vector<VAL_UNIT>  **Units,/*ͨ��ÿ����ֵ��Ӧ�ĵ�λ*/
        // ���ص������б�
        CPQDataList **pDataList);

    // PQDIF��ʽת����˫�ϸ�ʽ����������ʽ
    //##ModelId=4C5A867100B4
    bool PQDIF2SHPQ(
        // ��ת�����ļ���
        const char* pSrcFile, 
        // ת��Ŀ���ļ���
        const char* pDestFile);

    // ˫�ϸ�ʽ��������ת����PQDIF��ʽ�ļ�
    //##ModelId=4C5A867100B7
    bool SHPQ2PQDIF(
        // ��ת�����ļ���
        const char* pSrcFile, 
        // ת��Ŀ���ļ���
        const char* pDestFile);

    // ˫�ϸ�ʽ��������ת����PQDIF��ʽ�ļ�
    //##ModelId=4C5A867100BA
    bool SHPQ2PQDIF(SHPQDIF::PQFileHead* pFileHeader, vector<SHPQDIF::ChannelInf> *pChannelInfs, 
        // ��ת���������б�
        CPQDataList *pDataList, 
        // ת��Ŀ���ļ���
        const char* pDestFile);

	void ClearData( CPQDataList **DataList );

	void ClearChanInf( vector<SHPQDIF::ChannelInf> **ChannelInfs );

	void ClearUnits( vector<VAL_UNIT> **Units );

};



#endif /* PQDIFINTERFACE_H_HEADER_INCLUDED_B2ECBAB7 */
