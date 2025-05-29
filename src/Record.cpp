#include "Record.h"
#include <assert.h>
#include <time.h>

#ifdef UNIT_TEST
//#pragma comment( lib, "D:/work/��Ʒ����/SHDFR_PQ��������/������/SRC/Client/Deployment/zlib.lib" )
#endif

#define CHUNK 4096 //16384

#ifndef RECORD_CPP_INCLUDED
#define RECORD_CPP_INCLUDED

PQFileHead	g_FileHeader={0};
int StoragMeth[2] = {ID_SERIES_METHOD_VALUES,ID_SERIES_METHOD_VALUES};

Record::Record()
{
	m_pMemObjList = NULL;
	m_pMemObjTail = NULL;
	m_nTotalMemSize = 0;
}

Record::~Record()
{
}

//##ModelId=4C7C80FF0056
bool Record::WritePQDIFFile(FILE *pFile,const GUID& RecordType,UINT4 styleCompression, UINT4 algCompression, bool IsLastRecord)
{
	if( pFile == NULL || m_pMemObjList == NULL)
		return false;

	const int L_BUFLEN = 4096;
	char *pInBuf = new char[L_BUFLEN];
	char *pOutBuf = new char[L_BUFLEN];
	memset(pInBuf,0,L_BUFLEN);
	memset(pOutBuf,0,L_BUFLEN);

	unsigned int nCheckSum = 0;
	unsigned int nTmpCheckSum = 0;
	unsigned int nOutBufLen = L_BUFLEN;
	unsigned int nRemainData = 0;
	unsigned int nRemainMem = L_BUFLEN;
	unsigned int nCpyLen = 0;

	c_record_mainheader hdrRecord = {0};
	long	lHeaderPos = ftell(pFile);	//��¼��recordͷ���ļ��е�λ��
	fseek(pFile,sizeof(hdrRecord),SEEK_CUR); //ƫ�Ƽ�¼ͷ

	MemNode* pMemTail = m_pMemObjList;
	nRemainData = pMemTail->length;
	MemNode* pTmpNode = NULL;
	m_pMemObjList = NULL;//����б�
	m_pMemObjTail = NULL;

	//�����¼������
	while( pMemTail != NULL )
	{
		//pInBuf���������ٽ��д���
		while( nRemainMem > 0 )
		{
			nCpyLen = (nRemainMem > nRemainData)? nRemainData:nRemainMem;
			memcpy(&pInBuf[L_BUFLEN-nRemainMem],
				&((char*)pMemTail->obj)[pMemTail->length-nRemainData],nCpyLen);

			nRemainMem -= nCpyLen;
			nRemainData -= nCpyLen;

			//���ڴ���������ѱ������꣬ȡ��һ������
			if( nRemainData == 0 )
			{
				m_nTotalMemSize -= pMemTail->length;
				pTmpNode = pMemTail;
				pMemTail = pMemTail->pNext;

				delete pTmpNode->obj;
				pTmpNode->obj = NULL;
				delete pTmpNode;
				pTmpNode = NULL;

				if( pMemTail )
					nRemainData = pMemTail->length;
				else
					break;
			}
		}//end while( nRemainMem > 0 )

		if( styleCompression == ID_COMP_STYLE_RECORDLEVEL && algCompression == ID_COMP_ALG_ZLIB )
		{
			//ѹ��
			if( Z_OK != Compress_z(pInBuf,L_BUFLEN-nRemainMem,pOutBuf,nOutBufLen,nTmpCheckSum) )
			{
				delete []pInBuf;
				delete []pOutBuf;
				return false;
			}
			nCheckSum += nTmpCheckSum;
		}
		else
		{
			//��ѹ��
			nOutBufLen = L_BUFLEN-nRemainMem;
			memcpy(pOutBuf,pInBuf,nOutBufLen);
		}

		hdrRecord.sizeData += nOutBufLen;

		if( nOutBufLen != fwrite(pOutBuf,sizeof(char),nOutBufLen,pFile) )
		{
			delete []pInBuf;
			delete []pOutBuf;
			return false;
		}

		nRemainMem = L_BUFLEN;
	}//end �����¼������

	//�����¼ͷ
	if( IsLastRecord )//���һ����¼
	{
		hdrRecord.linkNextRecord = 0;
	}
	else
	{
		hdrRecord.linkNextRecord = ftell(pFile);//��һ��¼Ϊ��ǰ�ļ�λ��
	}
	hdrRecord.sizeHeader = sizeof(hdrRecord);
	hdrRecord.checksum = nCheckSum;
	memcpy(&hdrRecord.guidRecordSignature,&guidRecordSignaturePQDIF,sizeof(hdrRecord.guidRecordSignature));
	memcpy(&hdrRecord.tagRecordType,&RecordType,sizeof(RecordType));

	//д��¼ͷ
	fseek(pFile,lHeaderPos,SEEK_SET); //�ļ�λ���Ƶ��ļ�ͷ
	if( 1 != fwrite(&hdrRecord,sizeof(hdrRecord),1,pFile) )
	{
		delete []pInBuf;
		delete []pOutBuf;
		return false;
	}
	fseek(pFile,hdrRecord.linkNextRecord,SEEK_SET); //�ļ�λ���Ƶ���¼β

	m_nTotalMemSize = 0;//���ݴ�����

	delete []pInBuf;
	delete []pOutBuf;
	return true;
}

