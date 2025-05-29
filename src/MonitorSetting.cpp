#include "MonitorSetting.h"
#include <time.h>

#ifdef UNIT_TEST
#include "Record.cpp"
#endif

MonitorSetting::MonitorSetting()
{
	m_pCurrentChan = NULL;
	m_pTmpChanInfList = NULL;
	m_iChannelSettIdx = -1;
}

MonitorSetting::~MonitorSetting()
{
}

//##ModelId=4C5A86710090
bool MonitorSetting::CreateMS(int iType, vector<ChannelInf> *ChannelInfs)
{
	long                    lCollItems = 4; //添加4个属性
	long                    lIdxItem = 0;
	c_collection_element	*pCollElem = NULL;
	c_collection_element	*pCollChannelSets = NULL;
	c_collection_element	*pCollOneChannelSet = NULL;

	unsigned int	nAttrVal = 0;

	if( ChannelInfs == NULL )
		return false;

	//添加第一个collection
	pCollElem = AddCollection( lCollItems );
	if( pCollElem == NULL )
		return false;

	//填充collection元素
	
	//Effective时间
	time_t ttNow;
	TIMESTAMPPQDIF pqTime = {0};
	time( &ttNow );
	Time_t2PQTime(ttNow,&pqTime);
	AddScalar((const char *)&pqTime,ID_PHYS_TYPE_TIMESTAMPPQDIF,tagEffective,pCollElem[lIdxItem++]);

	//TimeInstall
	AddScalar((const char *)&pqTime,ID_PHYS_TYPE_TIMESTAMPPQDIF,tagTimeInstalled,pCollElem[lIdxItem++]);

	//tagUseCalibration
	nAttrVal = 0;
	AddScalar((const char *)&nAttrVal,ID_PHYS_TYPE_BOOLEAN4,tagUseCalibration,pCollElem[lIdxItem++]);

	//添加下一级collection(即ChannelSettingsArray，本级的第4个属性)
	//下一级collection属性的个数由通道数决定
	pCollChannelSets=AddCollection( ChannelInfs->size(),tagChannelSettingsArray,pCollElem[lIdxItem++] );
	if( pCollChannelSets == NULL )
		return false;

	long    lOneChanSetItems = 4;//定义一个通道定值的属性个数
	double dVal = 0.000;

	int		iChannelIdx = -1;
	vector<ChannelInf>::iterator chanIt = ChannelInfs->begin();
	for(;chanIt !=ChannelInfs->end();chanIt++)
	{
		iChannelIdx++;
		//再添加下一级collection(即OneChannelDefn)
		pCollOneChannelSet = AddCollection( lOneChanSetItems,tagOneChannelSetting,pCollChannelSets[iChannelIdx] );
		if( pCollOneChannelSet == NULL )
			return false;

		//添加通道定值信息
		long lIdxChSet = 0;

		//通道索引
		AddScalar((const char *)&iChannelIdx,ID_PHYS_TYPE_UNS_INTEGER4,tagChannelDefnIdx,pCollOneChannelSet[lIdxChSet++]);

		//上限定值
		dVal = (double)(*chanIt).HiLimt;
		AddScalar((const char *)&(dVal),ID_PHYS_TYPE_REAL8,tagTriggerHigh,pCollOneChannelSet[lIdxChSet++]);

		//下限定值
		dVal = (double)(*chanIt).LoLimt;
		AddScalar((const char *)&(dVal),ID_PHYS_TYPE_REAL8,tagTriggerLow,pCollOneChannelSet[lIdxChSet++]);

		//通道比例
		dVal = (double)(*chanIt).Rating;
		AddScalar((const char *)&(dVal),ID_PHYS_TYPE_REAL8,tagTriggerRate,pCollOneChannelSet[lIdxChSet++]);

	}//end for(;chanIt !=ChannelInfs->end();chanIt++)

	return true;
}

