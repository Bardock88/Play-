#pragma once

#include "Vif.h"

class CGIF;
class CINTC;

class CVif1 : public CVif
{
public:
	CVif1(unsigned int, CVpu&, CGIF&, CINTC&, uint8*, uint8*);
	virtual ~CVif1();

	void Reset() override;
	void SaveState(Framework::CZipArchiveWriter&) override;
	void LoadState(Framework::CZipArchiveReader&) override;

	uint32 GetTOP() const override;

	uint32 ReceiveDMA(uint32, uint32, uint32, bool) override;
	
	void ResumeProcessing() override;
	void AbortProcessing() override;

private:
	void ExecuteCommand(StreamType&, CODE) override;

	void Cmd_DIRECT(StreamType&, CODE);
	void Cmd_UNPACK(StreamType&, CODE, uint32) override;

	void PrepareMicroProgram() override;

	void ThreadProc();

	CGIF& m_gif;

	uint32 m_BASE;
	uint32 m_OFST;
	uint32 m_TOP;
	uint32 m_TOPS;

	enum
	{
		QWORD_SIZE = 0x10,
	};

	uint8 m_directQwordBuffer[QWORD_SIZE];
	uint32 m_directQwordBufferIndex = 0;

	std::thread m_vifThread;

	std::vector<uint128> m_dmaBuffer;
	std::mutex m_ringBufferMutex;
	std::condition_variable m_hasDataCondVar;
	std::condition_variable m_consumedDataCondVar;
	static const uint32 g_dmaBufferSize = 0x10000;
	bool m_dmaBufferAborted = false;
	uint32 m_dmaBufferWritePos = 0;
	uint32 m_dmaBufferReadPos = 0;
	uint32 m_dmaBufferContentsSize = 0;
};
