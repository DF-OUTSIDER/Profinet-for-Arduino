#include "Profinet.h"
/*
	Arduino has not a multithread environment and all Client functions are
	fully synchronous, so to save memory we can define telegrams and I/O
	data areas as globals, since only one client at time will use them.
*/

// ISO Connection Request telegram (contains also ISO Header and COTP Header)
	byte ISO_CR[] = {
		// TPKT (RFC1006 Header)
		0x03, // RFC 1006 ID (3)
		0x00, // Reserved, always 0
		0x00, // High part of packet lenght (entire frame, payload and TPDU included)
		0x16, // Low part of packet lenght (entire frame, payload and TPDU included)
		// COTP (ISO 8073 Header)
		0x11, // PDU Size Length
		0xE0, // CR - Connection Request ID
		0x00, // Dst Reference HI
		0x00, // Dst Reference LO
        0x00, // Src Reference HI
		0x01, // Src Reference LO
		0x00, // Class + Options Flags
		0xC0, // PDU Max Length ID
		0x01, // PDU Max Length HI
		0x0A, // PDU Max Length LO
		0xC1, // Src TSAP Identifier
		0x02, // Src TSAP Length (2 bytes)
        0x01, // Src TSAP HI (will be overwritten by ISOConnect())
		0x00, // Src TSAP LO (will be overwritten by ISOConnect())
		0xC2, // Dst TSAP Identifier
		0x02, // Dst TSAP Length (2 bytes)
		0x01, // Dst TSAP HI (will be overwritten by ISOConnect())
		0x02  // Dst TSAP LO (will be overwritten by ISOConnect())
	};

// S7 PDU Negotiation Telegram (contains also ISO Header and COTP Header)
	byte S7_PN[] = {
		0x03, 0x00, 0x00, 0x19, 0x02, 0xf0, 0x80, // TPKT + COTP (see above for info)
		0x32, 0x01, 0x00, 0x00, 0x04, 0x00, 0x00, 0x08, 0x00,
		0x00, 0xf0, 0x00, 0x00, 0x01, 0x00, 0x01,
		0x00, 0xf0 // PDU Length Requested = HI-LO 240 bytes
	};

// S7 Read/Write Request Header (contains also ISO Header and COTP Header)
	byte S7_RW[] = { // 31-35 bytes
		0x03, 0x00,
		0x00, 0x1f, // Telegram Length (Data Size + 31 or 35)
		0x02, 0xf0, 0x80, // COTP (see above for info)
		0x32,       // S7 Protocol ID
		0x01,       // Job Type
		0x00, 0x00, // Redundancy identification
		0x05, 0x00, // PDU Reference
		0x00, 0x0e, // Parameters Length
		0x00, 0x00, // Data Length = Size(bytes) + 4
		0x04,       // Function 4 Read Var, 5 Write Var
		0x01,       // Items count
		0x12,       // Var spec.
		0x0a,       // Length of remaining bytes
		0x10,       // Syntax ID
		S7WLByte,   // Transport Size (default, could be changed)
		0x00,0x00,  // Num Elements
		0x00,0x00,  // DB Number (if any, else 0)
		0x84,       // Area Type
		0x00, 0x00, 0x00, // Area Offset
		// WR area
		0x00,       // Reserved
		0x04,       // Transport size
		0x00, 0x00, // Data Length * 8 (if not timer or counter)
	};

#ifdef _EXTENDED

// S7 Get Block Info Request Header (contains also ISO Header and COTP Header)
	byte S7_BI[] = {
		0x03, 0x00, 0x00, 0x25, 0x02, 0xf0, 0x80, 0x32,
		0x07, 0x00, 0x00, 0x05, 0x00, 0x00, 0x08, 0x00,
		0x0c, 0x00, 0x01, 0x12, 0x04, 0x11, 0x43, 0x03,
		0x00, 0xff, 0x09, 0x00, 0x08, 0x30, 0x41,
		0x30, 0x30, 0x30, 0x30, 0x30, // ASCII DB Number
		0x41
	};

// S7 Put PLC in STOP state Request Header (contains also ISO Header and COTP Header)
	byte S7_STOP[] = {
		0x03, 0x00, 0x00, 0x21, 0x02, 0xf0, 0x80, 0x32,
		0x01, 0x00, 0x00, 0x0e, 0x00, 0x00, 0x10, 0x00,
		0x00, 0x29, 0x00, 0x00, 0x00, 0x00, 0x00, 0x09,
		0x50, 0x5f, 0x50, 0x52, 0x4f, 0x47, 0x52, 0x41,
		0x4d
	};

