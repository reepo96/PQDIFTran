#include "Observation.h"
#include <time.h>

#ifdef UNIT_TEST
#include "Record.cpp"
#include "CPQDataList.cpp"
#endif

Observation::Observation()
{
	m_iChannelInstIdx = -1;
	m_iSeriesInstIdx = -1;
	m_iCurrentChaIdx = -1;
	m_iCurrentHarm = 0;
	m_nTimes = NULL;
	m_iPointCount = 0;
	m_fTimes = NULL;
	m_pCurrentChaValues = NULL;

	memset(&tmDataStartTime,0,sizeof(tmDataStartTime));
}

Observation::~Observation()
{
	
}

//##ModelId=4C5A8671009E
bool Observation::AddData(PQFileHead *pFileHeader, CPQDataList *pDataList, int iFromPos, int& iDataNum)
{
	long                    lCollItems = 5; //���5������
	long                    lIdxItem = 0;
	c_collection_element	*pCollElem = NULL;
	c_collection_element	*pCollChanInsts = NULL;
	c_collection_element	*pCollOneChanInst = NULL;
	c_collection_element	*pCollSeriesInsts = NULL;
	c_collection_element	*pCollOneSeriesInsts = NULL;

	int		iSeriesNum = 2;	//�������������������У�

	unsigned int	nAttrVal = 0;
	PQData	*pPQData = pDataList->GetElem(iFromPos);
	if( pPQData == NULL )
		return true;

	if( pFileHeader == NULL || pDataList == NULL )
		return false;

	switch(pFileHeader->FileType)
	{
	case 1:
		iSeriesNum = 4;//4���У�ʱ�䡢�����С��ƽ��
		break;
	case 2:
		iSeriesNum = 4;//4���У�ʱ�䡢�����С��ƽ��
		break;
	case 4:
		iSeriesNum = 3;//3���У�ʱ�䡢���ƽ��
		break;
	case 8:
		iSeriesNum = 4;//4���У�ʱ�䡢�����ʡ����ƽ��
		break;
	case 16:
		iSeriesNum = 3;//3���У�ʱ�䡢���ƽ��
		break;
	case 32:
		iSeriesNum = 3;//3���У�ʱ�䡢��ѹ�䶯����ѹ�䶯Ƶ��
		break;
	case 64:
		iSeriesNum = 2;//2���У�ʱ�䡢��ʱ����
		break;
	case 128:
		iSeriesNum = 3;//3���У�ʱ�䡢г���й����ʡ�г���޹�����
		break;
	case 256:
		iSeriesNum = 5;//5���У�ʱ�䡢���й����ܡ����޹����ܡ����й����ܡ����޹�����
		break;
	}

	//��ӵ�һ��collection
	pCollElem = AddCollection( lCollItems );
	if( pCollElem == NULL )
		return false;

	//���collectionԪ��

	/*----------��ʼ�������ݶε�����-------->>>>>>>>>>>*/

	//ʱ��
	TIMESTAMPPQDIF pqTime = {0};
	if( pFileHeader->DataTimeType == 2)//����ʱ��
	{
		time_t ttTime = (time_t)pPQData->time;
		struct tm tmTime = *localtime(&ttTime);
		Time_t2PQTime(ttTime,&pqTime);
	}
	else //���ʱ��
	{
		int iMultiple = 1;	//��ı���
		switch( pFileHeader->DataTimeUnit )
		{
		case 1://��
			iMultiple = 1;
			break;
		case 2://����
			iMultiple = 60;
			break;
		case 3://Сʱ
			iMultiple = 3600;
			break;
		case 4://��
			iMultiple = 24*3600;
			break;
		default:
			iMultiple = 1;
			break;
		}

		time_t ttStart = mktime( &(pFileHeader->DataStartTime) );
		ttStart += pPQData->time * iMultiple;

		Time_t2PQTime(ttStart,&pqTime);
	}

	//���ݶ�����
	char szObName[64] = {0};
	struct tm tmStartTime = {0};
	PQTime2Tm(&pqTime,&tmStartTime);
	sprintf(szObName,"%d-%d-%d %d:%d:%d",tmStartTime.tm_year+1900,tmStartTime.tm_mon+1,tmStartTime.tm_mday,
		tmStartTime.tm_hour,tmStartTime.tm_min,tmStartTime.tm_sec);

	AddVector(szObName,strlen(szObName)+1,ID_PHYS_TYPE_CHAR1,tagObservationName,pCollElem[lIdxItem++]);
	
	//����ʱ��
	AddScalar((const char *)&pqTime,ID_PHYS_TYPE_TIMESTAMPPQDIF,tagTimeCreate,pCollElem[lIdxItem++]);

	//���ݿ�ʼʱ��
	AddScalar((const char *)&pqTime,ID_PHYS_TYPE_TIMESTAMPPQDIF,tagTimeStart,pCollElem[lIdxItem++]);

	//������ʽ
	nAttrVal = ID_TRIGGER_METH_NONE;
	AddScalar((const char *)&nAttrVal,ID_PHYS_TYPE_UNS_INTEGER4,tagTriggerMethodID,pCollElem[lIdxItem++]);

	/*<<<<<<<<<<<<------�������ݶε����Խ���--------*/

	//�����һ��collection(��tagChannelInstances�������ĵ�5������)
	//��һ��collection���Եĸ�����ͨ������г�����ĳ˻�����
	int iChaNum = pPQData->ChannelNum;
	int iHarmCount = 1;	//г��������Ϊ1����г�����ݣ�

	if(pFileHeader->FileType == 8)//г������
	{
		iHarmCount = (pPQData->valueNum-1)/2;//�޳��ܻ�����
	}
	if(pFileHeader->FileType == 16)//��г������
	{
		iHarmCount = pPQData->valueNum/2;
	}
	else if(pFileHeader->FileType == 128)//г������
	{
		//ÿһ��г���������й����޹����
		iHarmCount = (pPQData->valueNum-3)/2;
	}

	if( iHarmCount < 1 )
		return false;

	pCollChanInsts=AddCollection( iChaNum*iHarmCount,tagChannelInstances,pCollElem[lIdxItem++] );
	if( pCollChanInsts == NULL )
		return false;

	long    lOneChanInstItems = 2;//����һ��ͨ��ʵ�������Ը���
	int		iChannelInstIdx = -1;

	for(int i=0;i<iChaNum;i++) //����ͨ��
	{
		for(int j=0;j<iHarmCount;j++) //����г��
		{
			iChannelInstIdx++;
			//�������һ��collection(��tagOneChannelInst)
			pCollOneChanInst = AddCollection(lOneChanInstItems,tagOneChannelInst,pCollChanInsts[iChannelInstIdx]);
			if( pCollOneChanInst == NULL )
				return false;

			//���ͨ��ʵ����Ϣ
			long lChaInstInfIdx = 0;

			//��Ӧ��ͨ������
			AddScalar((const char *)&i,ID_PHYS_TYPE_UNS_INTEGER4,tagChannelDefnIdx,
				pCollOneChanInst[lChaInstInfIdx++]);

			//�������ʵ��
			pCollSeriesInsts = AddCollection(iSeriesNum,tagSeriesInstances,pCollOneChanInst[lChaInstInfIdx++]);
			if( pCollSeriesInsts == NULL )
				return false;

			//����ÿ����������
			char	*pTmpSeriesVal = NULL;
			INT1	iTmpTypePhysical = 0;
			long    lOneSeriesItems = 1;//����һ�����е����Ը���
			for(int m=0;m<iSeriesNum;m++)
			{
				if( (i!=0 || j!=0) && (m==0) )
				{
					lOneSeriesItems = 2;//���ǵ�һͨ����ʱ����������������
				}
				else
				{
					lOneSeriesItems = 1;//������1������
				}

				if( iHarmCount > 1 )
				{
					lOneSeriesItems++;//��г�����ټ�1������
				}

				//��ӵ�������ʵ��������
				pCollOneSeriesInsts = AddCollection(lOneSeriesItems,tagOneSeriesInstance,pCollSeriesInsts[m]);
				if( pCollOneSeriesInsts == NULL )
					return false;

				//������������
				int		iSeriesIdx = 0;

				//��ţ�г����������
				if( iHarmCount > 1 )
				{
					nAttrVal = j+1;
					AddScalar((const char *)&nAttrVal,ID_PHYS_TYPE_INTEGER2,tagChannelGroupID,
						pCollOneSeriesInsts[iSeriesIdx++]);
				}

				//���ǵ�һͨ����ʱ�����У����ǹ����һ��ͨ����ʱ������
				if( (i!=0 || j!=0) && (m==0) )
				{
					//�����һ��ͨ��
					nAttrVal = 0;
					AddScalar((const char *)&nAttrVal,ID_PHYS_TYPE_UNS_INTEGER4,tagSeriesShareChannelIdx,
						pCollOneSeriesInsts[iSeriesIdx++]);

					//�����һ������
					nAttrVal = 0;
					AddScalar((const char *)&nAttrVal,ID_PHYS_TYPE_UNS_INTEGER4,tagSeriesShareSeriesIdx,
						pCollOneSeriesInsts[iSeriesIdx++]);

					continue;
				}//end ���ǵ�һͨ����ʱ������

				int iTmpDataNum = iDataNum;
				if( m == 0 ) //ʱ��ֵ����
				{
					//����ʱ������
					pTmpSeriesVal = GeneratTimeSeries(pFileHeader,pDataList,iFromPos,iTmpDataNum,iTmpTypePhysical);
				}//end ʱ��ֵ����
				else //ֵ����
				{
					iTmpTypePhysical = ID_PHYS_TYPE_REAL4;
					//����ֵ����
					int poistion = 0;
					switch(pFileHeader->FileType)
					{
					case 8://г��
						if(m==1)
							poistion = 0;
						else
							poistion = j+1;
						break;
					case 128://г������
						if(m==1)
							poistion = 3+j;
						else
							poistion = 3+iHarmCount+j;
						break;
					default:
						poistion = m-1;
					}

					pTmpSeriesVal =GeneratValueSeries(pFileHeader,pDataList,iFromPos,iTmpDataNum,poistion,(i+1));
				}
				iDataNum = iTmpDataNum;

				if(pTmpSeriesVal && iTmpDataNum > 0)
				{
					AddVector(pTmpSeriesVal,iTmpDataNum,iTmpTypePhysical,
						tagSeriesValues,pCollOneSeriesInsts[iSeriesIdx++]);
				}
				else
				{
					if(pTmpSeriesVal)
					{
						delete []pTmpSeriesVal;
						pTmpSeriesVal = NULL;
					}
					continue;	//û�����ˣ�
				}

				if(pTmpSeriesVal)
				{
					delete []pTmpSeriesVal;
					pTmpSeriesVal = NULL;
				}

			}//end for(int m=0;m<2;m++) ����ÿ����������
				

		}//end ������г��

	}//end for(int i=0;i<iChaNum;i++) //����������ͨ��

	return true;
}

