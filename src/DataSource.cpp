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
	long                    lCollItems = 4; //���4������
	long                    lIdxItem = 0;
	c_collection_element	*pCollElem = NULL;
	c_collection_element	*pCollChannelDefns = NULL;
	c_collection_element	*pCollOneChannelDefn = NULL;
	c_collection_element	*pCollSeriesDefns = NULL;
	c_collection_element	*pCollOneSeriesDefn = NULL;

	unsigned int	nAttrVal = 0;
	int		iSeriesNum = 2;	//�������������������У�
	
	//������λ����������
	int iQTUnitId = ID_QU_NONE;		//������λ
	GUID guQTCharacter = ID_QC_NONE;//��������

	if( pChannelInfs == NULL )
		return false;

	//��ӵ�һ��collection
	pCollElem = AddCollection( lCollItems );
	if( pCollElem == NULL )
		return false;

	//���collectionԪ��
	
	//DataSource����
	AddScalar((const char *)&ID_DS_TYPE_MEASURE,ID_PHYS_TYPE_GUID,tagDataSourceTypeID,pCollElem[lIdxItem++]);

	//DataSource����
	//AddVector("SH DS",strlen("SH DS")+1,ID_PHYS_TYPE_CHAR1,tagNameDS,pCollElem[lIdxItem++]);
	char szDSName[64] = {0};
	switch(pFileHeader->FileType)
	{
	case 1:
		iSeriesNum = 4;//4���У�ʱ�䡢�����С��ƽ��
		strcpy(szDSName,"frequency trend");
		iQTUnitId = ID_QU_HERTZ;
		guQTCharacter = ID_QC_FREQUENCY;
		break;
	case 2:
		iSeriesNum = 4;//4���У�ʱ�䡢�����С��ƽ��
		strcpy(szDSName,"Volt/Cur trend");
		iQTUnitId = ID_QU_PERCENT;
		guQTCharacter = ID_QC_RMS;
		break;
	case 4:
		iSeriesNum = 3;//3���У�ʱ�䡢���ƽ��
		strcpy(szDSName,"UBalance trend");
		iQTUnitId = ID_QU_PERCENT;
		guQTCharacter = ID_QC_S0S1;
		break;
	case 8:
		iSeriesNum = 4;//4���У�ʱ�䡢�����ʡ����ƽ��
		strcpy(szDSName,"harmonious trend");
		break;
	case 16:
		iSeriesNum = 3;//3���У�ʱ�䡢���ƽ��
		strcpy(szDSName,"i-harmonious trend");
		break;
	case 32:
		iSeriesNum = 3;//3���У�ʱ�䡢��ѹ�䶯����ѹ�䶯Ƶ��
		strcpy(szDSName,"Volt fluctuantion trend");
		break;
	case 64:
		iSeriesNum = 2;//2���У�ʱ�䡢��ʱ����
		strcpy(szDSName,"flicker trend");
		iQTUnitId = ID_QU_NONE;
		guQTCharacter = ID_QC_FLKR_PST;
		break;
	case 128:
		iSeriesNum = 3;//3���У�ʱ�䡢г���й����ʡ�г���޹�����
		strcpy(szDSName,"H-Power trend");
		iQTUnitId = ID_QU_WATTS;
		break;
	case 256:
		iSeriesNum = 5;//5���У�ʱ�䡢���й����ܡ����޹����ܡ����й����ܡ����޹�����
		strcpy(szDSName,"energy trend");
		break;
	}
	AddVector(szDSName,strlen(szDSName)+1,ID_PHYS_TYPE_CHAR1,tagNameDS,pCollElem[lIdxItem++]);

	//Effectiveʱ��
	time_t ttNow;
	TIMESTAMPPQDIF pqTime = {0};
	time( &ttNow );
	Time_t2PQTime(ttNow,&pqTime);
	AddScalar((const char *)&pqTime,ID_PHYS_TYPE_TIMESTAMPPQDIF,tagEffective,pCollElem[lIdxItem++]);

	//�����һ��collection(��ChannelDefns�������ĵ�4������)
	//��һ��collection���Եĸ�����ͨ��������
	pCollChannelDefns=AddCollection( pChannelInfs->size(),tagChannelDefns,pCollElem[lIdxItem++] );
	if( pCollChannelDefns == NULL )
		return false;

	long    lOneChanDefItems = 7;//����һ��ͨ�������Ը���

	int		iChannelIdx = -1;
	vector<ChannelInf>::iterator chanIt = pChannelInfs->begin();
	for(;chanIt !=pChannelInfs->end();chanIt++)
	{
		iChannelIdx++;
		//�������һ��collection(��OneChannelDefn)
		pCollOneChannelDefn = AddCollection( lOneChanDefItems,tagOneChannelDefn,pCollChannelDefns[iChannelIdx] );
		if( pCollOneChannelDefn == NULL )
			return false;

		//���ͨ��������Ϣ
		long lIdxChDefn = 0;

		if( (*chanIt).ChannelNo <= 0 )//��·����
		{
			//����ͨ����
			AddScalar((const char *)&(*chanIt).LineNo,ID_PHYS_TYPE_UNS_INTEGER4,tagPhysicalChannel,
				pCollOneChannelDefn[lIdxChDefn++]);

			//ͨ������
			AddVector((*chanIt).LineName,strlen((*chanIt).LineName)+1,ID_PHYS_TYPE_CHAR1,
				tagChannelName,pCollOneChannelDefn[lIdxChDefn++]);

			//���(�����)
			nAttrVal = ID_PHASE_NONE;
			AddScalar((const char *)&nAttrVal,ID_PHYS_TYPE_UNS_INTEGER4,tagPhaseID,
				pCollOneChannelDefn[lIdxChDefn++]);
		}
		else //ͨ������
		{
			//����ͨ����
			AddScalar((const char *)&(*chanIt).ChannelNo,ID_PHYS_TYPE_UNS_INTEGER4,tagPhysicalChannel,
				pCollOneChannelDefn[lIdxChDefn++]);

			//ͨ������
			AddVector((*chanIt).ChannelName,strlen((*chanIt).ChannelName)+1,ID_PHYS_TYPE_CHAR1,
				tagChannelName,pCollOneChannelDefn[lIdxChDefn++]);

			//���
			int iPhase = ID_PHASE_AN;
			int iFirstChannelIdx = iChannelIdx-3;
			while( iFirstChannelIdx != iChannelIdx )
			{
				//���ǹ涨pChannelInfs�е�ͨ����Ϣ�Ǹ�����·��Ȼ�����˳����
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
						break;	//ͬһ��·�ĵ�һ��ͨ��
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
		}//end ͨ������

		//��������·���������
		AddVector((*chanIt).LineName,strlen((*chanIt).LineName)+1,ID_PHYS_TYPE_CHAR1,
			tagGroupName,pCollOneChannelDefn[lIdxChDefn++]);

		//�������
		AddScalar((const char *)&ID_QT_VALUELOG,ID_PHYS_TYPE_GUID,tagQuantityTypeID,
			pCollOneChannelDefn[lIdxChDefn++]);

		//��������ID
		nAttrVal = ID_QM_NONE;
		AddScalar((const char *)&nAttrVal,ID_PHYS_TYPE_UNS_INTEGER4,tagQuantityMeasuredID,
			pCollOneChannelDefn[lIdxChDefn++]);

		//������ж��壨��ͬ�������в�ͬ������������
		pCollSeriesDefns = AddCollection(iSeriesNum,tagSeriesDefns,pCollOneChannelDefn[lIdxChDefn++]);
		if( pCollSeriesDefns == NULL )
			return false;

		//����ÿ����������
		long    lOneSeriesItems = 4;//����һ�����е����Ը���
		for(int i=0;i<iSeriesNum;i++)
		{
			pCollOneSeriesDefn = AddCollection(lOneSeriesItems,tagOneSeriesDefn,pCollSeriesDefns[i]);
			if( pCollOneSeriesDefn == NULL )
				return false;

			//������������
			int		iSeriesIdx = 0;
			
			if( i == 0 )//ʱ��
			{
				//ֵ����
				AddScalar((const char *)&ID_SERIES_VALUE_TYPE_TIME,ID_PHYS_TYPE_GUID,tagValueTypeID,
					pCollOneSeriesDefn[iSeriesIdx++]);

				//����ֵ��֧�־���ʱ�䣿
				//if( pFileHeader->DataTimeType == 1) //���ʱ��
				//{
					iQTUnitId = ID_QU_SECONDS;
				/*}
				else //����ʱ��
				{
					iQTUnitId = ID_QU_TIMESTAMP;
				}*/
				guQTCharacter = ID_QC_NONE;
			}
			else //��ֵ
			{
				//ֵ����
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
				case 8://г��
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
				case 16://��г��
					if( (*chanIt).ChannelType == 1)
					{
						iQTUnitId = ID_QU_VOLTS;
					}
					else
					{
						iQTUnitId = ID_QU_AMPS;
					}
					guQTCharacter = ID_QC_IHRMS;
				case 128://г������
					if( i==1)
					{
						iQTUnitId = ID_QU_WATTS;
						guQTCharacter = ID_QC_P;//�й�����
					}
					else if( i==2)
					{
						iQTUnitId = ID_QU_VARS;
						guQTCharacter = ID_QC_Q;//�޹�����
					}
					break;
				case 256://����
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
						guQTCharacter = ID_QC_P_INTG_POS;//���й�����
					}
					else if( i==2 )
					{
						guQTCharacter = ID_QC_Q_INTG_POS;//���޹�����
					}
					else if( i==3 )
					{
						guQTCharacter = ID_QC_P_INTG_NEG;//���й�����
					}
					else if( i==4 )
					{
						guQTCharacter = ID_QC_Q_INTG_NEG;//���޹�����
					}
					break;
				}
			}//end ��ֵ

			//��λ
			AddScalar((const char *)&iQTUnitId,ID_PHYS_TYPE_UNS_INTEGER4,tagQuantityUnitsID,
				pCollOneSeriesDefn[iSeriesIdx++]);

			//��������
			AddScalar((const char *)&guQTCharacter,ID_PHYS_TYPE_GUID,tagQuantityCharacteristicID,
				pCollOneSeriesDefn[iSeriesIdx++]);

			//�洢��ʽ(ֱ�Ӵ洢��ʽ)
			nAttrVal = ID_SERIES_METHOD_VALUES;
			AddScalar((const char *)&nAttrVal,ID_PHYS_TYPE_UNS_INTEGER4,tagStorageMethodID,
				pCollOneSeriesDefn[iSeriesIdx++]);

		}//end ����ÿ����������

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

	if( PQDIF_IsEqualGUID( tag, tagChannelDefns )) //ȫ��ͨ���Ķ���
	{
		m_iChannelDefnIdx = -1;
		m_iSeriesDefnIdx = -1;
		return true;
	}
	else if( PQDIF_IsEqualGUID( tag, tagOneChannelDefn )) //һ��ͨ���Ķ���
	{
		memset(&m_CurrentChan,0,sizeof(m_CurrentChan));
		m_iChannelDefnIdx++;
		m_iSeriesDefnIdx = -1;

		m_CurrentChan.Index = m_iChannelDefnIdx;
		m_pChanInfList->push_back(m_CurrentChan);

		m_pCurrentChan = &(m_pChanInfList->at( m_pChanInfList->size()-1 ) );

		return true;
	}
	else if( PQDIF_IsEqualGUID( tag, tagSeriesDefns )) //ȫ�����еĶ���
	{
		m_iSeriesDefnIdx = -1;
		return true;
	}
	else if( PQDIF_IsEqualGUID( tag, tagOneSeriesDefn )) //ȫ�����еĶ���
	{
		m_iSeriesDefnIdx++;
		if( m_iSeriesDefnIdx > 1 )
		{
			return false;//Ŀǰ������ֻ֧�������е�����
		}
		return true;
	}

	return false;
}

