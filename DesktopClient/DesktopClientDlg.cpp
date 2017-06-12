
// DesktopClientDlg.cpp : implementation file
//

#include "stdafx.h"
#include "DesktopClient.h"
#include "DesktopClientDlg.h"
#include "afxdialogex.h"
#include <IPHlpApi.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

// CAboutDlg dialog used for App About

#define MAX_WIDTH 1920
#define MAX_HEIGHT 1080
#define UDP_QUEUE_LENGTH 255
#define BUFFER_LENGTH 63*1024 + 14

byte pbBmp32[MAX_WIDTH * MAX_HEIGHT * 4];
byte pbBufferSwapper[MAX_WIDTH * 4];

unsigned char pbData[MAX_WIDTH * MAX_HEIGHT * 3];
unsigned char pbBmpData[MAX_WIDTH * MAX_HEIGHT * 3];
unsigned char buff[BUFFER_LENGTH];
unsigned char tmpbuf[2048];
unsigned char pbStart[] = "Start";
unsigned char pbEnd[] = "End";

int nBorderWidth, nBorderHeight;
CRect rtWnd, rtClnt;

int nRead;
int nTotalRead = 0;

int nExpectedMajor = 1;
int nExpectedMinor = 1;

int nCurrentMajor = 1;
int nCurrentMinor = 1;

int nWidth;
int nHeight;
int nHeadSize;

bool bIsInitial = true;

struct UdpBuffer
{
	int					nBufferLength;
	int					nMajorSegment;
	int					nMinorSegment;
	bool				bOccupied;
	unsigned char		pbBuffer[63 * 1024 + 14];
};

struct UdpBuffer udpQueue[UDP_QUEUE_LENGTH];

class CAboutDlg : public CDialogEx
{
public:
	CAboutDlg();

// Dialog Data
	enum { IDD = IDD_ABOUTBOX };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

// Implementation
protected:
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialogEx(CAboutDlg::IDD)
{
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialogEx)
	
END_MESSAGE_MAP()


// CDesktopClientDlg dialog



CDesktopClientDlg::CDesktopClientDlg(CWnd* pParent /*=NULL*/)
	: CDialogEx(CDesktopClientDlg::IDD, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);

	if (!AfxSocketInit())
	{
		return;
	}


	//BOOL bRet = CAsyncSocket::Create(10215, SOCK_STREAM, FD_READ);

	//if (bRet != TRUE)
	//{
	//	UINT uErr = GetLastError();
	//	TCHAR szError[256];
	//	wsprintf(szError, L"Server Receive Socket Create() failed: %d", uErr);
	//	AfxMessageBox(szError);
	//}

}

void CDesktopClientDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CDesktopClientDlg, CDialogEx)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_WM_TIMER()
END_MESSAGE_MAP()


// CDesktopClientDlg message handlers

BOOL CDesktopClientDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// Add "About..." menu item to system menu.

	// IDM_ABOUTBOX must be in the system command range.
	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != NULL)
	{
		BOOL bNameValid;
		CString strAboutMenu;
		bNameValid = strAboutMenu.LoadString(IDS_ABOUTBOX);
		ASSERT(bNameValid);
		if (!strAboutMenu.IsEmpty())
		{
			pSysMenu->AppendMenu(MF_SEPARATOR);
			pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
		}
	}

	// Set the icon for this dialog.  The framework does this automatically
	//  when the application's main window is not a dialog
	SetIcon(m_hIcon, TRUE);			// Set big icon
	SetIcon(m_hIcon, FALSE);		// Set small icon

	// TODO: Add extra initialization here

	nWidth = 100;
	nHeight = 100;

	//Get Subnet masks and Ip
	getSubnetMask();

	m_Listener.SetParentDlg(this); 
	m_Receiver.SetParentDlg(this);

	for (int i = 0; i < UDP_QUEUE_LENGTH; i++)
	{
		udpQueue[i].bOccupied = false;
	}

	m_Listener.Create(10215);

	if (m_Listener.Listen() == FALSE)
	{
		AfxMessageBox(_T("Unble to Listen on that port, please try another port"), MB_OK, 0);
		m_Listener.Close();
	}

	m_UdpSocket.Create(0, SOCK_DGRAM);

	BOOL a = TRUE;
	m_UdpSocket.SetSockOpt(SO_BROADCAST, &a, sizeof(BOOL));


	SetTimer(1, 1000, NULL);
	m_Connected = false;
	m_Received = false;
	return TRUE;  // return TRUE  unless you set the focus to a control
}

void CDesktopClientDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	if ((nID & 0xFFF0) == IDM_ABOUTBOX)
	{
		CAboutDlg dlgAbout;
		dlgAbout.DoModal();
	}
	else
	{
		CDialogEx::OnSysCommand(nID, lParam);
	}
}

// If you add a minimize button to your dialog, you will need the code below
//  to draw the icon.  For MFC applications using the document/view model,
//  this is automatically done for you by the framework.

void CDesktopClientDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // device context for painting

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// Center icon in client rectangle
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// Draw the icon
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CPaintDC dc(this);

		int nRowLength = nWidth * 3 + nWidth * 3 % 4;
		int nDataLength = nRowLength * nHeight;
		int nSegmentIndex = 0;
		for (int i = 0; i < nDataLength; i += 3)
		{
			pbBmp32[nSegmentIndex * 4] = pbBmpData[i];
			pbBmp32[nSegmentIndex * 4 + 1] = pbBmpData[i + 1];
			pbBmp32[nSegmentIndex * 4 + 2] = pbBmpData[i + 2];
			pbBmp32[nSegmentIndex * 4 + 3] = 0;
				
			if (nSegmentIndex % nWidth == nWidth - 1 && (i + 3) % 4 != 0)
				i += 4 - (i + 3) % 4;
			nSegmentIndex ++;
		}
			
		for (int i = 0; i < nHeight / 2; i++)
		{
			memcpy(pbBufferSwapper, pbBmp32 + i * nWidth * 4, nWidth * 4);
			memcpy(pbBmp32 + i * nWidth * 4, pbBmp32 + (nHeight - i - 1) * nWidth * 4, nWidth * 4);
			memcpy(pbBmp32 + (nHeight - i - 1) * nWidth * 4, pbBufferSwapper, nWidth * 4);
		}

		CDC bmDC;
		bmDC.CreateCompatibleDC(&dc);

		CBitmap bmp;
		bmp.CreateCompatibleBitmap(&dc, nWidth, nHeight);
		bmp.SetBitmapBits(nWidth * nHeight * 4, pbBmp32);
		bmDC.SelectObject(&bmp);
		dc.BitBlt(0, 0, nWidth, nHeight, &bmDC, 0, 0, SRCCOPY);

		GetWindowRect(&rtWnd);
		GetClientRect(&rtClnt);

		nBorderWidth = rtWnd.Width() - rtClnt.Width();
		nBorderHeight = rtWnd.Height() - rtClnt.Height();

		SetWindowPos(&wndTop, rtWnd.left, rtWnd.right, nWidth + nBorderWidth, nHeight + nBorderHeight, SWP_NOMOVE | SWP_NOACTIVATE | SWP_NOZORDER);
		
		CDialogEx::OnPaint();
	}
}

// The system calls this function to obtain the cursor to display while the user drags
//  the minimized window.
HCURSOR CDesktopClientDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

void GetSegmentNumbers(unsigned char * pbBuffer, int * pnMajorSegment, int * pnMinorSegment)
{
	if (memcmp(buff, pbStart, 5) == 0)
	{
		memcpy(pnMajorSegment, pbBuffer + 5, 4);
		memcpy(pnMinorSegment, pbBuffer + 9, 2);
	}
	else
	{
		memcpy(pnMajorSegment, pbBuffer, 4);
		memcpy(pnMinorSegment, pbBuffer + 4, 2);
	}
}

int pnMajor[100000];
int pnMinor[100000];

int nSegmentCount = 0;

bool bIsReceivingData = false;
int nTmpRead = 0;