char * Observation::GeneratTimeSeries(PQFileHead *pFileHeader, CPQDataList *pDataList, int iFromPos, 
									  int& iDataNum,INT1& typePhysical)
{
	if( pFileHeader== NULL || pDataList == NULL )
		return NULL;

	char *pTimeSeries = NULL;
	char *pTmpData = NULL;
	TIMESTAMPPQDIF pqPointTime = {0};
	PQData	*pPQData = NULL;
	time_t	ttTime;

	if( pFileHeader->DataTimeType == 2)//����ʱ��
	{
		//PQDIF������ֵ��֧�־���ʱ��ID_PHYS_TYPE_TIMESTAMPPQDIF��
		//����Ǿ���ʱ�䣬���ǽ���ת�������ʱ��

		/*typePhysical = ID_PHYS_TYPE_TIMESTAMPPQDIF;
		pTimeSeries = new char[sizeof(TIMESTAMPPQDIF)*iDataNum];*/
		
		typePhysical = ID_PHYS_TYPE_UNS_INTEGER4;
		pTimeSeries = new char[sizeof(int)*iDataNum];
		if( pTimeSeries == NULL )
			return NULL;

		//memset(pTimeSeries,0,sizeof(TIMESTAMPPQDIF)*iDataNum);
		memset(pTimeSeries,0,sizeof(int)*iDataNum);

		pTmpData = pTimeSeries;

		pPQData = pDataList->GetElem(iFromPos);//get first point
		if( pPQData == NULL )
			return NULL;

		int iFirstTime = pPQData->time;	//��һ���ʱ��
		int iTime = 0;

		for(int i=0;i<iDataNum;i++)
		{
			pPQData = pDataList->GetElem(iFromPos+i);
			if( pPQData == NULL)//û��ô��㣿
			{
				iDataNum = i;
				return pTimeSeries;
			}

			iTime = pPQData->time - iFirstTime;

			if( iTime < 0 )//ʱ����ң�
			{
				iFirstTime = pPQData->time;
				iTime = 0;
			}

			memcpy(pTmpData,&iTime,sizeof(iTime));
			pTmpData += sizeof(iTime);

			/*ttTime = (time_t)pPQData->time;
			struct tm tmTime = *localtime(&ttTime);
			Time_t2PQTime(ttTime,&pqPointTime);
			memcpy(pTmpData,&pqPointTime,sizeof(pqPointTime));
			pTmpData += sizeof(pqPointTime);*/
		}

		return pTimeSeries;
	}
	else //���ʱ��
	{
		pPQData = pDataList->GetElem(iFromPos);//get first point
		if( pPQData == NULL )
			return NULL;

		int iFirstTime = pPQData->time;	//��һ���ʱ��
		int iTime = 0;

		typePhysical = ID_PHYS_TYPE_UNS_INTEGER4;
		pTimeSeries = new char[sizeof(int)*iDataNum];
		if( pTimeSeries == NULL )
			return NULL;

		memset(pTimeSeries,0,sizeof(int)*iDataNum);
		pTmpData = pTimeSeries;

		int iMultiple = 1;	//��ı���
		switch( pFileHeader->DataTimeUnit )
		{
		case 1://��
			iMultiple = 1;
			break;
		case 2://����
			iMultiple = 60;
			break;
		case 3://Сʱ
			iMultiple = 3600;
			break;
		case 4://��
			iMultiple = 24*3600;
			break;
		default:
			iMultiple = 1;
			break;
		}

		for(int i=0;i<iDataNum;i++)
		{
			pPQData = pDataList->GetElem(iFromPos+i);
			if( pPQData == NULL)//û��ô��㣿
			{
				iDataNum = i;
				return pTimeSeries;
			}

			iTime = (pPQData->time - iFirstTime)*iMultiple;
			memcpy(pTmpData,&iTime,sizeof(iTime));
			pTmpData += sizeof(iTime);
		}

		return pTimeSeries;
	}

	return NULL;
}