//##ModelId=4C7CA96E00E7
void Record::ReInit()
{
}

//##ModelId=4C5A8671004C
int Record::ParseOneRecord(FILE* pFile, c_record_mainheader& hdrRecord, UINT4 styleCompression, UINT4 algCompression, bool bHeaderHasRead)
{
	bool        status = false;
    size_t      countRead;
    UINT4       checksumBlock;
    SIZE4       sizeDecompressed = 0;
	char		*pBodyData = NULL;
	vector<UN_COMP_SEG>	SegList;
	int			iRet = 0;
	unsigned int	nBodySize = 0;

	if( pFile == NULL )
		return -1;

	if( !bHeaderHasRead )
	{
		countRead = fread( &hdrRecord, sizeof( hdrRecord ), 1, pFile );
		if ( feof( pFile ) || countRead != 1 )
		{
			return -1;
		}
	}

	if ( hdrRecord.sizeHeader != sizeof( hdrRecord ) )
	{
		return -1;
	}
	if( ! PQDIF_IsEqualGUID( hdrRecord.guidRecordSignature, guidRecordSignaturePQDIF ) )
	{
		return -1;
	}

	//��ȡ������¼��body
	pBodyData = new char[hdrRecord.sizeData];
	if( pBodyData == NULL )
		return -1;

	countRead = fread( pBodyData, 1, hdrRecord.sizeData, pFile );
	if( hdrRecord.sizeData != countRead )
	{
		delete []pBodyData;
		return -1;
	}
	nBodySize = countRead;

	//ѹ����ʽ
	if( styleCompression == ID_COMP_STYLE_RECORDLEVEL )//��¼ѹ��
	{
		switch(algCompression)//ѹ���㷨
		{
		case ID_COMP_ALG_ZLIB:	//zlibѹ���㷨
			iRet = UnCompress_z(pBodyData,hdrRecord.sizeData,SegList);
			delete []pBodyData;
			pBodyData = NULL;

			nBodySize = 0;
			if( iRet==Z_OK )
			{
				//���������ܳ���
				vector<UN_COMP_SEG>::iterator it = SegList.begin();
				for(;it != SegList.end();it++)
				{
					nBodySize += (*it).nDataLen;
				}
				pBodyData = new char[nBodySize];
				if( pBodyData == NULL )
					return -1;

				//��������
				char *pTmpData = pBodyData;
				it = SegList.begin();
				for(;it != SegList.end();it++)
				{
					memcpy(pTmpData,(*it).pData,(*it).nDataLen);
					pTmpData += (*it).nDataLen;
					delete []((*it).pData);
				}
				SegList.clear();
			}
			else//��ѹʧ��
			{
				vector<UN_COMP_SEG>::iterator it = SegList.begin();
				for(;it != SegList.end();it++)
				{
					delete []((*it).pData);
				}
				SegList.clear();
				return -1;
			}//end ��ѹʧ��
		}//end switch
	}//end ��¼ѹ��
	else if( styleCompression == ID_COMP_STYLE_TOTALFILE )
	{
		//1.5�汾�󣬲�֧�������ļ���ѹ����ʽ
		delete []pBodyData;
		return -1;
	}

	//��������
	if( ParseCollection(pBodyData,nBodySize,0) )
	{
		delete []pBodyData;
		return 0;
	}

	delete []pBodyData;
	return -1;
}

