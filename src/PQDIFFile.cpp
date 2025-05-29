#include "PQDIFFile.h"
#include <stdio.h>

#ifdef UNIT_TEST
#include "CPQDataList.cpp"
#include "Record.cpp"
#include "Container.cpp"
#include "DataSource.cpp"
#include "MonitorSetting.cpp"
#include "Observation.cpp"

#endif

//##ModelId=4C5B6A8303E7
bool PQDIFFile::PQDIF2SHPQ(const char* pSrcFile, PQFileHead* pFileHeader, vector<ChannelInf>& ChannelInfs, 
						   vector<PQDIFInterFace::VAL_UNIT>  *Units,CPQDataList& DataList)
{
	if( pSrcFile == NULL || pFileHeader == NULL )
		return false;

	FILE *pFile = fopen(pSrcFile,"rb");
	if( pFile == NULL )
		return false;

	//分析头
	PQFileHead *pTmpHeader = &g_FileHeader;
	if( !HeaderRecord.ParseFile(pFile,pTmpHeader) )
		return false;

	//分析通道定义
	if( !ds.ParseFile(pFile,&ChannelInfs,pTmpHeader,Units) )
		return false;

	//分析通道定值
	if( !ms.ParseFile(pFile,&ChannelInfs,pTmpHeader) )
		return false;

	//分析数据
	if( !DataRecord.ParseFile(pFile,DataList,pTmpHeader) )
		return false;

	memcpy(pFileHeader,&g_FileHeader,sizeof(PQFileHead) );

	pFileHeader->ActiveChNum = ChannelInfs.size();
	pFileHeader->TotalPoint = DataList.GetElemNum();

	PQData *pTmpData = DataList.GetElem(1);
	if( pTmpData )
	{
		pFileHeader->PerPointSize = sizeof(int) + pTmpData->ChannelNum*pTmpData->valueNum*sizeof(float);
	}

	return true;
}

//##ModelId=4C5B6A8303EC
bool PQDIFFile::PQDIF2SHPQ(const char* pSrcFile, const char* pDestFile)
{
	PQFileHead	FileHeader = {0};
	vector<ChannelInf> ChannelInfs;
	vector<PQDIFInterFace::VAL_UNIT>  Units;
	CPQDataList PQDataList(0X78FFFFFF,false);

	if( pSrcFile == NULL || pDestFile == NULL )
		return false;

	if( !PQDIF2SHPQ(pSrcFile,&FileHeader,ChannelInfs,&Units,PQDataList) )
	{
		ClearData( PQDataList );
		ChannelInfs.clear();
		return false;
	}

	//写文件
	FILE *pFile = fopen(pDestFile,"wb");
	if( pFile == NULL )
	{
		ClearData( PQDataList );
		ChannelInfs.clear();
		return false;
	}

	//文件头
	if( 1 != fwrite(&FileHeader,sizeof(PQFileHead),1,pFile ) )
	{
		ClearData( PQDataList );
		ChannelInfs.clear();
		return false;
	}

	//通道信息
	vector<ChannelInf>::iterator it = ChannelInfs.begin();
	for(;it != ChannelInfs.end();it++)
	{
		if( 1 != fwrite(&(*it),sizeof(ChannelInf),1,pFile ) )
		{
			ClearData( PQDataList );
			ChannelInfs.clear();
			return false;
		}
	}

	//写数据
	int iPoint = PQDataList.GetElemNum();
	for(int i=1;i<=iPoint;i++)
	{
		PQData *pData = PQDataList.GetElem(i);
		if( pData == NULL || pData->ppValues == NULL || *pData->ppValues == NULL)
		{
			ClearData( PQDataList );
			ChannelInfs.clear();
			return false;
		}

		if( pData->isUseFloatTime )
			pData->time = (int)pData->fTime;

		if( 1 != fwrite(&(pData->time),sizeof(int),1,pFile ) )
		{
			ClearData( PQDataList );
			ChannelInfs.clear();
			return false;
		}

		if( pData->ChannelNum*pData->valueNum 
			!= fwrite(&(pData->ppValues[0][0]),sizeof(float),pData->ChannelNum*pData->valueNum,pFile ) )
		{
			ClearData( PQDataList );
			ChannelInfs.clear();
			return false;
		}
	}//end for

	ClearData( PQDataList );
	ChannelInfs.clear();
	return true;
}

