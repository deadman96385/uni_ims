/*****************************************************************
RSA类源文件：	RSA.cpp
版本：			0.31 (2009.5.27)
说明：			适用于MFC，128-1024位RSA运算
作者：			Allen Yu
*****************************************************************/

#include "rsa_sprd.h"
//允许生成1120位（二进制）的中间结果
//#define		BIGINT_MAXLEN 35
//允许生成2112位（二进制）的中间结果
#define		BIGINT_MAXLEN 66
#define		DEC 10
#define		HEX 16

int padding_add_PKCS1_type_1(unsigned char *to, int tlen,
	     const unsigned char *from, int flen);
int padding_check_PKCS1_type_1(unsigned char *to, int tlen,
	     const unsigned char *from, int flen, int num);
int padding_add_PKCS1_type_2(unsigned char *to, int tlen,
	     const unsigned char *from, int flen);
int padding_check_PKCS1_type_2(unsigned char *to, int tlen,
	     const unsigned char *from, int flen, int num);
//	大整数结构
typedef struct
{
	//	大数在0x1 0000 0000进制(2^32)下的长度
	int	m_iLength32;

	//	用数组记录大数在0x1 0000 0000进制下每一位的值
	unsigned int	m_ulValue[BIGINT_MAXLEN];
} SBigInt;


SBigInt	RSA_C,RSA_M,RSA_N,RSA_D,RSA_E;

SBigInt	RSA_P,RSA_Q;			//	调试方便放着，完成后放protected

//	小素数表
const static int PrimeTable[550]=
{   3,    5,    7,    11,   13,   17,   19,   23,   29,   31,
    37,   41,   43,   47,   53,   59,   61,   67,   71,   73,
    79,   83,   89,   97,   101,  103,  107,  109,  113,  127,
    131,  137,  139,  149,  151,  157,  163,  167,  173,  179,
    181,  191,  193,  197,  199,  211,  223,  227,  229,  233,
    239,  241,  251,  257,  263,  269,  271,  277,  281,  283,
    293,  307,  311,  313,  317,  331,  337,  347,  349,  353,
    359,  367,  373,  379,  383,  389,  397,  401,  409,  419,
    421,  431,  433,  439,  443,  449,  457,  461,  463,  467,
    479,  487,  491,  499,  503,  509,  521,  523,  541,  547,
    557,  563,  569,  571,  577,  587,  593,  599,  601,  607,
    613,  617,  619,  631,  641,  643,  647,  653,  659,  661,
    673,  677,  683,  691,  701,  709,  719,  727,  733,  739,
    743,  751,  757,  761,  769,  773,  787,  797,  809,  811,
    821,  823,  827,  829,  839,  853,  857,  859,  863,  877,
    881,  883,  887,  907,  911,  919,  929,  937,  941,  947,
    953,  967,  971,  977,  983,  991,  997,  1009, 1013, 1019,
    1021, 1031, 1033, 1039, 1049, 1051, 1061, 1063, 1069, 1087,
    1091, 1093, 1097, 1103, 1109, 1117, 1123, 1129, 1151, 1153,
    1163, 1171, 1181, 1187, 1193, 1201, 1213, 1217, 1223, 1229,
    1231, 1237, 1249, 1259, 1277, 1279, 1283, 1289, 1291, 1297,
    1301, 1303, 1307, 1319, 1321, 1327, 1361, 1367, 1373, 1381,
    1399, 1409, 1423, 1427, 1429, 1433, 1439, 1447, 1451, 1453,
    1459, 1471, 1481, 1483, 1487, 1489, 1493, 1499, 1511, 1523,
    1531, 1543, 1549, 1553, 1559, 1567, 1571, 1579, 1583, 1597,
    1601, 1607, 1609, 1613, 1619, 1621, 1627, 1637, 1657, 1663,
    1667, 1669, 1693, 1697, 1699, 1709, 1721, 1723, 1733, 1741,
    1747, 1753, 1759, 1777, 1783, 1787, 1789, 1801, 1811, 1823,
    1831, 1847, 1861, 1867, 1871, 1873, 1877, 1879, 1889, 1901,
    1907, 1913, 1931, 1933, 1949, 1951, 1973, 1979, 1987, 1993,
    1997, 1999, 2003, 2011, 2017, 2027, 2029, 2039, 2053, 2063,
    2069, 2081, 2083, 2087, 2089, 2099, 2111, 2113, 2129, 2131,
    2137, 2141, 2143, 2153, 2161, 2179, 2203, 2207, 2213, 2221,
    2237, 2239, 2243, 2251, 2267, 2269, 2273, 2281, 2287, 2293,
    2297, 2309, 2311, 2333, 2339, 2341, 2347, 2351, 2357, 2371,
    2377, 2381, 2383, 2389, 2393, 2399, 2411, 2417, 2423, 2437,
    2441, 2447, 2459, 2467, 2473, 2477, 2503, 2521, 2531, 2539,
    2543, 2549, 2551, 2557, 2579, 2591, 2593, 2609, 2617, 2621,
    2633, 2647, 2657, 2659, 2663, 2671, 2677, 2683, 2687, 2689,
    2693, 2699, 2707, 2711, 2713, 2719, 2729, 2731, 2741, 2749,
    2753, 2767, 2777, 2789, 2791, 2797, 2801, 2803, 2819, 2833,
    2837, 2843, 2851, 2857, 2861, 2879, 2887, 2897, 2903, 2909,
    2917, 2927, 2939, 2953, 2957, 2963, 2969, 2971, 2999, 3001,
    3011, 3019, 3023, 3037, 3041, 3049, 3061, 3067, 3079, 3083,
    3089, 3109, 3119, 3121, 3137, 3163, 3167, 3169, 3181, 3187,
    3191, 3203, 3209, 3217, 3221, 3229, 3251, 3253, 3257, 3259,
    3271, 3299, 3301, 3307, 3313, 3319, 3323, 3329, 3331, 3343,
    3347, 3359, 3361, 3371, 3373, 3389, 3391, 3407, 3413, 3433,
    3449, 3457, 3461, 3463, 3467, 3469, 3491, 3499, 3511, 3517,
    3527, 3529, 3533, 3539, 3541, 3547, 3557, 3559, 3571, 3581,
    3583, 3593, 3607, 3613, 3617, 3623, 3631, 3637, 3643, 3659,
    3671, 3673, 3677, 3691, 3697, 3701, 3709, 3719, 3727, 3733,
    3739, 3761, 3767, 3769, 3779, 3793, 3797, 3803, 3821, 3823,
    3833, 3847, 3851, 3853, 3863, 3877, 3881, 3889, 3907, 3911,
    3917, 3919, 3923, 3929, 3931, 3943, 3947, 3967, 3989, 4001
};