char * Observation::GeneratValueSeries(PQFileHead *pFileHeader, CPQDataList *pDataList, int iFromPos, 
									  int& iDataNum,int poistion,int iChaNo)
{
	if( pFileHeader== NULL || pDataList == NULL )
		return NULL;

	PQData	*pPQData = NULL;
	float *pValSeries = new float[iDataNum];
	if( pValSeries == NULL )
		return NULL;

	memset( pValSeries,0,sizeof(float)*iDataNum );

	for(int i=0;i<iDataNum;i++)
	{
		pPQData = pDataList->GetElem(iFromPos+i);
		if( pPQData == NULL || pPQData->ppValues==NULL || *(pPQData->ppValues) == NULL)//û��ô��㣿
		{
			iDataNum = i;
			return (char*)pValSeries;
		}

		pValSeries[i] = pPQData->ppValues[iChaNo-1][poistion];
	}

	return (char*)pValSeries;
}

//##ModelId=4C5A867100A2
bool Observation::ParseFile(FILE* pFile, CPQDataList& DataList, PQFileHead* pFileHeader)
{
	if( pFileHeader == NULL || pFile == NULL)
		return false;

	c_record_mainheader	hdrRecord = {0};
	unsigned int nCompressStyle = ID_COMP_STYLE_NONE;
	
	if( pFileHeader->CompressType != 0 )
	{
		nCompressStyle = ID_COMP_STYLE_RECORDLEVEL;
	}

	do
	{
		int iRet = ParseOneRecord(pFile,hdrRecord,nCompressStyle,pFileHeader->CompressType,false);
		if( iRet == -1 || iRet == 1 )
		{
			return false;
		}
		else if( iRet == 2 )
		{
			return false;
		}
		else if( iRet == 0 )
		{
			int iChanNum = m_ChaValues.size() ;	//ͨ������
			int iValueNum = 0;	//ÿ����ӵ��ֵ�ĸ���(��г����ÿ�����ж��)
			bool isFirstRecord = false;	//�ǵ�һ����¼?
			float  fTimeOffset = 0.00;	//�������ݿ�ʼʱ���������������ݿ�ʼʱ���ƫ��

			if( DataList.GetElemNum() == 0 )
				isFirstRecord = true;
			
			if( iChanNum == 0 )
				return false;

			map<int,map<int,float*>*>::iterator it = m_ChaValues.begin();
			iValueNum = ((*it).second)->size();

			//���ǵ�һ����¼�������ʱ��
			if( !isFirstRecord && g_FileHeader.DataTimeType ==1 )
			{
				if( m_fTimes == NULL)
					return false;
				//���㱾�����ݿ�ʼʱ���������������ݿ�ʼʱ���ƫ��
				time_t ttFileStartTime = mktime(&(g_FileHeader.DataStartTime));
				time_t ttThisStartTime = mktime(&tmDataStartTime);

				fTimeOffset = (float)(ttThisStartTime - ttFileStartTime);
				if( fTimeOffset <= 0.00)// || fTimeOffset > 6000000.00 )
				{
					//�����쳣?
					return false;
				}
			}
			else if( isFirstRecord ) //��һ����¼
			{
				//��һ�εĿ�ʼʱ����Ϊ�������ݵĿ�ʼʱ��
				memcpy( &(g_FileHeader.DataStartTime),&tmDataStartTime,sizeof(struct tm) );
			}

			//������ת����PQData�ṹ��������DataList��
			for(int i = 0;i<m_iPointCount;i++)//ÿ����
			{
				PQData *pPQData = new PQData(iChanNum,iValueNum);
				pPQData->ChannelNum = iChanNum;

				//����ʱ��
				if( g_FileHeader.DataTimeType == 2 )//�Ǿ���ʱ��
				{
					if( m_nTimes == NULL )
						return false;

					pPQData->isUseFloatTime = false;
					pPQData->time = m_nTimes[i];
				}
				else	//���ʱ��
				{
					pPQData->isUseFloatTime = true;
					pPQData->fTime = m_fTimes[i] +fTimeOffset;
				}

				//��������
				int iChanIndex = -1;	//ͨ������
				int iValIndex = -1;		//ֵ����

				map<int,float*>::iterator valIt;
				it = m_ChaValues.begin();

				if( pPQData->ppValues == NULL || *(pPQData->ppValues) == NULL)
					return false;

				for(;it != m_ChaValues.end();it++)//ÿһ��ͨ��
				{
					iChanIndex++;

					map<int,float*> *pValMap = (*it).second;
					if( pValMap == NULL )
						return NULL;

					iValIndex = -1;

					valIt = pValMap->begin();
					for(;valIt != pValMap->end();valIt++)//ÿ��������ֵ
					{
						//pfValָ���һ����ֵ�ǻ��������У��ڶ������Ƕ���г�����еȵ�
						float *pfVal = (*valIt).second;
						if( pfVal == NULL)
							return false;

						iValIndex++;

						//pPQData->ppValues�а�[ͨ��][г������]��ʽ�洢,����
						//{ {1��г��,2��г��,..N��г��,},/*ͨ��1*/
						//  {1��г��,2��г��,..N��г��,},/*ͨ��2*/
						//    ....
						//  {1��г��,2��г��,..N��г��,}/*ͨ��N*/ }

						//pfVal��[��˳��]��ʽ�洢����{��1,��2,...��N}
						pPQData->ppValues[iChanIndex][iValIndex] = pfVal[i];
					}//end ÿ��������ֵ
				}//end ÿһ��ͨ��

				DataList.AddElem(pPQData);

			}//end ÿ����
		}

		//���³�ʼ����¼
		m_iChannelInstIdx = -1;
		m_iSeriesInstIdx = -1;
		m_iCurrentChaIdx = -1;
		m_iCurrentHarm = 0;

		map<int,map<int,float*>*>::iterator it = m_ChaValues.begin();
		for(;it != m_ChaValues.end();it++)//ÿһ��ͨ��
		{
			map<int,float*> *pValMap = (*it).second;
			if( pValMap == NULL )
				continue;

			map<int,float*>::iterator valIt = pValMap->begin();
			for(;valIt != pValMap->end();valIt++)
			{
				float *pfVal = (*valIt).second;
				if( pfVal )
					delete pfVal;
			}
			pValMap->clear();
			delete pValMap;
			pValMap = NULL;
		}
		m_ChaValues.clear();

		m_pCurrentChaValues = NULL;
		//end���³�ʼ����¼

		if( hdrRecord.linkNextRecord == 0 )
		{
			return true;
		}
		fseek( pFile, hdrRecord.linkNextRecord, SEEK_SET );//������һ����¼

	}while( hdrRecord.linkNextRecord != 0 && !feof(pFile) );

	return true;
}