int err = 0;

void CDesktopClientDlg::OnReceive()
{
	if (m_Received == false)
		m_Received = true;



	if (bIsReceivingData == true) {
		return;
	}

	bIsReceivingData = true;
	CString strSendersIp;
	UINT uSendersPort;

	//Sleep(40);

	nRead = 0;

	do {
		//Sleep(20);	
		nTmpRead = m_Receiver.Receive(tmpbuf, 1024, 0);

		//err = m_Receiver.GetLastError();
		
		if (nTmpRead == SOCKET_ERROR)
		{
			
			//AfxMessageBox(_T("Could not receive"), MB_OK, 0);
			//return;
			break;
		}
		

		memcpy(buff + nRead, tmpbuf, nTmpRead);
		nRead += nTmpRead;
	} while (nTmpRead > 0);

	m_Receiver.Send("Next", strlen("Next"));

	//while (buff[nRead - 3] != 'E' || buff[nRead - 2] == 'n' || buff[nRead - 1] == 'd') {
	//	nRead = m_Receiver.Receive(buff, BUFFER_LENGTH, 0);
	//	Sleep(5);
	//}

	//nRead = ReceiveFromEx(buff, BUFFER_LENGTH, strSendersIp, uSendersPort);

	switch (nRead)
	{
	case 0:
		m_Receiver.Close();
		break;
	case SOCKET_ERROR:
		if (GetLastError() != WSAEWOULDBLOCK)
		{
			AfxMessageBox(L"Error occurred");
			m_Receiver.Close();
		}
		break;
	default:

		GetSegmentNumbers(buff, &nCurrentMajor, &nCurrentMinor);

		if (bIsInitial == true)
		{
			nExpectedMajor = nCurrentMajor + 1;
			nExpectedMinor = 1;
			bIsInitial = false;
		}

	//	pnMajor[nSegmentCount] = nCurrentMajor;
	//	pnMinor[nSegmentCount++] = nCurrentMinor;

	//	if (nCurrentMajor != nExpectedMajor || nCurrentMinor != nExpectedMinor)
	//	{
	//		/*int i = 0;
	//		for (i = 0; i < UDP_QUEUE_LENGTH; i++)
	//		{
	//			if (udpQueue[i].bOccupied == false)
	//			{
	//				break;
	//			}
	//		}
	//
	//		if (i != UDP_QUEUE_LENGTH)
	//		{
	//			udpQueue[i].bOccupied = true;
	//			udpQueue[i].nBufferLength = nRead;
	//			udpQueue[i].nMajorSegment = nCurrentMajor;
	//			udpQueue[i].nMinorSegment = nCurrentMinor;
	//			memcpy(udpQueue[i].pbBuffer, buff, nRead);
	//		}
	//
	//		nRead = 0;*/
	//
	//		nExpectedMajor = nCurrentMajor + 1;
	//		nExpectedMinor = 1;
	//
	//		nTotalRead = 0;
	//	}
	//	else
		{
			if (memcmp(buff + nRead - 3, pbEnd, 3) == 0)
			{
				if (memcmp(buff, pbStart, 5) == 0)
				{
					memcpy(pbData + nTotalRead, buff + 11, nRead - 14);
					nTotalRead += nRead - 14;
				}
				else
				{
					memcpy(pbData + nTotalRead, buff + 6, nRead - 9);
					nTotalRead += nRead - 9;
				}

				m_TonyJpegDecoder.ReadJpgHeader(pbData, nTotalRead, nWidth, nHeight, nHeadSize);
				m_TonyJpegDecoder.DecompressImage(pbData + nHeadSize, pbBmpData);

				nTotalRead = 0;
				InvalidateRect(CRect(0, 0, nWidth, nHeight), FALSE);

				nExpectedMajor++;
				nExpectedMinor = 1;
			}
			else if (memcmp(buff, pbStart, 5) == 0)
			{
				memcpy(pbData + nTotalRead, buff + 11, nRead - 11);
				nTotalRead += nRead - 11;
				nExpectedMinor++;
			}
			else
			{
				memcpy(pbData + nTotalRead, buff + 6, nRead - 6);
				nTotalRead += nRead - 6;
				nExpectedMinor++;
			}
		}
		
		break;
	}

	bIsReceivingData = false;
}