//	构造RSA对象并初始化为零
void RSA_initialize()
{
	int i;

    for(i=0;i<BIGINT_MAXLEN;i++)
	{
		RSA_C.m_ulValue[i]=0;
		RSA_D.m_ulValue[i]=0;
		RSA_E.m_ulValue[i]=0;
		RSA_M.m_ulValue[i]=0;
		RSA_N.m_ulValue[i]=0;
		RSA_P.m_ulValue[i]=0;
		RSA_Q.m_ulValue[i]=0;
	}

	RSA_C.m_iLength32=1;
	RSA_D.m_iLength32=1;
	RSA_E.m_iLength32=1;
	RSA_M.m_iLength32=1;
	RSA_N.m_iLength32=1;
	RSA_P.m_iLength32=1;
	RSA_Q.m_iLength32=1;
}

//	解构RSA对象
void RSA_deinitialize()
{
}

/****************************************************************************************
大数比较
调用方式：	两比较值
返回值：	若In<In2返回-1；若=返回0；若>返回1
****************************************************************************************/
int		Cmp(SBigInt *sIn, SBigInt *sIn2)
{
	int	i;

    if(sIn->m_iLength32>sIn2->m_iLength32)
		return 1;
    if(sIn->m_iLength32<sIn2->m_iLength32)
		return -1;

    for(i=sIn->m_iLength32-1;i>=0;i--)
    {
        if(sIn->m_ulValue[i]>sIn2->m_ulValue[i])
			return 1;
        if(sIn->m_ulValue[i]<sIn2->m_ulValue[i])
			return -1;
    }

    return 0;
}

/****************************************************************************************
大数赋值
调用方式：	后一参数赋值给前一参数
返回值：	无
****************************************************************************************/
void	Mov_bigint(SBigInt *sOut, SBigInt *sIn)
{
	int	i;

//	if(*sOut==*sIn)
//		return;

    sOut->m_iLength32=sIn->m_iLength32;

    for(i=0;i<BIGINT_MAXLEN;i++)
		sOut->m_ulValue[i]=sIn->m_ulValue[i];

	return;
}

void	Mov_long(SBigInt *sOut, unsigned long A)
{
	int	i;

	if(A>0xffffffff)
    {
        sOut->m_iLength32=2;
        sOut->m_ulValue[1]=(unsigned long)(A>>32);
        sOut->m_ulValue[0]=(unsigned long)A;
    }
    else
    {
        sOut->m_iLength32=1;
        sOut->m_ulValue[0]=(unsigned long)A;
    }

	for(i=sOut->m_iLength32;i<BIGINT_MAXLEN;i++)
		sOut->m_ulValue[i]=0;
}

/****************************************************************************************
大数相加
调用形式：	后两参数想加赋值给前一参数，输出对象可以是第一个输入对象本身
返回值：	无
****************************************************************************************/
//	sOut可同SIn
void	Add_bigint(SBigInt *sOut, SBigInt *sIn, SBigInt *sIn2)
{
    unsigned			carry=0;
    unsigned long	sum=0;
	int					i;

    if(sIn->m_iLength32<sIn2->m_iLength32)
		sOut->m_iLength32=sIn2->m_iLength32;
	else
		sOut->m_iLength32=sIn->m_iLength32;

    for(i=0;i<sOut->m_iLength32;i++)
    {
		sum=sIn->m_ulValue[i];
		sum=sum+sIn2->m_ulValue[i]+carry;
        sOut->m_ulValue[i]=(unsigned long)sum;
        carry=(unsigned)(sum>>32);
    }

    sOut->m_ulValue[sOut->m_iLength32]=carry;
    sOut->m_iLength32+=carry;

	for(i=sOut->m_iLength32;i<BIGINT_MAXLEN;i++)
		sOut->m_ulValue[i]=0;

    return;
}