//##ModelId=4C5A8671004E
bool Record::ParseCollection(char* pRecordBody, SIZE4 sizeOfBody, SIZE4 nOffSet)
{
	if( pRecordBody == NULL || sizeof(c_collection) > (sizeOfBody-nOffSet) )
		return false;

	SIZE4	lBodyRemainLen = sizeOfBody - nOffSet - sizeof(c_collection);
	c_collection collection = {0};
	c_vector        vector = {0};
	c_collection_element *  arrayElements;
	bool	accepted = false;
	long	size = 0;
	long	sizeVerify = 0;
	char	*pdata = NULL;

	char *pCurrentBodyData = pRecordBody +nOffSet;//ָ��ǰҪ���������

	memcpy(&collection,pCurrentBodyData,sizeof(c_collection) );
	pCurrentBodyData += sizeof(c_collection);
	nOffSet += sizeof(c_collection);

	if ((collection.count < 0L) || (collection.count > 100000L))
	{
		printf( "ERROR:Parsing of this collection aborted.  Invalid collection size count: %lX (%ld)\n", collection.count);
		return false;
	}

	if( lBodyRemainLen < sizeof(c_collection_element)*collection.count )
		return false;

	//copy collection element array
	arrayElements = new c_collection_element[ collection.count ];
	if( arrayElements == NULL )
		return false;

	memcpy(arrayElements,pCurrentBodyData,sizeof(c_collection_element)*collection.count );
	pCurrentBodyData += sizeof(c_collection_element)*collection.count;
	nOffSet += sizeof(c_collection_element)*collection.count;
	lBodyRemainLen -= sizeof(c_collection_element)*collection.count;

	for(int idx = 0; idx < collection.count; idx++ )
	{
		if( ! arrayElements[ idx ].isEmbedded )//Ԫ�����ݷ���Ƕ��
		{
			if( arrayElements[ idx ].link.linkElement >= sizeOfBody )
			{
				printf( "******** A relative position (.linkElement) is past the end of the record.\n" );
				return false;
			}

			//Ԫ��������Body�ڵ�ƫ�Ƶ�ַ
			nOffSet = arrayElements[ idx ].link.linkElement;
			pCurrentBodyData = pRecordBody +nOffSet;
		}

		//�ж�Ԫ�ص�����
		switch( arrayElements[ idx ].typeElement )
		{
		case ID_ELEMENT_TYPE_COLLECTION: //��collection
			accepted = AcceptCollection( arrayElements[ idx ].tagElement, 1 );
			if( accepted )
			{
				//������һ��collection
				accepted = ParseCollection( pRecordBody, sizeOfBody, nOffSet );
			}
			break;
		case ID_ELEMENT_TYPE_SCALAR:	//��scalar
			//�õ������������ͳ���
			size = GetNumBytesOfType( arrayElements[ idx ].typePhysical );

			//���scalar������
			if( arrayElements[ idx ].isEmbedded )//��Ƕ������
			{
				pdata = (char *) arrayElements[ idx ].valueEmbedded;
			}
			else //����Ƕ������
			{
				//����4�ֽڵ����������򲹵�4�ֽ�������
				sizeVerify = padSizeTo4Bytes(  size );
				assert( sizeVerify == arrayElements[ idx ].link.sizeElement );

				if ( sizeVerify != arrayElements[ idx ].link.sizeElement )
				{
					printf( "******** link.sizeElement (%d) not equal to size of scalar padded to 4 byte multiple (%d).\n",
						arrayElements[ idx ].link.sizeElement, sizeVerify);
				}

				pdata = new char[ sizeVerify ];
				memcpy(pdata,pCurrentBodyData,sizeVerify);
			}

			accepted = AcceptScalar(arrayElements[ idx ].tagElement,0,arrayElements[ idx ].typePhysical,(void *) pdata);
			if( ! arrayElements[ idx ].isEmbedded )
			{
				delete [] pdata;
				pdata = NULL;
			}
			break;

		case ID_ELEMENT_TYPE_VECTOR:	//��vector
			//���vectorͷ
			memcpy( &vector, pCurrentBodyData,sizeof( vector ));
			pCurrentBodyData += sizeof( vector );
			size = GetNumBytesOfType( arrayElements[ idx ].typePhysical );

			//����4�ֽڵ����������򲹵�4�ֽ�������
			sizeVerify = padSizeTo4Bytes( sizeof( c_vector ) + ( size * vector.count ) );
			assert( sizeVerify == arrayElements[ idx ].link.sizeElement );

			if ( sizeVerify != arrayElements[ idx ].link.sizeElement )
			{
				printf( "******** link.sizeElement (%d) not equal to size of vector padded to 4 byte multiple (%d).\n",
					arrayElements[ idx ].link.sizeElement, sizeVerify);
			}
			pdata = new char[ size * vector.count ];
			if( pdata == NULL )
			{
				accepted = false;
				break;
			}
			memcpy(pdata,pCurrentBodyData,size * vector.count);
			accepted = AcceptVector(arrayElements[ idx ].tagElement,0,arrayElements[ idx ].typePhysical,
                        &vector,(void *) pdata);
			delete []pdata;
			pdata = NULL;
			break;
		}//end swtich

	}//end for(int idx = 0; idx < collection.count; idx++ )

	delete [] arrayElements;
    return accepted;
}

