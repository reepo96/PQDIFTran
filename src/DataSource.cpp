#include "DataSource.h"
#include <time.h>

#ifdef UNIT_TEST
#include "Record.cpp"
#endif

DataSource::DataSource()
{
	m_iQuantityMeasuredID = 0;
	m_iChannelDefnIdx = -1;
	m_iSeriesDefnIdx = -1;
	m_pChanInfList = NULL;
	m_pCurrentChan = NULL;
	m_Units = NULL;
}

DataSource::~DataSource()
{
	
}

//##ModelId=4C5A86710082
bool DataSource::CreateDS(PQFileHead *pFileHeader, vector<ChannelInf> *pChannelInfs)
{
	long                    lCollItems = 4; //添加4个属性
	long                    lIdxItem = 0;
	c_collection_element	*pCollElem = NULL;
	c_collection_element	*pCollChannelDefns = NULL;
	c_collection_element	*pCollOneChannelDefn = NULL;
	c_collection_element	*pCollSeriesDefns = NULL;
	c_collection_element	*pCollOneSeriesDefn = NULL;

	unsigned int	nAttrVal = 0;
	int		iSeriesNum = 2;	//序列数（至少两个序列）
	
	//数量单位、数量性质
	int iQTUnitId = ID_QU_NONE;		//数量单位
	GUID guQTCharacter = ID_QC_NONE;//数量性质

	if( pChannelInfs == NULL )
		return false;

	//添加第一个collection
	pCollElem = AddCollection( lCollItems );
	if( pCollElem == NULL )
		return false;

	//填充collection元素
	
	//DataSource类型
	AddScalar((const char *)&ID_DS_TYPE_MEASURE,ID_PHYS_TYPE_GUID,tagDataSourceTypeID,pCollElem[lIdxItem++]);

	//DataSource名称
	//AddVector("SH DS",strlen("SH DS")+1,ID_PHYS_TYPE_CHAR1,tagNameDS,pCollElem[lIdxItem++]);
	char szDSName[64] = {0};
	switch(pFileHeader->FileType)
	{
	case 1:
		iSeriesNum = 4;//4序列：时间、最大、最小、平均
		strcpy(szDSName,"frequency trend");
		iQTUnitId = ID_QU_HERTZ;
		guQTCharacter = ID_QC_FREQUENCY;
		break;
	case 2:
		iSeriesNum = 4;//4序列：时间、最大、最小、平均
		strcpy(szDSName,"Volt/Cur trend");
		iQTUnitId = ID_QU_PERCENT;
		guQTCharacter = ID_QC_RMS;
		break;
	case 4:
		iSeriesNum = 3;//3序列：时间、最大、平均
		strcpy(szDSName,"UBalance trend");
		iQTUnitId = ID_QU_PERCENT;
		guQTCharacter = ID_QC_S0S1;
		break;
	case 8:
		iSeriesNum = 4;//4序列：时间、畸变率、最大、平均
		strcpy(szDSName,"harmonious trend");
		break;
	case 16:
		iSeriesNum = 3;//3序列：时间、最大、平均
		strcpy(szDSName,"i-harmonious trend");
		break;
	case 32:
		iSeriesNum = 3;//3序列：时间、电压变动、电压变动频度
		strcpy(szDSName,"Volt fluctuantion trend");
		break;
	case 64:
		iSeriesNum = 2;//2序列：时间、短时闪变
		strcpy(szDSName,"flicker trend");
		iQTUnitId = ID_QU_NONE;
		guQTCharacter = ID_QC_FLKR_PST;
		break;
	case 128:
		iSeriesNum = 3;//3序列：时间、谐波有功功率、谐波无功功率
		strcpy(szDSName,"H-Power trend");
		iQTUnitId = ID_QU_WATTS;
		break;
	case 256:
		iSeriesNum = 5;//5序列：时间、正有功电能、正无功电能、负有功电能、负无功电能
		strcpy(szDSName,"energy trend");
		break;
	}
	AddVector(szDSName,strlen(szDSName)+1,ID_PHYS_TYPE_CHAR1,tagNameDS,pCollElem[lIdxItem++]);

	//Effective时间
	time_t ttNow;
	TIMESTAMPPQDIF pqTime = {0};
	time( &ttNow );
	Time_t2PQTime(ttNow,&pqTime);
	AddScalar((const char *)&pqTime,ID_PHYS_TYPE_TIMESTAMPPQDIF,tagEffective,pCollElem[lIdxItem++]);

	//添加下一级collection(即ChannelDefns，本级的第4个属性)
	//下一级collection属性的个数由通道数决定
	pCollChannelDefns=AddCollection( pChannelInfs->size(),tagChannelDefns,pCollElem[lIdxItem++] );
	if( pCollChannelDefns == NULL )
		return false;

	long    lOneChanDefItems = 7;//定义一个通道的属性个数

	int		iChannelIdx = -1;
	vector<ChannelInf>::iterator chanIt = pChannelInfs->begin();
	for(;chanIt !=pChannelInfs->end();chanIt++)
	{
		iChannelIdx++;
		//再添加下一级collection(即OneChannelDefn)
		pCollOneChannelDefn = AddCollection( lOneChanDefItems,tagOneChannelDefn,pCollChannelDefns[iChannelIdx] );
		if( pCollOneChannelDefn == NULL )
			return false;

		//添加通道定义信息
		long lIdxChDefn = 0;

		if( (*chanIt).ChannelNo <= 0 )//线路数据
		{
			//物理通道号
			AddScalar((const char *)&(*chanIt).LineNo,ID_PHYS_TYPE_UNS_INTEGER4,tagPhysicalChannel,
				pCollOneChannelDefn[lIdxChDefn++]);

			//通道名称
			AddVector((*chanIt).LineName,strlen((*chanIt).LineName)+1,ID_PHYS_TYPE_CHAR1,
				tagChannelName,pCollOneChannelDefn[lIdxChDefn++]);

			//相别(无相别)
			nAttrVal = ID_PHASE_NONE;
			AddScalar((const char *)&nAttrVal,ID_PHYS_TYPE_UNS_INTEGER4,tagPhaseID,
				pCollOneChannelDefn[lIdxChDefn++]);
		}
		else //通道数据
		{
			//物理通道号
			AddScalar((const char *)&(*chanIt).ChannelNo,ID_PHYS_TYPE_UNS_INTEGER4,tagPhysicalChannel,
				pCollOneChannelDefn[lIdxChDefn++]);

			//通道名称
			AddVector((*chanIt).ChannelName,strlen((*chanIt).ChannelName)+1,ID_PHYS_TYPE_CHAR1,
				tagChannelName,pCollOneChannelDefn[lIdxChDefn++]);

			//相别
			int iPhase = ID_PHASE_AN;
			int iFirstChannelIdx = iChannelIdx-3;
			while( iFirstChannelIdx != iChannelIdx )
			{
				//我们规定pChannelInfs中的通道信息是根据线路，然后按相别顺序存放
				if( iFirstChannelIdx < 0 )
				{
					iFirstChannelIdx++;
					continue;
				}
				else
				{
					ChannelInf temChan = pChannelInfs->at(iFirstChannelIdx);
					if( temChan.LineNo == (*chanIt).LineNo )
					{
						break;	//同一线路的第一个通道
					}
					else
					{
						iFirstChannelIdx++;
					}
				}//end if( iPreChannelIdx < 0 )
			}

			int iChanOffset = iChannelIdx - iFirstChannelIdx;
			switch( iChanOffset )
			{
			case 0:
				iPhase = ID_PHASE_AN;
				break;
			case 1:
				iPhase = ID_PHASE_BN;
				break;
			case 2:
				iPhase = ID_PHASE_CN;
				break;
			case 3:
				iPhase = ID_PHASE_NG;
				break;
			}

			AddScalar((const char *)&iPhase,ID_PHYS_TYPE_UNS_INTEGER4,tagPhaseID,
				pCollOneChannelDefn[lIdxChDefn++]);
		}//end 通道数据

		//组名（线路？间隔？）
		AddVector((*chanIt).LineName,strlen((*chanIt).LineName)+1,ID_PHYS_TYPE_CHAR1,
			tagGroupName,pCollOneChannelDefn[lIdxChDefn++]);

		//数量类别
		AddScalar((const char *)&ID_QT_VALUELOG,ID_PHYS_TYPE_GUID,tagQuantityTypeID,
			pCollOneChannelDefn[lIdxChDefn++]);

		//数量测量ID
		nAttrVal = ID_QM_NONE;
		AddScalar((const char *)&nAttrVal,ID_PHYS_TYPE_UNS_INTEGER4,tagQuantityMeasuredID,
			pCollOneChannelDefn[lIdxChDefn++]);

		//添加序列定义（不同的数据有不同的序列数量）
		pCollSeriesDefns = AddCollection(iSeriesNum,tagSeriesDefns,pCollOneChannelDefn[lIdxChDefn++]);
		if( pCollSeriesDefns == NULL )
			return false;

		//定义每个序列属性
		long    lOneSeriesItems = 4;//定义一个序列的属性个数
		for(int i=0;i<iSeriesNum;i++)
		{
			pCollOneSeriesDefn = AddCollection(lOneSeriesItems,tagOneSeriesDefn,pCollSeriesDefns[i]);
			if( pCollOneSeriesDefn == NULL )
				return false;

			//定义序列属性
			int		iSeriesIdx = 0;
			
			if( i == 0 )//时间
			{
				//值类型
				AddScalar((const char *)&ID_SERIES_VALUE_TYPE_TIME,ID_PHYS_TYPE_GUID,tagValueTypeID,
					pCollOneSeriesDefn[iSeriesIdx++]);

				//序列值不支持绝对时间？
				//if( pFileHeader->DataTimeType == 1) //相对时间
				//{
					iQTUnitId = ID_QU_SECONDS;
				/*}
				else //绝对时间
				{
					iQTUnitId = ID_QU_TIMESTAMP;
				}*/
				guQTCharacter = ID_QC_NONE;
			}
			else //数值
			{
				//值类型
				AddScalar((const char *)&ID_SERIES_VALUE_TYPE_VAL,ID_PHYS_TYPE_GUID,tagValueTypeID,
					pCollOneSeriesDefn[iSeriesIdx++]);

				switch( pFileHeader->FileType )
				{
				case 2:
					if( (*chanIt).ChannelType == 1)
					{
						iQTUnitId = ID_QU_PERCENT;
					}
					else
					{
						iQTUnitId = ID_QU_AMPS;
					}
					guQTCharacter = ID_QC_RMS;
				case 8://谐波
					if( (*chanIt).ChannelType == 1)
					{
						iQTUnitId = ID_QU_VOLTS;
					}
					else
					{
						iQTUnitId = ID_QU_AMPS;
					}
					guQTCharacter = ID_QC_HRMS;
					break;
				case 16://间谐波
					if( (*chanIt).ChannelType == 1)
					{
						iQTUnitId = ID_QU_VOLTS;
					}
					else
					{
						iQTUnitId = ID_QU_AMPS;
					}
					guQTCharacter = ID_QC_IHRMS;
				case 128://谐波功率
					if( i==1)
					{
						iQTUnitId = ID_QU_WATTS;
						guQTCharacter = ID_QC_P;//有功功率
					}
					else if( i==2)
					{
						iQTUnitId = ID_QU_VARS;
						guQTCharacter = ID_QC_Q;//无功功率
					}
					break;
				case 256://电能
					if( i==1 || i==3 )
					{
						iQTUnitId = ID_QU_WATTHOURS;						
					}
					else if( i==2 || i==4 )
					{
						iQTUnitId = ID_QU_VARHOURS;
					}

					if( i==1 )
					{
						guQTCharacter = ID_QC_P_INTG_POS;//正有功电能
					}
					else if( i==2 )
					{
						guQTCharacter = ID_QC_Q_INTG_POS;//正无功电能
					}
					else if( i==3 )
					{
						guQTCharacter = ID_QC_P_INTG_NEG;//负有功电能
					}
					else if( i==4 )
					{
						guQTCharacter = ID_QC_Q_INTG_NEG;//负无功电能
					}
					break;
				}
			}//end 数值

			//单位
			AddScalar((const char *)&iQTUnitId,ID_PHYS_TYPE_UNS_INTEGER4,tagQuantityUnitsID,
				pCollOneSeriesDefn[iSeriesIdx++]);

			//数量性质
			AddScalar((const char *)&guQTCharacter,ID_PHYS_TYPE_GUID,tagQuantityCharacteristicID,
				pCollOneSeriesDefn[iSeriesIdx++]);

			//存储方式(直接存储方式)
			nAttrVal = ID_SERIES_METHOD_VALUES;
			AddScalar((const char *)&nAttrVal,ID_PHYS_TYPE_UNS_INTEGER4,tagStorageMethodID,
				pCollOneSeriesDefn[iSeriesIdx++]);

		}//end 定义每个序列属性

	}//end for(;chanIt !=pChannelInfs->end();chanIt++)

	return true;
}

