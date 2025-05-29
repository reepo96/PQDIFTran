#include "PQDIFInterFace.h"
#include "PQDIFFile.h"

//##ModelId=4C5A867100B1
bool PQDIFInterFace::PQDIF2SHPQ(const char* pSrcFile, PQFileHead* pFileHeader, 
								vector<ChannelInf> **ChannelInfs, 
								vector<VAL_UNIT>  **Units,
								CPQDataList **pDataList)
{
	if( ChannelInfs == NULL || pDataList == NULL || pFileHeader == NULL 
		|| pSrcFile == NULL || Units == NULL)
		return false;

	PQDIFFile	PQFile;

	*ChannelInfs = new vector<SHPQDIF::ChannelInf>;
	*pDataList = new CPQDataList(0x78ffffff,false);
	*Units = new vector<VAL_UNIT>;
	return PQFile.PQDIF2SHPQ(pSrcFile,pFileHeader,**ChannelInfs,*Units,**pDataList);
}

//##ModelId=4C5A867100B4
bool PQDIFInterFace::PQDIF2SHPQ(const char* pSrcFile, const char* pDestFile)
{
	PQDIFFile	PQFile;
	return PQFile.PQDIF2SHPQ(pSrcFile,pDestFile);
}

//##ModelId=4C5A867100B7
bool PQDIFInterFace::SHPQ2PQDIF(const char* pSrcFile, const char* pDestFile)
{
	PQDIFFile	PQFile;
	return PQFile.SHPQ2PQDIF(pSrcFile,pDestFile);
}

//##ModelId=4C5A867100BA
bool PQDIFInterFace::SHPQ2PQDIF(SHPQDIF::PQFileHead* pFileHeader, vector<SHPQDIF::ChannelInf> *pChannelInfs, CPQDataList *pDataList, const char* pDestFile)
{
	PQDIFFile	PQFile;
	FILE *pDFileHandle = fopen(pDestFile,"wb");
	if( pDFileHandle == NULL )
	{
		return false;
	}
	//创建PQDIF文件头
	if( !PQFile.CreateFileHeader(pFileHeader,pDFileHandle))
	{
		fclose(pDFileHandle);
		return false;
	}

	bool bRT = PQFile.SHPQ2PQDIF(pFileHeader,pChannelInfs,pDataList,pDFileHandle);
	fclose(pDFileHandle);
	return bRT;
}

void PQDIFInterFace::ClearData( CPQDataList **DataList )
{
	if( DataList == NULL || *DataList == NULL )
		return;

	PQDIFFile	PQFile;
	PQFile.ClearData(**DataList);

	delete *DataList;
	*DataList = NULL;
}

void PQDIFInterFace::ClearChanInf( vector<SHPQDIF::ChannelInf> **ChannelInfs )
{
	if( ChannelInfs == NULL || *ChannelInfs == NULL )
		return;

	(*ChannelInfs)->clear();
	delete *ChannelInfs;
	*ChannelInfs = NULL;
}

void PQDIFInterFace::ClearUnits( vector<VAL_UNIT> **Units )
{
	if( Units == NULL || *Units == NULL )
		return;

	(*Units)->clear();
	delete *Units;
	*Units = NULL;
}

