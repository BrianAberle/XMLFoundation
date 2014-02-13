#include "GHTTPMultiPartPOST.h"


class LowLevelStaticController : public DataController
{
	int m_nBytesIn;
	int m_BytesExpected;
	int m_fd;
	char *m_Buf1;
public:
	LowLevelStaticController(int nBytesIn, int nBytesExpected, int fd, char *Buf1);

	int GetTotalBytesIn(){return m_nBytesIn;}
	virtual int GetExpectedDataSize(){return m_BytesExpected;}
	virtual char *GetBuffer1(){return m_Buf1;};
	virtual void Disconnect();

	// returns 1 on success, -1 on error, 0 when no more data, pnMoreBytes will be size of the the data found
	virtual int NextDataChunk(int *pnMoreBytes, char *pDest, int nMax, int nSecondsTimeout, int nMicrosecondsTimeout);
	virtual int NextDataChunk(int *pnMoreBytes);

	~LowLevelStaticController(){};
};