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

	//����ͷ
	PQFileHead *pTmpHeader = &g_FileHeader;
	if( !HeaderRecord.ParseFile(pFile,pTmpHeader) )
		return false;

	//����ͨ������
	if( !ds.ParseFile(pFile,&ChannelInfs,pTmpHeader,Units) )
		return false;

	//����ͨ����ֵ
	if( !ms.ParseFile(pFile,&ChannelInfs,pTmpHeader) )
		return false;

	//��������
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

	//д�ļ�
	FILE *pFile = fopen(pDestFile,"wb");
	if( pFile == NULL )
	{
		ClearData( PQDataList );
		ChannelInfs.clear();
		return false;
	}

	//�ļ�ͷ
	if( 1 != fwrite(&FileHeader,sizeof(PQFileHead),1,pFile ) )
	{
		ClearData( PQDataList );
		ChannelInfs.clear();
		return false;
	}

	//ͨ����Ϣ
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

	//д����
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
	
	unsigned int *pEleInfs=NULL;//����Ԫ����Ϣ
	struct SHPQDIF::ChaCfgInf *pLineCfg = NULL;//��·������Ϣ
	struct SHPQDIF::ChaCfgInf *pChannelCfg = NULL;//ͨ��������Ϣ

	int	iVolLineNum = 0;	//��ѹ��·��
	int	iCurLineNum = 0;	//������·��
	int	iVolChaNum = 0;		//��ѹͨ����
	int	iCurChaNum = 0;		//����ͨ����

	//���ļ�
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

	//���ļ�ͷ
	if( 1 != fread(&FileHeader,sizeof(FileHeader),1,pFile) || FileHeader.DataEleNum == 0)
	{
		fclose(pFile);
		return false;
	}

	//����PQDIF�ļ�ͷ
	if( !CreateFileHeader(&FileHeader,pDFileHandle))
		return false;

	//��ȡ����Ԫ����Ϣ
	pEleInfs = new unsigned int[FileHeader.DataEleNum*3];
	if( FileHeader.DataEleNum*3 != fread((void *)pEleInfs,sizeof(unsigned int),FileHeader.DataEleNum*3,pFile) )
	{
		fclose(pFile);
		return false;
	}
	int tt = ftell(pFile);

	//����·������Ϣ
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

	//��ͨ��������Ϣ
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

	long lElsSetPos = ftell(pFile);//����Ԫ�ض�ֵ��Ϣ��ƫ��λ��
	struct SHPQDIF::PQNormalSetInf *pNorSet = NULL;
	struct SHPQDIF::PQHSetInf *pHSet = NULL;

	size_t	nReadLen = 0;

	ChannelInfs = new vector<SHPQDIF::ChannelInf>[FileHeader.DataEleNum];

	//ѭ�����������Ԫ�ض�ֵ��Ϣ
	for(int i=0;i<FileHeader.DataEleNum;i++)
	{
		CPQDataList *PQDataList = new CPQDataList(0X78FFFFFF,false);
		dataLists.push_back(PQDataList);

		/*-->��ȡԪ�ض�ֵ��Ϣ-->*/

		//�ļ�ָ���Ȼص���ֵ��Ϣͷ
		fseek(pFile,lElsSetPos,SEEK_SET);

		//�ټ��㱾Ԫ�ض�ֵ���ݵ�ƫ��
		long lSetOffset = 0;
		for(int j=0;j<i;j++)
		{
			lSetOffset += pEleInfs[j*3 +1];
		}

		//�ļ�ָ��ָ������Ԫ�صĶ�ֵ��Ϣ
		fseek(pFile,lSetOffset,SEEK_CUR);

		tt = ftell(pFile);

		//��ȡ��ֵ
		unsigned int nEleType = pEleInfs[i*3];//Ԫ������
		int iSetInfNum = 0;
		int iCount = 0;

		if( nEleType==1 || nEleType==4 )//��·��ص�
		{
			if( pLineCfg == NULL )
				return false;

			if( pEleInfs[i*3+1] > 0 ) //�ж�ֵ��Ϣ
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
				if( nEleType == 1 && pLineCfg[m].ChannelType !=1)//Ƶ�ʣ�ֻȡĸ����Ϣ
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
		}//end ��·��ص�
		else //ͨ����ص�
		{
			if( pChannelCfg == NULL )
				return false;

			if( pEleInfs[i*3+1] > 0 ) //�ж�ֵ��Ϣ
			{
				if( nEleType != 8 )//��г��
				{
					iSetInfNum = pEleInfs[i*3+1]/sizeof(SHPQDIF::PQNormalSetInf);
					if( iSetInfNum * sizeof(SHPQDIF::PQNormalSetInf) != pEleInfs[i*3+1] )
					{
						fclose(pFile);
						return false;
					}

					if( nEleType == 64 )
					{
						//�����Ŷ�ʱ����ͳ�ʱ����Ķ�ֵ���������ʱ���䶨ֵ
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
				else//г��
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
				//�������������ǵ�ѹͨ������ѹ������ѹ�������ݣ�������ȫ��ͨ����
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
		}//end ͨ����ص�

		/*<--��ȡԪ�ض�ֵ��Ϣ end<--*/
	}

	std::string sData;	//����������ѹ����������
	std::string sTmpData;
	char *pTmpData = NULL;
	char *pTmpData2 = NULL;
	Record rec;
	int rt = 0;
	int iDataLen = 0;
	int iDataLen2 = 0;
	bool bResult = true;

	if(FileHeader.CompressType == 1)//����zibѹ��
	{
		rec.InitZip();
	}

	while( 1 )
	{
		if( feof(pFile) )
			break;

		pTmpData = &(sData.operator [](0));
		unsigned int *pEleTypes = NULL;//���������Ԫ������		
		unsigned int *pPointSize = NULL;//�����ֽڴ�С

		if( iDataLen > 8 )
		{
			pEleTypes = (unsigned int *)pTmpData;//���������Ԫ������
			pTmpData += sizeof(unsigned int);
			pPointSize = (unsigned int *)pTmpData;//�����ֽڴ�С
			pTmpData += sizeof(unsigned int);			
		}

		//�������ѹ��һ����ȫ��������
		while( iDataLen<8 || pPointSize == NULL || iDataLen < *pPointSize )
		{
			sTmpData.resize(4096);
			iDataLen2 = 4096;
			pTmpData2 = &(sTmpData.operator [](0));
			if(FileHeader.CompressType == 1)//����zibѹ��
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
			pEleTypes = (unsigned int *)pTmpData;//���������Ԫ������
			pTmpData += sizeof(unsigned int);
			pPointSize = (unsigned int *)pTmpData;//�����ֽڴ�С
			pTmpData += sizeof(unsigned int);

			if( (iDataLen < *pPointSize) && (rt==0 || iDataLen2<=0) )
			{
				bResult = false;
				iDataLen = 0;
				break;
			}

		}//end �������ѹ��һ����ȫ��������

		if( !bResult)
		{
			break;
		}

		int iTime = 0;
		memcpy(&iTime,pTmpData,sizeof(int));//�õ�ʱ��
		pTmpData += sizeof(int);

		//ѭ������ǰ�������Ԫ������
		for(int i=0;i<FileHeader.DataEleNum;i++)
		{
			/*-->��ȡԪ������ begin-->*/
			unsigned int nEleType = pEleInfs[i*3];//Ԫ������
			unsigned int nEleDataLen = pEleInfs[i*3+2];//Ԫ��һ����ĳ���

			if( (*pEleTypes & nEleType) != nEleType ) //����û�б�Ԫ�ص�����
			{
				continue;
			}
			
			//ÿ�����ж��ٸ�ֵ(��ȥ4�ֽڵ�ʱ��)��ÿ��ֵ����float��
			int iValNum = nEleDataLen/sizeof(float);

			int iChaValNum = 0;//ÿ��ͨ��ֵ����
			int iChaNum  = 0;//ͨ��ֵ����

			if( nEleType==1 )
			{
				iChaNum = iVolLineNum;
				iChaValNum =iValNum/iVolLineNum;//ÿ����·ֵ����
			}
			else if( nEleType==4 )
			{
				iChaNum = FileHeader.ActiveLNum;
				iChaValNum =iValNum/FileHeader.ActiveLNum;//ÿ����·ֵ����
			}
			else if( nEleType==32 || nEleType==64)//��ѹͨ����ص�
			{
				iChaNum = iVolChaNum;
				iChaValNum =iValNum/iVolChaNum;//ÿ����ѹͨ��ֵ����
			}
			else if( nEleType==128 || nEleType==256 )//����ͨ����ص�
			{
				iChaNum = iCurChaNum;
				iChaValNum =iValNum/iCurChaNum;//ÿ����ѹͨ��ֵ����
			}
			else
			{
				iChaNum = FileHeader.ActiveChNum;
				iChaValNum =iValNum/FileHeader.ActiveChNum;//ÿ��ͨ��ֵ����
			}

			//ȡ����
			PQData *pPqData = new PQData(iChaNum,iChaValNum);
			pPqData->ChannelNum = iChaNum;
			pPqData->isUseFloatTime = false;
			memcpy(&(pPqData->time),&iTime,sizeof(int));//�õ�ʱ��
			memcpy(&(pPqData->ppValues[0][0]),pTmpData,nEleDataLen);//�õ�ֵ

			if( !(dataLists.operator [](i))->AddElem(pPqData) )
			{
				bResult = false;
				break;
			}

			pTmpData = pTmpData + nEleDataLen;//��һԪ��
		}//end for(int i=0;i<FileHeader.DataEleNum;i++) �����굱ǰ��

		if( !bResult)
		{
			break;
		}

		iDataLen -= (*pPointSize+8);//��ȥ���ݳ��ȼ�ÿ��8�ֽڵ����ݵ���Ϣ
		sData.erase(0,(*pPointSize+8));

		int iTmp = sData.size();
		iTmp = 0;
	}//end while(iDataLen > 0 ) ������ȫ������

	if(FileHeader.CompressType == 1)//����zibѹ��
	{
		rec.EndZip();
	}


	//����д��PQDIF�ļ�
	for(int i=0;i<FileHeader.DataEleNum;i++)
	{
		unsigned int nEleType = pEleInfs[i*3];//Ԫ������
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

	//ѹ����Ϣ
	int iCompStyle = ID_COMP_STYLE_NONE;
	/*if( pFileHeader->CompressType != 0 )
		iCompStyle = ID_COMP_STYLE_RECORDLEVEL;*/

	//�����ļ�ͷ
	if( !HeaderRecord.CreateContainerRecord(pFileHeader) )
		return false;

	//д�ļ�ͷ
	if( !HeaderRecord.WritePQDIFFile(pFile,tagContainer,iCompStyle,pFileHeader->CompressType,false) )
		return false;
}

//##ModelId=4C5B6A840011
bool PQDIFFile::SHPQ2PQDIF(SHPQDIF::PQFileHead* pFileHeader, vector<SHPQDIF::ChannelInf> *pChannelInfs, CPQDataList *pDataList, FILE *pFile)
{
	if(pFileHeader==NULL || pChannelInfs == NULL || pDataList==NULL || pFile==NULL )
		return false;

	//ѹ����Ϣ
	int iCompStyle = ID_COMP_STYLE_NONE;
	pFileHeader->CompressType = 0 ;//��ѹ��
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
	int iAddNum = 1000;//ÿ�����ݶμӵ����ݵ���

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