void CDesktopClientDlg::OnAccept() {
	
	CString strIP;
	UINT port;
	if (m_Listener.Accept(m_Receiver))
	{
		m_Receiver.GetSockName(strIP, port);
		CString m_Status = CString("Client Connected,IP:") + strIP;
		m_Receiver.Send("Next", strlen("Next"));
		UpdateData(FALSE);
	}
	else
	{
		AfxMessageBox(_T("Cannot Accept Connection"), MB_OK, 0);
	}

	m_Connected = true;
	m_Received = false;
}

void CDesktopClientDlg::OnClose() {
	m_Listener.Close();
	m_Receiver.Close();
	//m_UdpSocket.Close();
	m_Connected = false;

}

void CDesktopClientDlg::OnTimer(UINT_PTR nIdEvent) {
	if (m_Received == false)
		m_Receiver.Send("Next", strlen("Next"));
	for (int i = 0; i < mBroadcastList2.size(); i++){
		m_UdpSocket.SendTo("Signal", 6, 10216, mBroadcastList2[i]);
	}
}

int CDesktopClientDlg::getSubnetMask()
{
		
    /* Declare and initialize variables */

// It is possible for an adapter to have multiple
// IPv4 addresses, gateways, and secondary WINS servers
// assigned to the adapter. 
//
// Note that this sample code only prints out the 
// first entry for the IP address/mask, and gateway, and
// the primary and secondary WINS server for each adapter. 

    PIP_ADAPTER_INFO pAdapterInfo;
    PIP_ADAPTER_INFO pAdapter = NULL;
    DWORD dwRetVal = 0;
    UINT i;

/* variables used to print DHCP time info */
    struct tm newtime;
    char buffer[32];
    errno_t error;

    ULONG ulOutBufLen = sizeof (IP_ADAPTER_INFO);
    pAdapterInfo = (IP_ADAPTER_INFO *) malloc(sizeof (IP_ADAPTER_INFO));
    if (pAdapterInfo == NULL) {
        printf("Error allocating memory needed to call GetAdaptersinfo\n");
        //return 1;
    }
// Make an initial call to GetAdaptersInfo to get
// the necessary size into the ulOutBufLen variable
    if (GetAdaptersInfo(pAdapterInfo, &ulOutBufLen) == ERROR_BUFFER_OVERFLOW) {
        free(pAdapterInfo);
        pAdapterInfo = (IP_ADAPTER_INFO *) malloc(ulOutBufLen);
        if (pAdapterInfo == NULL) {
            printf("Error allocating memory needed to call GetAdaptersinfo\n");
            //return 1;
        }
    }

    if ((dwRetVal = GetAdaptersInfo(pAdapterInfo, &ulOutBufLen)) == NO_ERROR) {
        pAdapter = pAdapterInfo;
        while (pAdapter) {
            printf("\tComboIndex: \t%d\n", pAdapter->ComboIndex);
            printf("\tAdapter Name: \t%s\n", pAdapter->AdapterName);
            printf("\tAdapter Desc: \t%s\n", pAdapter->Description);
            printf("\tAdapter Addr: \t");
            for (i = 0; i < pAdapter->AddressLength; i++) {
                if (i == (pAdapter->AddressLength - 1))
                    printf("%.2X\n", (int) pAdapter->Address[i]);
                else
                    printf("%.2X-", (int) pAdapter->Address[i]);
            }
            printf("\tIndex: \t%d\n", pAdapter->Index);
            printf("\tType: \t");
            switch (pAdapter->Type) {
            case MIB_IF_TYPE_OTHER:
                printf("Other\n");
                break;
            case MIB_IF_TYPE_ETHERNET:
                printf("Ethernet\n");
                break;
            case MIB_IF_TYPE_TOKENRING:
                printf("Token Ring\n");
                break;
            case MIB_IF_TYPE_FDDI:
                printf("FDDI\n");
                break;
            case MIB_IF_TYPE_PPP:
                printf("PPP\n");
                break;
            case MIB_IF_TYPE_LOOPBACK:
                printf("Lookback\n");
                break;
            case MIB_IF_TYPE_SLIP:
                printf("Slip\n");
                break;
            default:
                printf("Unknown type %ld\n", pAdapter->Type);
                break;
            }

            printf("\tIP Address: \t%s\n",
                   pAdapter->IpAddressList.IpAddress.String);
            printf("\tIP Mask: \t%s\n", pAdapter->IpAddressList.IpMask.String);
            printf("\tGateway: \t%s\n", pAdapter->GatewayList.IpAddress.String);


			byte iIpAddr [4];
			byte iSubMsk [4];
			char * nIpAddr = pAdapter->IpAddressList.IpAddress.String;
			char * nSubMsk = pAdapter->IpAddressList.IpMask.String;

			//strtok ip address.
			char *pch;
			pch = strtok(nIpAddr, ".");

			byte ii = 0;
			while (pch != NULL)
			{
				iIpAddr[ii++] = atoi(pch);
				pch = strtok(NULL, ".");
			}

			//strtok ip submsk.
			char *pch2;
			pch2 = strtok(nSubMsk, ".");

			byte iii = 0;
			while (pch2 != NULL)
			{
				iSubMsk[iii++] = atoi(pch2);
				pch2 = strtok(NULL, ".");
			}

			CString str;
			str.Format(L"%d.%d.%d.%d", (unsigned char)(iIpAddr[0] | ~iSubMsk[0]), (unsigned char)(iIpAddr[1] | ~iSubMsk[1]), (unsigned char)(iIpAddr[2] | ~iSubMsk[2]), (unsigned char)(iIpAddr[3] | ~iSubMsk[3]));
			mBroadcastList2.push_back(str);

            printf("\t***\n");

            if (pAdapter->DhcpEnabled) {																						
                printf("\tDHCP Enabled: Yes\n");																				
                printf("\t  DHCP Server: \t%s\n",																				
                       pAdapter->DhcpServer.IpAddress.String);																	
																																
                printf("\t  Lease Obtained: ");																					
                /* Display local time */																						
                error = _localtime32_s(&newtime, (__time32_t*) &pAdapter->LeaseObtained);										
                if (error)																										
                    printf("Invalid Argument to _localtime32_s\n");																
                else {																											
                    // Convert to an ASCII representation 																		
                    error = asctime_s(buffer, 32, &newtime);																	
                    if (error)																									
                        printf("Invalid Argument to asctime_s\n");																
                    else																										
                        /* asctime_s returns the string terminated by \n\0 */													
                        printf("%s", buffer);																					
                }																												
																																
                printf("\t  Lease Expires:  ");																					
                error = _localtime32_s(&newtime, (__time32_t*) &pAdapter->LeaseExpires);
                if (error)
                    printf("Invalid Argument to _localtime32_s\n");
                else {
                    // Convert to an ASCII representation 
                    error = asctime_s(buffer, 32, &newtime);
                    if (error)
                        printf("Invalid Argument to asctime_s\n");
                    else
                        /* asctime_s returns the string terminated by \n\0 */
                        printf("%s", buffer);
                }
            } else
                printf("\tDHCP Enabled: No\n");

            if (pAdapter->HaveWins) {
                printf("\tHave Wins: Yes\n");
                printf("\t  Primary Wins Server:    %s\n",
                       pAdapter->PrimaryWinsServer.IpAddress.String);
                printf("\t  Secondary Wins Server:  %s\n",
                       pAdapter->SecondaryWinsServer.IpAddress.String);
            } else
                printf("\tHave Wins: No\n");
            pAdapter = pAdapter->Next;
            printf("\n");
        }
    } else {
        printf("GetAdaptersInfo failed with error: %d\n", dwRetVal);

    }
    if (pAdapterInfo)
        free(pAdapterInfo);


    return 0;
}