//##ModelId=4C7369BB01BD
bool Observation::AcceptCollection(const GUID& tag, int level)
{
	if( PQDIF_IsEqualGUID( tag, tagChannelInstances )) //ȫ��ͨ��ʵ��
	{
		m_iChannelInstIdx = -1;
		m_iSeriesInstIdx = -1;
		return true;
	}
	else if( PQDIF_IsEqualGUID( tag, tagOneChannelInst )) //һ��ͨ��ʵ��
	{
		m_iChannelInstIdx++;
		m_iSeriesInstIdx = -1;
		return true;
	}
	else if( PQDIF_IsEqualGUID( tag, tagSeriesInstances )) //ͨ��ȫ������ʵ��
	{
		m_iSeriesInstIdx = -1;
		return true;
	}
	else if( PQDIF_IsEqualGUID( tag, tagOneSeriesInstance )) //ͨ������һ��ʵ��
	{
		m_iSeriesInstIdx++;
		return true;
	}

	return false;
}

//##ModelId=4C7369BB01C0
bool Observation::AcceptScalar(const GUID& tag, int level, INT1 typePhysical, void* pdata)
{
	TIMESTAMPPQDIF	pqTime = {0};

	if( m_iChannelInstIdx == -1 && m_iSeriesInstIdx == -1) //����ͨ��������
	{
		if( PQDIF_IsEqualGUID(tag,tagTimeStart) )//���ݿ�ʼʱ��
		{
			if( ID_PHYS_TYPE_TIMESTAMPPQDIF != typePhysical )
				return false;

			memcpy(&pqTime,pdata,sizeof(pqTime));
			//ת��ʱ��
			PQTime2Tm(&pqTime,&tmDataStartTime);
			return true;
		}
	}//end ����ͨ��������
	if( m_iChannelInstIdx >= 0 ) //ͨ��ʵ��������
	{
		if( PQDIF_IsEqualGUID(tag,tagChannelDefnIdx) )//ͨ��ʵ����Ӧ��ͨ������
		{
			if( ID_PHYS_TYPE_UNS_INTEGER4 != typePhysical )
				return false;

			m_iCurrentChaIdx = *((int*)pdata);
			return true;
		}
		else if( PQDIF_IsEqualGUID(tag,tagChannelGroupID) )//ͨ��ʵ����Ӧ��г������
		{
			if( ID_PHYS_TYPE_INTEGER2 != typePhysical )
				return false;

			m_iCurrentHarm = *((short*)pdata);
			return true;
		}
	}//end ����ͨ��������

	if( m_iSeriesInstIdx >= 0 ) //���е�����
	{
		if( PQDIF_IsEqualGUID(tag,tagSeriesScale) )//���ж�Ӧ�ı���ϵ��
		{
			double dScale = 0.00;
			if(!GetSimpVectorArrayValue(pdata,typePhysical,0,dScale))
				return false;

			m_SeriesScale.insert(map<int,double>::value_type(m_iSeriesInstIdx,dScale));
			return true;
		}
		else if( PQDIF_IsEqualGUID(tag,tagSeriesOffset) )//���ж�Ӧ����Ư
		{
			double dOffset = 0.00;
			if(!GetSimpVectorArrayValue(pdata,typePhysical,0,dOffset))
				return false;

			m_SeriesOffset.insert(map<int,double>::value_type(m_iSeriesInstIdx,dOffset));
			return true;
		}
	}

	return true;
}