//	sOut可同SIn
void	Add_long(SBigInt *sOut, SBigInt *sIn, unsigned long ulIn)
{
    unsigned long	sum;
	int					i;

	Mov_bigint(sOut,sIn);

    sum=sIn->m_ulValue[0];
	sum+=ulIn;
    sOut->m_ulValue[0]=(unsigned long)sum;
    if(sum>0xffffffff)
    {
        i=1;
        while(sIn->m_ulValue[i]==0xffffffff)
		{
			sOut->m_ulValue[i]=0;
			i++;
		}
        sOut->m_ulValue[i]++;
        if(sOut->m_iLength32==i)
			sOut->m_iLength32++;
    }

    return;
}

/****************************************************************************************
大数相减
调用形式：	后两参数想减赋值给前一参数，输出对象可以是第一个输入对象本身
返回值：	无
****************************************************************************************/
//	sOut可同SIn
void	Sub_bigint(SBigInt *sOut, SBigInt *sIn, SBigInt *sIn2)
{
    unsigned			carry=0;
    unsigned long	num;
	int					i;

    if(Cmp(sIn,sIn2)<=0)
	{
		Mov_long(sOut,0);
		return;
	}
//	else
//		Mov(sOut,sIn);
	sOut->m_iLength32=sIn->m_iLength32;

    for(i=0;i<sIn->m_iLength32;i++)
    {
        if((sIn->m_ulValue[i]>sIn2->m_ulValue[i])||((sIn->m_ulValue[i]==sIn2->m_ulValue[i])&&(carry==0)))
        {
            sOut->m_ulValue[i]=sIn->m_ulValue[i]-carry-sIn2->m_ulValue[i];
            carry=0;
        }
        else
        {
            num=0x100000000+sIn->m_ulValue[i];
            sOut->m_ulValue[i]=(unsigned long)(num-carry-sIn2->m_ulValue[i]);
            carry=1;
        }
    }
    while(sOut->m_ulValue[sOut->m_iLength32-1]==0)
		sOut->m_iLength32--;

	for(i=sOut->m_iLength32;i<BIGINT_MAXLEN;i++)
		sOut->m_ulValue[i]=0;

    return;
}

//	sOut可同SIn
void	Sub_long(SBigInt *sOut, SBigInt *sIn, unsigned long ulIn)
{
	unsigned long num;
	int i;

	sOut->m_iLength32=sIn->m_iLength32;

    if(sIn->m_ulValue[0]>=ulIn)
	{
		sOut->m_ulValue[0]=sIn->m_ulValue[0]-ulIn;

		for(i=1;i<BIGINT_MAXLEN;i++)
			sOut->m_ulValue[i]=sIn->m_ulValue[i];

		return;
	}

    if(sIn->m_iLength32==1)
	{
		Mov_long(sOut,0);
		return;
	}

	num=0x100000000+sIn->m_ulValue[0];
    sOut->m_ulValue[0]=(unsigned long)(num-ulIn);
    i=1;
    while(sIn->m_ulValue[i]==0)
	{
		sOut->m_ulValue[i]=0xffffffff;
		i++;
	}
    sOut->m_ulValue[i]=sIn->m_ulValue[i]-1;
    if(sOut->m_ulValue[i]==0)
		sOut->m_iLength32--;

	for(i=sOut->m_iLength32;i<BIGINT_MAXLEN;i++)
		sOut->m_ulValue[i]=0;

    return;
}

//	sOut可同SIn
void Mul_long(SBigInt *sOut, SBigInt *sIn, unsigned long ulIn)
{
    unsigned long	mul;
    unsigned long		carry;
	int					i;

	carry=0;
	sOut->m_iLength32=sIn->m_iLength32;

    for(i=0;i<sOut->m_iLength32;i++)
    {
        mul=sIn->m_ulValue[i];
		mul=mul*ulIn+carry;
        sOut->m_ulValue[i]=(unsigned long)mul;
        carry=(unsigned long)(mul>>32);
    }

    if(carry)
	{
		sOut->m_ulValue[sOut->m_iLength32]=carry;
		sOut->m_iLength32++;
	}

	for(i=sOut->m_iLength32;i<BIGINT_MAXLEN;i++)
		sOut->m_ulValue[i]=0;

    return;
}

