#ifndef MONITORSETTING_H_HEADER_INCLUDED_B2ECC0BE
#define MONITORSETTING_H_HEADER_INCLUDED_B2ECC0BE
#include "Record.h"
#include <vector>

using namespace std;

//##ModelId=4C5A8671008E
class MonitorSetting : public Record
{
  public:
	MonitorSetting();
	virtual ~MonitorSetting();

    //##ModelId=4C5A86710090
    bool CreateMS(
        // 数据类型，参见自定义格式中电能质量趋势数据类型的定义
        int iType, vector<ChannelInf> *ChannelInfs);

    //##ModelId=4C5A86710092
    bool ParseFile(FILE* pFile, vector<ChannelInf>* ChannelInfs, PQFileHead* pFileHeader);

  protected:
    //##ModelId=4C733B3E01CB
    bool AcceptCollection(const GUID& tag, int level);

    //##ModelId=4C733B3E01CE
    bool AcceptScalar(const GUID& tag, int level, INT1 typePhysical, void* pdata);

    //##ModelId=4C733B3E01DB
    bool AcceptVector(const GUID& tag, int level, INT1 typePhysical, c_vector* pVector, void* pdata);

    //##ModelId=4C733B3E01E1
    int ParseOneRecord(FILE* pFile, c_record_mainheader& hdrRecord, UINT4 styleCompression, UINT4 algCompression, bool bHeaderHasRead);

    //##ModelId=4C733CA60154
    ChannelInf *m_pCurrentChan;

    //##ModelId=4C733CA60164
    int m_iChannelSettIdx;

    //##ModelId=4C733CA60166
    vector<ChannelInf> *m_pTmpChanInfList;

};



#endif /* MONITORSETTING_H_HEADER_INCLUDED_B2ECC0BE */