//##ModelId=4C5B6A84000E
bool PQDIFFile::SHPQ2PQDIF(const char* pSrcFile, const char* pDestFile)
{
	if( pSrcFile == NULL || pDestFile == NULL )
		return false;

	SHPQDIF::PQFileHead	FileHeader = {0};
	ChannelInf		ChanInf = {0};
	vector<SHPQDIF::ChannelInf> *ChannelInfs = NULL;
	vector<CPQDataList*> dataLists;	
	
	unsigned int *pEleInfs=NULL;//数据元素信息
	struct SHPQDIF::ChaCfgInf *pLineCfg = NULL;//线路配置信息
	struct SHPQDIF::ChaCfgInf *pChannelCfg = NULL;//通道配置信息

	int	iVolLineNum = 0;	//电压线路数
	int	iCurLineNum = 0;	//电流线路数
	int	iVolChaNum = 0;		//电压通道数
	int	iCurChaNum = 0;		//电流通道数

	//读文件
	FILE *pFile = fopen(pSrcFile,"rb+");
	if( pFile == NULL )
	{
		return false;
	}

	FILE *pDFileHandle = fopen(pDestFile,"wb");
	if( pDFileHandle == NULL )
	{
		return false;
	}

	//读文件头
	if( 1 != fread(&FileHeader,sizeof(FileHeader),1,pFile) || FileHeader.DataEleNum == 0)
	{
		fclose(pFile);
		return false;
	}

	//创建PQDIF文件头
	if( !CreateFileHeader(&FileHeader,pDFileHandle))
		return false;

	//读取数据元素信息
	pEleInfs = new unsigned int[FileHeader.DataEleNum*3];
	if( FileHeader.DataEleNum*3 != fread((void *)pEleInfs,sizeof(unsigned int),FileHeader.DataEleNum*3,pFile) )
	{
		fclose(pFile);
		return false;
	}
	int tt = ftell(pFile);

	//读线路配置信息
	if(FileHeader.ActiveLNum > 0 && FileHeader.ActiveLNum < 37)
	{
		pLineCfg = new SHPQDIF::ChaCfgInf[FileHeader.ActiveLNum];
		if( FileHeader.ActiveLNum != fread((void *)pLineCfg,sizeof(SHPQDIF::ChaCfgInf),FileHeader.ActiveLNum,pFile) )
		{
			fclose(pFile);
			return false;
		}

		for(int m=0;m<FileHeader.ActiveLNum;m++)
		{
			if( pLineCfg[m].ChannelType ==1)
			{
				iVolLineNum++;
			}
			else
			{
				iCurLineNum++;
			}
		}
	}
	tt = ftell(pFile);

	//读通道配置信息
	if(FileHeader.ActiveChNum > 0 && FileHeader.ActiveChNum < 97)
	{
		pChannelCfg = new SHPQDIF::ChaCfgInf[FileHeader.ActiveChNum];
		if( FileHeader.ActiveChNum != fread((void *)pChannelCfg,sizeof(SHPQDIF::ChaCfgInf),FileHeader.ActiveChNum,pFile) )
		{
			fclose(pFile);
			return false;
		}

		for(int m=0;m<FileHeader.ActiveChNum;m++)
		{
			if( pChannelCfg[m].ChannelType ==1)
			{
				iVolChaNum++;
			}
			else
			{
				iCurChaNum++;
			}
		}
	}

	long lElsSetPos = ftell(pFile);//数据元素定值信息的偏移位置
	struct SHPQDIF::PQNormalSetInf *pNorSet = NULL;
	struct SHPQDIF::PQHSetInf *pHSet = NULL;

	size_t	nReadLen = 0;

	ChannelInfs = new vector<SHPQDIF::ChannelInf>[FileHeader.DataEleNum];

	//循环处理各数据元素定值信息
	for(int i=0;i<FileHeader.DataEleNum;i++)
	{
		CPQDataList *PQDataList = new CPQDataList(0X78FFFFFF,false);
		dataLists.push_back(PQDataList);

		/*-->读取元素定值信息-->*/

		//文件指针先回到定值信息头
		fseek(pFile,lElsSetPos,SEEK_SET);

		//再计算本元素定值数据的偏移
		long lSetOffset = 0;
		for(int j=0;j<i;j++)
		{
			lSetOffset += pEleInfs[j*3 +1];
		}

		//文件指针指向本类型元素的定值信息
		fseek(pFile,lSetOffset,SEEK_CUR);

		tt = ftell(pFile);

		//读取定值
		unsigned int nEleType = pEleInfs[i*3];//元素类型
		int iSetInfNum = 0;
		int iCount = 0;

		if( nEleType==1 || nEleType==4 )//线路相关的
		{
			if( pLineCfg == NULL )
				return false;

			if( pEleInfs[i*3+1] > 0 ) //有定值信息
			{
				iSetInfNum = pEleInfs[i*3+1]/sizeof(SHPQDIF::PQNormalSetInf);
				if( iSetInfNum * sizeof(SHPQDIF::PQNormalSetInf) != pEleInfs[i*3+1] )
				{
					fclose(pFile);
					return false;
				}

				pNorSet = new SHPQDIF::PQNormalSetInf[iSetInfNum];
				nReadLen = fread((void *)pNorSet,sizeof(SHPQDIF::PQNormalSetInf),iSetInfNum,pFile);
				if( iSetInfNum != nReadLen )
				{
					fclose(pFile);
					return false;
				}
			}

			for(int m=0;m<FileHeader.ActiveLNum;m++)
			{
				if( nEleType == 1 && pLineCfg[m].ChannelType !=1)//频率，只取母线信息
				{
					continue;
				}

				SHPQDIF::ChannelInf chaninf;
				memset(&chaninf,0,sizeof(chaninf));
				memcpy(&chaninf,&pLineCfg[m],sizeof(SHPQDIF::ChaCfgInf));

				if( pNorSet != NULL )
				{
					if( iCount >= iSetInfNum )
					{
						fclose(pFile);
						return false;
					}
					chaninf.HiLimt = pNorSet[iCount].HiLimt;
					chaninf.LoLimt = pNorSet[iCount].LoLimt;
					chaninf.Ratio = pNorSet[iCount].Ratio;
					chaninf.Rating = pNorSet[iCount].Rating;
				}
				ChannelInfs[i].push_back(chaninf);
				iCount++;
			}//end for

			if( pNorSet != NULL )
			{
				delete []pNorSet;
				pNorSet = NULL;
			}
		}//end 线路相关的
		else //通道相关的
		{
			if( pChannelCfg == NULL )
				return false;

			if( pEleInfs[i*3+1] > 0 ) //有定值信息
			{
				if( nEleType != 8 )//非谐波
				{
					iSetInfNum = pEleInfs[i*3+1]/sizeof(SHPQDIF::PQNormalSetInf);
					if( iSetInfNum * sizeof(SHPQDIF::PQNormalSetInf) != pEleInfs[i*3+1] )
					{
						fclose(pFile);
						return false;
					}

					if( nEleType == 64 )
					{
						//闪变存放短时闪变和长时闪变的定值，我们需短时闪变定值
						iSetInfNum = iSetInfNum/2;
					}

					pNorSet = new SHPQDIF::PQNormalSetInf[iSetInfNum];
					nReadLen = fread((void *)pNorSet,sizeof(SHPQDIF::PQNormalSetInf),iSetInfNum,pFile);
					if( iSetInfNum != nReadLen )
					{
						int pos = ftell(pFile);
						fclose(pFile);
						return false;
					}
				}
				else//谐波
				{
					pHSet = new SHPQDIF::PQHSetInf[FileHeader.ActiveChNum];
					if( FileHeader.ActiveChNum != 
						fread((void *)pHSet,sizeof(SHPQDIF::PQHSetInf),FileHeader.ActiveChNum,pFile) )
					{
						fclose(pFile);
						return false;
					}
				}
			}

			for(int m=0;m<FileHeader.ActiveChNum;m++)
			{
				//波动、闪变需是电压通道（电压包含电压电流数据，所以需全部通道）
				if( ( nEleType == 32 || nEleType == 64)
					&& pChannelCfg[m].ChannelType !=1)
				{
					continue;
				}
				else if( (nEleType == 128 || nEleType == 256 )
					&& pChannelCfg[m].ChannelType !=2)
				{
					continue;
				}

				SHPQDIF::ChannelInf chaninf;
				memset(&chaninf,0,sizeof(chaninf));
				memcpy(&chaninf,&pChannelCfg[m],sizeof(SHPQDIF::ChaCfgInf));

				if( pNorSet != NULL )
				{
					if( iCount < iSetInfNum )
					{
						chaninf.HiLimt = pNorSet[iCount].HiLimt;
						chaninf.LoLimt = pNorSet[iCount].LoLimt;
						chaninf.Ratio = pNorSet[iCount].Ratio;
						chaninf.Rating = pNorSet[iCount].Rating;
					}
					iCount ++;
				}
				else if(pHSet != NULL )
				{
					chaninf.OddTrgLev = pHSet[m].OddTrgLev;
					chaninf.EvenTrgLev = pHSet[m].EvenTrgLev;
					memcpy(chaninf.HaTrgLev,pHSet[m].HaTrgLev,sizeof(chaninf.HaTrgLev) );
					chaninf.Ratio = pHSet[m].Ratio;
					chaninf.Rating = pHSet[m].Rating;
				}
				ChannelInfs[i].push_back(chaninf);
			}//end for

			if( pNorSet != NULL )
			{
				delete []pNorSet;
				pNorSet = NULL;
			}
			if( pHSet != NULL )
			{
				delete []pHSet;
				pHSet = NULL;
			}
		}//end 通道相关的

		/*<--读取元素定值信息 end<--*/
	}

	std::string sData;	//保存读出或解压出来的数据
	std::string sTmpData;
	char *pTmpData = NULL;
	char *pTmpData2 = NULL;
	Record rec;
	int rt = 0;
	int iDataLen = 0;
	int iDataLen2 = 0;
	bool bResult = true;

	if(FileHeader.CompressType == 1)//数据zib压缩
	{
		rec.InitZip();
	}

	while( 1 )
	{
		if( feof(pFile) )
			break;

		pTmpData = &(sData.operator [](0));
		unsigned int *pEleTypes = NULL;//本点包含的元素类型		
		unsigned int *pPointSize = NULL;//本点字节大小

		if( iDataLen > 8 )
		{
			pEleTypes = (unsigned int *)pTmpData;//本点包含的元素类型
			pTmpData += sizeof(unsigned int);
			pPointSize = (unsigned int *)pTmpData;//本点字节大小
			pTmpData += sizeof(unsigned int);			
		}

		//读出或解压出一个点全部的数据
		while( iDataLen<8 || pPointSize == NULL || iDataLen < *pPointSize )
		{
			sTmpData.resize(4096);
			iDataLen2 = 4096;
			pTmpData2 = &(sTmpData.operator [](0));
			if(FileHeader.CompressType == 1)//数据zib压缩
			{
				rt = rec.UnCompress_z(pFile,sTmpData,iDataLen2);
				if( rt == -1 )
					return false;
			}
			else
			{
				iDataLen2 = fread(pTmpData2,1,4096,pFile);
			}

			int iTmp = sData.size();
			sData.resize(iTmp+iDataLen2);
			pTmpData = &(sData.operator [](0));
			pTmpData += iTmp;

			pTmpData2 = &(sTmpData.operator [](0));
			memcpy(pTmpData,pTmpData2,iDataLen2);

			iTmp = sData.size();
			iDataLen += iDataLen2;

			if( iDataLen < 8 )
			{
				bResult = false;
				iDataLen = 0;
				break;
			}

			pTmpData = &(sData.operator [](0));
			pEleTypes = (unsigned int *)pTmpData;//本点包含的元素类型
			pTmpData += sizeof(unsigned int);
			pPointSize = (unsigned int *)pTmpData;//本点字节大小
			pTmpData += sizeof(unsigned int);

			if( (iDataLen < *pPointSize) && (rt==0 || iDataLen2<=0) )
			{
				bResult = false;
				iDataLen = 0;
				break;
			}

		}//end 读出或解压出一个点全部的数据

		if( !bResult)
		{
			break;
		}

		int iTime = 0;
		memcpy(&iTime,pTmpData,sizeof(int));//得到时间
		pTmpData += sizeof(int);

		//循环处理当前点各数据元素数据
		for(int i=0;i<FileHeader.DataEleNum;i++)
		{
			/*-->读取元素数据 begin-->*/
			unsigned int nEleType = pEleInfs[i*3];//元素类型
			unsigned int nEleDataLen = pEleInfs[i*3+2];//元素一个点的长度

			if( (*pEleTypes & nEleType) != nEleType ) //本点没有本元素的数据
			{
				continue;
			}
			
			//每个点有多少个值(除去4字节的时间)，每个值都是float型
			int iValNum = nEleDataLen/sizeof(float);

			int iChaValNum = 0;//每个通道值个数
			int iChaNum  = 0;//通道值个数

			if( nEleType==1 )
			{
				iChaNum = iVolLineNum;
				iChaValNum =iValNum/iVolLineNum;//每条线路值个数
			}
			else if( nEleType==4 )
			{
				iChaNum = FileHeader.ActiveLNum;
				iChaValNum =iValNum/FileHeader.ActiveLNum;//每条线路值个数
			}
			else if( nEleType==32 || nEleType==64)//电压通道相关的
			{
				iChaNum = iVolChaNum;
				iChaValNum =iValNum/iVolChaNum;//每个电压通道值个数
			}
			else if( nEleType==128 || nEleType==256 )//电流通道相关的
			{
				iChaNum = iCurChaNum;
				iChaValNum =iValNum/iCurChaNum;//每个电压通道值个数
			}
			else
			{
				iChaNum = FileHeader.ActiveChNum;
				iChaValNum =iValNum/FileHeader.ActiveChNum;//每个通道值个数
			}

			//取数据
			PQData *pPqData = new PQData(iChaNum,iChaValNum);
			pPqData->ChannelNum = iChaNum;
			pPqData->isUseFloatTime = false;
			memcpy(&(pPqData->time),&iTime,sizeof(int));//得到时间
			memcpy(&(pPqData->ppValues[0][0]),pTmpData,nEleDataLen);//得到值

			if( !(dataLists.operator [](i))->AddElem(pPqData) )
			{
				bResult = false;
				break;
			}

			pTmpData = pTmpData + nEleDataLen;//下一元素
		}//end for(int i=0;i<FileHeader.DataEleNum;i++) 处理完当前点

		if( !bResult)
		{
			break;
		}

		iDataLen -= (*pPointSize+8);//减去数据长度及每点8字节的数据点信息
		sData.erase(0,(*pPointSize+8));

		int iTmp = sData.size();
		iTmp = 0;
	}//end while(iDataLen > 0 ) 处理完全部数据

	if(FileHeader.CompressType == 1)//数据zib压缩
	{
		rec.EndZip();
	}


	//数据写入PQDIF文件
	for(int i=0;i<FileHeader.DataEleNum;i++)
	{
		unsigned int nEleType = pEleInfs[i*3];//元素类型
		FileHeader.FileType = nEleType;
		CPQDataList *PQDataList = dataLists.at(i);
		bool bRT = SHPQ2PQDIF(&FileHeader,&ChannelInfs[i],PQDataList,pDFileHandle);
		ChannelInfs[i].clear();
		ClearData(*PQDataList);
		delete PQDataList;
		PQDataList = NULL;
	}
	dataLists.clear();

	if( pLineCfg != NULL )
	{
		delete []pLineCfg;
		pLineCfg = NULL;
	}
	if( pChannelCfg != NULL )
	{
		delete []pChannelCfg;
		pChannelCfg = NULL;
	}
	if( ChannelInfs != NULL )
	{
		delete []ChannelInfs;
		ChannelInfs = NULL;
	}
	if( pNorSet != NULL )
	{
		delete []pNorSet;
		pNorSet = NULL;
	}
	if( pHSet != NULL )
	{
		delete []pHSet;
		pHSet = NULL;
	}

	fclose(pFile);
	fclose(pDFileHandle);

	return true;
}

