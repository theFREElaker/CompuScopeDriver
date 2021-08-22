#include "csSaveFile.h"



void csSaveFile(CGageSystem	*m_pGageSystem, CSHANDLE myHandle, QString& fileName, char **  pHeaderStart, char ** f, int32& i32totalSize, int32& length)
{
    Q_UNUSED (fileName);
    Q_UNUSED (myHandle);
	i32totalSize = 0;			// size of the SIGX_HEADER Header including all the channel descriptions and triggers configs
    unsigned char		u8padding;				// size padding to fit the header in the 256 bytes boundary
	char				* pHeader;				//the buffer used to save all the header info 
	//char				* pHeaderStart;			// pointer at the start of the header buffer
	SIGX_HEADER			* pSigxHeader;			//SIGX header structure
	int32				i32res = 0;				// testing the return of the API functions used
    ARRAY_BOARDINFO		BoardInfo;// = { 0 };		// Boardinfo structure the populate the header buffer
	//CGageSystem			*m_pGageSystem;
	//CsTestQt m;
	//*m_pGageSystem=*m.GetGageSystem();
	//m_pGageSystem->Initialize();
	//m_pGageSystem = CsTestQt(0,0)->m_pGageSystem;
	//CsTestQt::

	i32totalSize = sizeof(SIGX_HEADER) + (m_pGageSystem->getSystemInfo().u32ChannelCount * sizeof(CHANNEL_DESC)) + (m_pGageSystem->getSystemInfo().u32TriggerMachineCount * sizeof(CSTRIGGERCONFIG));
	u8padding = i32totalSize % 256;
	if (u8padding != 0)
	{
		i32totalSize = i32totalSize - u8padding + 256;
	}

    #ifdef Q_OS_WIN
        pHeader = (char *)VirtualAlloc(NULL, i32totalSize, MEM_COMMIT, PAGE_READWRITE);
    #else
        pHeader = (char *)malloc(i32totalSize);
    #endif

	*(pHeaderStart) = pHeader;

	pSigxHeader = (SIGX_HEADER *)pHeader;
	//---------------------------------------------------------------------------------------------------------------------------------------------
	pSigxHeader->u32Size = sizeof(SIGX_HEADER);
	pSigxHeader->u32Version = 1.1;
	pSigxHeader->BoardInfo.u32Size = sizeof(CSBOARDINFO);
    //pSigxHeader->szDescription[256];								// "User Comments !!";
	pSigxHeader->CsSystemInfo.u32Size = sizeof(CSSYSTEMINFO);

	i32res = CsGetSystemInfo(m_pGageSystem->getSystemHandle(), &pSigxHeader->CsSystemInfo);
	if (CS_FAILED(i32res))
	{
		//Error();
		return;
	}

	BoardInfo.u32BoardCount = 1;
	BoardInfo.aBoardInfo[0].u32Size = sizeof(CSBOARDINFO);
	BoardInfo.aBoardInfo[0].u32BoardIndex = 1;					//we are using one board for the moment

	i32res = CsDrvGetBoardsInfo(m_pGageSystem->getSystemHandle(), &BoardInfo);
	if (CS_FAILED(i32res))
	{
		//Error();
		return;
	}

	pSigxHeader->BoardInfo = BoardInfo.aBoardInfo[0];
	pSigxHeader->CardIndex = pSigxHeader->BoardInfo.u32BoardIndex;
	pSigxHeader->NbOfChannels = pSigxHeader->CsSystemInfo.u32ChannelCount;
	pSigxHeader->NbOfTriggers = pSigxHeader->CsSystemInfo.u32TriggerMachineCount;

	i32res = CsGet(m_pGageSystem->getSystemHandle(), 0, CS_SEGMENTTAIL_SIZE_BYTES, &pSigxHeader->TailSize);
	if (CS_FAILED(i32res))
	{
		//Error();
		return;
	}

	pSigxHeader->CsAcqCfg.u32Size = sizeof(CSACQUISITIONCONFIG);
	i32res = CsGet(m_pGageSystem->getSystemHandle(), CS_ACQUISITION, CS_ACQUISITION_CONFIGURATION, &pSigxHeader->CsAcqCfg);
	if (CS_FAILED(i32res))
	{
		//Error();
		return;
	}

	pHeader = pHeader + sizeof(SIGX_HEADER);

	//---------------------------------------------------------------------------------------------------------------------------------------------

    for (unsigned int i = 0; i < m_pGageSystem->getSystemInfo().u32ChannelCount; i++)
	{
		CHANNEL_DESC *pChannelDesc;
		pChannelDesc = (CHANNEL_DESC *)pHeader;
		pChannelDesc->CsChannelCfg.u32Size = sizeof(CSCHANNELCONFIG);
		pChannelDesc->CsChannelCfg.u32ChannelIndex = i + 1;
        //pChannelDesc->szChannelLabel; //dont know what to write !!
		i32res = CsGet(m_pGageSystem->getSystemHandle(), CS_CHANNEL, CS_ACQUISITION_CONFIGURATION, &pChannelDesc->CsChannelCfg);


		pHeader = pHeader + sizeof(CHANNEL_DESC);

		if (CS_FAILED(i32res))
		{
			//Error();
			return;
		}
	}

	//return;

	//---------------------------------------------------------------------------------------------------------------------------------------------

    for (unsigned int i = 0; i < m_pGageSystem->getSystemInfo().u32TriggerMachineCount; i++)
	{
		CSTRIGGERCONFIG * pTriggerConf = (CSTRIGGERCONFIG *)pHeader;
		pTriggerConf->u32Size = sizeof(CSTRIGGERCONFIG);
		pTriggerConf->u32TriggerIndex = i + 1;
		i32res = CsGet(m_pGageSystem->getSystemHandle(), CS_TRIGGER, CS_ACQUISITION_CONFIGURATION, pTriggerConf);
		pHeader = pHeader + sizeof(CSTRIGGERCONFIG);

		if (CS_FAILED(i32res))
		{
			//Error();
			return;
		}
	}
	//--------------------------------------------------------------------------------------------------------------------------------
	//data transfere will be using pLastMem pointer and Interleaved mode. 

    IN_PARAMS_TRANSFERDATA inParams;// = { 0 };
    OUT_PARAMS_TRANSFERDATA outParams;// = { 0 };
	length = 0;
	length = (m_pGageSystem->getAcquisitionConfig().i64Depth + m_pGageSystem->getAcquisitionConfig().i64TriggerHoldoff + 64)*m_pGageSystem->getAcquisitionConfig().u32Mode;//*m_pGageSystem->getAcquisitionConfig().u32SegmentCount; //(m_pGageSystem->getAcquisitionConfig().i64Depth + m_pGageSystem->getAcquisitionConfig().i64TriggerHoldoff + 64)*m_pGageSystem->getAcquisitionConfig().u32SegmentCount;

    #ifdef Q_OS_WIN
        char * pLastMem = (char *)VirtualAlloc(NULL, length, MEM_COMMIT, PAGE_READWRITE);
    #else
        char *pLastMem = (char *)malloc(length);
    #endif

	*(f) = pLastMem;

	m_pGageSystem->getAcquisitionConfig().u32Mode;
	inParams.u16Channel = 1;
	inParams.u32Mode = TxMODE_DATA_INTERLEAVED;

	int ExpectedStartAddr;
	ExpectedStartAddr = m_pGageSystem->getAcquisitionConfig().i64TriggerHoldoff > 0 ? -m_pGageSystem->getAcquisitionConfig().i64TriggerHoldoff : m_pGageSystem->getAcquisitionConfig().i64TriggerDelay;
	inParams.i64StartAddress = ExpectedStartAddr;
	inParams.pDataBuffer = (void *)pLastMem;
	inParams.i64Length = m_pGageSystem->getAcquisitionConfig().i64Depth + m_pGageSystem->getAcquisitionConfig().i64TriggerHoldoff + 64;
	inParams.hNotifyEvent = NULL;

    for (unsigned int j = 1; j <= m_pGageSystem->getAcquisitionConfig().u32SegmentCount; j++)
	{
		inParams.u32Segment = j;

		i32res = CsTransfer(m_pGageSystem->getSystemHandle(), &inParams, &outParams);
		pLastMem = pLastMem + outParams.i64ActualLength + 64;
		if (CS_FAILED(i32res))
		{
			//Error();
			return;
		}

	}


	//---------------------------------------------------------------------------------------------------------------------------------------------


}