//##ModelId=4C7369BB01C5
bool Observation::AcceptVector(const GUID& tag, int level, INT1 typePhysical, c_vector* pVector, void* pdata)
{
	if( pVector == NULL || pVector->count == 0 || pdata == NULL)
		return false;

	long	lPoint = 0;

	if( PQDIF_IsEqualGUID(tag,tagSeriesValues) )
	{
		//��һͨ����һ����
		if(m_iChannelInstIdx == 0 && m_iSeriesInstIdx == 0 )
		{
			void *pValue = GetSeriesData(StoragMeth[0],typePhysical,1.00,0.00,pVector->count,pdata,lPoint);
			m_iPointCount = lPoint;

			if( pValue == NULL)
			{
				if( pValue != NULL && typePhysical != ID_PHYS_TYPE_TIMESTAMPPQDIF)
				{
					delete []pValue;
					pValue = NULL;
					return false;
				}
				return false;
			}

			//ʱ��ת��
			if( typePhysical == ID_PHYS_TYPE_TIMESTAMPPQDIF )
			{
				if( m_nTimes != NULL )
				{
					delete []m_nTimes;
					m_nTimes = NULL;
				}
				m_nTimes = new unsigned int[m_iPointCount];
				if( m_nTimes == NULL )
					return false;

				time_t ttTime = 0;
				TIMESTAMPPQDIF *pPQTime = (TIMESTAMPPQDIF*)pValue;

				for(long i=0;i<lPoint;i++)
				{				
					PQTime2Time_t(pPQTime,ttTime);
					m_nTimes[i] = (unsigned int)ttTime;
					pPQTime++;
				}//end for
			}//end if( typePhysical == ID_PHYS_TYPE_TIMESTAMPPQDIF )
			else
			{
				if( m_fTimes != NULL )
				{
					delete []m_fTimes;
					m_fTimes = NULL;
				}
				m_fTimes = new float[m_iPointCount];
				if( m_fTimes == NULL )
				{
					delete []pValue;
					return false;
				}

				double *pdVal = (double*)pValue;
				for(long i=0;i<lPoint;i++)
				{
					m_fTimes[i] = (float)*pdVal;//*((float*)pdVal);
					pdVal ++;
				}//end for
				
				delete []pValue;
				pValue = NULL;
			}//end ʱ��ת��
		}//end ��һͨ����һ����
		else if(m_iChannelInstIdx > 0 && m_iSeriesInstIdx == 0 )
		{
			return true;
		}
		else if(m_iChannelInstIdx >= 0 && m_iSeriesInstIdx == 1 )//�ڶ�����
		{
			if( m_iCurrentChaIdx < 0 )
				return false;

			if( typePhysical == ID_PHYS_TYPE_COMPLEX8 || typePhysical == ID_PHYS_TYPE_COMPLEX16 ||
				typePhysical == ID_PHYS_TYPE_TIMESTAMPPQDIF || typePhysical == ID_PHYS_TYPE_GUID)
			{
				//�ڶ����в�֧����Щ��������
				return false;
			}

			//����ͨ����Ӧ��ֵ
			map<int,map<int,float*>*>::iterator it = m_ChaValues.find(m_iCurrentChaIdx);
			if( it == m_ChaValues.end() )//û�ҵ�
			{
				m_pCurrentChaValues = new map<int,float*>;
				m_ChaValues.insert( map<int,map<int,float*>*>::value_type(m_iCurrentChaIdx,m_pCurrentChaValues) );
			}
			else //�ҵ�
			{
				m_pCurrentChaValues = (*it).second;
			}

			if( m_pCurrentChaValues == NULL)
				return false;

			//�������ж�Ӧ�ı�������Ư
			double dScale = 1.00;
			double dOffset = 0.00;
			map<int,double>::iterator it2 = m_SeriesScale.find(m_iSeriesInstIdx);
			if( it2 != m_SeriesScale.end() )
				dScale = (*it2).second;

			it2 = m_SeriesOffset.find(m_iSeriesInstIdx);
			if( it2 != m_SeriesOffset.end() )
				dOffset = (*it2).second;

			//ȡ��ֵ
			void *pValue = GetSeriesData(StoragMeth[1],typePhysical,dScale,dOffset,pVector->count,pdata,lPoint);
			if( m_iPointCount != lPoint || pValue == NULL )
			{
				if( pValue )
					delete []pValue;
				return false;
			}

			//����ֵ
			float *pfDestValue = new float[m_iPointCount];
			if( pfDestValue == NULL )
			{
				if( pValue )
					delete []pValue;
				return false;
			}

			double *pdVal = (double*)pValue;
			for(int i=0;i< m_iPointCount;i++)
			{
				pfDestValue[i] = (float)(*pdVal);
				pdVal++;
			}

			m_pCurrentChaValues->insert( map<int,float*>::value_type(m_iCurrentHarm,pfDestValue) );
		}

	}//end if( PQDIF_IsEqualGUID(tag,tagSeriesValues) )

	return true;
}