//##ModelId=4C5A86710084
bool DataSource::ParseFile(FILE* pFile, vector<ChannelInf> *ChannelInfs,PQFileHead* pFileHeader,vector<PQDIFInterFace::VAL_UNIT>  *Units)
{
	if( pFile == NULL || ChannelInfs==NULL )
		return false;

	m_pChanInfList = ChannelInfs;
	m_Units = Units;

	c_record_mainheader	hdrRecord = {0};
	unsigned int nCompressStyle = ID_COMP_STYLE_NONE;

	if( pFileHeader->CompressType != 0 )
	{
		nCompressStyle = ID_COMP_STYLE_RECORDLEVEL;
	}

	int iRet = ParseOneRecord(pFile,hdrRecord,nCompressStyle,g_FileHeader.CompressType,false);
	if( iRet != 0 )
	{
		return false;
	}

	return true;
}

//##ModelId=4C71F43F019D
bool DataSource::AcceptCollection(const GUID& tag, int level)
{
	if( m_pChanInfList == NULL)
		return false;

	if( PQDIF_IsEqualGUID( tag, tagChannelDefns )) //全部通道的定义
	{
		m_iChannelDefnIdx = -1;
		m_iSeriesDefnIdx = -1;
		return true;
	}
	else if( PQDIF_IsEqualGUID( tag, tagOneChannelDefn )) //一个通道的定义
	{
		memset(&m_CurrentChan,0,sizeof(m_CurrentChan));
		m_iChannelDefnIdx++;
		m_iSeriesDefnIdx = -1;

		m_CurrentChan.Index = m_iChannelDefnIdx;
		m_pChanInfList->push_back(m_CurrentChan);

		m_pCurrentChan = &(m_pChanInfList->at( m_pChanInfList->size()-1 ) );

		return true;
	}
	else if( PQDIF_IsEqualGUID( tag, tagSeriesDefns )) //全部序列的定义
	{
		m_iSeriesDefnIdx = -1;
		return true;
	}
	else if( PQDIF_IsEqualGUID( tag, tagOneSeriesDefn )) //全部序列的定义
	{
		m_iSeriesDefnIdx++;
		if( m_iSeriesDefnIdx > 1 )
		{
			return false;//目前我们暂只支持两序列的数据
		}
		return true;
	}

	return false;
}