//##ModelId=4C71F37A012E
bool Record::AcceptCollection(const GUID& tag, int level)
{
	return false;
}

//##ModelId=4C7234DE001C
bool Record::AcceptScalar(const GUID& tag, int level, INT1 typePhysical, void* pdata)
{
	return false;
}

//##ModelId=4C72464D032C
bool Record::AcceptVector(const GUID& tag, int level, INT1 typePhysical, c_vector* pVector, void* pdata)
{
	return false;
}

bool Record::AddMemObjToList(void *pMem,unsigned int nMemLen)
{
	if( pMem == NULL )
		return false;

	MemNode *pNewNode = new MemNode();
	if( pNewNode == NULL)
		return false;

	//�������ڴ水˳�򱣴浽�ڴ��б���
	pNewNode->obj = pMem;
	pNewNode->length = nMemLen;
	pNewNode->pNext = NULL;

	if( m_pMemObjTail == NULL )
	{
		m_pMemObjList = pNewNode;
	}
	else
	{
		m_pMemObjTail->pNext = pNewNode;
	}
	m_pMemObjTail = pNewNode;

	return true;
}

//##ModelId=4C5A8671004A
c_collection_element *  Record::AddCollection(const int iElemCount)
{
	SIZE4 size = sizeof( c_collection ) + ( iElemCount * sizeof( c_collection_element ) );
    size = padSizeTo4Bytes( size );//���Ȳ���4�ֽ�������

	char *pNewMem = new char[size];
	if( pNewMem == NULL )
		return NULL;

	memset(pNewMem,0,size);
	c_collection *pColl = (c_collection*)pNewMem;
	pColl->count = iElemCount;

	if( !AddMemObjToList((void *)pNewMem,size) )
	{
		delete []pNewMem;
		return NULL;
	}
	m_nTotalMemSize += size;	//�ڴ���������

	c_collection_element *pCollElement = (c_collection_element*)( pNewMem+sizeof(c_collection) );

	return pCollElement;
}

//##ModelId=4C7DA141004E
c_collection_element *  Record::AddCollection(const int iElemCount, const GUID &tag, c_collection_element &ce)
{
	c_collection_element *pElement = NULL;

	ce.link.linkElement = m_nTotalMemSize;//����ǰһ���Ժ���Ϊ���������ݴ�ŵ�ַ

	pElement = AddCollection( iElemCount );	//Ϊ���������ݷ����ڴ�
	if( pElement )
	{
		//����Ԫ������
		ce.tagElement = tag;
		ce.typeElement = ID_ELEMENT_TYPE_COLLECTION;
		ce.typePhysical = 0;
		ce.isEmbedded = false;
		ce.reserved = 0;

		//���㼯�����ݴ�С
		SIZE4 size = sizeof( c_collection ) + ( iElemCount * sizeof( c_collection_element ) );
		size = padSizeTo4Bytes( size );//���Ȳ���4�ֽ�������

		ce.link.sizeElement = size;
	}

	return pElement;
}

