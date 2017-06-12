
// DesktopClientDlg.h : header file
//

#pragma once

#include <afxsock.h>
#include "MiniJpegDec.h"
#include "TonyJpegDecoder.h"
#include "MyEchoSocket.h"

#include <algorithm>
#include <vector>

using namespace std;

// CDesktopClientDlg dialog
class CDesktopClientDlg : public CDialogEx
{
// Construction
public:
	CDesktopClientDlg(CWnd* pParent = NULL);	// standard constructor

// Dialog Data
	enum { IDD = 102 };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support

// Implementation
protected:
	HICON m_hIcon;

	// Generated message map functions
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();

	DECLARE_MESSAGE_MAP()

	

private:
	CMiniJpegDecoder m_MiniJpegDecoder;
	CTonyJpegDecoder m_TonyJpegDecoder;

	MyEchoSocket m_Listener;
	MyEchoSocket m_Receiver;

	CAsyncSocket m_UdpSocket;

	bool			m_Connected;
	bool			m_Received;
public:
	void OnReceive();
	void OnAccept();
	void OnClose();

	int getSubnetMask();

	vector <CString> mBroadcastList2;

	void OnTimer(UINT_PTR nIdEvent);

	WSAEVENT NewEvent;
};