//##ModelId=4C7369BB01CC
int Observation::ParseOneRecord(FILE* pFile, c_record_mainheader& hdrRecord, UINT4 styleCompression, UINT4 algCompression, bool bHeaderHasRead)
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
	if(  iRecordType != 4 )
	{
		return iRecordType;//����Observation��¼
	}

	//���ø��ຯ��
	return Record::ParseOneRecord(pFile,hdrRecord,styleCompression,algCompression,true);
}


void *Observation::GetSeriesData
( 
 UINT4   idStorageMethod, 
 INT1    idPhysicalType,
 double  rScale, 
 double  rOffset, 
 long    countRawPoints,
 void *  pvectSeriesArray,
 long&   countPoints
 )
{
	double *    pdValues = NULL;
	double *    prTempValues;
	long        idxPoint;
	long        idxRaw;

	//�õ�ʵ�ʵ���
	countPoints = GetSeriesCount( idStorageMethod, idPhysicalType, countRawPoints, pvectSeriesArray );

	if( idStorageMethod & ID_SERIES_METHOD_VALUES )
	{
		if( idPhysicalType == ID_PHYS_TYPE_COMPLEX8 || idPhysicalType == ID_PHYS_TYPE_COMPLEX16 ||
			idPhysicalType == ID_PHYS_TYPE_TIMESTAMPPQDIF || idPhysicalType == ID_PHYS_TYPE_GUID)
		{
			//���ӵ���������
			return (void*)pvectSeriesArray;
		}

		//ֱ�Ӵ洢��ʽ
		if( countPoints > 0 && countPoints == countRawPoints )
		{
			pdValues = new double[ countPoints ];
			if( pdValues == NULL )
				return NULL;

			prTempValues = pdValues;
			for( idxPoint = 0; idxPoint < countPoints; idxPoint++, prTempValues++ )
            {
                GetSimpVectorArrayValue( pvectSeriesArray, idPhysicalType, idxPoint, *prTempValues );
            }
		}
	}
	else if( idStorageMethod & ID_SERIES_METHOD_INCREMENT )
	{
		//�����洢��ʽ
		if( idPhysicalType == ID_PHYS_TYPE_COMPLEX8 || idPhysicalType == ID_PHYS_TYPE_COMPLEX16 ||
			idPhysicalType == ID_PHYS_TYPE_TIMESTAMPPQDIF || idPhysicalType == ID_PHYS_TYPE_GUID)
		{
			//���ڸ��ӵ��������ݣ���֧�ִ��ִ洢��ʽ
			return NULL;
		}

		double      valueNext;
		double      rCount;
		double      rRate ;
		long        countChanges;
		long        idxChange;
		long        countPointsThisChange;

		if( countPoints > 0 )
		{
			pdValues = new double[ countPoints ];
			if( pdValues )
			{
				prTempValues = pdValues;
				valueNext = 0.0;

				//�ӵ�һ�����õ�changes������
				GetSimpVectorArrayValue( pvectSeriesArray, idPhysicalType, 0, rCount );
				countChanges = (long) rCount;

				//����ȫ����changes
				for( idxChange = 0; idxChange < countChanges; idxChange++ )
				{
					//��������������һ������ÿ��2������һ��changes��
					idxRaw = ( idxChange * 2 ) + 1;

					//ȡ�øñ����µĵ���
					GetSimpVectorArrayValue( pvectSeriesArray, idPhysicalType, idxRaw + 0, rCount );
					//ȡ����������
					GetSimpVectorArrayValue( pvectSeriesArray, idPhysicalType, idxRaw + 1, rRate );

					countPointsThisChange = (long) rCount;

					//����ÿ�����ֵ
					for( idxPoint = 0; idxPoint < countPointsThisChange; idxPoint++, prTempValues++ )
					{
						*prTempValues = valueNext;

						//��һ�����ǰһ��������rRate
						valueNext += rRate;
					}   //  for( points );
				}   //  for( changes )

			}
		}
	}
	else
	{
		return NULL;//�������������ִ洢��ʽ�е�һ��
	}

	//��������ݻ�ʹ�ñ�������Ư��ʽ�洢
	if( idStorageMethod & ID_SERIES_METHOD_SCALED )
	{
		if( pdValues && countPoints > 0 )
		{
			prTempValues = pdValues;
			for( idxPoint = 0; idxPoint < countPoints; idxPoint++, prTempValues++ )
			{
				*prTempValues *= rScale;//�˱���ϵ��
				*prTempValues += rOffset;//������Ư
			}
		}
	}

	return (void*)pdValues;
}

 long Observation::GetSeriesCount
	 ( 
	 UINT4   idStorageMethod,//�洢��ʽ
	 INT1    idPhysicalType, //������������
	 long    countRawPoints, //ԭ����(vector�ṹ�е�count)
	 void *  pvectSeriesArray //ָ������
	 )
 {
	 long    countPoints = 0;

	 if( idStorageMethod & ID_SERIES_METHOD_VALUES )
	 {
		 //ֱ�Ӵ洢��ʽ�����е�������ԭ����
		 countPoints = countRawPoints;
	 }
	 else if( idStorageMethod & ID_SERIES_METHOD_INCREMENT )
	 {
		 //������ʽ�洢
		 long    idxRaw = 0;
		 double  rCount = 0;

		 //������������
		 if( countRawPoints > 0 )
		 {
			 //��һ�����Ƕ������ڶ����ǵ�һ�εĵ�����Ȼ���һ��������һ�εĵ���
			 for( idxRaw = 1; idxRaw < countRawPoints; idxRaw += 2 )
			 {
				 rCount = 0;
				 GetSimpVectorArrayValue( pvectSeriesArray, idPhysicalType, idxRaw, rCount );
				 countPoints += (long) rCount;
			 }
		 }
	 }

	 return countPoints;
 }

 bool Observation::GetSimpVectorArrayValue
	 ( 
	 void *  pdata, 
	 INT1    typePhysical, 
	 int     idx, 
	 double& value 
	 )
 {
	 bool    status = true;
	 if( pdata )
	 {
		 switch( typePhysical )
		 {
		 case ID_PHYS_TYPE_INTEGER1:
			 {
				 INT1 * pvalues = (INT1 *) pdata;
				 value = (double) pvalues[ idx ];
			 }
			 break;

		 case ID_PHYS_TYPE_INTEGER2:
			 {
				 INT2 * pvalues = (INT2 *) pdata;
				 value = (double) pvalues[ idx ];
			 }
			 break;

		 case ID_PHYS_TYPE_INTEGER4:
			 {
				 INT4 * pvalues = (INT4 *) pdata;
				 value = (double) pvalues[ idx ];
			 }
			 break;

		 case ID_PHYS_TYPE_UNS_INTEGER1:
			 {
				 UINT1 * pvalues = (UINT1 *) pdata;
				 value = (double) pvalues[ idx ];
			 }
			 break;

		 case ID_PHYS_TYPE_UNS_INTEGER2:
			 {
				 UINT2 * pvalues = (UINT2 *) pdata;
				 value = (double) pvalues[ idx ];
			 }
			 break;

		 case ID_PHYS_TYPE_UNS_INTEGER4:
			 {
				 UINT4 * pvalues = (UINT4 *) pdata;
				 value = (double) pvalues[ idx ];
			 }
			 break;

		 case ID_PHYS_TYPE_REAL4:
			 {
				 REAL4 * pvalues = (REAL4 *) pdata;
				 value = (double) pvalues[ idx ];
			 }
			 break;

		 case ID_PHYS_TYPE_REAL8:
			 {
				 REAL8 * pvalues = (REAL8 *) pdata;
				 value = (double) pvalues[ idx ];
			 }
			 break;

		 default:
			 status = false;
			 break;
		 }
	 }
	 else
	 {
		 status = false;
	 }

	 return status;
 }