//##ModelId=4C7B2AF100FA
void *Record::AddScalar(const char *pData, INT1 typePhysical, const GUID &tag, c_collection_element &ce)
{
	if( pData == NULL )
		return NULL;

	SIZE4 nDataSize = GetNumBytesOfType(typePhysical);

	ce.tagElement = tag;
	ce.typeElement = ID_ELEMENT_TYPE_SCALAR;
	ce.typePhysical = typePhysical;
	ce.reserved = 0;

	if( nDataSize <= 8 )//���ݷ���ce�ṹ��
	{
		ce.isEmbedded = true;
		memset(ce.valueEmbedded,0,sizeof(ce.valueEmbedded) );
		memcpy(&(ce.valueEmbedded),pData,nDataSize);

		return NULL;
	}
	else //���ݷ��ڽṹ��
	{
		//���Ȳ���4�ֽ�������
		SIZE4 size = padSizeTo4Bytes( nDataSize );

		char *pNewVal = new char[size];
		if( pNewVal == NULL)
			return NULL;

		memset(pNewVal,0,size);
		memcpy(pNewVal,pData,nDataSize);
		ce.isEmbedded = false;
		ce.link.linkElement = m_nTotalMemSize;//���ӵ�ַΪ��¼��ǰ�ڴ�����
		ce.link.sizeElement = size;
		
		//�������ڴ水˳�򱣴浽�ڴ��б���
		if( !AddMemObjToList((void *)pNewVal,size) )
		{
			delete []pNewVal;
			return NULL;
		}
		m_nTotalMemSize += size; //�ڴ���������

		return pNewVal;
	}
	return NULL;
}

//##ModelId=4C7B4A2801A3
void *Record::AddVector(const char *pData, const unsigned int nDataCount, INT1 typePhysical, const GUID &tag, c_collection_element &ce)
{
	if( pData == NULL)
		return NULL;

	SIZE4 lDataSize = sizeof(c_vector) + nDataCount * GetNumBytesOfType(typePhysical);//���ݳ���
	SIZE4 lSize = padSizeTo4Bytes( lDataSize );	//���Ȳ���4�ֽ�������

	char *pNewVal = new char[lSize];
	if( pNewVal == NULL )
		return NULL;

	memset(pNewVal,0,lSize);

	c_vector *pVector = (c_vector*)pNewVal;
	pVector->count = nDataCount;
	memcpy(pNewVal+sizeof(c_vector),pData,lDataSize-sizeof(c_vector));

	ce.tagElement = tag;
	ce.typeElement = ID_ELEMENT_TYPE_VECTOR;
	ce.typePhysical = typePhysical;
	ce.reserved = 0;
	ce.isEmbedded = false;
	ce.link.linkElement = m_nTotalMemSize;//���ӵ�ַΪ��¼��ǰ�ڴ�����
	ce.link.sizeElement = lSize;

	//�������ڴ水˳�򱣴浽�ڴ��б���
	if( !AddMemObjToList((void *)pNewVal,lSize) )
	{
		delete []pNewVal;
		return NULL;
	}
	m_nTotalMemSize += lSize; //�ڴ���������

	return pNewVal;
}

long Record::GetNumBytesOfType(long idType)
{
	long    rc = 1L;

	switch (idType)
	{
	case ID_PHYS_TYPE_BOOLEAN1:
	case ID_PHYS_TYPE_CHAR1:
	case ID_PHYS_TYPE_INTEGER1:
	case ID_PHYS_TYPE_UNS_INTEGER1:
		rc = 1;
		break;

	case ID_PHYS_TYPE_INTEGER2:
	case ID_PHYS_TYPE_UNS_INTEGER2:
		rc = 2;
		break;

	case ID_PHYS_TYPE_INTEGER4:
	case ID_PHYS_TYPE_UNS_INTEGER4:
	case ID_PHYS_TYPE_REAL4:
		rc = 4;
		break;

	case ID_PHYS_TYPE_REAL8:
	case ID_PHYS_TYPE_COMPLEX8:
		rc = 8;
		break;

	case ID_PHYS_TYPE_COMPLEX16:
		rc = 16;
		break;

	case ID_PHYS_TYPE_TIMESTAMPPQDIF:
		rc = sizeof( TIMESTAMPPQDIF );
		break;

	case ID_PHYS_TYPE_GUID:
		rc = sizeof( GUID );
		break;

	default:
		rc = 1;
		break;
	}
	return rc;
}

