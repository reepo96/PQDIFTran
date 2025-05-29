#include "Container.h"
#include <time.h>

#ifdef UNIT_TEST
#include "Record.cpp"
#endif

Container::Container()
{
	memset(&g_FileHeader,0,sizeof(g_FileHeader) );
}

Container::~Container()
{
}

//##ModelId=4C5A86710055
bool Container::CreateContainerRecord(const PQFileHead* pFileHeader)
{
	long                    lCollItems = 6; //添加6个属性
	long                    lIdxItem = 0;
	c_collection_element	*pCollElem = NULL;
	
	if( pFileHeader == NULL )
		return false;

	//添加第一个collection
	pCollElem = AddCollection( lCollItems );
	if( pCollElem ) //填充collection元素
	{
		//填充版本信息（v1.5）
		UINT4 nVersion[4] = {0};
		nVersion[0] = 1;
		nVersion[1] = 5;
		AddVector((const char *)&nVersion[0],4,ID_PHYS_TYPE_UNS_INTEGER4,tagVersionInfo,pCollElem[lIdxItem++]);

		//填充文件名
		AddVector("SHPQFile.pqd",strlen("SHPQFile.pqd")+1,ID_PHYS_TYPE_CHAR1,tagFileName,pCollElem[lIdxItem++]);

		//创建时间
		time_t ttNow;
		TIMESTAMPPQDIF pqTime = {0};
		time( &ttNow );
		Time_t2PQTime(ttNow,&pqTime);
		AddScalar((const char *)&pqTime,ID_PHYS_TYPE_TIMESTAMPPQDIF,tagCreation,pCollElem[lIdxItem++]);

		//标题
		AddVector(pFileHeader->FileTitle,strlen(pFileHeader->FileTitle)+1,ID_PHYS_TYPE_CHAR1,tagTitle,pCollElem[lIdxItem++]);

		//压缩信息
		int iCompStyle = ID_COMP_STYLE_NONE;
		if( pFileHeader->CompressType != 0 )
			iCompStyle = ID_COMP_STYLE_RECORDLEVEL;

		AddScalar((const char *)&iCompStyle,ID_PHYS_TYPE_UNS_INTEGER4,tagCompressionStyleID,pCollElem[lIdxItem++]);
		AddScalar((const char *)&pFileHeader->CompressType,ID_PHYS_TYPE_UNS_INTEGER4,
			tagCompressionAlgorithmID,pCollElem[lIdxItem++]);		
	}

	return true;
}

//##ModelId=4C5A86710057
bool Container::ParseFile(FILE* pFile, PQFileHead* pFileHeader)
{
	if( pFile == NULL || pFileHeader == NULL )
		return false;

	c_record_mainheader	hdrRecord = {0};
	int iRet = ParseOneRecord(pFile,hdrRecord,ID_COMP_STYLE_NONE,ID_COMP_ALG_NONE,false);
	if( iRet != 0 )
	{
		return false;
	}

	memcpy(pFileHeader,&g_FileHeader,sizeof(PQFileHead));
	return true;
}

//##ModelId=4C73356600F4
bool Container::AcceptCollection(const GUID& tag, int level)
{
	return true;
}

//##ModelId=4C73356600F7
bool Container::AcceptScalar(const GUID& tag, int level, INT1 typePhysical, void* pdata)
{
	if( pdata == NULL )
		return false;

	if( PQDIF_IsEqualGUID( tag, tagCompressionAlgorithmID )) //压缩算法
	{
		if( ID_PHYS_TYPE_UNS_INTEGER4 != typePhysical )
			return false;
		g_FileHeader.CompressType = *((int*)pdata);
		return true;
	}

	return true;
}

//##ModelId=4C7335660103
bool Container::AcceptVector(const GUID& tag, int level, INT1 typePhysical, c_vector* pVector, void* pdata)
{
	if( pdata == NULL || pVector == NULL )
		return false;

	int iCpyLen = 0;

	if( PQDIF_IsEqualGUID( tag, tagVersionInfo )) //版本信息
	{
		if( pVector->count != 4 && ID_PHYS_TYPE_UNS_INTEGER4 != typePhysical )
			return false;

		int *pVal = (int*)pdata;
		g_FileHeader.Ver = *pVal<<8;
		pVal++;
		g_FileHeader.Ver += *pVal & 0X00FF;
		return true;
	}

	if( PQDIF_IsEqualGUID( tag, tagTitle )) //标题信息
	{
		if( pVector->count == 0 || ID_PHYS_TYPE_CHAR1 != typePhysical )
			return false;
		
		iCpyLen = ( pVector->count > sizeof(g_FileHeader.FileTitle)-1 )?
			sizeof(g_FileHeader.FileTitle)-1 : pVector->count;
		memcpy(g_FileHeader.FileTitle,pdata,iCpyLen);
		return true;
	}

	return true;
}

//##ModelId=4C7335660109
int Container::ParseOneRecord(FILE* pFile, c_record_mainheader& hdrRecord, UINT4 styleCompression, UINT4 algCompression, bool bHeaderHasRead)
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
	if(  iRecordType != 1 )
	{
		return iRecordType;//不是Container记录
	}

	//Container记录是不被压缩的
	return Record::ParseOneRecord(pFile,hdrRecord,ID_COMP_STYLE_NONE,ID_COMP_ALG_NONE,true);
}