//##ModelId=4C7235760367
bool DataSource::AcceptScalar(const GUID& tag, int level, INT1 typePhysical, void* pdata)
{
	GUID	guid;

	if( m_iChannelDefnIdx >= 0 && m_iSeriesDefnIdx == -1) //ͨ���Ķ���
	{
		if( PQDIF_IsEqualGUID(tag,tagPhysicalChannel) )//����ͨ����
		{
			if( ID_PHYS_TYPE_UNS_INTEGER4 != typePhysical )
				return false;

			m_pCurrentChan->ChannelNo = *((int*)pdata);
			return true;
		}
		else if( PQDIF_IsEqualGUID(tag,tagQuantityMeasuredID) )//��������������ID
		{
			if( ID_PHYS_TYPE_UNS_INTEGER4 != typePhysical )
				return false;

			m_iQuantityMeasuredID = *((int*)pdata);
			return true;
		}
	}//end ͨ���Ķ���
	else if( m_iSeriesDefnIdx == 0)//��һ���ж���
	{
		if( PQDIF_IsEqualGUID(tag,tagValueTypeID) )//��������
		{
			if( ID_PHYS_TYPE_GUID != typePhysical )
				return false;

			memcpy(&guid,pdata,sizeof(GUID) );
			if( !PQDIF_IsEqualGUID(guid,ID_SERIES_VALUE_TYPE_TIME) )
			{
				//����ֻ�����һ������������Ϊʱ�����͵�����
				return false;
			}
			return true;
		}//end ��������
		else if( PQDIF_IsEqualGUID(tag,tagQuantityUnitsID) )//���ݵ�λ
		{
			if( ID_PHYS_TYPE_UNS_INTEGER4 != typePhysical )
				return false;

			int iUnits = *((int*)pdata);
			memcpy(&guid,pdata,sizeof(GUID) );
			if( iUnits == ID_QU_SECONDS )//��λ����
			{
				g_FileHeader.DataTimeType = 1;//���ʱ��
				g_FileHeader.DataTimeUnit = 1;
				return true;
			}
			else if( iUnits == ID_QU_TIMESTAMP )//����ʱ��,����TIMESTAMPPQDIF
			{
				g_FileHeader.DataTimeType = 2;
				return true;
			}
			return false;//���������͵Ĳ�����
		}//end ���ݵ�λ

	}//end ��һ���ж���
	else if( m_iSeriesDefnIdx == 1)//�ڶ����ж���
	{
		if( PQDIF_IsEqualGUID(tag,tagQuantityCharacteristicID) )//��������
		{
			if( ID_PHYS_TYPE_GUID != typePhysical )
				return false;

			memcpy(&guid,pdata,sizeof(GUID) );
			if( PQDIF_IsEqualGUID(guid,ID_QC_RMS) ) //��Чֵ
			{
				if( m_iQuantityMeasuredID == ID_QM_VOLTAGE )
				{
					g_FileHeader.FileType = 2;//��ѹ����
				}
				else if( m_iQuantityMeasuredID == ID_QM_CURRENT )
				{
					g_FileHeader.FileType = 8;//��������
				}
				return true;
			}
			else if( PQDIF_IsEqualGUID(guid,ID_QC_FREQUENCY) ) //Ƶ��
			{
				g_FileHeader.FileType = 1;	//Ƶ��
			}
			else if( PQDIF_IsEqualGUID(guid,ID_QC_FLKR_PST) || PQDIF_IsEqualGUID(guid,ID_QC_FLKR_PLT))
			{
				g_FileHeader.FileType = 7;	//����
			}
			else if( PQDIF_IsEqualGUID(guid,ID_QC_HRMS) )
			{
				g_FileHeader.FileType = 4;	//г��
			}
			else if( PQDIF_IsEqualGUID(guid,ID_QC_P) )
			{
				g_FileHeader.FileType = 4;	//г������
			}

			return true;
		}//end ��������

		//������λ
		if( m_iChannelDefnIdx == 0 && PQDIF_IsEqualGUID(tag,tagQuantityUnitsID) && m_Units !=NULL)
		{
			if( ID_PHYS_TYPE_UNS_INTEGER4 != typePhysical )
				return false;

			int iUnit = *((int*)pdata);

			if( iUnit == ID_QU_VOLTS ) //����
			{
				m_Units->push_back(PQDIFInterFace::VOLT);
			}
			else if( iUnit == ID_QU_AMPS ) //����
			{
				m_Units->push_back(PQDIFInterFace::AMP);
			}
			else if( iUnit == ID_QU_HERTZ )//Ƶ��
			{
				m_Units->push_back(PQDIFInterFace::HERTZ);
			}
			else if( iUnit == ID_QU_DEGREES)//��
			{
				m_Units->push_back(PQDIFInterFace::DEGRESS);
			}
			else if( iUnit == ID_QU_PERCENT )//�ٷֱ�
			{
				m_Units->push_back(PQDIFInterFace::PERCENT);
			}
			else if( iUnit == ID_QU_WATTS )//����
			{
				m_Units->push_back(PQDIFInterFace::WATTS);
			}
			else
			{
				m_Units->push_back(PQDIFInterFace::NONE);
			}

		}

		return true;
	}//end �ڶ����ж���

	if( PQDIF_IsEqualGUID(tag,tagStorageMethodID) )//�洢��ʽ
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
	}//end �洢��ʽ

	return true;
}