/****************************************************************************************
大数相乘
调用形式：	后两参数想乘赋值给前一参数，输出对象"不"!!!!可以是第一个输入对象
返回值：	无
****************************************************************************************/
//	sOut不可同SIn !!!!!!!!
void Mul_bigint(SBigInt *sOut, SBigInt *sIn, SBigInt *sIn2)
{
	unsigned long	sum,mul=0,carry=0;
	int					i,j,k;

    if(sIn2->m_iLength32==1)
	{
		Mul_long(sOut,sIn,sIn2->m_ulValue[0]);
		return;
	}

	sOut->m_iLength32=sIn->m_iLength32+sIn2->m_iLength32-1;
    for(i=0;i<sOut->m_iLength32;i++)
	{
		sum=carry;
		carry=0;
		for(j=0;j<sIn2->m_iLength32;j++)
		{
			k=i-j;
            if((k>=0)&&(k<sIn->m_iLength32))
			{
				mul=sIn->m_ulValue[k];
				mul=mul*sIn2->m_ulValue[j];
			    carry+=mul>>32;
				mul=mul&0xffffffff;
				sum+=mul;
			}
        }
		carry+=sum>>32;
		sOut->m_ulValue[i]=(unsigned long)sum;
	}
	if(carry)
	{
		sOut->m_iLength32++;
		sOut->m_ulValue[sOut->m_iLength32-1]=(unsigned long)carry;
	}

	for(i=sOut->m_iLength32;i<BIGINT_MAXLEN;i++)
		sOut->m_ulValue[i]=0;

    return;
}


//	sOut可同SIn
void	Div_long(SBigInt *sOut, SBigInt *sIn, unsigned long ulIn)
{
    unsigned long	div,mul;
    unsigned long		carry=0;
	int					i;

    sOut->m_iLength32=sIn->m_iLength32;

    if(sIn->m_iLength32==1)
	{
		sOut->m_ulValue[0]=sIn->m_ulValue[0]/ulIn;
		return;
	}

    for(i=sIn->m_iLength32-1;i>=0;i--)
    {
        div=carry;
        div=(div<<32)+sIn->m_ulValue[i];
        sOut->m_ulValue[i]=(unsigned long)(div/ulIn);
        mul=(div/ulIn)*ulIn;
        carry=(unsigned long)(div-mul);
    }
    if(sOut->m_ulValue[sOut->m_iLength32-1]==0)
		sOut->m_iLength32--;

	for(i=sOut->m_iLength32;i<BIGINT_MAXLEN;i++)
		sOut->m_ulValue[i]=0;

    return;
}

/****************************************************************************************
大数相除
调用形式：	后两参数想除赋值给前一参数，输出对象"不"!!!!可以是第一个输入对象
返回值：	无
****************************************************************************************/
//	sOut不可同SIn!!!!!!!!
void	Div_bigint(SBigInt *sOut, SBigInt *sIn, SBigInt *sIn2)
{
    unsigned			i,len;
    unsigned long	num,div;
	SBigInt				sTemp1,sTemp2,sTemp3;

	if(sIn2->m_iLength32==1)
	{
		Div_long(sOut, sIn, sIn2->m_ulValue[0]);
		return;
	}

    Mov_bigint(&sTemp1,sIn);
	Mov_long(sOut,0);

    while(Cmp(&sTemp1,sIn2)>=0)
    {
		div=sTemp1.m_ulValue[sTemp1.m_iLength32-1];
		num=sIn2->m_ulValue[sIn2->m_iLength32-1];
		len=sTemp1.m_iLength32-sIn2->m_iLength32;
		if((div==num)&&(len==0))
		{
			Add_long(sOut,sOut,1);
			break;
		}
		if((div<=num)&&len)
		{
			len--;
			div=(div<<32)+sTemp1.m_ulValue[sTemp1.m_iLength32-2];
		}
		div=div/(num+1);
		Mov_long(&sTemp2,div);
		if(len)
		{
			sTemp2.m_iLength32+=len;
			for(i=sTemp2.m_iLength32-1;i>=len;i--)
				sTemp2.m_ulValue[i]=sTemp2.m_ulValue[i-len];
			for(i=0;i<len;i++)
				sTemp2.m_ulValue[i]=0;
		}
		Add_bigint(sOut,sOut,&sTemp2);
		Mul_bigint(&sTemp3,sIn2,&sTemp2);
		Sub_bigint(&sTemp1,&sTemp1,&sTemp3);
    }
    return;
}


/****************************************************************************************
大数求模
调用形式：	后两参数求余赋值给前一参数，输出对象可以是第一个输入对象本身
返回值：	无或者unsigned long
****************************************************************************************/
//	sOut可同SIn
void Mod_bigint(SBigInt *sOut, SBigInt *sIn, SBigInt *sIn2)
{
    SBigInt				sTemp1,sTemp2;
	unsigned long	div,num;
    unsigned long		carry=0;
	unsigned			i,len;

    Mov_bigint(sOut,sIn);
    while(Cmp(sOut,sIn2)>=0)
    {
		div=sOut->m_ulValue[sOut->m_iLength32-1];
		num=sIn2->m_ulValue[sIn2->m_iLength32-1];
		len=sOut->m_iLength32-sIn2->m_iLength32;
		if((div==num)&&(len==0))
		{
			Sub_bigint(sOut,sOut,sIn2);
			break;
		}
		if((div<=num)&&len)
		{
			len--;
			div=(div<<32)+sOut->m_ulValue[sOut->m_iLength32-2];
		}
		div=div/(num+1);
		Mov_long(&sTemp1,div);
		Mul_bigint(&sTemp2,sIn2,&sTemp1);
		if(len)
		{
			sTemp2.m_iLength32+=len;
			for(i=sTemp2.m_iLength32-1;i>=len;i--)
				sTemp2.m_ulValue[i]=sTemp2.m_ulValue[i-len];
			for(i=0;i<len;i++)
				sTemp2.m_ulValue[i]=0;
		}
		Sub_bigint(sOut,sOut,&sTemp2);
    }

    return;
}

