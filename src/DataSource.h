#ifndef DATASOURCE_H_HEADER_INCLUDED_B2EC81CE
#define DATASOURCE_H_HEADER_INCLUDED_B2EC81CE
#include "Record.h"
#include <vector>

using namespace std;

//##ModelId=4C5A86710080
class DataSource : public Record
{
  public:
    DataSource();
	virtual ~DataSource();

    //##ModelId=4C5A86710082
    bool CreateDS(
        // 文件头
        PQFileHead *pFileHeader, vector<ChannelInf> *pChannelInfs);

    //##ModelId=4C5A86710084
	bool ParseFile(FILE* pFile, vector<ChannelInf> *ChannelInfs,PQFileHead* pFileHeader,vector<PQDIFInterFace::VAL_UNIT>  *Units);

  protected:
    //##ModelId=4C71F43F019D
    bool AcceptCollection(const GUID& tag, int level);

    //##ModelId=4C7235760367
    bool AcceptScalar(const GUID& tag, int level, INT1 typePhysical, void* pdata);

    //##ModelId=4C72469B02ED
    bool AcceptVector(const GUID& tag, int level, INT1 typePhysical, c_vector* pVector, void* pdata);

    //##ModelId=4C7315F40376
    int ParseOneRecord(FILE* pFile, c_record_mainheader& hdrRecord, UINT4 styleCompression, UINT4 algCompression, bool bHeaderHasRead);

    //##ModelId=4C71F1B001B1
    ChannelInf m_CurrentChan;

	ChannelInf *m_pCurrentChan;

    //##ModelId=4C71F1FD01A2
    int m_iChannelDefnIdx;

    //##ModelId=4C71F21601C2
    int m_iSeriesDefnIdx;

    //##ModelId=4C722A930250
    vector<ChannelInf> *m_pChanInfList;

	vector<PQDIFInterFace::VAL_UNIT>  *m_Units;

	int		m_iQuantityMeasuredID;	//测量的数据类型

};



#endif /* DATASOURCE_H_HEADER_INCLUDED_B2EC81CE */