// S7 Put PLC in RUN state Request Header (contains also ISO Header and COTP Header)
	byte S7_START[] = {
		0x03, 0x00, 0x00, 0x25, 0x02, 0xf0, 0x80, 0x32,
		0x01, 0x00, 0x00, 0x0f, 0x00, 0x00, 0x14, 0x00,
		0x00, 0x28, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0xfd, 0x00, 0x00, 0x09, 0x50, 0x5f, 0x50, 0x52,
		0x4f, 0x47, 0x52, 0x41, 0x4d
	};

// S7 Get PLC Status Request Header (contains also ISO Header and COTP Header)
	byte S7_PLCGETS[] = {
		0x03, 0x00, 0x00, 0x21, 0x02, 0xf0, 0x80, 0x32,
		0x07, 0x00, 0x00, 0x2c, 0x00, 0x00, 0x08, 0x00,
		0x08, 0x00, 0x01, 0x12, 0x04, 0x11, 0x44, 0x01,
		0x00, 0xff, 0x09, 0x00, 0x04, 0x04, 0x24, 0x00,
		0x00
	};

#endif // _EXTENDED

	TPDU PDU;

#ifdef _S7HELPER

	S7Helper S7;
//-----------------------------------------------------------------------------
bool S7Helper::BitAt(void *Buffer, int ByteIndex, byte BitIndex)
{
	byte mask[] = {0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80};
	pbyte Pointer = pbyte(Buffer) + ByteIndex;

	if (BitIndex>7)
		return false;
	else
		return (*Pointer & mask[BitIndex]);
}
//-----------------------------------------------------------------------------
bool S7Helper::BitAt(int ByteIndex, int BitIndex)
{
	return BitAt(&PDU.DATA[0], ByteIndex, BitIndex);
}
//-----------------------------------------------------------------------------
byte S7Helper::ByteAt(void *Buffer, int index)
{
	pbyte Pointer = pbyte(Buffer) + index;
	return *Pointer;
}
//-----------------------------------------------------------------------------
byte S7Helper::ByteAt(int index)
{
	return ByteAt(&PDU.DATA, index);
}
//-----------------------------------------------------------------------------
word S7Helper::WordAt(void *Buffer, int index)
{
	word hi=(*(pbyte(Buffer) + index))<<8;
	return hi+*(pbyte(Buffer) + index+1);
}
//-----------------------------------------------------------------------------
word S7Helper::WordAt(int index)
{
	return WordAt(&PDU.DATA, index);
}
//-----------------------------------------------------------------------------
dword S7Helper::DWordAt(void *Buffer, int index)
{
	pbyte pb;
	dword dw1;

	pb=pbyte(Buffer) + index;
	dw1=*pb;dw1<<=8;
	pb=pbyte(Buffer) + index + 1;
	dw1+=*pb;dw1<<=8;
	pb=pbyte(Buffer) + index + 2;
	dw1+=*pb;dw1<<=8;
	pb=pbyte(Buffer) + index + 3;
	dw1+=*pb;
	return dw1;
}
//-----------------------------------------------------------------------------
dword S7Helper::DWordAt(int index)
{
	return DWordAt(&PDU.DATA, index);
}
//-----------------------------------------------------------------------------
float S7Helper::FloatAt(void *Buffer, int index)
{
	dword dw = DWordAt(Buffer, index);
	return *(pfloat(&dw));
}
//-----------------------------------------------------------------------------
float S7Helper::FloatAt(int index)
{
	return FloatAt(&PDU.DATA, index);
}
//-----------------------------------------------------------------------------
integer S7Helper::IntegerAt(void *Buffer, int index)
{
	word w = WordAt(Buffer, index);
	return *(pinteger(&w));
}
//-----------------------------------------------------------------------------
integer S7Helper::IntegerAt(int index)
{
	return IntegerAt(&PDU.DATA, index);
}
//-----------------------------------------------------------------------------
long S7Helper::DintAt(void *Buffer, int index)
{
	dword dw = DWordAt(Buffer, index);
	return *(pdint(&dw));
}
//-----------------------------------------------------------------------------
long S7Helper::DintAt(int index)
{
	return DintAt(&PDU.DATA, index);
}
//-----------------------------------------------------------------------------
void S7Helper::SetBitAt(void *Buffer, int ByteIndex, int BitIndex, bool Value)
{
	byte Mask[] = {0x01,0x02,0x04,0x08,0x10,0x20,0x40,0x80};
	pbyte Pointer = pbyte(Buffer) + ByteIndex;
	if (BitIndex < 0) BitIndex = 0;
	if (BitIndex > 7) BitIndex = 7;

	if (Value)
		*Pointer=*Pointer | Mask[BitIndex];
	else
		*Pointer=*Pointer & ~Mask[BitIndex];
}
//-----------------------------------------------------------------------------
void S7Helper::SetBitAt(int ByteIndex, int BitIndex, bool Value)
{
	SetBitAt(&PDU.DATA, ByteIndex, BitIndex, Value);
}
//-----------------------------------------------------------------------------
void S7Helper::SetByteAt(void *Buffer, int index, byte value)
{
  *(pbyte(Buffer)+index)=value;
}
//-----------------------------------------------------------------------------
void S7Helper::SetByteAt(int index, byte value)
{
  PDU.DATA[index]=value;
}
//-----------------------------------------------------------------------------
void S7Helper::SetIntAt(void *Buffer, int index, integer value)
{
  *(pbyte(Buffer)+index)  =byte(value>>8);
  *(pbyte(Buffer)+index+1)=byte(value & 0x00FF);
}
//-----------------------------------------------------------------------------
void S7Helper::SetIntAt(int index, integer value)
{
	SetIntAt(&PDU.DATA, index, value);
}
//-----------------------------------------------------------------------------
void S7Helper::SetDIntAt(void *Buffer, int index, dint value)
{
  *(pbyte(Buffer)+index)  =byte((value >> 24) & 0xFF);
  *(pbyte(Buffer)+index+1)=byte((value >> 16) & 0xFF);
  *(pbyte(Buffer)+index+2)=byte((value >> 8) & 0xFF);
  *(pbyte(Buffer)+index+3)=byte(value & 0x00FF);
}
//-----------------------------------------------------------------------------
void S7Helper::SetDIntAt(int index, dint value)
{
	SetDIntAt(&PDU.DATA, index, value);
}
//-----------------------------------------------------------------------------
void S7Helper::SetWordAt(void *Buffer, int index, word value)
{
  *(pbyte(Buffer)+index)  =byte(value>>8);
  *(pbyte(Buffer)+index+1)=byte(value & 0x00FF);
}
//-----------------------------------------------------------------------------
void S7Helper::SetWordAt(int index, word value)
{
	SetWordAt(&PDU.DATA, index, value);
}
//-----------------------------------------------------------------------------
void S7Helper::SetDWordAt(void *Buffer, int index, dword value)
{
  *(pbyte(Buffer)+index)  =byte((value >> 24) & 0xFF);
  *(pbyte(Buffer)+index+1)=byte((value >> 16) & 0xFF);
  *(pbyte(Buffer)+index+2)=byte((value >> 8) & 0xFF);
  *(pbyte(Buffer)+index+3)=byte(value & 0x00FF);
}
//-----------------------------------------------------------------------------
void S7Helper::SetDWordAt(int index, word value)
{
	SetDWordAt(&PDU.DATA, index, value);
}
//-----------------------------------------------------------------------------
void S7Helper::SetFloatAt(void *Buffer, int index, float value)
{
  pdword dvalue = pdword(&value);

  *(pbyte(Buffer)+index)  =byte((*dvalue >> 24) & 0xFF);
  *(pbyte(Buffer)+index+1)=byte((*dvalue >> 16) & 0xFF);
  *(pbyte(Buffer)+index+2)=byte((*dvalue >> 8) & 0xFF);
  *(pbyte(Buffer)+index+3)=byte(*dvalue & 0x00FF);
}
//-----------------------------------------------------------------------------
void S7Helper::SetFloatAt(int index, float value)
{
	SetFloatAt(&PDU.DATA, index, value);
}
//-----------------------------------------------------------------------------
char * S7Helper::StringAt(void *Buffer, int index)
{
  return pchar(Buffer+index);
}
//-----------------------------------------------------------------------------
char * S7Helper::StringAt(int index)
{
  return pchar(&PDU.DATA[index]);
}
//-----------------------------------------------------------------------------
void S7Helper::SetStringAt(void *Buffer, int index, char *value)
{
  strcpy(pchar(Buffer+index),value);
}
//-----------------------------------------------------------------------------
void S7Helper::SetStringAt(int index, char *value)
{
  strcpy(pchar(&PDU.DATA[index]),value);
}

