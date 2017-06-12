/****************************************************************************
*	Author:			Dr. Tony Lin											*
*	Email:			lintong@cis.pku.edu.cn									*
*	Release Date:	Dec. 2002												*
*																			*
*	Name:			TonyJpegLib, rewritten from IJG codes					*
*	Source:			IJG v.6a JPEG LIB										*
*	Purpose£º		Support real jpeg file, with readable code				*
*																			*
*	Acknowlegement:	Thanks for great IJG, and Chris Losinger				*
*																			*
*	Legal Issues:	(almost same as IJG with followings)					*
*																			*
*	1. We don't promise that this software works.							*
*	2. You can use this software for whatever you want.						*
*	You don't have to pay.													*
*	3. You may not pretend that you wrote this software. If you use it		*
*	in a program, you must acknowledge somewhere. That is, please			*
*	metion IJG, and Me, Dr. Tony Lin.										*
*																			*
*****************************************************************************/


typedef struct {
  int component_id;		/* identifier for this component (0..255) */
  int component_index;		/* its index in SOF or cinfo->comp_info[] */
  int h_samp_factor;		/* horizontal sampling factor (1..4) */
  int v_samp_factor;		/* vertical sampling factor (1..4) */
  int quant_tbl_no;		/* quantization table selector (0..3) */
} jpeg_component_info;



// Derived data constructed for each Huffman table 
typedef struct{
	int				mincode[17];	// smallest code of length k 
	int				maxcode[18];	// largest code of length k (-1 if none) 
	int				valptr[17];		// huffval[] index of 1st symbol of length k
	unsigned char	bits[17];		// bits[k] = # of symbols with codes of 
	unsigned char	huffval[256];	// The symbols, in order of incr code length 
	int				look_nbits[256];// # bits, or 0 if too long
	unsigned char	look_sym[256];	// symbol, or unused
} HUFFTABLE;



class CTonyJpegDecoder
{

private:
	
	////////////////////////////////////////////////////////////////////////////
	//	Following are initialized when create a new decoder

	unsigned short m_nQuality, m_nScale;

	unsigned char m_tblRange[5*256+128];
	
	//	To speed up, we save YCbCr=>RGB color map tables
	int m_CrToR[256], m_CrToG[256],	m_CbToB[256], m_CbToG[256];

	//	To speed up, we precompute two DCT quant tables
	unsigned short m_qtblY[64], m_qtblCbCr[64];

	HUFFTABLE m_htblYDC, m_htblYAC, m_htblCbCrDC, m_htblCbCrAC;
	
	////////////////////////////////////////////////////////////////////////////

	////////////////////////////////////////////////////////////////////////////
	//	Following data are initialized for decoding every image

	unsigned short m_nWidth, m_nHeight, m_nMcuSize, m_nBlocksInMcu;

	int m_dcY, m_dcCb, m_dcCr;

	int m_nGetBits, m_nGetBuff, m_nDataBytesLeft;

	unsigned char * m_pData;

	int m_nPrecision, m_nComponent;

	int restart_interval, restarts_to_go, unread_marker, next_restart_num;

	jpeg_component_info comp_info[3];

	////////////////////////////////////////////////////////////////////////////

private:

	void InitDecoder( void );
	
	void SetRangeTable( void );

	void InitColorTable( void );

	void InitQuantTable( void );

	void ScaleQuantTable(
			unsigned short* tblRst,		//result quant table
			unsigned short* tblStd,		//standard quant table
			unsigned short* tblAan		//scale factor for AAN dct
			);

	void InitHuffmanTable( void );

	void ComputeHuffmanTable(HUFFTABLE * dtbl);

	bool DecompressOneTile(	unsigned char * pBgr );

	void YCbCrToBGREx(	
		unsigned char * pYCbCr,	//in, Y: 256 bytes; Cb: 64 bytes; Cr: 64 bytes 
		unsigned char * pBgr	//out, BGR format, 16*16*3 = 768 bytes
		);

	void InverseDct( 
		short* coef, 			//in, dct coefficients, length = 64
		unsigned char* data, 	//out, 64 bytes		
		int nBlock				//block index: 0~3:Y; 4:Cb; 5:Cr
		);
	
	void HuffmanDecode( 
		short* coef,			//	out, DCT coefficients
		int iBlock				//	0,1,2,3:Y; 4:Cb; 5:Cr
		);

	int GetCategory( HUFFTABLE* htbl );

	void FillBitBuffer( void );

	int GetBits(int nbits);
	
	int SpecialDecode( HUFFTABLE* htbl, int nMinBits );

	int ValueFromCategory(int nCate, int nOffset);

	// new added 
	int read_markers (
	unsigned char *pInBuf,	//in, source data, in jpg format
	int cbInBuf,			//in, count bytes for in buffer
	int& nWidth,			//out, image width in pixels
	int& nHeight,			//out, image height
	int& nHeadSize			//out, header size in bytes
	);

	int ReadOneMarker( void );

	void GetDqt( void );

	void get_sof(bool is_prog, bool is_arith);

	void get_dht(void);

	void get_sos(void);

	void SkipMarker(void);

	void get_dri(void);

	void read_restart_marker (void);

public:	

	CTonyJpegDecoder();	
	~CTonyJpegDecoder();
	
	bool ReadJpgHeader(	
		unsigned char *pInBuf,	//in, source data, in jpp format
		int cbInBuf,			//in, count bytes for in buffer
		int& nWidth,			//out, image width in pixels
		int& nHeight,			//out, image height
		int& nHeadSize			//out, header size in bytes
		);
	
	bool DecompressImage(	
		unsigned char *pInBuf,	//in, source data, in jpg format
		unsigned char *pOutBuf	//out, destination buffer, bgr format
		);
};