SIZE4 Record::padSizeTo4Bytes( SIZE4 sizeOrig )
{
	SIZE4   remainder;

	remainder = ( sizeOrig % 4 );

	//  Already multiple of 4?
	if( remainder == 0 )
		return sizeOrig;

	//  Pad it
	return sizeOrig + ( 4 - remainder );
}

int Record::Compress_z(const char *pSrc,unsigned int nSrcLen,char *pDest,unsigned int& nDestLen,unsigned int& nCheckSum)
{
	if( pSrc == NULL || pDest == NULL )
		return Z_ERRNO;

	int ret,flush;
    unsigned have;
    z_stream strm;
	char in[CHUNK] = {0};
	char out[CHUNK] = {0};
	unsigned int nOutLen = 0;
	unsigned int nRemainLen = nSrcLen;
	unsigned int nCpyLen = 0;

	nCheckSum = 0;

    /* allocate deflate state */
    strm.zalloc = Z_NULL;
    strm.zfree = Z_NULL;
    strm.opaque = Z_NULL;

	strm.data_type = Z_BINARY;
    ret = deflateInit(&strm,1);
    if (ret != Z_OK)
        return ret;

	nCheckSum = crc32(0L, Z_NULL, 0);

	while( nRemainLen > 0 )
	{
		nCpyLen = ( nRemainLen > CHUNK )? CHUNK:nRemainLen;
		memcpy(in,pSrc,nCpyLen);
		nRemainLen -= nCpyLen;
		pSrc += nCpyLen;

		strm.avail_in = nCpyLen;
		strm.total_in = strm.avail_in;
        strm.next_in = (unsigned char*)(&in[0]);

		flush = ( nRemainLen ==0 )?Z_FINISH:Z_NO_FLUSH;

		nCheckSum = crc32( nCheckSum, (const Bytef *)&in, nCpyLen );

		strm.avail_out = CHUNK;
		strm.next_out = (unsigned char*)(&out[0]);
		strm.total_out = 0;

		ret = deflate(&strm,flush);    /* no bad return value */
		have = CHUNK - strm.avail_out;

		if( (nOutLen+have) > nDestLen )
		{
			(void)deflateEnd(&strm);
			return Z_ERRNO;
		}

		memcpy(pDest,out,have);
		pDest += have;
		nOutLen += have;
	}

	nDestLen = nOutLen;

    /* clean up and return */
    (void)deflateEnd(&strm);
    return Z_OK;
}

int Record::UnCompress_z(const char *pSrc,unsigned int nSrcLen,vector<UN_COMP_SEG> &SegList)
{
	int ret;
    z_stream strm;
    char in[CHUNK];

	UN_COMP_SEG	seg;
	memset(&seg,0,sizeof(UN_COMP_SEG) );

	if( pSrc == NULL )
		return Z_ERRNO;

	unsigned int nRemainLen = nSrcLen;
	unsigned int nCpyLen = 0;

    /* allocate inflate state */
    strm.zalloc = Z_NULL;
    strm.zfree = Z_NULL;
    strm.opaque = Z_NULL;
    strm.avail_in = 0;
    strm.next_in = Z_NULL;
    ret = inflateInit(&strm);
    if (ret != Z_OK)
        return ret;

    /* decompress until deflate stream ends or end of file */
    while( nRemainLen > 0 )
	{
		nCpyLen = ( nRemainLen > CHUNK )? CHUNK:nRemainLen;
		memcpy(in,pSrc,nCpyLen);
		nRemainLen -= nCpyLen;
		pSrc += nCpyLen;

        strm.avail_in = nCpyLen;
        strm.next_in = (unsigned char*)(&in[0]);

        /* run inflate() on input until output buffer not full */
        do {
			seg.pData = new char[CHUNK];
			if( seg.pData == NULL )
				return Z_ERRNO;

            strm.avail_out = CHUNK;
            strm.next_out = (unsigned char*)(seg.pData);
            ret = inflate(&strm,Z_NO_FLUSH);
            assert(ret != Z_STREAM_ERROR);  /* state not clobbered */
            switch (ret)
			{
            case Z_NEED_DICT:
                ret = Z_DATA_ERROR;     /* and fall through */
            case Z_DATA_ERROR:
            case Z_MEM_ERROR:
                (void)inflateEnd(&strm);
				delete []seg.pData;
				seg.pData = NULL;
                return ret;
            }

            seg.nDataLen = CHUNK - strm.avail_out;
			SegList.push_back(seg);
			
		} while (strm.avail_out == 0);

	}

    /* clean up and return */
    (void)inflateEnd(&strm);
    return ret == Z_STREAM_END ? Z_OK : Z_DATA_ERROR;
}

