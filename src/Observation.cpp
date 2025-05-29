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
	long                    lCollItems = 5; //添加5个属性
	long                    lIdxItem = 0;
	c_collection_element	*pCollElem = NULL;
	c_collection_element	*pCollChanInsts = NULL;
	c_collection_element	*pCollOneChanInst = NULL;
	c_collection_element	*pCollSeriesInsts = NULL;
	c_collection_element	*pCollOneSeriesInsts = NULL;

	int		iSeriesNum = 2;	//序列数（至少两个序列）

	unsigned int	nAttrVal = 0;
	PQData	*pPQData = pDataList->GetElem(iFromPos);
	if( pPQData == NULL )
		return true;

	if( pFileHeader == NULL || pDataList == NULL )
		return false;

	switch(pFileHeader->FileType)
	{
	case 1:
		iSeriesNum = 4;//4序列：时间、最大、最小、平均
		break;
	case 2:
		iSeriesNum = 4;//4序列：时间、最大、最小、平均
		break;
	case 4:
		iSeriesNum = 3;//3序列：时间、最大、平均
		break;
	case 8:
		iSeriesNum = 4;//4序列：时间、畸变率、最大、平均
		break;
	case 16:
		iSeriesNum = 3;//3序列：时间、最大、平均
		break;
	case 32:
		iSeriesNum = 3;//3序列：时间、电压变动、电压变动频度
		break;
	case 64:
		iSeriesNum = 2;//2序列：时间、短时闪变
		break;
	case 128:
		iSeriesNum = 3;//3序列：时间、谐波有功功率、谐波无功功率
		break;
	case 256:
		iSeriesNum = 5;//5序列：时间、正有功电能、正无功电能、负有功电能、负无功电能
		break;
	}

	//添加第一个collection
	pCollElem = AddCollection( lCollItems );
	if( pCollElem == NULL )
		return false;

	//填充collection元素

	/*----------开始处理数据段的属性-------->>>>>>>>>>>*/

	//时间
	TIMESTAMPPQDIF pqTime = {0};
	if( pFileHeader->DataTimeType == 2)//绝对时间
	{
		time_t ttTime = (time_t)pPQData->time;
		struct tm tmTime = *localtime(&ttTime);
		Time_t2PQTime(ttTime,&pqTime);
	}
	else //相对时间
	{
		int iMultiple = 1;	//秒的倍数
		switch( pFileHeader->DataTimeUnit )
		{
		case 1://秒
			iMultiple = 1;
			break;
		case 2://分钟
			iMultiple = 60;
			break;
		case 3://小时
			iMultiple = 3600;
			break;
		case 4://天
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

	//数据段名称
	char szObName[64] = {0};
	struct tm tmStartTime = {0};
	PQTime2Tm(&pqTime,&tmStartTime);
	sprintf(szObName,"%d-%d-%d %d:%d:%d",tmStartTime.tm_year+1900,tmStartTime.tm_mon+1,tmStartTime.tm_mday,
		tmStartTime.tm_hour,tmStartTime.tm_min,tmStartTime.tm_sec);

	AddVector(szObName,strlen(szObName)+1,ID_PHYS_TYPE_CHAR1,tagObservationName,pCollElem[lIdxItem++]);
	
	//创建时间
	AddScalar((const char *)&pqTime,ID_PHYS_TYPE_TIMESTAMPPQDIF,tagTimeCreate,pCollElem[lIdxItem++]);

	//数据开始时间
	AddScalar((const char *)&pqTime,ID_PHYS_TYPE_TIMESTAMPPQDIF,tagTimeStart,pCollElem[lIdxItem++]);

	//触发方式
	nAttrVal = ID_TRIGGER_METH_NONE;
	AddScalar((const char *)&nAttrVal,ID_PHYS_TYPE_UNS_INTEGER4,tagTriggerMethodID,pCollElem[lIdxItem++]);

	/*<<<<<<<<<<<<------处理数据段的属性结束--------*/

	//添加下一级collection(即tagChannelInstances，本级的第5个属性)
	//下一级collection属性的个数由通道数及谐波数的乘积决定
	int iChaNum = pPQData->ChannelNum;
	int iHarmCount = 1;	//谐波次数（为1即非谐波数据）

	if(pFileHeader->FileType == 8)//谐波趋势
	{
		iHarmCount = (pPQData->valueNum-1)/2;//剔除总畸变率
	}
	if(pFileHeader->FileType == 16)//间谐波趋势
	{
		iHarmCount = pPQData->valueNum/2;
	}
	else if(pFileHeader->FileType == 128)//谐波功率
	{
		//每一次谐波功率由有功和无功组成
		iHarmCount = (pPQData->valueNum-3)/2;
	}

	if( iHarmCount < 1 )
		return false;

	pCollChanInsts=AddCollection( iChaNum*iHarmCount,tagChannelInstances,pCollElem[lIdxItem++] );
	if( pCollChanInsts == NULL )
		return false;

	long    lOneChanInstItems = 2;//定义一个通道实例的属性个数
	int		iChannelInstIdx = -1;

	for(int i=0;i<iChaNum;i++) //所有通道
	{
		for(int j=0;j<iHarmCount;j++) //所有谐波
		{
			iChannelInstIdx++;
			//再添加下一级collection(即tagOneChannelInst)
			pCollOneChanInst = AddCollection(lOneChanInstItems,tagOneChannelInst,pCollChanInsts[iChannelInstIdx]);
			if( pCollOneChanInst == NULL )
				return false;

			//添加通道实例信息
			long lChaInstInfIdx = 0;

			//对应的通道索引
			AddScalar((const char *)&i,ID_PHYS_TYPE_UNS_INTEGER4,tagChannelDefnIdx,
				pCollOneChanInst[lChaInstInfIdx++]);

			//添加序列实例
			pCollSeriesInsts = AddCollection(iSeriesNum,tagSeriesInstances,pCollOneChanInst[lChaInstInfIdx++]);
			if( pCollSeriesInsts == NULL )
				return false;

			//定义每个序列属性
			char	*pTmpSeriesVal = NULL;
			INT1	iTmpTypePhysical = 0;
			long    lOneSeriesItems = 1;//定义一个序列的属性个数
			for(int m=0;m<iSeriesNum;m++)
			{
				if( (i!=0 || j!=0) && (m==0) )
				{
					lOneSeriesItems = 2;//不是第一通道的时间序列有两个属性
				}
				else
				{
					lOneSeriesItems = 1;//其它有1个属性
				}

				if( iHarmCount > 1 )
				{
					lOneSeriesItems++;//有谐波，再加1个属性
				}

				//添加单个序列实例的属性
				pCollOneSeriesInsts = AddCollection(lOneSeriesItems,tagOneSeriesInstance,pCollSeriesInsts[m]);
				if( pCollOneSeriesInsts == NULL )
					return false;

				//定义序列属性
				int		iSeriesIdx = 0;

				//组号（谐波次数？）
				if( iHarmCount > 1 )
				{
					nAttrVal = j+1;
					AddScalar((const char *)&nAttrVal,ID_PHYS_TYPE_INTEGER2,tagChannelGroupID,
						pCollOneSeriesInsts[iSeriesIdx++]);
				}

				//不是第一通道的时间序列，我们共享第一个通道的时间序列
				if( (i!=0 || j!=0) && (m==0) )
				{
					//共享第一个通道
					nAttrVal = 0;
					AddScalar((const char *)&nAttrVal,ID_PHYS_TYPE_UNS_INTEGER4,tagSeriesShareChannelIdx,
						pCollOneSeriesInsts[iSeriesIdx++]);

					//共享第一个序列
					nAttrVal = 0;
					AddScalar((const char *)&nAttrVal,ID_PHYS_TYPE_UNS_INTEGER4,tagSeriesShareSeriesIdx,
						pCollOneSeriesInsts[iSeriesIdx++]);

					continue;
				}//end 不是第一通道的时间序列

				int iTmpDataNum = iDataNum;
				if( m == 0 ) //时间值序列
				{
					//生成时间序列
					pTmpSeriesVal = GeneratTimeSeries(pFileHeader,pDataList,iFromPos,iTmpDataNum,iTmpTypePhysical);
				}//end 时间值序列
				else //值序列
				{
					iTmpTypePhysical = ID_PHYS_TYPE_REAL4;
					//生成值序列
					int poistion = 0;
					switch(pFileHeader->FileType)
					{
					case 8://谐波
						if(m==1)
							poistion = 0;
						else
							poistion = j+1;
						break;
					case 128://谐波功率
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
					continue;	//没数据了？
				}

				if(pTmpSeriesVal)
				{
					delete []pTmpSeriesVal;
					pTmpSeriesVal = NULL;
				}

			}//end for(int m=0;m<2;m++) 定义每个序列属性
				

		}//end 处理完谐波

	}//end for(int i=0;i<iChaNum;i++) //处理完所有通道

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

	if( pFileHeader->DataTimeType == 2)//绝对时间
	{
		//PQDIF的序列值不支持绝对时间ID_PHYS_TYPE_TIMESTAMPPQDIF吗？
		//如果是绝对时间，我们将它转换成相对时间

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

		int iFirstTime = pPQData->time;	//第一点的时间
		int iTime = 0;

		for(int i=0;i<iDataNum;i++)
		{
			pPQData = pDataList->GetElem(iFromPos+i);
			if( pPQData == NULL)//没那么多点？
			{
				iDataNum = i;
				return pTimeSeries;
			}

			iTime = pPQData->time - iFirstTime;

			if( iTime < 0 )//时间错乱？
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
	else //相对时间
	{
		pPQData = pDataList->GetElem(iFromPos);//get first point
		if( pPQData == NULL )
			return NULL;

		int iFirstTime = pPQData->time;	//第一点的时间
		int iTime = 0;

		typePhysical = ID_PHYS_TYPE_UNS_INTEGER4;
		pTimeSeries = new char[sizeof(int)*iDataNum];
		if( pTimeSeries == NULL )
			return NULL;

		memset(pTimeSeries,0,sizeof(int)*iDataNum);
		pTmpData = pTimeSeries;

		int iMultiple = 1;	//秒的倍数
		switch( pFileHeader->DataTimeUnit )
		{
		case 1://秒
			iMultiple = 1;
			break;
		case 2://分钟
			iMultiple = 60;
			break;
		case 3://小时
			iMultiple = 3600;
			break;
		case 4://天
			iMultiple = 24*3600;
			break;
		default:
			iMultiple = 1;
			break;
		}

		for(int i=0;i<iDataNum;i++)
		{
			pPQData = pDataList->GetElem(iFromPos+i);
			if( pPQData == NULL)//没那么多点？
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
		if( pPQData == NULL || pPQData->ppValues==NULL || *(pPQData->ppValues) == NULL)//没那么多点？
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
			int iChanNum = m_ChaValues.size() ;	//通道个数
			int iValueNum = 0;	//每个点拥有值的个数(如谐波，每个点有多个)
			bool isFirstRecord = false;	//是第一个记录?
			float  fTimeOffset = 0.00;	//本段数据开始时间与整个数据数据开始时间的偏移

			if( DataList.GetElemNum() == 0 )
				isFirstRecord = true;
			
			if( iChanNum == 0 )
				return false;

			map<int,map<int,float*>*>::iterator it = m_ChaValues.begin();
			iValueNum = ((*it).second)->size();

			//不是第一条记录且是相对时间
			if( !isFirstRecord && g_FileHeader.DataTimeType ==1 )
			{
				if( m_fTimes == NULL)
					return false;
				//计算本段数据开始时间与整个数据数据开始时间的偏移
				time_t ttFileStartTime = mktime(&(g_FileHeader.DataStartTime));
				time_t ttThisStartTime = mktime(&tmDataStartTime);

				fTimeOffset = (float)(ttThisStartTime - ttFileStartTime);
				if( fTimeOffset <= 0.00)// || fTimeOffset > 6000000.00 )
				{
					//数据异常?
					return false;
				}
			}
			else if( isFirstRecord ) //第一条记录
			{
				//第一段的开始时间作为整个数据的开始时间
				memcpy( &(g_FileHeader.DataStartTime),&tmDataStartTime,sizeof(struct tm) );
			}

			//将数据转换成PQData结构，并放入DataList中
			for(int i = 0;i<m_iPointCount;i++)//每个点
			{
				PQData *pPQData = new PQData(iChanNum,iValueNum);
				pPQData->ChannelNum = iChanNum;

				//处理时间
				if( g_FileHeader.DataTimeType == 2 )//是绝对时间
				{
					if( m_nTimes == NULL )
						return false;

					pPQData->isUseFloatTime = false;
					pPQData->time = m_nTimes[i];
				}
				else	//相对时间
				{
					pPQData->isUseFloatTime = true;
					pPQData->fTime = m_fTimes[i] +fTimeOffset;
				}

				//处理数据
				int iChanIndex = -1;	//通道索引
				int iValIndex = -1;		//值索引

				map<int,float*>::iterator valIt;
				it = m_ChaValues.begin();

				if( pPQData->ppValues == NULL || *(pPQData->ppValues) == NULL)
					return false;

				for(;it != m_ChaValues.end();it++)//每一个通道
				{
					iChanIndex++;

					map<int,float*> *pValMap = (*it).second;
					if( pValMap == NULL )
						return NULL;

					iValIndex = -1;

					valIt = pValMap->begin();
					for(;valIt != pValMap->end();valIt++)//每个点所有值
					{
						//pfVal指向第一序列值是基波的序列，第二序列是二次谐波序列等等
						float *pfVal = (*valIt).second;
						if( pfVal == NULL)
							return false;

						iValIndex++;

						//pPQData->ppValues中按[通道][谐波次数]方式存储,即：
						//{ {1次谐波,2次谐波,..N次谐波,},/*通道1*/
						//  {1次谐波,2次谐波,..N次谐波,},/*通道2*/
						//    ....
						//  {1次谐波,2次谐波,..N次谐波,}/*通道N*/ }

						//pfVal按[点顺序]方式存储，即{点1,点2,...点N}
						pPQData->ppValues[iChanIndex][iValIndex] = pfVal[i];
					}//end 每个点所有值
				}//end 每一个通道

				DataList.AddElem(pPQData);

			}//end 每个点
		}

		//重新初始化记录
		m_iChannelInstIdx = -1;
		m_iSeriesInstIdx = -1;
		m_iCurrentChaIdx = -1;
		m_iCurrentHarm = 0;

		map<int,map<int,float*>*>::iterator it = m_ChaValues.begin();
		for(;it != m_ChaValues.end();it++)//每一个通道
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
		//end重新初始化记录

		if( hdrRecord.linkNextRecord == 0 )
		{
			return true;
		}
		fseek( pFile, hdrRecord.linkNextRecord, SEEK_SET );//处理下一条记录

	}while( hdrRecord.linkNextRecord != 0 && !feof(pFile) );

	return true;
}

//##ModelId=4C7369BB01BD
bool Observation::AcceptCollection(const GUID& tag, int level)
{
	if( PQDIF_IsEqualGUID( tag, tagChannelInstances )) //全部通道实例
	{
		m_iChannelInstIdx = -1;
		m_iSeriesInstIdx = -1;
		return true;
	}
	else if( PQDIF_IsEqualGUID( tag, tagOneChannelInst )) //一个通道实例
	{
		m_iChannelInstIdx++;
		m_iSeriesInstIdx = -1;
		return true;
	}
	else if( PQDIF_IsEqualGUID( tag, tagSeriesInstances )) //通道全部序列实例
	{
		m_iSeriesInstIdx = -1;
		return true;
	}
	else if( PQDIF_IsEqualGUID( tag, tagOneSeriesInstance )) //通道序列一个实例
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

	if( m_iChannelInstIdx == -1 && m_iSeriesInstIdx == -1) //整个通道的属性
	{
		if( PQDIF_IsEqualGUID(tag,tagTimeStart) )//数据开始时间
		{
			if( ID_PHYS_TYPE_TIMESTAMPPQDIF != typePhysical )
				return false;

			memcpy(&pqTime,pdata,sizeof(pqTime));
			//转换时间
			PQTime2Tm(&pqTime,&tmDataStartTime);
			return true;
		}
	}//end 整个通道的属性
	if( m_iChannelInstIdx >= 0 ) //通道实例的属性
	{
		if( PQDIF_IsEqualGUID(tag,tagChannelDefnIdx) )//通道实例对应的通道索引
		{
			if( ID_PHYS_TYPE_UNS_INTEGER4 != typePhysical )
				return false;

			m_iCurrentChaIdx = *((int*)pdata);
			return true;
		}
		else if( PQDIF_IsEqualGUID(tag,tagChannelGroupID) )//通道实例对应的谐波次数
		{
			if( ID_PHYS_TYPE_INTEGER2 != typePhysical )
				return false;

			m_iCurrentHarm = *((short*)pdata);
			return true;
		}
	}//end 整个通道的属性

	if( m_iSeriesInstIdx >= 0 ) //序列的属性
	{
		if( PQDIF_IsEqualGUID(tag,tagSeriesScale) )//序列对应的比例系数
		{
			double dScale = 0.00;
			if(!GetSimpVectorArrayValue(pdata,typePhysical,0,dScale))
				return false;

			m_SeriesScale.insert(map<int,double>::value_type(m_iSeriesInstIdx,dScale));
			return true;
		}
		else if( PQDIF_IsEqualGUID(tag,tagSeriesOffset) )//序列对应的零漂
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
		//第一通道第一序列
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

			//时间转换
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
			}//end 时间转换
		}//end 第一通道第一序列
		else if(m_iChannelInstIdx > 0 && m_iSeriesInstIdx == 0 )
		{
			return true;
		}
		else if(m_iChannelInstIdx >= 0 && m_iSeriesInstIdx == 1 )//第二序列
		{
			if( m_iCurrentChaIdx < 0 )
				return false;

			if( typePhysical == ID_PHYS_TYPE_COMPLEX8 || typePhysical == ID_PHYS_TYPE_COMPLEX16 ||
				typePhysical == ID_PHYS_TYPE_TIMESTAMPPQDIF || typePhysical == ID_PHYS_TYPE_GUID)
			{
				//第二序列不支持这些物理数据
				return false;
			}

			//查找通道对应的值
			map<int,map<int,float*>*>::iterator it = m_ChaValues.find(m_iCurrentChaIdx);
			if( it == m_ChaValues.end() )//没找到
			{
				m_pCurrentChaValues = new map<int,float*>;
				m_ChaValues.insert( map<int,map<int,float*>*>::value_type(m_iCurrentChaIdx,m_pCurrentChaValues) );
			}
			else //找到
			{
				m_pCurrentChaValues = (*it).second;
			}

			if( m_pCurrentChaValues == NULL)
				return false;

			//查找序列对应的比例和零漂
			double dScale = 1.00;
			double dOffset = 0.00;
			map<int,double>::iterator it2 = m_SeriesScale.find(m_iSeriesInstIdx);
			if( it2 != m_SeriesScale.end() )
				dScale = (*it2).second;

			it2 = m_SeriesOffset.find(m_iSeriesInstIdx);
			if( it2 != m_SeriesOffset.end() )
				dOffset = (*it2).second;

			//取得值
			void *pValue = GetSeriesData(StoragMeth[1],typePhysical,dScale,dOffset,pVector->count,pdata,lPoint);
			if( m_iPointCount != lPoint || pValue == NULL )
			{
				if( pValue )
					delete []pValue;
				return false;
			}

			//拷贝值
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
		return iRecordType;//不是Observation记录
	}

	//调用父类函数
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

	//得到实际点数
	countPoints = GetSeriesCount( idStorageMethod, idPhysicalType, countRawPoints, pvectSeriesArray );

	if( idStorageMethod & ID_SERIES_METHOD_VALUES )
	{
		if( idPhysicalType == ID_PHYS_TYPE_COMPLEX8 || idPhysicalType == ID_PHYS_TYPE_COMPLEX16 ||
			idPhysicalType == ID_PHYS_TYPE_TIMESTAMPPQDIF || idPhysicalType == ID_PHYS_TYPE_GUID)
		{
			//复杂的物理数据
			return (void*)pvectSeriesArray;
		}

		//直接存储方式
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
		//增量存储方式
		if( idPhysicalType == ID_PHYS_TYPE_COMPLEX8 || idPhysicalType == ID_PHYS_TYPE_COMPLEX16 ||
			idPhysicalType == ID_PHYS_TYPE_TIMESTAMPPQDIF || idPhysicalType == ID_PHYS_TYPE_GUID)
		{
			//对于复杂的物理数据，不支持此种存储方式
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

				//从第一个数得到changes的数量
				GetSimpVectorArrayValue( pvectSeriesArray, idPhysicalType, 0, rCount );
				countChanges = (long) rCount;

				//处理全部的changes
				for( idxChange = 0; idxChange < countChanges; idxChange++ )
				{
					//计算索引（除第一个数，每隔2个数是一个changes）
					idxRaw = ( idxChange * 2 ) + 1;

					//取得该比例下的点数
					GetSimpVectorArrayValue( pvectSeriesArray, idPhysicalType, idxRaw + 0, rCount );
					//取得增量比例
					GetSimpVectorArrayValue( pvectSeriesArray, idPhysicalType, idxRaw + 1, rRate );

					countPointsThisChange = (long) rCount;

					//计算每个点的值
					for( idxPoint = 0; idxPoint < countPointsThisChange; idxPoint++, prTempValues++ )
					{
						*prTempValues = valueNext;

						//后一个点比前一个点增量rRate
						valueNext += rRate;
					}   //  for( points );
				}   //  for( changes )

			}
		}
	}
	else
	{
		return NULL;//必须有以上两种存储方式中的一种
	}

	//保存的数据还使用比例、零漂方式存储
	if( idStorageMethod & ID_SERIES_METHOD_SCALED )
	{
		if( pdValues && countPoints > 0 )
		{
			prTempValues = pdValues;
			for( idxPoint = 0; idxPoint < countPoints; idxPoint++, prTempValues++ )
			{
				*prTempValues *= rScale;//乘比例系数
				*prTempValues += rOffset;//加上零漂
			}
		}
	}

	return (void*)pdValues;
}

 long Observation::GetSeriesCount
	 ( 
	 UINT4   idStorageMethod,//存储方式
	 INT1    idPhysicalType, //数据物理类型
	 long    countRawPoints, //原点数(vector结构中的count)
	 void *  pvectSeriesArray //指向数据
	 )
 {
	 long    countPoints = 0;

	 if( idStorageMethod & ID_SERIES_METHOD_VALUES )
	 {
		 //直接存储方式，序列点数就是原点数
		 countPoints = countRawPoints;
	 }
	 else if( idStorageMethod & ID_SERIES_METHOD_INCREMENT )
	 {
		 //增量方式存储
		 long    idxRaw = 0;
		 double  rCount = 0;

		 //读整个比例段
		 if( countRawPoints > 0 )
		 {
			 //第一个数是段数，第二数是第一段的点数，然后隔一个数是下一段的点数
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