//	sOut可同SIn
unsigned long	Mod_long(SBigInt *sIn, unsigned long ulIn)
{
    unsigned long	div;
    unsigned long		carry=0;
	int					i;

    if(sIn->m_iLength32==1)
		return sIn->m_ulValue[0]%ulIn;

    for(i=sIn->m_iLength32-1;i>=0;i--)
    {
        div=sIn->m_ulValue[i]+carry*0x100000000;
        carry=(unsigned long)(div%ulIn);
    }

    return carry;
}

/****************************************************************************************
从字符串按10进制或16进制格式输入到大数
调用格式：	后字符串参数赋值给前一SBigInt参数，字符串格式只能为10或16
返回值：	无
****************************************************************************************/
/*void SetParameter_char(SBigInt *sOut, unsigned char * str, int system)*/
/*{*/
    /*int		i,strLen,k;*/
	/*SBigInt	sTemp;*/

	/*strLen=strlen((char *)str);//str.GetLength();*/

	/*for(i=0;i<BIGINT_MAXLEN;i++)*/
		/*sOut->m_ulValue[i]=0;*/

    /*for(i=0;i<strLen;i++)*/
    /*{*/
       /*Mul_long(&sTemp,sOut,system);*/

       /*if((str[i]>='0')&&(str[i]<='9'))*/
		   /*k=str[i]-48;*/
       /*else if((str[i]>='A')&&(str[i]<='F'))*/
		   /*k=str[i]-55;*/
       /*else if((str[i]>='a')&&(str[i]<='f'))*/
		   /*k=str[i]-87;*/
       /*else k=0;*/

	   /*Add_long(sOut,&sTemp,k);*/
    /*}*/

	/*return;*/
/*}*/

void SetParameter_int(SBigInt *sOut, unsigned char ulIn[], int iLength32)
{
    int		i;

    memcpy(sOut->m_ulValue,ulIn,iLength32*sizeof(unsigned int));
	/*for(i=0;i<iLength32;i++)*/
		/*sOut->m_ulValue[i]=ulIn[i];*/


	for(i=iLength32;i<BIGINT_MAXLEN;i++)
		sOut->m_ulValue[i]=0;

	sOut->m_iLength32=iLength32;

	return;
}
void SetParameter_char_openssl(SBigInt *sOut, unsigned char * str, int strLen)
{
       invert_char(str,strLen);
       SetParameter_int(sOut, str,strLen>>2);
	 invert_char(str,strLen);//recover the original input
	return;
}

/****************************************************************************************
将大数按10进制或16进制格式输出为字符串
调用格式：	把大数赋值给字符串，字符串格式只能为10或16
返回值：	无，参数str被赋值为N的sys进制字符串
****************************************************************************************/
void Put(unsigned char * str,int str_buflen, SBigInt *sOut, unsigned int system)
{
    unsigned char 		t[16]={'0','1','2','3','4','5','6','7','8','9','A','B','C','D','E','F'};
    int			a;
    unsigned char		ch;
	SBigInt		sTemp,sTemp2;

	int index=str_buflen-1;

    if((sOut->m_iLength32==1)&&(sOut->m_ulValue[0]==0))
	{
		str[0]='0';
		str[1]='\0';
		return;
	}

	str[index]='\0';

    Mov_bigint(&sTemp,sOut);

    while(sTemp.m_ulValue[sTemp.m_iLength32-1]>0)
    {
		a=Mod_long(&sTemp,system);
        ch=t[a];
        //str.Insert(0,ch);
        //start insert to head
        index-=1;
	  str[index]=ch;
	 //end insert to head

		Div_long(&sTemp2,&sTemp,system);
		Mov_bigint(&sTemp,&sTemp2);
    }

    if (index>0)
    {
        int i=0;
	  while (index<str_buflen)
		str[i++]=str[index++];
    }
}

/****************************************************************************************
求M指数D对N的模
调用方式：	第一个为输出，不能和输入相同
返回值：	sOut=sIn^sE MOD sN
****************************************************************************************/
void	RsaTrans(SBigInt *sOut, SBigInt *sIn, SBigInt *sE, SBigInt *sN)
{
	int				i,j,k;
	int				n;
	unsigned long	num;

	SBigInt			sTemp1,sTemp2;

	//	k为指数的bit数
	k=sE->m_iLength32*32-32;
	num=sE->m_ulValue[sE->m_iLength32-1];

	while(num)
	{
		num=num>>1;
		k++;
	}

	Mov_bigint(sOut,sIn);

	for(i=k-2;i>=0;i--)
	{
		Mul_long(&sTemp1, sOut, sOut->m_ulValue[sOut->m_iLength32-1]);
		Mod_bigint(&sTemp1,&sTemp1, sN);

        for(n=1;n<sOut->m_iLength32;n++)
		{

			for(j=sTemp1.m_iLength32;j>0;j--)
				sTemp1.m_ulValue[j]=sTemp1.m_ulValue[j-1];

			sTemp1.m_ulValue[0]=0;
			sTemp1.m_iLength32++;

			Mul_long(&sTemp2,sOut,sOut->m_ulValue[sOut->m_iLength32-n-1]);
			Add_bigint(&sTemp1,&sTemp1,&sTemp2);
			Mod_bigint(&sTemp1,&sTemp1,sN);
		}

		Mov_bigint(sOut,&sTemp1);

		if((sE->m_ulValue[i>>5]>>(i&31))&1)
		{
			Mul_long(&sTemp1,sIn,sOut->m_ulValue[sOut->m_iLength32-1]);
			Mod_bigint(&sTemp1,&sTemp1,sN);

            for(n=1;n<sOut->m_iLength32;n++)
			{
			    for(j=sTemp1.m_iLength32;j>0;j--)
					sTemp1.m_ulValue[j]=sTemp1.m_ulValue[j-1];

			    sTemp1.m_ulValue[0]=0;
			    sTemp1.m_iLength32++;

				Mul_long(&sTemp2,sIn,sOut->m_ulValue[sOut->m_iLength32-n-1]);
				Add_bigint(&sTemp1,&sTemp1,&sTemp2);
				Mod_bigint(&sTemp1,&sTemp1,sN);
			}
			Mov_bigint(sOut,&sTemp1);
		}
	}
    return;
}