#endif // _S7HELPER

//-----------------------------------------------------------------------------
// Ethernet initialization
//-----------------------------------------------------------------------------
void EthernetInit(uint8_t *mac, IPAddress ip)
{
#ifdef S7WIRED
  #ifdef M5STACK_LAN
      SPI.begin(18, 19, 23, -1);
      Ethernet.init(26);
  #endif
      // Start Ethernet
      Ethernet.begin(mac, ip);
#endif
}
//-----------------------------------------------------------------------------
S7Client::S7Client()
{
	// Default TSAP values for connectiong as PG to a S7300 (Rack 0, Slot 2)
	LocalTSAP_HI = 0x01;
	LocalTSAP_LO = 0x00;
	RemoteTSAP_HI= 0x01;
	RemoteTSAP_LO= 0x02;
	ConnType = PG;
	Connected = false;
	LastError = 0;
	PDULength = 0;
	RecvTimeout = 500; // 500 ms

#ifdef S7WIFI
    TCPClient = new(WiFiClient);
#endif
#ifdef S7WIRED
    TCPClient = new(EthernetClient);
#endif
}
//-----------------------------------------------------------------------------
S7Client::~S7Client()
{
	Disconnect();
	delete TCPClient;
}
//-----------------------------------------------------------------------------
int S7Client::SetLastError(int Error)
{
	LastError=Error;
	return Error;
}
//-----------------------------------------------------------------------------
int S7Client::WaitForData(uint16_t Size, uint16_t Timeout)
{
	unsigned long Elapsed = millis();
	uint16_t BytesReady;

	do
	{
		// I don't like next function because imho is buggy, it returns 0 also if _sock==MAX_SOCK_NUM
		// but it belongs to the standard Ethernet library and there are too many dependencies to skip them.
		// So be very carefully with the munber of the clients, they must be <=4.
		BytesReady=TCPClient->available();
		if (BytesReady<Size)
			delayMicroseconds(500);
		else
			return SetLastError(0);

		// Check for rollover - should happen every 52 days without turning off Arduino.
		if (millis()<Elapsed)
			Elapsed=millis(); // Resets the counter, in the worst case we will wait some additional millisecs.

	}while(millis()-Elapsed<Timeout);

	// Here we are in timeout zone, if there's something into the buffer, it must be discarded.
	if (BytesReady>0)
		TCPClient->flush();
	else
	{
		if (!TCPClient->connected())
			return SetLastError(errTCPConnectionReset);
	}

	return SetLastError(errTCPDataRecvTout);
}
//-----------------------------------------------------------------------------
int S7Client::IsoPduSize()
{
	uint16_t Size = PDU.H[2];
	return (Size<<8) + PDU.H[3];
}
//-----------------------------------------------------------------------------
int S7Client::RecvPacket(uint8_t *buf, uint16_t Size)
{
	WaitForData(Size,RecvTimeout);
	if (LastError!=0)
		return LastError;
	if (TCPClient->read(buf, Size)==0)
		return SetLastError(errTCPConnectionReset);
	return SetLastError(0);
}
//-----------------------------------------------------------------------------
void S7Client::SetConnectionParams(IPAddress Address, uint16_t LocalTSAP, uint16_t RemoteTSAP)
{
	Peer = Address;
	LocalTSAP_HI = LocalTSAP>>8;
	LocalTSAP_LO = LocalTSAP & 0x00FF;
	RemoteTSAP_HI = RemoteTSAP>>8;
	RemoteTSAP_LO = RemoteTSAP & 0x00FF;
}
//-----------------------------------------------------------------------------
void S7Client::SetConnectionType(uint16_t ConnectionType)
{
	ConnType = ConnectionType;
}
//-----------------------------------------------------------------------------
int S7Client::ConnectTo(IPAddress Address, uint16_t Rack, uint16_t Slot)
{
	SetConnectionParams(Address, 0x0100, (ConnType<<8)+(Rack * 0x20) + Slot);
	return Connect();
}
//-----------------------------------------------------------------------------
int S7Client::Connect()
{
	LastError = 0;
	if (!Connected)
	{
		TCPConnect();
		if (LastError==0) // First stage : TCP Connection
		{
			ISOConnect();
			if (LastError==0) // Second stage : ISOTCP (ISO 8073) Connection
			{
				LastError=NegotiatePduLength(); // Third stage : S7 PDU negotiation
			}
		}
	}
	Connected=LastError==0;
	return LastError;
}
//-----------------------------------------------------------------------------
void S7Client::Disconnect()
{
	if (Connected)
	{
		TCPClient->stop();
		Connected = false;
		PDULength = 0;
		LastError = 0;
	}
}
//-----------------------------------------------------------------------------
int S7Client::TCPConnect()
{
	if (TCPClient->connect(Peer, isotcp))
		return SetLastError(0);
	else
		return SetLastError(errTCPConnectionFailed);
}
//-----------------------------------------------------------------------------
int S7Client::RecvISOPacket(uint16_t *Size)
{
	bool Done = false;
	pbyte Target = pbyte(&PDU.H[0])+Shift;
	LastError=0;

	while ((LastError==0) && !Done)
	{
		// Get TPKT (4 bytes)
		RecvPacket(PDU.H, 4);
		if (LastError==0)
		{
			*Size = IsoPduSize();
			// Check 0 bytes Data Packet (only TPKT+COTP - 7 bytes)
			if (*Size==7)
				RecvPacket(PDU.H, 3); // Skip remaining 3 bytes and Done is still false
			else
			{
				if ((*Size>MaxPduSize) || (*Size<MinPduSize))
					LastError=errISOInvalidPDU;
				else
					Done = true; // a valid Length !=7 && >16 && <247
			}
		}
	}
	if (LastError==0)
	{
		RecvPacket(PDU.H, 3); // Skip remaining 3 COTP bytes
		LastPDUType=PDU.H[1]; // Stores PDU Type, we need it
		*Size-=ISOSize;
		// We need to align with PDU.DATA
		RecvPacket(Target, *Size);
	}
	if (LastError!=0)
		TCPClient->flush();
	return LastError;
}
//-----------------------------------------------------------------------------
int S7Client::ISOConnect()
{
	bool Done = false;
	uint16_t Length;
	// Setup TSAPs
	ISO_CR[16]=LocalTSAP_HI;
	ISO_CR[17]=LocalTSAP_LO;
	ISO_CR[20]=RemoteTSAP_HI;
	ISO_CR[21]=RemoteTSAP_LO;

	if (TCPClient->write(&ISO_CR[0], sizeof(ISO_CR))==sizeof(ISO_CR))
	{
		RecvISOPacket(&Length);
		if ((LastError==0) && (Length==15)) // 15 = 22 (sizeof CC telegram) - 7 (sizeof Header)
		{
			if (LastPDUType==CC) // Connection confirm
				return 0;
			else
				return SetLastError(errISOInvalidPDU);
		}
		else
			return LastError;
	}
	else
		return SetLastError(errISOConnectionFailed);
}
//-----------------------------------------------------------------------------
int S7Client::NegotiatePduLength()
{
	uint16_t Length;
	if (TCPClient->write(&S7_PN[0], sizeof(S7_PN))==sizeof(S7_PN))
	{
		RecvISOPacket(&Length);
		if (LastError==0)
		{
			// check S7 Error
			if ((Length==20) && (PDU.H[27]==0) && (PDU.H[28]==0))  // 20 = size of Negotiate Answer
			{
				PDULength = PDU.RAW[35];
				PDULength = (PDULength<<8) + PDU.RAW[36]; // Value negotiated
				if (PDULength>0)
				    return 0;
				else
					return SetLastError(errISONegotiatingPDU);
			}
			else
				return SetLastError(errISONegotiatingPDU);
		}
		else
			return LastError;
	}
	else
		return SetLastError(errISONegotiatingPDU);
}
//-----------------------------------------------------------------------------
int S7Client::ReadArea(int Area, uint16_t DBNumber, uint16_t Start, uint16_t Amount, int WordLen, void *ptrData)
{
	unsigned long Address;
	unsigned long LongStart=Start;
	uint16_t NumElements;
	uint16_t MaxElements;
	uint16_t TotElements;
	uint16_t SizeRequested;
	uint16_t Length;

	pbyte Target;
	uintptr_t Offset = 0;
	int WordSize = 1;

	LastError=0;

	// If we are addressing Timers or counters the element size is 2
	if ((Area==S7AreaCT) || (Area==S7AreaTM))
		WordSize = 2;

	if (WordLen==S7WLBit) // Only one bit can be transferred in S7Protocol when WordLen==S7WLBit
		Amount=1;

    MaxElements=(PDULength-18) / WordSize; // 18 = Reply telegram header
	TotElements=Amount;
	// If we use the internal buffer only, we cannot exced the PDU limit
	if (ptrData==NULL)
	{
		if (TotElements>MaxElements)
			TotElements=MaxElements;
	}

    while ((TotElements>0) && (LastError==0))
    {
        NumElements=TotElements;
        if (NumElements>MaxElements)
           NumElements=MaxElements;

		SizeRequested =NumElements * WordSize;

		Target=pbyte(ptrData)+Offset;

		// Setup the telegram
		memcpy(&PDU.H, S7_RW, Size_RD);

		// Set DB Number
		PDU.H[27] = Area;
		if (Area==S7AreaDB)
		{
			PDU.H[25] = DBNumber>>8;
			PDU.H[26] = DBNumber & 0x00FF;
		}

		// Adjusts Start and word length
		if ((WordLen == S7WLBit) || (Area==S7AreaCT) || (Area==S7AreaTM))
		{
			Address = LongStart;
			if (WordLen==S7WLBit)
				PDU.H[22]=S7WLBit;
			else {
				if (Area==S7AreaCT)
					PDU.H[22]=S7WLCounter;
				else
					PDU.H[22]=S7WLTimer;
			}
		}
		else
			Address = LongStart<<3;

		// Num elements
		PDU.H[23]=NumElements<<8;
		PDU.H[24]=NumElements;

		// Address into the PLC
        PDU.H[30] = Address & 0x000000FF;
        Address = Address >> 8;
		PDU.H[29] = Address & 0x000000FF;
        Address = Address >> 8;
        PDU.H[28] = Address & 0x000000FF;

		if (TCPClient->write(&PDU.H[0], Size_RD)==Size_RD)
		{
			RecvISOPacket(&Length);
			if (LastError==0)
			{
				if (Length>=18)
				{
					if ((Length-18==SizeRequested) && (PDU.H[31]==0xFF))
					{
						if (ptrData!=NULL)
							memcpy(Target, &PDU.DATA[0], SizeRequested); // Copies in the user's buffer
						Offset+=SizeRequested;
					}
					else
						LastError = errS7DataRead;
				}
				else
					LastError = errS7InvalidPDU;
			}
		}
		else
			LastError = errTCPDataSend;

		TotElements -= NumElements;
        LongStart += NumElements*WordSize;
	}
	return LastError;
}
//-----------------------------------------------------------------------------
int S7Client::ReadArea(int Area, uint16_t DBNumber, uint16_t Start, uint16_t Amount, void *ptrData)
{
	return ReadArea(Area, DBNumber, Start, Amount, S7WLByte, ptrData);
}
//-----------------------------------------------------------------------------
int S7Client::ReadBit(int Area, uint16_t DBNumber, uint16_t BitStart, bool &Bit)
{
	return ReadArea(Area, DBNumber, BitStart, 1, S7WLBit, &Bit);
}
//-----------------------------------------------------------------------------
int S7Client::WriteArea(int Area, uint16_t DBNumber, uint16_t Start, uint16_t Amount, int WordLen, void *ptrData)
{
	unsigned long Address;
	unsigned long LongStart=Start;
	uint16_t NumElements;
	uint16_t MaxElements;
	uint16_t TotElements;
	uint16_t DataSize;
	uint16_t IsoSize;
	uint16_t Length;

	pbyte Source;
	uintptr_t Offset = 0;
	int WordSize = 1;

	LastError=0;

	// If we are addressing Timers or counters the element size is 2
	if ((Area==S7AreaCT) || (Area==S7AreaTM))
		WordSize = 2;

	if (WordLen==S7WLBit) // Only one bit can be transferred in S7Protocol when WordLen==S7WLBit
		Amount=1;

    MaxElements=(PDULength-35) / WordSize; // 35 = Write telegram header
	TotElements=Amount;
	if (ptrData==NULL)
	{
		if (TotElements>MaxElements)
			TotElements=MaxElements;
	}

    while ((TotElements>0) && (LastError==0))
    {
        NumElements=TotElements;
        if (NumElements>MaxElements)
           NumElements=MaxElements;
		// If we use the internal buffer only, we cannot exced the PDU limit
		DataSize=NumElements*WordSize;
		IsoSize=Size_WR+DataSize;

		// Setup the telegram
		memcpy(&PDU.H, S7_RW, Size_WR);
		// Whole telegram Size
		PDU.H[2]=IsoSize>>8;
		PDU.H[3]=IsoSize & 0x00FF;
		// Data Length
		Length=DataSize+4;
		PDU.H[15]=Length>>8;
		PDU.H[16]=Length & 0x00FF;
		// Function
		PDU.H[17]=0x05;
		// Set DB Number
		PDU.H[27] = Area;
		if (Area==S7AreaDB)
		{
			PDU.H[25] = DBNumber>>8;
			PDU.H[26] = DBNumber & 0x00FF;
		}
		// Adjusts Start and word length
		if ((WordLen == S7WLBit) || (Area==S7AreaCT) || (Area==S7AreaTM))
		{
			Address = LongStart;
			Length = DataSize;
			if (WordLen==S7WLBit)
				PDU.H[22]=S7WLBit;
			else {
				if (Area==S7AreaCT)
					PDU.H[22]=S7WLCounter;
				else
					PDU.H[22]=S7WLTimer;
			}
		}
		else
		{
			Address = LongStart<<3;
			Length = DataSize<<3;
		}
		// Num elements
		PDU.H[23]=NumElements<<8;
		PDU.H[24]=NumElements;
		// Address into the PLC
        PDU.H[30] = Address & 0x000000FF;
        Address = Address >> 8;
		PDU.H[29] = Address & 0x000000FF;
        Address = Address >> 8;
        PDU.H[28] = Address & 0x000000FF;

		// Transport Size
		switch (WordLen)
		{
			case S7WLBit:
				PDU.H[32] = TS_ResBit;
				break;
			case S7WLCounter:
			case S7WLTimer:
				PDU.H[32] = TS_ResOctet;
				break;
			default:
				PDU.H[32] = TS_ResByte; // byte/word/dword etc.
				break;
		};

		// Length
		PDU.H[33]=Length>>8;
		PDU.H[34]=Length & 0x00FF;
		// Copy data
		Source=pbyte(ptrData)+Offset;
		if (ptrData!=NULL)
			memcpy(&PDU.RAW[35], Source, DataSize);

		if (TCPClient->write(&PDU.H[0], IsoSize)==IsoSize)
		{
			RecvISOPacket(&Length);
			if (LastError==0)
			{
				if (Length==15)
				{
					if ((PDU.H[27]!=0x00) || (PDU.H[28]!=0x00) || (PDU.H[31]!=0xFF))
						LastError = errS7DataWrite;
				}
				else
					LastError = errS7InvalidPDU;
			}
		}
		else
			LastError = errTCPDataSend;

		Offset+=DataSize;
		TotElements -= NumElements;
        LongStart += NumElements*WordSize;
	}
	return LastError;
}
//-----------------------------------------------------------------------------
int S7Client::WriteArea(int Area, uint16_t DBNumber, uint16_t Start, uint16_t Amount, void *ptrData)
{
	return WriteArea(Area, DBNumber, Start, Amount, S7WLByte, ptrData);
}
//-----------------------------------------------------------------------------
int S7Client::WriteBit(int Area, uint16_t DBNumber, uint16_t BitIndex, bool Bit)
{
	bool BitToWrite=Bit;
	return WriteArea(Area, DBNumber, BitIndex, 1, S7WLBit, &BitToWrite);
}
//-----------------------------------------------------------------------------
int S7Client::WriteBit(int Area, uint16_t DBNumber, uint16_t ByteIndex, uint16_t BitInByte, bool Bit)
{
	bool BitToWrite=Bit;
	return WriteArea(Area, DBNumber, ByteIndex*8+BitInByte, 1, S7WLBit, &BitToWrite);
}
//-----------------------------------------------------------------------------
#ifdef _EXTENDED