bool PQDIFFile::CreateFileHeader(SHPQDIF::PQFileHead* pFileHeader,FILE *pFile)
{
	if( pFile == NULL )
	{
		return false;
	}

	//压缩信息
	int iCompStyle = ID_COMP_STYLE_NONE;
	/*if( pFileHeader->CompressType != 0 )
		iCompStyle = ID_COMP_STYLE_RECORDLEVEL;*/

	//生成文件头
	if( !HeaderRecord.CreateContainerRecord(pFileHeader) )
		return false;

	//写文件头
	if( !HeaderRecord.WritePQDIFFile(pFile,tagContainer,iCompStyle,pFileHeader->CompressType,false) )
		return false;
}

//##ModelId=4C5B6A840011
bool PQDIFFile::SHPQ2PQDIF(SHPQDIF::PQFileHead* pFileHeader, vector<SHPQDIF::ChannelInf> *pChannelInfs, CPQDataList *pDataList, FILE *pFile)
{
	if(pFileHeader==NULL || pChannelInfs == NULL || pDataList==NULL || pFile==NULL )
		return false;

	//压缩信息
	int iCompStyle = ID_COMP_STYLE_NONE;
	pFileHeader->CompressType = 0 ;//不压缩
	if( pFileHeader->CompressType != 0 )
		iCompStyle = ID_COMP_STYLE_RECORDLEVEL;
	
	if( !ds.CreateDS(pFileHeader,pChannelInfs) )
		return false;

	if( !ds.WritePQDIFFile(pFile,tagRecDataSource,iCompStyle,pFileHeader->CompressType,false) )
		return false;

	if( !ms.CreateMS(0,pChannelInfs) )
		return false;

	if( !ms.WritePQDIFFile(pFile,tagRecMonitorSettings,iCompStyle,pFileHeader->CompressType,false) )
		return false;

	int iFromPos = 1;
	int iAddNum = 1000;//每个数据段加的数据点数

	do
	{
		if( !DataRecord.AddData(pFileHeader,pDataList,iFromPos,iAddNum) )
			return false;

		if( iAddNum < 1000 )
		{
			if( !DataRecord.WritePQDIFFile(pFile,tagRecObservation,iCompStyle,pFileHeader->CompressType,true) )
				return false;
			break;
		}
		else
		{
			iFromPos += iAddNum;
			if( !DataRecord.WritePQDIFFile(pFile,tagRecObservation,iCompStyle,pFileHeader->CompressType,false) )
				return false;
		}
	}while(1);

	return true;
}

void PQDIFFile::ClearData( CPQDataList& DataList )
{
	DataList.clear();
}