/****************************************************************************************
加密
调用方式：	无
返回值：	C=M^E MOD N, 成功返回1，否则非1值
****************************************************************************************/
int	prv_Encrypt()
{
	if(Cmp(&RSA_M,&RSA_N)>=0)
	{
		return	-1;
	}

	RsaTrans(&RSA_C,& RSA_M,&RSA_E, &RSA_N);

    return 1;
}

/****************************************************************************************
解密
调用方式：	无
返回值：	M=C^D MOD N， 成功返回1，否则非1值
****************************************************************************************/
int	pub_Decrypt()
{
	if(Cmp(&RSA_C,&RSA_N)>=0)
	{
		return	-1;
	}

	RsaTrans(&RSA_M, &RSA_C, &RSA_D, &RSA_N);

	return 1;
}

/****************************************************************************************
加密
调用方式：	无
返回值：	C=M^E MOD N, 成功返回1，否则非1值
****************************************************************************************/
int	pub_Encrypt()
{
	if(Cmp(&RSA_M,&RSA_N)>=0)
	{
		return	-1;
	}

	RsaTrans(&RSA_C,& RSA_M,&RSA_D, &RSA_N);

    return 1;
}

/****************************************************************************************
解密
调用方式：	无
返回值：	M=C^D MOD N， 成功返回1，否则非1值
****************************************************************************************/
int	prv_Decrypt()
{
	if(Cmp(&RSA_C,&RSA_N)>=0)
	{
		return	-1;
	}

	RsaTrans(&RSA_M, &RSA_C, &RSA_E, &RSA_N);

	return 1;
}
bool  RSA_PrvEnc(unsigned char* prv_E, unsigned char* mod_N,int  bitLen_N, unsigned char *from,int flen, unsigned char*to )
{
   int byteLen_N=bitLen_N>>3;
   unsigned char textpadded[byteLen_N];
       RSA_initialize();
       if (prv_E!=NULL)
	     SetParameter_char_openssl(&RSA_E,prv_E,byteLen_N);
	if (mod_N!=NULL)
	     SetParameter_char_openssl(&RSA_N,mod_N,byteLen_N);


        if (padding_add_PKCS1_type_1(textpadded, byteLen_N,
	     from, flen)<=0)
	     return false;
	 //RSA_invert_char(textpadded,byteLen_N);

	 SetParameter_char_openssl(&RSA_M,textpadded,byteLen_N);

	 if (prv_Encrypt()<0)
	 	return false;

        /*unsigned int *cipher_int=to;*/

        memcpy(to,(unsigned char *)RSA_C.m_ulValue,RSA_C.m_iLength32*sizeof(unsigned int));

        invert_char(to,byteLen_N);
	  return true;
}