int S7Client::GetDBSize(uint16_t DBNumber, uint16_t *Size)
{
	uint16_t Length;
	LastError=0;
	*Size=0;
	// Setup the telegram
	memcpy(&PDU.H[0], S7_BI, sizeof(S7_BI));
	// Set DB Number
    PDU.RAW[31]=(DBNumber / 10000)+0x30;
    DBNumber=DBNumber % 10000;
    PDU.RAW[32]=(DBNumber / 1000)+0x30;
    DBNumber=DBNumber % 1000;
    PDU.RAW[33]=(DBNumber / 100)+0x30;
    DBNumber=DBNumber % 100;
    PDU.RAW[34]=(DBNumber / 10)+0x30;
    DBNumber=DBNumber % 10;
    PDU.RAW[35]=(DBNumber / 1)+0x30;
	if (TCPClient->write(&PDU.H[0], sizeof(S7_BI))==sizeof(S7_BI))
	{
		RecvISOPacket(&Length);
		if (LastError==0)
		{
			if (Length>25) // 26 is the minimum expected
			{
				if ((PDU.RAW[37]==0x00) && (PDU.RAW[38]==0x00) && (PDU.RAW[39]==0xFF))
				{
					*Size=PDU.RAW[83];
					*Size=(*Size<<8)+PDU.RAW[84];
				}
				else
					LastError = errS7Function;
			}
			else
				LastError = errS7InvalidPDU;
		}
	}
	else
		LastError = errTCPDataSend;

	return LastError;
}
//-----------------------------------------------------------------------------
int S7Client::DBGet(uint16_t DBNumber, void *ptrData, uint16_t *Size)
{
	uint16_t Length;
	int Result;

	Result=GetDBSize(DBNumber, &Length);
	if (Result==0)
	{
		if (Length<=*Size) // Check if the buffer supplied is big enough
		{
			Result=ReadArea(S7AreaDB, DBNumber, 0, Length, ptrData);
			if (Result==0)
				*Size=Length;
		}
		else
			Result=errBufferTooSmall;
	}

	return Result;
}
//-----------------------------------------------------------------------------
int S7Client::PlcStop()
{
	uint16_t Length;
	LastError=0;
	// Setup the telegram
	memcpy(&PDU.H, S7_STOP, sizeof(S7_STOP));
	if (TCPClient->write(&PDU.H[0], sizeof(S7_STOP))==sizeof(S7_STOP))
	{
		RecvISOPacket(&Length);
		if (LastError==0)
		{
			if (Length>12) // 13 is the minimum expected
			{
				if ((PDU.H[27]!=0x00) || (PDU.H[28]!=0x00))
					LastError = errS7Function;
			}
			else
				LastError = errS7InvalidPDU;
		}
	}
	else
		LastError = errTCPDataSend;

	return LastError;
}
//-----------------------------------------------------------------------------
int S7Client::PlcStart()
{
	uint16_t Length;
	LastError=0;
	// Setup the telegram
	memcpy(&PDU.H, S7_START, sizeof(S7_START));
	if (TCPClient->write(&PDU.H[0], sizeof(S7_START))==sizeof(S7_START))
	{
		RecvISOPacket(&Length);
		if (LastError==0)
		{
			if (Length>12) // 13 is the minimum expected
			{
				if ((PDU.H[27]!=0x00) || (PDU.H[28]!=0x00))
					LastError = errS7Function;
			}
			else
				LastError = errS7InvalidPDU;
		}
	}
	else
		LastError = errTCPDataSend;

	return LastError;
}
//-----------------------------------------------------------------------------
int S7Client::GetPlcStatus(int *Status)
{
	uint16_t Length;
	LastError=0;
	// Setup the telegram
	memcpy(&PDU.H, S7_PLCGETS, sizeof(S7_PLCGETS));
	if (TCPClient->write(&PDU.H[0], sizeof(S7_PLCGETS))==sizeof(S7_PLCGETS))
	{
		RecvISOPacket(&Length);
		if (LastError==0)
		{
			if (Length>53) // 54 is the minimum expected
			{
				switch (PDU.RAW[54])
				{
					case S7CpuStatusUnknown :
					case S7CpuStatusRun     :
					case S7CpuStatusStop    : *Status=PDU.RAW[54];
					break;
					default :
					// Since RUN status is always 0x08 for all CPUs and CPs, STOP status
					// sometime can be coded as 0x03 (especially for old cpu...)
						*Status=S7CpuStatusStop;
				}
			}
			else
				LastError = errS7InvalidPDU;
		}
	}
	else
		LastError = errTCPDataSend;

	return LastError;
}
//-----------------------------------------------------------------------------
int S7Client::IsoExchangeBuffer(uint16_t *Size)
{
	LastError=0;

	if (TCPClient->write(&PDU.H[0], int(Size))==*Size)
		RecvISOPacket(Size);
	else
		LastError = errTCPDataSend;

	return LastError;
}
//-----------------------------------------------------------------------------
void S7Client::ErrorText(int Error, char *Text, int TextLen)
{

}
#endif // _EXTENDED
//-----------------------------------------------------------------------------