int Record::InitZip()
{
	int ret;

    /* allocate inflate state */
    m_strm.zalloc = Z_NULL;
    m_strm.zfree = Z_NULL;
    m_strm.opaque = Z_NULL;
    m_strm.avail_in = 0;
    m_strm.next_in = Z_NULL;
    ret = inflateInit(&m_strm);
	return ret;
}

int Record::UnCompress_z(FILE *pSrcFile,std::string& sOutData,int& iDataLen)
{
	if( pSrcFile == NULL )
		return -1;

	char in[CHUNK];
	int ret;
	int iHasWriteLen = 0;

	sOutData.resize(CHUNK);

	m_strm.avail_in = fread(&in[0],1,CHUNK,pSrcFile);
	if (m_strm.avail_in==0)
	{
		return 0;
	}

	m_strm.next_in = (unsigned char*)(&in[0]);
	do {
		m_strm.avail_out = CHUNK;
		m_strm.next_out = (unsigned char*)( &(sOutData.operator [](iHasWriteLen)) );

		ret = inflate(&m_strm,Z_NO_FLUSH);
		switch (ret)
		{
		case Z_NEED_DICT:
		case Z_DATA_ERROR:
		case Z_MEM_ERROR:
			return -1;
		}
		iHasWriteLen += CHUNK - m_strm.avail_out;
		iDataLen = iHasWriteLen;
		sOutData.resize(iHasWriteLen+CHUNK);
	}while(m_strm.avail_out == 0);

	return 1;
}

int Record::EndZip()
{
	(void)inflateEnd(&m_strm);
	return 0;
}

int Record::GetRecordType( const GUID& tag)
{
	if( PQDIF_IsEqualGUID( tag, tagContainer ) )
	{
		return 1;
	}

	if( PQDIF_IsEqualGUID( tag, tagRecDataSource ) )
	{
		return 2;
	}

	if( PQDIF_IsEqualGUID( tag, tagRecMonitorSettings ) )
	{
		return 3;
	}

	if( PQDIF_IsEqualGUID( tag, tagRecObservation ) )
	{
		return 4;
	}

	return -1;
}

bool Record::PQTime2Tm( TIMESTAMPPQDIF *pPQTime,struct tm *pTmTime)
{
	if( pPQTime == NULL || pTmTime == NULL)
		return false;

    time_t      ttTrans;
    long        daysSince1970;	
	long    seconds;

	daysSince1970 = pPQTime->day - EXCEL_DAYCOUNT_ADJUST;
    ttTrans = ( daysSince1970 ) * SECONDS_PER_DAY;
    seconds = (long) pPQTime->sec;
    ttTrans += seconds;
    
    *pTmTime = *localtime( &ttTrans );

    return true;
}

bool Record::PQTime2Time_t( TIMESTAMPPQDIF *pPQTime,time_t & ttTime)
{
	if( pPQTime == NULL)
		return false;

    long    daysSince1970;
	long    seconds;

	daysSince1970 = pPQTime->day - EXCEL_DAYCOUNT_ADJUST;
    ttTime = ( daysSince1970 ) * SECONDS_PER_DAY;
    seconds = (long) pPQTime->sec;
    ttTime += seconds;

    return true;
}

bool Record::Time_t2PQTime( time_t & ttTime,TIMESTAMPPQDIF *pPQTime)
{
	if( pPQTime == NULL)
		return false;

	long    daysSince1970;

	daysSince1970 = ttTime/SECONDS_PER_DAY;
	pPQTime->day = daysSince1970 + EXCEL_DAYCOUNT_ADJUST;
	pPQTime->sec = ttTime%SECONDS_PER_DAY;

	return true;
}

#endif