//##ModelId=4C7235760367
bool DataSource::AcceptScalar(const GUID& tag, int level, INT1 typePhysical, void* pdata)
{
	GUID	guid;

	if( m_iChannelDefnIdx >= 0 && m_iSeriesDefnIdx == -1) //通道的定义
	{
		if( PQDIF_IsEqualGUID(tag,tagPhysicalChannel) )//物理通道号
		{
			if( ID_PHYS_TYPE_UNS_INTEGER4 != typePhysical )
				return false;

			m_pCurrentChan->ChannelNo = *((int*)pdata);
			return true;
		}
		else if( PQDIF_IsEqualGUID(tag,tagQuantityMeasuredID) )//测量的数据类型ID
		{
			if( ID_PHYS_TYPE_UNS_INTEGER4 != typePhysical )
				return false;

			m_iQuantityMeasuredID = *((int*)pdata);
			return true;
		}
	}//end 通道的定义
	else if( m_iSeriesDefnIdx == 0)//第一序列定义
	{
		if( PQDIF_IsEqualGUID(tag,tagValueTypeID) )//数据类型
		{
			if( ID_PHYS_TYPE_GUID != typePhysical )
				return false;

			memcpy(&guid,pdata,sizeof(GUID) );
			if( !PQDIF_IsEqualGUID(guid,ID_SERIES_VALUE_TYPE_TIME) )
			{
				//我们只处理第一序列数据类型为时间类型的数据
				return false;
			}
			return true;
		}//end 数据类型
		else if( PQDIF_IsEqualGUID(tag,tagQuantityUnitsID) )//数据单位
		{
			if( ID_PHYS_TYPE_UNS_INTEGER4 != typePhysical )
				return false;

			int iUnits = *((int*)pdata);
			memcpy(&guid,pdata,sizeof(GUID) );
			if( iUnits == ID_QU_SECONDS )//单位是秒
			{
				g_FileHeader.DataTimeType = 1;//相对时间
				g_FileHeader.DataTimeUnit = 1;
				return true;
			}
			else if( iUnits == ID_QU_TIMESTAMP )//绝对时间,类型TIMESTAMPPQDIF
			{
				g_FileHeader.DataTimeType = 2;
				return true;
			}
			return false;//是其它类型的不处理
		}//end 数据单位

	}//end 第一序列定义
	else if( m_iSeriesDefnIdx == 1)//第二序列定义
	{
		if( PQDIF_IsEqualGUID(tag,tagQuantityCharacteristicID) )//数据特征
		{
			if( ID_PHYS_TYPE_GUID != typePhysical )
				return false;

			memcpy(&guid,pdata,sizeof(GUID) );
			if( PQDIF_IsEqualGUID(guid,ID_QC_RMS) ) //有效值
			{
				if( m_iQuantityMeasuredID == ID_QM_VOLTAGE )
				{
					g_FileHeader.FileType = 2;//电压类型
				}
				else if( m_iQuantityMeasuredID == ID_QM_CURRENT )
				{
					g_FileHeader.FileType = 8;//电流类型
				}
				return true;
			}
			else if( PQDIF_IsEqualGUID(guid,ID_QC_FREQUENCY) ) //频率
			{
				g_FileHeader.FileType = 1;	//频率
			}
			else if( PQDIF_IsEqualGUID(guid,ID_QC_FLKR_PST) || PQDIF_IsEqualGUID(guid,ID_QC_FLKR_PLT))
			{
				g_FileHeader.FileType = 7;	//闪变
			}
			else if( PQDIF_IsEqualGUID(guid,ID_QC_HRMS) )
			{
				g_FileHeader.FileType = 4;	//谐波
			}
			else if( PQDIF_IsEqualGUID(guid,ID_QC_P) )
			{
				g_FileHeader.FileType = 4;	//谐波功率
			}

			return true;
		}//end 数据类型

		//数量单位
		if( m_iChannelDefnIdx == 0 && PQDIF_IsEqualGUID(tag,tagQuantityUnitsID) && m_Units !=NULL)
		{
			if( ID_PHYS_TYPE_UNS_INTEGER4 != typePhysical )
				return false;

			int iUnit = *((int*)pdata);

			if( iUnit == ID_QU_VOLTS ) //伏特
			{
				m_Units->push_back(PQDIFInterFace::VOLT);
			}
			else if( iUnit == ID_QU_AMPS ) //安培
			{
				m_Units->push_back(PQDIFInterFace::AMP);
			}
			else if( iUnit == ID_QU_HERTZ )//频率
			{
				m_Units->push_back(PQDIFInterFace::HERTZ);
			}
			else if( iUnit == ID_QU_DEGREES)//度
			{
				m_Units->push_back(PQDIFInterFace::DEGRESS);
			}
			else if( iUnit == ID_QU_PERCENT )//百分比
			{
				m_Units->push_back(PQDIFInterFace::PERCENT);
			}
			else if( iUnit == ID_QU_WATTS )//瓦特
			{
				m_Units->push_back(PQDIFInterFace::WATTS);
			}
			else
			{
				m_Units->push_back(PQDIFInterFace::NONE);
			}

		}

		return true;
	}//end 第二序列定义

	if( PQDIF_IsEqualGUID(tag,tagStorageMethodID) )//存储方式
	{
		if( ID_PHYS_TYPE_UNS_INTEGER4 != typePhysical )
			return false;

		if( m_iSeriesDefnIdx == 0 || m_iSeriesDefnIdx == 1)
		{
			StoragMeth[m_iSeriesDefnIdx] = *((int*)pdata);
			return true;
		}
		else
		{
			return false;
		}
	}//end 存储方式

	return true;
}

