#include <climits>

#include "PrefixCode.h"

struct XADCodeTreeNode
{
	int branches[2];
};

struct XADCodeTableEntry
{
	uint32_t length;
	int32_t value;
};

static inline XADCodeTreeNode *NodePointer(XADPrefixCode *self, int node) { return &self->tree[node]; }
static inline int Branch(XADPrefixCode *self, int node, int bit) { return NodePointer(self, node)->branches[bit]; }
static inline void SetBranch(XADPrefixCode *self, int node, int bit, int nextnode) { NodePointer(self, node)->branches[bit] = nextnode; }

static inline int LeftBranch(XADPrefixCode *self, int node) { return Branch(self, node, 0); }
static inline int RightBranch(XADPrefixCode *self, int node) { return Branch(self, node, 1); }
static inline void SetLeftBranch(XADPrefixCode *self, int node, int nextnode) { SetBranch(self, node, 0, nextnode); }
static inline void SetRightBranch(XADPrefixCode *self, int node, int nextnode) { SetBranch(self, node, 1, nextnode); }

static inline int LeafValue(XADPrefixCode *self, int node) { return LeftBranch(self, node); }
static inline void SetLeafValue(XADPrefixCode *self, int node, int value) { SetLeftBranch(self, node, value); SetRightBranch(self, node, value); }

static inline void SetEmptyNode(XADPrefixCode *self, int node) { SetLeftBranch(self, node, -1); SetRightBranch(self, node, -2); }

static inline bool IsInvalidNode(XADPrefixCode *self, int node) { return node < 0; }
static inline bool IsOpenBranch(XADPrefixCode *self, int node, int bit) { return IsInvalidNode(self, Branch(self, node, bit)); }
static inline bool IsEmptyNode(XADPrefixCode *self, int node) { return LeftBranch(self, node) == -1 && RightBranch(self, node) == -2; }
static inline bool IsLeafNode(XADPrefixCode *self, int node) { return LeftBranch(self, node) == RightBranch(self, node); }

static inline int NewNode(XADPrefixCode *self)
{
	self->tree.resize(self->numentries + 1);
	SetEmptyNode(self, self->numentries);
	return self->numentries++;
}

bool CSInputNextSymbolUsingCode(CSInputBuffer *buf, XADPrefixCode *code, int &outSymbol)
{
	if (!code->table1)
		code->_makeTable();

	unsigned int bits;
	if (!CSInputPeekBitString(buf, code->tablesize, bits))
		return false;

	int length = code->table1[bits].length;
	int value = code->table1[bits].value;

	if (length < 0)
		return false;

	if (length <= code->tablesize)
	{
		if (!CSInputSkipPeekedBits(buf, length))
			return false;
		outSymbol = value;
		return true;
	}

	if (!CSInputSkipPeekedBits(buf, code->tablesize))
		return false;

	int node = value;
	while (!IsLeafNode(code, node))
	{
		unsigned int bit;
		if (!CSInputNextBit(buf, bit))
			return false;

		if (IsOpenBranch(code, node, bit))
			return false;
		node = Branch(code, node, bit);
	}
	outSymbol = LeafValue(code, node);
	return true;
}

bool CSInputNextSymbolUsingCodeLE(CSInputBuffer *buf, XADPrefixCode *code, int &outSymbol)
{
	if (!code->table2)
		code->_makeTableLE();

	unsigned int bits;
	if (!CSInputPeekBitStringLE(buf, code->tablesize, bits))
		return false;

	int length = code->table2[bits].length;
	int value = code->table2[bits].value;

	if (length < 0)
		return false;

	if (length <= code->tablesize)
	{
		if (!CSInputSkipPeekedBitsLE(buf, length))
			return false;
		outSymbol = value;
		return true;
	}

	if (!CSInputSkipPeekedBitsLE(buf, code->tablesize))
		return false;

	int node = value;
	while (!IsLeafNode(code, node))
	{
		unsigned int bit;
		if (!CSInputNextBitLE(buf, bit))
			return false;

		if (IsOpenBranch(code, node, bit))
			return false;
		node = Branch(code, node, bit);
	}
	outSymbol = LeafValue(code, node);
	return true;
}

