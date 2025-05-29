#ifndef CONTAINER_H_HEADER_INCLUDED_B2ECCDE9
#define CONTAINER_H_HEADER_INCLUDED_B2ECCDE9
#include "Record.h"

//##ModelId=4C5A86710054
class Container : public Record
{
protected:

  public:
	Container();

	virtual ~Container();

    //##ModelId=4C5A86710055
    bool CreateContainerRecord(const PQFileHead* pFileHeader);

    //##ModelId=4C5A86710057
    bool ParseFile(FILE* pFile, PQFileHead* pFileHeader);

  protected:
    //##ModelId=4C73356600F4
    bool AcceptCollection(const GUID& tag, int level);

    //##ModelId=4C73356600F7
    bool AcceptScalar(const GUID& tag, int level, INT1 typePhysical, void* pdata);

    //##ModelId=4C7335660103
    bool AcceptVector(const GUID& tag, int level, INT1 typePhysical, c_vector* pVector, void* pdata);

    //##ModelId=4C7335660109
    int ParseOneRecord(FILE* pFile, c_record_mainheader& hdrRecord, UINT4 styleCompression, UINT4 algCompression, bool bHeaderHasRead);

};



#endif /* CONTAINER_H_HEADER_INCLUDED_B2ECCDE9 */