//##ModelId=4C72469B02ED
bool DataSource::AcceptVector(const GUID& tag, int level, INT1 typePhysical, c_vector* pVector, void* pdata)
{
	int iCpyLen = 0;

	if( m_iChannelDefnIdx >= 0 && m_iSeriesDefnIdx == -1) //通道的定义
	{
		if( PQDIF_IsEqualGUID(tag,tagChannelName) )//通道名
		{
			if( pVector->count == 0 || ID_PHYS_TYPE_CHAR1 != typePhysical )
				return false;

			iCpyLen = ( pVector->count > sizeof(m_pCurrentChan->ChannelName)-1 )?
				sizeof(m_pCurrentChan->ChannelName)-1 : pVector->count;
			memcpy(m_pCurrentChan->ChannelName,pdata,iCpyLen);
			return true;
		}
		else if( PQDIF_IsEqualGUID(tag,tagGroupName) )//组名(间隔名)
		{
			if( pVector->count == 0 || ID_PHYS_TYPE_CHAR1 != typePhysical )
				return false;

			iCpyLen = ( pVector->count > sizeof(m_pCurrentChan->LineName)-1 )?
				sizeof(m_pCurrentChan->LineName)-1 : pVector->count;
			memcpy(m_pCurrentChan->LineName,pdata,iCpyLen);
			return true;
		}
	}//end 通道的定义

	return true;
}

//##ModelId=4C7315F40376
int DataSource::ParseOneRecord(FILE* pFile, c_record_mainheader& hdrRecord, UINT4 styleCompression, UINT4 algCompression, bool bHeaderHasRead)
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
	if(  iRecordType != 2 )
	{
		return iRecordType;//不是DataSource记录
	}

	//调用父类函数
	return Record::ParseOneRecord(pFile,hdrRecord,styleCompression,algCompression,true);
}