/*int CSInputNextSymbolUsingCode(CSInputBuffer *buf,XADPrefixCode *code)
{
	int node=0;
	while(!IsLeafNode(code,node))
	{
		int bit=CSInputNextBit(buf);
		if(IsOpenBranch(code,node,bit)) [NSException raise:XADInvalidPrefixCodeException format:@"Invalid prefix code in bitstream"];
		node=Branch(code,node,bit);
	}
	return LeafValue(code,node);
}

int CSInputNextSymbolUsingCodeLE(CSInputBuffer *buf,XADPrefixCode *code)
{
	int node=0;
	while(!IsLeafNode(code,node))
	{
		int bit=CSInputNextBitLE(buf);
		if(IsOpenBranch(code,node,bit)) [NSException raise:XADInvalidPrefixCodeException format:@"Invalid prefix code in bitstream"];
		node=Branch(code,node,bit);
	}
	return LeafValue(code,node);
}*/


XADPrefixCode *XADPrefixCode::prefixCode()
{
	XADPrefixCode *self = new XADPrefixCode();
	self->init();
	return self;
}

XADPrefixCode *XADPrefixCode::prefixCodeWithLengths(const int *lengths, int numsymbols, int maxlength, bool zeros)
{
	XADPrefixCode *self = prefixCode();
	if (!self->initWithLengths(lengths, numsymbols, maxlength, zeros))
	{
		delete self;
		self = nullptr;
	}

	return self;
}

void XADPrefixCode::init()
{
	tree.resize(1);
	SetEmptyNode(this, 0);
	numentries = 1;
	minlength = INT_MAX;
	maxlength = INT_MIN;
	isstatic = false;

	table1 = table2 = NULL;
}

bool XADPrefixCode::initWithLengths(const int *lengths, int numsymbols, int maxcodelength, bool zeros)
{
	int code = 0, symbolsleft = numsymbols;

	for (int length = 1; length <= maxcodelength; length++)
	{
		for (int i = 0; i < numsymbols; i++)
		{
			if (lengths[i] != length)
				continue;

			// Instead of reversing to get a low-bit-first code, we shift and use high-bit-first.
			if (zeros)
			{
				if (!this->addValueHighBitFirst(i, code, length))
					return false;
			}
			else
			{
				if (!this->addValueHighBitFirst(i, ~code, length))
					return false;
			}
			code++;
			if (--symbolsleft == 0)
				return true; // early exit if all codes have been handled
		}
		code <<= 1;
	}

	return true;
}


XADPrefixCode::XADPrefixCode()
	: numentries(0)
	, minlength(0)
	, maxlength(0)
	, isstatic(false)
	, currnode(0)
	, tablesize(0)
	, table1(nullptr)
	, table2(nullptr)
{
}

XADPrefixCode::~XADPrefixCode()
{
	delete[] table1;
	delete[] table2;
}

bool XADPrefixCode::addValueHighBitFirst(int value, uint32_t code, int length)
{
	return this->addValueHighBitFirst(value, code, length, length);
}

bool XADPrefixCode::addValueHighBitFirst(int value, uint32_t code, int length, int repeatpos)
{
	if (isstatic)
		return false;

	delete[] table1;
	delete[] table2;
	table1 = table2 = NULL;

	if (length > maxlength) maxlength = length;
	if (length < minlength) minlength = length;

	repeatpos = length - 1 - repeatpos;
	if (repeatpos == 0 || (repeatpos >= 0 && (((code >> (repeatpos - 1)) & 3) == 0 || ((code >> (repeatpos - 1)) & 3) == 3)))
		return false;

	int lastnode = 0;
	for (int bitpos = length - 1; bitpos >= 0; bitpos--)
	{
		int bit = (code >> bitpos) & 1;

		if (IsLeafNode(this, lastnode))
			return false;

		if (bitpos == repeatpos)
		{
			if (!IsOpenBranch(this, lastnode, bit))
				return false;

			int repeatnode = NewNode(this);
			int nextnode = NewNode(this);

			SetBranch(this, lastnode, bit, repeatnode);
			SetBranch(this, repeatnode, bit, repeatnode);
			SetBranch(this, repeatnode, bit ^ 1, nextnode);
			lastnode = nextnode;

			bitpos++; // terminating bit already handled, skip it
		}
		else
		{
			if (IsOpenBranch(this, lastnode, bit))
				SetBranch(this, lastnode, bit, NewNode(this));
			lastnode = Branch(this, lastnode, bit);
		}

	}

	if (!IsEmptyNode(this, lastnode))
		return false;
	SetLeafValue(this, lastnode, value);

	return true;
}