//##ModelId=4C72469B02ED
bool DataSource::AcceptVector(const GUID& tag, int level, INT1 typePhysical, c_vector* pVector, void* pdata)
{
	int iCpyLen = 0;

	if( m_iChannelDefnIdx >= 0 && m_iSeriesDefnIdx == -1) //ͨ���Ķ���
	{
		if( PQDIF_IsEqualGUID(tag,tagChannelName) )//ͨ����
		{
			if( pVector->count == 0 || ID_PHYS_TYPE_CHAR1 != typePhysical )
				return false;

			iCpyLen = ( pVector->count > sizeof(m_pCurrentChan->ChannelName)-1 )?
				sizeof(m_pCurrentChan->ChannelName)-1 : pVector->count;
			memcpy(m_pCurrentChan->ChannelName,pdata,iCpyLen);
			return true;
		}
		else if( PQDIF_IsEqualGUID(tag,tagGroupName) )//����(�����)
		{
			if( pVector->count == 0 || ID_PHYS_TYPE_CHAR1 != typePhysical )
				return false;

			iCpyLen = ( pVector->count > sizeof(m_pCurrentChan->LineName)-1 )?
				sizeof(m_pCurrentChan->LineName)-1 : pVector->count;
			memcpy(m_pCurrentChan->LineName,pdata,iCpyLen);
			return true;
		}
	}//end ͨ���Ķ���

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
		return iRecordType;//����DataSource��¼
	}

	//���ø��ຯ��
	return Record::ParseOneRecord(pFile,hdrRecord,styleCompression,algCompression,true);
}

