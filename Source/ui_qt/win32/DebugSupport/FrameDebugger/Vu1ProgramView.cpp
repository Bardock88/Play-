#include "Vu1ProgramView.h"
#include "FrameDump.h"
#include "Ps2Const.h"
#include "win32/VerticalSplitter.h"
#include "win32/HorizontalSplitter.h"

#define WNDSTYLE (WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN)

enum
{
	WM_MACHINESTATECHANGE = WM_USER + 1,
	WM_RUNNINGSTATECHANGE
};

CVu1ProgramView::CVu1ProgramView(HWND parent, const RECT& rect, CVu1Vm& virtualMachine)
    : m_virtualMachine(virtualMachine)
    , m_vuMemPacketAddress(0)
{
	Create(0, Framework::Win32::CDefaultWndClass::GetName(), NULL, WNDSTYLE, rect, parent, NULL);
	SetClassPtr();

	virtualMachine.OnMachineStateChange.connect(std::bind(&CVu1ProgramView::OnMachineStateChange, this));
	virtualMachine.OnRunningStateChange.connect(std::bind(&CVu1ProgramView::OnRunningStateChange, this));

	m_mainSplitter = std::make_unique<Framework::Win32::CVerticalSplitter>(m_hWnd, GetClientRect());

	m_subSplitter = std::make_unique<Framework::Win32::CHorizontalSplitter>(*m_mainSplitter, GetClientRect());
	m_memoryTab = std::make_unique<CTabHost>(*m_mainSplitter, GetClientRect());

	m_memoryView = std::make_unique<CMemoryViewMIPS>(*m_memoryTab, GetClientRect(), m_virtualMachine, m_virtualMachine.GetVu1Context());
	m_packetView = std::make_unique<CGifPacketView>(*m_memoryTab, GetClientRect());

	m_memoryTab->InsertTab(_T("VU Memory"), m_memoryView.get());
	m_memoryTab->InsertTab(_T("Packet"), m_packetView.get());

	m_disAsm = std::make_unique<CDisAsmVu>(*m_subSplitter, GetClientRect(), m_virtualMachine, m_virtualMachine.GetVu1Context());

	m_regView = std::make_unique<CRegViewVU>(*m_subSplitter, GetClientRect(), m_virtualMachine.GetVu1Context());
	m_regView->Show(SW_SHOW);

	m_mainSplitter->SetChild(0, *m_subSplitter);
	m_mainSplitter->SetChild(1, *m_memoryTab);
	m_mainSplitter->SetMasterChild(1);
	m_mainSplitter->SetEdgePosition(0.65);

	m_subSplitter->SetChild(0, *m_disAsm);
	m_subSplitter->SetChild(1, *m_regView);
	m_subSplitter->SetMasterChild(1);
	m_subSplitter->SetEdgePosition(0.65);
}

void CVu1ProgramView::UpdateState(CGSHandler* gs, CGsPacketMetadata* metadata, DRAWINGKICK_INFO*)
{
#ifdef DEBUGGER_INCLUDED
	memcpy(m_virtualMachine.GetMicroMem1(), metadata->microMem1, PS2::MICROMEM1SIZE);
	memcpy(m_virtualMachine.GetVuMem1(), metadata->vuMem1, PS2::VUMEM1SIZE);
	memcpy(&m_virtualMachine.GetVu1Context()->m_State, &metadata->vu1State, sizeof(MIPSSTATE));
	m_virtualMachine.SetVpu1Top(metadata->vpu1Top);
	m_virtualMachine.SetVpu1Itop(metadata->vpu1Itop);
	m_vuMemPacketAddress = metadata->vuMemPacketAddress;
#endif
	OnMachineStateChangeMsg();
}

void CVu1ProgramView::StepVu1()
{
	m_virtualMachine.StepVu1();
}

long CVu1ProgramView::OnSize(unsigned int, unsigned int, unsigned int)
{
	m_mainSplitter->SetSizePosition(GetClientRect());
	return TRUE;
}

LRESULT CVu1ProgramView::OnWndProc(unsigned int nMsg, WPARAM wParam, LPARAM lParam)
{
	switch(nMsg)
	{
	case WM_MACHINESTATECHANGE:
		OnMachineStateChangeMsg();
		return FALSE;
		break;
	case WM_RUNNINGSTATECHANGE:
		OnRunningStateChangeMsg();
		return FALSE;
		break;
	}
	return CWindow::OnWndProc(nMsg, wParam, lParam);
}

void CVu1ProgramView::OnMachineStateChange()
{
	PostMessage(m_hWnd, WM_MACHINESTATECHANGE, 0, 0);
}

void CVu1ProgramView::OnRunningStateChange()
{
	PostMessage(m_hWnd, WM_RUNNINGSTATECHANGE, 0, 0);
}

void CVu1ProgramView::OnMachineStateChangeMsg()
{
	m_disAsm->HandleMachineStateChange();
	m_memoryView->HandleMachineStateChange();
	m_packetView->SetPacket(m_virtualMachine.GetVuMem1() + m_vuMemPacketAddress, PS2::VUMEM1SIZE - m_vuMemPacketAddress);
	m_packetView->Update();
	m_regView->Update();
}

void CVu1ProgramView::OnRunningStateChangeMsg()
{
	auto newState = m_virtualMachine.GetStatus();
	m_disAsm->HandleRunningStateChange(newState);
	m_memoryView->HandleRunningStateChange(newState);
}