int  RSA_PubDec(unsigned char* pub_D,unsigned char* mod_N,int  bitLen_N,unsigned char *from,unsigned char *to)
{
      int byteLen_N=bitLen_N>>3;
      unsigned char text[byteLen_N];
	/*unsigned int * text_int=text;*/
       RSA_initialize();
       if (pub_D!=NULL)
	     SetParameter_char_openssl(&RSA_D,pub_D,4);
	if (mod_N!=NULL)
	     SetParameter_char_openssl(&RSA_N,mod_N,byteLen_N);

        SetParameter_char_openssl(&RSA_C,from,byteLen_N);

        pub_Decrypt();

        memcpy(text, (unsigned char *)RSA_M.m_ulValue, RSA_M.m_iLength32*sizeof(unsigned int));
        /*for (int i=0;i<RSA_M.m_iLength32;i++)*/
        /*{*/
            /*text_int[i]=RSA_M.m_ulValue[i];*/
        /*}*/

	  invert_char(text,byteLen_N);

         return padding_check_PKCS1_type_1(to,byteLen_N,
	     text+1, byteLen_N-1,byteLen_N);
}
bool  RSA_PubEnc(unsigned char* pub_D, unsigned char* mod_N,int  bitLen_N, unsigned char *from,int flen, unsigned char*to )
{
   int byteLen_N=bitLen_N>>3;
   unsigned char textpadded[byteLen_N];
	RSA_initialize();
       if (pub_D!=NULL)
       {
	     SetParameter_char_openssl(&RSA_D,pub_D,4);
       }
	if (mod_N!=NULL)
	{
	     SetParameter_char_openssl(&RSA_N,mod_N,byteLen_N);
	}
        if (padding_add_PKCS1_type_2(textpadded, byteLen_N,
	     from, flen)<=0)
	     return false;

	 SetParameter_char_openssl(&RSA_M,textpadded,byteLen_N);

	 if (pub_Encrypt()<0)
	 	return false;

        memcpy(to,(unsigned char *)RSA_C.m_ulValue,RSA_C.m_iLength32*sizeof(unsigned int));
        /*unsigned int *cipher_int=to;*/
        /*printf("cipher_int=%p\n",cipher_int);*/

        /*for (int i=0;i<RSA_C.m_iLength32;i++)*/
        /*{*/
        /*printf("cipher_int=%p\n",cipher_int);*/
            /*cipher_int[i]=RSA_C.m_ulValue[i];*/
        /*}*/

        invert_char(to,byteLen_N);
	  return true;
}
int RSA_PrvDec(unsigned char* prv_E,unsigned char* mod_N,int  bitLen_N, unsigned char *from,unsigned char *to)
{
      int byteLen_N=bitLen_N>>3;
      unsigned char text[byteLen_N];
	//unsigned int * text_int=text;
	RSA_initialize();
       if (prv_E!=NULL)
       {
	     SetParameter_char_openssl(&RSA_E,prv_E,byteLen_N);
       }

	if (mod_N!=NULL)
	{
	     SetParameter_char_openssl(&RSA_N,mod_N,byteLen_N);
	}

        SetParameter_char_openssl(&RSA_C,from,byteLen_N);

        prv_Decrypt();

        memcpy(text, (unsigned char *)RSA_M.m_ulValue, RSA_M.m_iLength32*sizeof(unsigned int));
        /*for (int i=0;i<RSA_M.m_iLength32;i++)*/
        /*{*/
            /*text_int[i]=RSA_M.m_ulValue[i];*/
        /*}*/

	  invert_char(text,byteLen_N);

         return padding_check_PKCS1_type_2(to,byteLen_N,
	     text+1, byteLen_N-1,byteLen_N);
}


/****************************************************************************************
求随机数
调用方式：	无
返回值：	unsigned long值
****************************************************************************************/ 
#define RAND_MAX ((1U << 31) - 1)
static int g_seed = 0;
static int read_reg = 1;

int myrand()
{
    if(read_reg)
    {
        g_seed = 504723;
        if(g_seed>0) read_reg = 0;
    }
return g_seed = (g_seed * 1103515245 + 12345) & RAND_MAX;
}

unsigned long  Rand()
{
    int ulNum;
    ulNum=myrand()*0x10000+myrand();
    return ulNum;
}

/****************************************************************************************
拉宾米勒算法测试素数
调用方式：	输入一个大数
返回值：	若sIn为素数，返回TRUE，否则返回FALSE
****************************************************************************************/
bool Rab(SBigInt *sIn)
{
	int				i,j,iPassMark;
	SBigInt			sTempDiv,sN_1,sTempA,sTemp1;

    for(i=0;i<550;i++)
	{
		if(Mod_long(sIn,PrimeTable[i])==0)
			return false;
	}

    Sub_long(&sN_1, sIn, 1);

	sTemp1.m_iLength32=0;

    for(i=0;i<8;i++)
    {

        iPassMark=0;
		Mov_long(&sTempA,myrand()*myrand());
		Mov_bigint(&sTempDiv,&sN_1);

        while((sTempDiv.m_ulValue[0]&1)==0)
		{
            for(j=0;j<sTempDiv.m_iLength32;j++)
			{
			    sTempDiv.m_ulValue[j]=sTempDiv.m_ulValue[j]>>1;
			    if(sTempDiv.m_ulValue[j+1]&1)
					sTempDiv.m_ulValue[j]=sTempDiv.m_ulValue[j] | 0x80000000;
			}

		    if(sTempDiv.m_ulValue[sTempDiv.m_iLength32-1]==0)
				sTempDiv.m_iLength32--;

			RsaTrans(&sTemp1,&sTempA,&sTempDiv,sIn);


			if(Cmp(&sTemp1,&sN_1)==0)
			{
				iPassMark=1;
				break;
			}
		}
		if((sTemp1.m_iLength32==1)&&(sTemp1.m_ulValue[0]==1))
			iPassMark=1;

		if(iPassMark==0)
			return false;
	}

	return true;

/*	算法
	this=N=input
	N_1=N-1

    for(i=0;i<5;i++)
    {
        pass=0;
        A=rand();
		T=N_1;
        while(T最低位=0)
		{
				T=T/2;
				I=(A^T)% N;
				if(I==N_1)
					{pass=1;break;}
		}
		if(I==1))
			pass=1;
		if(pass==0)
			return 0;
	}
    return 1;
*/
}