//##ModelId=4C5A86710092
bool MonitorSetting::ParseFile(FILE* pFile, vector<ChannelInf>* ChannelInfs, PQFileHead* pFileHeader)
{
	if( pFile == NULL || ChannelInfs==NULL || pFileHeader == NULL)
		return false;

	m_pTmpChanInfList = ChannelInfs;

	c_record_mainheader	hdrRecord = {0};
	unsigned int nCompressStyle = ID_COMP_STYLE_NONE;
	
	if( pFileHeader->CompressType != 0 )
	{
		nCompressStyle = ID_COMP_STYLE_RECORDLEVEL;
	}

	int iRet = ParseOneRecord(pFile,hdrRecord,nCompressStyle,pFileHeader->CompressType,false);
	if( iRet != 0 )
	{
		return false;
	}

	return true;
}

//##ModelId=4C733B3E01CB
bool MonitorSetting::AcceptCollection(const GUID& tag, int level)
{
	if( PQDIF_IsEqualGUID( tag, tagChannelSettingsArray ) ) //全部通道的定值
	{
		m_iChannelSettIdx = -1;
		m_pCurrentChan = NULL;
		return true;
	}
	return true;
}

//##ModelId=4C733B3E01CE
bool MonitorSetting::AcceptScalar(const GUID& tag, int level, INT1 typePhysical, void* pdata)
{
	if( PQDIF_IsEqualGUID(tag,tagChannelDefnIdx) )//通道索引
	{
		if( ID_PHYS_TYPE_UNS_INTEGER4 != typePhysical )
			return false;
		m_iChannelSettIdx = *((int*)pdata);

		if( m_pTmpChanInfList == NULL )
			return false;

		try
		{
			m_pCurrentChan = &(m_pTmpChanInfList->at(m_iChannelSettIdx));
			return true;
		}
		catch(...)
		{
			return false;
		}//end try
	}//end 通道索引
	else if(m_iChannelSettIdx == -1 || m_pCurrentChan == NULL)
	{
		return true;
	}

	double dValue = 0.0000;

	if( PQDIF_IsEqualGUID(tag,tagCalRatio) )//通道比例
	{
		if( ID_PHYS_TYPE_REAL8 != typePhysical )
			return false;
		dValue = *((double*)pdata);
		m_pCurrentChan->Rating = (int)dValue;
	}
	else if( PQDIF_IsEqualGUID(tag,tagTriggerHigh) )//通道高限
	{
		if( ID_PHYS_TYPE_REAL8 != typePhysical )
			return false;
		dValue = *((double*)pdata);
		m_pCurrentChan->HiLimt = (float)dValue;
	}
	else if( PQDIF_IsEqualGUID(tag,tagTriggerLow) )//通道低限
	{
		if( ID_PHYS_TYPE_REAL8 != typePhysical )
			return false;
		dValue = *((double*)pdata);
		m_pCurrentChan->LoLimt = (float)dValue;
	}

	return true;
}

//##ModelId=4C733B3E01DB
bool MonitorSetting::AcceptVector(const GUID& tag, int level, INT1 typePhysical, c_vector* pVector, void* pdata)
{
	return true;
}

//##ModelId=4C733B3E01E1
int MonitorSetting::ParseOneRecord(FILE* pFile, c_record_mainheader& hdrRecord, UINT4 styleCompression, UINT4 algCompression, bool bHeaderHasRead)
{
	if( pFile == NULL )
		return -1;
	
	if( !bHeaderHasRead )
	{
		int countRead = fread( &hdrRecord, sizeof( hdrRecord ), 1, pFile );
		if ( feof( pFile ) || countRead != 1 )
		{
			return -1;
		}
	}

	int iRecordType = GetRecordType( hdrRecord.tagRecordType );
	if(  iRecordType != 3 )
	{
		return iRecordType;//不是MonitorSetting记录
	}

	//调用父类函数
	return Record::ParseOneRecord(pFile,hdrRecord,styleCompression,algCompression,true);
}