static uint32_t Reverse32(uint32_t val)
{
	val = ((val >> 1) & 0x55555555) | ((val & 0x55555555) << 1);
	val = ((val >> 2) & 0x33333333) | ((val & 0x33333333) << 2);
	val = ((val >> 4) & 0x0F0F0F0F) | ((val & 0x0F0F0F0F) << 4);
	val = ((val >> 8) & 0x00FF00FF) | ((val & 0x00FF00FF) << 8);
	return (val >> 16) | (val << 16);
}

static uint32_t ReverseN(uint32_t val, int length)
{
	return Reverse32(val) >> (32 - length);
}

bool XADPrefixCode::addValueLowBitFirst(int value, uint32_t code, int length)
{
	return this->addValueHighBitFirst(value, ReverseN(code, length), length, length);
}

bool XADPrefixCode::addValueLowBitFirst(int value, uint32_t code, int length, int repeatpos)
{
	return this->addValueHighBitFirst(value, ReverseN(code, length), length, repeatpos);
}

void XADPrefixCode::startBuildingTree()
{
	currnode = 0;
	stack.resize(0);
}

void XADPrefixCode::startZeroBranch()
{
	int newNode = NewNode(this);
	SetBranch(this, currnode, 0, newNode);
	this->_pushNode();
	currnode = newNode;
}

void XADPrefixCode::startOneBranch()
{
	int newNode = NewNode(this);
	SetBranch(this, currnode, 1, newNode);
	this->_pushNode();
	currnode = newNode;
}

void XADPrefixCode::finishBranches()
{
	this->_popNode();
}

void XADPrefixCode::makeLeafWithValue(int value)
{
	SetLeafValue(this, currnode, value);
	this->_popNode();
}

void XADPrefixCode::_pushNode()
{
	stack.push_back(currnode);
}

void XADPrefixCode::_popNode()
{
	if (stack.size() == 0) return; // the final pop will underflow the stack otherwise
	int num = stack.back();
	stack.pop_back();
	currnode = num;
}

static void MakeTable(XADPrefixCode *code, int node, XADCodeTableEntry *table, int depth, int maxdepth)
{
	int currtablesize = 1 << (maxdepth - depth);

	if (IsLeafNode(code, node))
	{
		for (int i = 0; i < currtablesize; i++)
		{
			table[i].length = depth;
			table[i].value = LeafValue(code, node);
		}
	}
	else if (IsInvalidNode(code, node))
	{
		for (int i = 0; i < currtablesize; i++) table[i].length = -1;
	}
	else
	{
		if (depth == maxdepth)
		{
			table[0].length = maxdepth + 1;
			table[0].value = node;
		}
		else
		{
			MakeTable(code, LeftBranch(code, node), table, depth + 1, maxdepth);
			MakeTable(code, RightBranch(code, node), table + currtablesize / 2, depth + 1, maxdepth);
		}
	}
}

static void MakeTableLE(XADPrefixCode *code, int node, XADCodeTableEntry *table, int depth, int maxdepth)
{
	int currtablesize = 1 << (maxdepth - depth);
	int currstride = 1 << depth;

	if (IsLeafNode(code, node))
	{
		for (int i = 0; i < currtablesize; i++)
		{
			table[i*currstride].length = depth;
			table[i*currstride].value = LeafValue(code, node);
		}
	}
	else if (IsInvalidNode(code, node))
	{
		for (int i = 0; i < currtablesize; i++) table[i*currstride].length = -1;
	}
	else
	{
		if (depth == maxdepth)
		{
			table[0].length = maxdepth + 1;
			table[0].value = node;
		}
		else
		{
			MakeTableLE(code, LeftBranch(code, node), table, depth + 1, maxdepth);
			MakeTableLE(code, RightBranch(code, node), table + currstride, depth + 1, maxdepth);
		}
	}
}

#define TableMaxSize 10

void XADPrefixCode::_makeTable()
{
	if (table1) return;

	if (maxlength < minlength) tablesize = TableMaxSize; // no code lengths recorded
	else if (maxlength >= TableMaxSize) tablesize = TableMaxSize;
	else tablesize = maxlength;

	table1 = new XADCodeTableEntry[1 << tablesize];

	MakeTable(this, 0, table1, 0, tablesize);
}

void XADPrefixCode::_makeTableLE()
{
	if (table2) return;

	if (maxlength < minlength) tablesize = TableMaxSize; // no code lengths recorded
	else if (maxlength >= TableMaxSize) tablesize = TableMaxSize;
	else tablesize = maxlength;

	table2 = new XADCodeTableEntry[1 << tablesize];

	MakeTableLE(this, 0, table2, 0, tablesize);
}