/****************************************************************************************
产生随机素数
调用方法：	输入2^32进制长度，输出大素数
返回值：	sOut被赋值为一个iLength32长度（0x100000000进制长度）的素数
****************************************************************************************/
void GetPrime(SBigInt *sOut, int iLength32)
{
    int				i;

    sOut->m_iLength32=iLength32;
	iLength32--;

	for(i=sOut->m_iLength32;i<BIGINT_MAXLEN;i++)
		sOut->m_ulValue[i]=0;


	do
	{
		for(i=0;i<sOut->m_iLength32;i++)
			sOut->m_ulValue[i]=Rand();


		sOut->m_ulValue[0]=sOut->m_ulValue[0] | 0x11;							//	减少Rab判断时间
		sOut->m_ulValue[iLength32]=sOut->m_ulValue[iLength32] | 0x80000000;	//	保证N的长度

	}
	while(Rab(sOut)==false);
}

/****************************************************************************************
求不定方程ax-by=1的最小整数解
调用方式：	输入大数和模,满足：Out*In mod N=1
返回值：	无
****************************************************************************************/
void	Euc(SBigInt *sOut, SBigInt *sIn,SBigInt *sN)
{
	SBigInt		sTemp1,sTemp2,sTemp3,sTemp4,sTemp5,sTemp6;
	int			x,y;

	Mov_bigint(&sTemp1,sN);
	Mov_bigint(&sTemp2,sIn);
	Mov_long(&sTemp3,0);
	Mov_long(&sTemp4,1);
	x=1;
	y=1;

	while(sTemp2.m_iLength32!=1 || sTemp2.m_ulValue[0]!=0)
	{
		Div_bigint(&sTemp5,&sTemp1,&sTemp2);
		Mod_bigint(&sTemp6,&sTemp1,&sTemp2);
		Mov_bigint(&sTemp1,&sTemp2);
		Mov_bigint(&sTemp2,&sTemp6);
		Mov_bigint(&sTemp6,&sTemp4);
		Mul_bigint(sOut,&sTemp4,&sTemp5);
		Mov_bigint(&sTemp4,sOut);

		if(x==y)
		{
			if(Cmp(&sTemp3,&sTemp4)>=0)
			{
				Sub_bigint(sOut,&sTemp3,&sTemp4);
				Mov_bigint(&sTemp4,sOut);
			}
			else
			{
				Sub_bigint(&sTemp4,&sTemp4,&sTemp3);
				y=0;
			}
		}
		else
		{
			Add_bigint(&sTemp4,&sTemp4,&sTemp3);
			x=1-x;
			y=1-y;
		}
		Mov_bigint(&sTemp3,&sTemp6);


	}

	if(x==0)
		Sub_bigint(sOut,sN,&sTemp3);
	else
		Mov_bigint(sOut,&sTemp3);

	return;
}

/****************************************************************************************
初始化RSA
调用方式：	输入D和RSA运算长度，产生私钥和各种中间变量
返回值：	无
****************************************************************************************/
bool	RSA_Create(unsigned char  * pubExp, int	bitLen_N,unsigned char* mod_N,unsigned char* prvExp)
{
	int		i;

	int iLength32=bitLen_N>>6;
	//iLength32=iLength32>>1;

	if ((mod_N==NULL)||(prvExp==NULL))
	{
		printf("RSA_Create() failed due to (mod_N=0x%x,prvExp=0x%x),mod_N,prvExp");
		return false;
	}
start:
	RSA_initialize();
	SetParameter_char_openssl(&RSA_D,pubExp,4);

/*	for(i=0;i<sD->m_iLength32;i++)
		RSA_D.m_ulValue[i]=sD->m_ulValue[i];
	for(i=sD->m_iLength32;i<BIGINT_MAXLEN;i++)
		RSA_D.m_ulValue[i]=0;
*/
	GetPrime(&RSA_P,iLength32);

	GetPrime(&RSA_Q,iLength32);

	Mul_bigint(&RSA_N,&RSA_P,&RSA_Q);
	Sub_long(&RSA_P,&RSA_P,1);
	Sub_long(&RSA_Q,&RSA_Q,1);
	Mul_bigint(&RSA_C,&RSA_P,&RSA_Q);
	Euc(&RSA_E,&RSA_D,&RSA_C);
	Mov_long(&RSA_C,0);

/*
length of private exponent should the same as modulus,if not try to recreate
*/
	if (RSA_E.m_iLength32<(iLength32<<1))
		goto start;

    memcpy(prvExp,(unsigned char *)(RSA_E.m_ulValue), RSA_E.m_iLength32*(sizeof(unsigned int)));
      /*unsigned int *byte2int=prvExp;*/

        /*for (i=0;i<RSA_E.m_iLength32;i++)*/
            /*byte2int[i]=RSA_E.m_ulValue[i];*/

	 invert_char(prvExp,bitLen_N>>3);

    memcpy(mod_N,(unsigned char *)(RSA_N.m_ulValue), RSA_E.m_iLength32*(sizeof(unsigned int)));
        /*byte2int=mod_N;*/

        /*for (i=0;i<RSA_E.m_iLength32;i++)*/
            /*byte2int[i]=RSA_N.m_ulValue[i];*/

        invert_char(mod_N,bitLen_N>>3);
	return true;
}
