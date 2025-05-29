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
    // PQDIF��ʽת����˫�ϸ�ʽ����������ʽ
    //##ModelId=4C5B6A8303E7
    bool PQDIF2SHPQ(
        // ��ת�����ļ���
        const char* pSrcFile, PQFileHead* pFileHeader, vector<ChannelInf>& ChannelInfs, 
		vector<PQDIFInterFace::VAL_UNIT>  *Units,
        // ���ص������б�
        CPQDataList& DataList);

    // PQDIF��ʽת����˫�ϸ�ʽ����������ʽ
    //##ModelId=4C5B6A8303EC
    bool PQDIF2SHPQ(
        // ��ת�����ļ���
        const char* pSrcFile, 
        // ת��Ŀ���ļ���
        const char* pDestFile);

    // ˫�ϸ�ʽ��������ת����PQDIF��ʽ�ļ�
    //##ModelId=4C5B6A84000E
    bool SHPQ2PQDIF(
        // ��ת�����ļ���
        const char* pSrcFile, 
        // ת��Ŀ���ļ���
        const char* pDestFile);

	//�����ļ�ͷ��Ϣ����PQDIF��ʽ���ļ�ͷ�������浽�ļ���
	bool CreateFileHeader(SHPQDIF::PQFileHead* pFileHeader,FILE *pFile);

    // ˫�ϸ�ʽ��������ת����PQDIF��ʽ�ļ�
    //##ModelId=4C5B6A840011
    bool SHPQ2PQDIF(SHPQDIF::PQFileHead* pFileHeader, vector<SHPQDIF::ChannelInf> *pChannelInfs, 
        // ��ת���������б�
        CPQDataList *pDataList, 
        // ת��Ŀ���ļ���
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
