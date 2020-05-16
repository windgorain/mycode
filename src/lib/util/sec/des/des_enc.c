
#include "bs.h"

#include "utl/des_utl.h"

#include "des_locl.h"


static void DES_encrypt1(UINT *data, DES_key_schedule *ks, int enc)
	{
	register UINT l,r,t,u;
	register UINT *s;

	r=data[0];
	l=data[1];

	IP(r,l);
	/* Things have been modified so that the initial rotate is
	 * done outside the loop.  This required the
	 * g_DES_SPtrans values in sp.h to be rotated 1 bit to the right.
	 * One perl script later and things have a 5% speed up on a sparc2.
	 * Thanks to Richard Outerbridge <71755.204@CompuServe.COM>
	 * for pointing this out. */
	/* clear the top bits on machines with 8byte longs */
	/* shift left by 2 */
	r=ROTATE(r,29)&0xffffffffL;
	l=ROTATE(l,29)&0xffffffffL;

	s=ks->ks->deslong;

	if (enc)
	{
		D_ENCRYPT(l,r, 0); /*  1 */
		D_ENCRYPT(r,l, 2); /*  2 */
		D_ENCRYPT(l,r, 4); /*  3 */
		D_ENCRYPT(r,l, 6); /*  4 */
		D_ENCRYPT(l,r, 8); /*  5 */
		D_ENCRYPT(r,l,10); /*  6 */
		D_ENCRYPT(l,r,12); /*  7 */
		D_ENCRYPT(r,l,14); /*  8 */
		D_ENCRYPT(l,r,16); /*  9 */
		D_ENCRYPT(r,l,18); /*  10 */
		D_ENCRYPT(l,r,20); /*  11 */
		D_ENCRYPT(r,l,22); /*  12 */
		D_ENCRYPT(l,r,24); /*  13 */
		D_ENCRYPT(r,l,26); /*  14 */
		D_ENCRYPT(l,r,28); /*  15 */
		D_ENCRYPT(r,l,30); /*  16 */
	}
	else
	{
		D_ENCRYPT(l,r,30); /* 16 */
		D_ENCRYPT(r,l,28); /* 15 */
		D_ENCRYPT(l,r,26); /* 14 */
		D_ENCRYPT(r,l,24); /* 13 */
		D_ENCRYPT(l,r,22); /* 12 */
		D_ENCRYPT(r,l,20); /* 11 */
		D_ENCRYPT(l,r,18); /* 10 */
		D_ENCRYPT(r,l,16); /*  9 */
		D_ENCRYPT(l,r,14); /*  8 */
		D_ENCRYPT(r,l,12); /*  7 */
		D_ENCRYPT(l,r,10); /*  6 */
		D_ENCRYPT(r,l, 8); /*  5 */
		D_ENCRYPT(l,r, 6); /*  4 */
		D_ENCRYPT(r,l, 4); /*  3 */
		D_ENCRYPT(l,r, 2); /*  2 */
		D_ENCRYPT(r,l, 0); /*  1 */
	}

	/* rotate and clear the top bits on machines with 8byte longs */
	l=ROTATE(l,3)&0xffffffffL;
	r=ROTATE(r,3)&0xffffffffL;

	FP(r,l);
	data[0]=l;
	data[1]=r;
	l=r=t=u=0;
}

static void DES_encrypt2(UINT *data, DES_key_schedule *ks, int enc)
{
	register UINT l,r,t,u;
	register UINT *s;

	r=data[0];
	l=data[1];

	/* Things have been modified so that the initial rotate is
	 * done outside the loop.  This required the
	 * g_DES_SPtrans values in sp.h to be rotated 1 bit to the right.
	 * One perl script later and things have a 5% speed up on a sparc2.
	 * Thanks to Richard Outerbridge <71755.204@CompuServe.COM>
	 * for pointing this out. */
	/* clear the top bits on machines with 8byte longs */
	r=ROTATE(r,29)&0xffffffffL;
	l=ROTATE(l,29)&0xffffffffL;

	s=ks->ks->deslong;
	/* I don't know if it is worth the effort of loop unrolling the
	 * inner loop */
	if (enc)
	{
		D_ENCRYPT(l,r, 0); /*  1 */
		D_ENCRYPT(r,l, 2); /*  2 */
		D_ENCRYPT(l,r, 4); /*  3 */
		D_ENCRYPT(r,l, 6); /*  4 */
		D_ENCRYPT(l,r, 8); /*  5 */
		D_ENCRYPT(r,l,10); /*  6 */
		D_ENCRYPT(l,r,12); /*  7 */
		D_ENCRYPT(r,l,14); /*  8 */
		D_ENCRYPT(l,r,16); /*  9 */
		D_ENCRYPT(r,l,18); /*  10 */
		D_ENCRYPT(l,r,20); /*  11 */
		D_ENCRYPT(r,l,22); /*  12 */
		D_ENCRYPT(l,r,24); /*  13 */
		D_ENCRYPT(r,l,26); /*  14 */
		D_ENCRYPT(l,r,28); /*  15 */
		D_ENCRYPT(r,l,30); /*  16 */
	}
	else
	{
		D_ENCRYPT(l,r,30); /* 16 */
		D_ENCRYPT(r,l,28); /* 15 */
		D_ENCRYPT(l,r,26); /* 14 */
		D_ENCRYPT(r,l,24); /* 13 */
		D_ENCRYPT(l,r,22); /* 12 */
		D_ENCRYPT(r,l,20); /* 11 */
		D_ENCRYPT(l,r,18); /* 10 */
		D_ENCRYPT(r,l,16); /*  9 */
		D_ENCRYPT(l,r,14); /*  8 */
		D_ENCRYPT(r,l,12); /*  7 */
		D_ENCRYPT(l,r,10); /*  6 */
		D_ENCRYPT(r,l, 8); /*  5 */
		D_ENCRYPT(l,r, 6); /*  4 */
		D_ENCRYPT(r,l, 4); /*  3 */
		D_ENCRYPT(l,r, 2); /*  2 */
		D_ENCRYPT(r,l, 0); /*  1 */
	}
	/* rotate and clear the top bits on machines with 8byte longs */
	data[0]=ROTATE(l,3)&0xffffffffL;
	data[1]=ROTATE(r,3)&0xffffffffL;
	l=r=t=u=0;
}

static void DES_encrypt3(UINT *data, DES_key_schedule *ks1,
		  DES_key_schedule *ks2, DES_key_schedule *ks3)
{
	register UINT l,r;

	l=data[0];
	r=data[1];
	IP(l,r);
	data[0]=l;
	data[1]=r;
	DES_encrypt2((UINT *)data,ks1,DES_ENCRYPT);
	DES_encrypt2((UINT *)data,ks2,DES_DECRYPT);
	DES_encrypt2((UINT *)data,ks3,DES_ENCRYPT);
	l=data[0];
	r=data[1];
	FP(r,l);
	data[0]=l;
	data[1]=r;
}

static void DES_decrypt3(UINT *data, DES_key_schedule *ks1,
		  DES_key_schedule *ks2, DES_key_schedule *ks3)
{
	register UINT l,r;

	l=data[0];
	r=data[1];
	IP(l,r);
	data[0]=l;
	data[1]=r;
	DES_encrypt2((UINT *)data,ks3,DES_DECRYPT);
	DES_encrypt2((UINT *)data,ks2,DES_ENCRYPT);
	DES_encrypt2((UINT *)data,ks1,DES_DECRYPT);
	l=data[0];
	r=data[1];
	FP(r,l);
	data[0]=l;
	data[1]=r;
}

#if 0 /* 没用到的函数编译告警 */
static void DES_ncbc_encrypt(const unsigned char *in, unsigned char *out, long length,
		     DES_key_schedule *_schedule, DES_cblock *ivec, int enc)
{
	register UINT tin0,tin1;
	register UINT tout0,tout1,xor0,xor1;
	register long l=length;
	UINT tin[2];
	unsigned char *iv;

	iv = &(*ivec)[0];

	if (enc)
	{
		c2l(iv,tout0);
		c2l(iv,tout1);
		for (l-=8; l>=0; l-=8)
		{
			c2l(in,tin0);
			c2l(in,tin1);
			tin0^=tout0; tin[0]=tin0;
			tin1^=tout1; tin[1]=tin1;
			DES_encrypt1((UINT *)tin,_schedule,DES_ENCRYPT);
			tout0=tin[0]; l2c(tout0,out);
			tout1=tin[1]; l2c(tout1,out);
		}
		if (l != -8)
		{
			c2ln(in,tin0,tin1,l+8);
			tin0^=tout0; tin[0]=tin0;
			tin1^=tout1; tin[1]=tin1;
			DES_encrypt1((UINT *)tin,_schedule,DES_ENCRYPT);
			tout0=tin[0]; l2c(tout0,out);
			tout1=tin[1]; l2c(tout1,out);
		}

		iv = &(*ivec)[0];
		l2c(tout0,iv);
		l2c(tout1,iv);
	}
	else
	{
		c2l(iv,xor0);
		c2l(iv,xor1);
		for (l-=8; l>=0; l-=8)
		{
			c2l(in,tin0); tin[0]=tin0;
			c2l(in,tin1); tin[1]=tin1;
			DES_encrypt1((UINT *)tin,_schedule,DES_DECRYPT);
			tout0=tin[0]^xor0;
			tout1=tin[1]^xor1;
			l2c(tout0,out);
			l2c(tout1,out);
			xor0=tin0;
			xor1=tin1;
		}
		if (l != -8)
		{
			c2l(in,tin0); tin[0]=tin0;
			c2l(in,tin1); tin[1]=tin1;
			DES_encrypt1((UINT *)tin,_schedule,DES_DECRYPT);
			tout0=tin[0]^xor0;
			tout1=tin[1]^xor1;
			l2cn(tout0,tout1,out,l+8);
			xor0=tin0;
			xor1=tin1;
		}

		iv = &(*ivec)[0];
		l2c(xor0,iv);
		l2c(xor1,iv);
	}
	tin0=tin1=tout0=tout1=xor0=xor1=0;
	tin[0]=tin[1]=0;
}
#endif

static inline VOID DES_ede3_cbc_encrypt(const unsigned char *input, unsigned char *output,
			  long length, DES_key_schedule *ks1,
			  DES_key_schedule *ks2, DES_key_schedule *ks3,
			  DES_cblock *ivec, int enc)
{
	register UINT tin0,tin1;
	register UINT tout0,tout1,xor0,xor1;
	register const unsigned char *in;
	unsigned char *out;
	register long l=length;
	UINT tin[2];
	unsigned char *iv;

	in=input;
	out=output;
	iv = &(*ivec)[0];

	if (enc)
	{
		c2l(iv,tout0);
		c2l(iv,tout1);
		for (l-=8; l>=0; l-=8)
		{
			c2l(in,tin0);
			c2l(in,tin1);
			tin0^=tout0;
			tin1^=tout1;

			tin[0]=tin0;
			tin[1]=tin1;
			DES_encrypt3((UINT *)tin,ks1,ks2,ks3);
			tout0=tin[0];
			tout1=tin[1];

			l2c(tout0,out);
			l2c(tout1,out);
		}
		if (l != -8)
		{
			c2ln(in,tin0,tin1,l+8);
			tin0^=tout0;
			tin1^=tout1;

			tin[0]=tin0;
			tin[1]=tin1;
			DES_encrypt3((UINT *)tin,ks1,ks2,ks3);
			tout0=tin[0];
			tout1=tin[1];

			l2c(tout0,out);
			l2c(tout1,out);
		}
		iv = &(*ivec)[0];
		l2c(tout0,iv);
		l2c(tout1,iv);
	}
	else
	{
		register UINT t0,t1;

		c2l(iv,xor0);
		c2l(iv,xor1);
		for (l-=8; l>=0; l-=8)
		{
			c2l(in,tin0);
			c2l(in,tin1);

			t0=tin0;
			t1=tin1;

			tin[0]=tin0;
			tin[1]=tin1;
			DES_decrypt3((UINT *)tin,ks1,ks2,ks3);
			tout0=tin[0];
			tout1=tin[1];

			tout0^=xor0;
			tout1^=xor1;
			l2c(tout0,out);
			l2c(tout1,out);
			xor0=t0;
			xor1=t1;
		}
		if (l != -8)
		{
			c2l(in,tin0);
			c2l(in,tin1);
			
			t0=tin0;
			t1=tin1;

			tin[0]=tin0;
			tin[1]=tin1;
			DES_decrypt3((UINT *)tin,ks1,ks2,ks3);
			tout0=tin[0];
			tout1=tin[1];
		
			tout0^=xor0;
			tout1^=xor1;
			l2cn(tout0,tout1,out,l+8);
			xor0=t0;
			xor1=t1;
		}

		iv = &(*ivec)[0];
		l2c(xor0,iv);
		l2c(xor1,iv);
	}
	tin0=tin1=tout0=tout1=xor0=xor1=0;
	tin[0]=tin[1]=0;
}

static inline VOID DES_ecb3_encrypt(const_DES_cblock *input, DES_cblock *output,
              DES_key_schedule *ks1, DES_key_schedule *ks2,
              DES_key_schedule *ks3,
              int enc)
{
    register UINT l0,l1;
    UINT ll[2];
    const unsigned char *in = &(*input)[0];
    unsigned char *out = &(*output)[0];

    c2l(in,l0);
    c2l(in,l1);
    ll[0]=l0;
    ll[1]=l1;
    if (enc)
        DES_encrypt3(ll,ks1,ks2,ks3);
    else
        DES_decrypt3(ll,ks1,ks2,ks3);
    l0=ll[0];
    l1=ll[1];
    l2c(l0,out);
    l2c(l1,out);
}

static inline void DES_ede3_cbcm_encrypt(const unsigned char *in, unsigned char *out,
	     long length, DES_key_schedule *ks1, DES_key_schedule *ks2,
	     DES_key_schedule *ks3, DES_cblock *ivec1, DES_cblock *ivec2,
	     int enc)
{
    register UINT tin0,tin1;
    register UINT tout0,tout1,xor0,xor1,m0,m1;
    register long l=length;
    UINT tin[2];
    unsigned char *iv1,*iv2;

    iv1 = &(*ivec1)[0];
    iv2 = &(*ivec2)[0];

    if (enc)
	{
    	c2l(iv1,m0);
    	c2l(iv1,m1);
    	c2l(iv2,tout0);
    	c2l(iv2,tout1);
    	for (l-=8; l>=-7; l-=8)
	    {
    	    tin[0]=m0;
    	    tin[1]=m1;
    	    DES_encrypt1(tin,ks3,1);
    	    m0=tin[0];
    	    m1=tin[1];

    	    if(l < 0)
    		{
        		c2ln(in,tin0,tin1,l+8);
    		}
    	    else
    		{
        		c2l(in,tin0);
        		c2l(in,tin1);
    		}
    	    tin0^=tout0;
    	    tin1^=tout1;

    	    tin[0]=tin0;
    	    tin[1]=tin1;
    	    DES_encrypt1(tin,ks1,1);
    	    tin[0]^=m0;
    	    tin[1]^=m1;
    	    DES_encrypt1(tin,ks2,0);
    	    tin[0]^=m0;
    	    tin[1]^=m1;
    	    DES_encrypt1(tin,ks1,1);
    	    tout0=tin[0];
    	    tout1=tin[1];

    	    l2c(tout0,out);
    	    l2c(tout1,out);
	    }
    	iv1=&(*ivec1)[0];
    	l2c(m0,iv1);
    	l2c(m1,iv1);

    	iv2=&(*ivec2)[0];
    	l2c(tout0,iv2);
    	l2c(tout1,iv2);
	}
    else
	{
    	register UINT t0,t1;

    	c2l(iv1,m0);
    	c2l(iv1,m1);
    	c2l(iv2,xor0);
    	c2l(iv2,xor1);
    	for (l-=8; l>=-7; l-=8)
	    {
    	    tin[0]=m0;
    	    tin[1]=m1;
    	    DES_encrypt1(tin,ks3,1);
    	    m0=tin[0];
    	    m1=tin[1];

    	    c2l(in,tin0);
    	    c2l(in,tin1);

    	    t0=tin0;
    	    t1=tin1;

    	    tin[0]=tin0;
    	    tin[1]=tin1;
    	    DES_encrypt1(tin,ks1,0);
    	    tin[0]^=m0;
    	    tin[1]^=m1;
    	    DES_encrypt1(tin,ks2,1);
    	    tin[0]^=m0;
    	    tin[1]^=m1;
    	    DES_encrypt1(tin,ks1,0);
    	    tout0=tin[0];
    	    tout1=tin[1];

    	    tout0^=xor0;
    	    tout1^=xor1;
    	    if(l < 0)
    		{
    		l2cn(tout0,tout1,out,l+8);
    		}
    	    else
    		{
    		l2c(tout0,out);
    		l2c(tout1,out);
    		}
    	    xor0=t0;
    	    xor1=t1;
	    }

    	iv1=&(*ivec1)[0];
    	l2c(m0,iv1);
    	l2c(m1,iv1);

    	iv2=&(*ivec2)[0];
    	l2c(xor0,iv2);
    	l2c(xor1,iv2);
	}
    tin0=tin1=tout0=tout1=xor0=xor1=0;
    tin[0]=tin[1]=0;
}

static inline void DES_ede3_cfb64_encrypt(const unsigned char *in, unsigned char *out,
	    long length, DES_key_schedule *ks1,
	    DES_key_schedule *ks2, DES_key_schedule *ks3,
	    DES_cblock *ivec, INT *num, int enc)
{
	register UINT v0,v1;
	register long l=length;
	register INT n= *num;
	UINT ti[2];
	unsigned char *iv,c,cc;

	iv=&(*ivec)[0];
	if (enc)
	{
		while (l--)
		{
			if (n == 0)
			{
				c2l(iv,v0);
				c2l(iv,v1);

				ti[0]=v0;
				ti[1]=v1;
				DES_encrypt3(ti,ks1,ks2,ks3);
				v0=ti[0];
				v1=ti[1];

				iv = &(*ivec)[0];
				l2c(v0,iv);
				l2c(v1,iv);
				iv = &(*ivec)[0];
			}
			c= *(in++)^iv[n];
			*(out++)=c;
			iv[n]=c;
			n=(n+1)&0x07;
		}
	}
	else
	{
		while (l--)
		{
			if (n == 0)
			{
				c2l(iv,v0);
				c2l(iv,v1);

				ti[0]=v0;
				ti[1]=v1;
				DES_encrypt3(ti,ks1,ks2,ks3);
				v0=ti[0];
				v1=ti[1];

				iv = &(*ivec)[0];
				l2c(v0,iv);
				l2c(v1,iv);
				iv = &(*ivec)[0];
			}
			cc= *(in++);
			c=iv[n];
			iv[n]=cc;
			*(out++)=c^cc;
			n=(n+1)&0x07;
		}
	}
	v0=v1=ti[0]=ti[1]=c=cc=0;
	*num=n;
}

static inline void DES_ede3_cfb_encrypt(const unsigned char *in,unsigned char *out,
	  int numbits,long length,DES_key_schedule *ks1,
	  DES_key_schedule *ks2,DES_key_schedule *ks3,
	  DES_cblock *ivec,int enc)
{
	register UINT d0,d1,v0,v1;
	register unsigned long l=length,n=((unsigned int)numbits+7)/8;
	register int num=numbits,i;
	UINT ti[2];
	unsigned char *iv;
	unsigned char ovec[16];

	if (num > 64) return;
	iv = &(*ivec)[0];
	c2l(iv,v0);
	c2l(iv,v1);
	if (enc)
	{
		while (l >= n)
		{
			l-=n;
			ti[0]=v0;
			ti[1]=v1;
			DES_encrypt3(ti,ks1,ks2,ks3);
			c2ln(in,d0,d1,n);
			in+=n;
			d0^=ti[0];
			d1^=ti[1];
			l2cn(d0,d1,out,n);
			out+=n;
			/* 30-08-94 - eay - changed because l>>32 and
			 * l<<32 are bad under gcc :-( */
			if (num == 32)
			{
                v0=v1; 
                v1=d0; 
            }
			else if (num == 64)
			{
                v0=d0;
                v1=d1;
            }
			else
			{
				iv=&ovec[0];
				l2c(v0,iv);
				l2c(v1,iv);
				l2c(d0,iv);
				l2c(d1,iv);
				/* shift ovec left most of the bits... */
				memmove(ovec,ovec+num/8,8+(num%8 ? 1 : 0));
				/* now the remaining bits */
				if(num%8 != 0)
				{
					for(i=0 ; i < 8 ; ++i)
					{
    					ovec[i]<<=num%8;
    					ovec[i]|=ovec[i+1]>>(8-num%8);
					}
				}
				iv=&ovec[0];
				c2l(iv,v0);
				c2l(iv,v1);
			}
		}
	}
	else
	{
		while (l >= n)
		{
			l-=n;
			ti[0]=v0;
			ti[1]=v1;
			DES_encrypt3(ti,ks1,ks2,ks3);
			c2ln(in,d0,d1,n);
			in+=n;
			/* 30-08-94 - eay - changed because l>>32 and
			 * l<<32 are bad under gcc :-( */
			if (num == 32)
			{
                v0=v1; 
                v1=d0; 
            }
			else if (num == 64)
			{
                v0=d0;
                v1=d1;
            }
			else
			{
				iv=&ovec[0];
				l2c(v0,iv);
				l2c(v1,iv);
				l2c(d0,iv);
				l2c(d1,iv);
				/* shift ovec left most of the bits... */
				memmove(ovec,ovec+num/8,8+(num%8 ? 1 : 0));
				/* now the remaining bits */
				if(num%8 != 0)
				{
					for(i=0 ; i < 8 ; ++i)
					{
    					ovec[i]<<=num%8;
    					ovec[i]|=ovec[i+1]>>(8-num%8);
					}
				}
				iv=&ovec[0];
				c2l(iv,v0);
				c2l(iv,v1);
			}
			d0^=ti[0];
			d1^=ti[1];
			l2cn(d0,d1,out,n);
			out+=n;
		}
	}
	iv = &(*ivec)[0];
	l2c(v0,iv);
	l2c(v1,iv);
	v0=v1=d0=d1=ti[0]=ti[1]=0;
}

static inline void DES_ede3_ofb64_encrypt(register const unsigned char *in,
	    register unsigned char *out, long length,
	    DES_key_schedule *k1, DES_key_schedule *k2,
	    DES_key_schedule *k3, DES_cblock *ivec,
	    INT *num)
{
	register UINT v0,v1;
	register INT n= *num;
	register long l=length;
	DES_cblock d;
	register char *dp;
	UINT ti[2];
	unsigned char *iv;
	INT save=0;

	iv = &(*ivec)[0];
	c2l(iv,v0);
	c2l(iv,v1);
	ti[0]=v0;
	ti[1]=v1;
	dp=(char *)d;
	l2c(v0,dp);
	l2c(v1,dp);
	while (l--)
	{
		if (n == 0)
		{
			/* ti[0]=v0; */
			/* ti[1]=v1; */
			DES_encrypt3(ti,k1,k2,k3);
			v0=ti[0];
			v1=ti[1];

			dp=(char *)d;
			l2c(v0,dp);
			l2c(v1,dp);
			save++;
		}
		*(out++)= *(in++)^d[n];
		n=(n+1)&0x07;
	}
	if (save)
	{
		iv = &(*ivec)[0];
		l2c(v0,iv);
		l2c(v1,iv);
	}
	v0=v1=ti[0]=ti[1]=0;
	*num=n;
}

void DES_Ecb_encrypt
(
    IN const_DES_cblock *input,
    OUT DES_cblock *output,
    IN DES_key_schedule *ks,
    IN int enc
)
{
	register UINT l;
	UINT ll[2];
	const unsigned char *in = &(*input)[0];
	unsigned char *out = &(*output)[0];

	c2l(in,l); ll[0]=l;
	c2l(in,l); ll[1]=l;
	DES_encrypt1(ll,ks,enc);
	l=ll[0]; l2c(l,out);
	l=ll[1]; l2c(l,out);
	l=ll[0]=ll[1]=0;
}


void DES_Ecb3Encrypt
(
    IN DES_cblock *pstInput,
    OUT DES_cblock *pstOutput,
    IN DES_key_schedule *pstKs1, 
    IN DES_key_schedule *pstKs2,
    IN DES_key_schedule *pstKs3,
    IN BOOL_T bIsEnc
)
{
    DES_ecb3_encrypt(pstInput, pstOutput, pstKs1, pstKs2, pstKs3, bIsEnc);
}

void DES_Ede3CbcEncrypt
(
    IN UCHAR *pucInput,
    OUT UCHAR *pucOutput,
    IN INT lLength,
    IN DES_key_schedule *pstKs1,
    IN DES_key_schedule *pstKs2,
    IN DES_key_schedule *pstKs3,
    INOUT DES_cblock *pstIvec,
    IN BOOL_T bIsEnc
)
{
    DES_ede3_cbc_encrypt(pucInput, pucOutput, lLength, pstKs1, pstKs2, pstKs3, pstIvec, bIsEnc);
}

void DES_Ede3CbcmEncrypt
(
    IN UCHAR *pucIn,
    OUT UCHAR *pucOut,
    IN long lLength, 
    IN DES_key_schedule *pstKs1,
    IN DES_key_schedule *pstKs2,
    IN DES_key_schedule *pstKs3, 
    INOUT DES_cblock *pstIvec1, 
    INOUT DES_cblock *pstIvec2,
    IN BOOL_T bIsEnc
)
{
    DES_ede3_cbcm_encrypt(pucIn, pucOut, lLength, pstKs1, pstKs2, pstKs3, pstIvec1, pstIvec2, bIsEnc);
}

void DES_Ede3Cfb64Encrypt
(
    IN UCHAR *pucIn,
    OUT UCHAR *pucOut,
    IN long lLength, 
    IN DES_key_schedule *pstKs1,
    IN DES_key_schedule *pstKs2,
    IN DES_key_schedule *pstKs3, 
    INOUT DES_cblock *pstIvec,
	INOUT INT *plNum, 
	IN BOOL_T bIsEnc
)
{
    DES_ede3_cfb64_encrypt(pucIn, pucOut, lLength, pstKs1, pstKs2, pstKs3, pstIvec, plNum, bIsEnc);
}

void DES_Ede3CfbEncrypt
(
    IN UCHAR *pucIn,
    OUT UCHAR *pucOut,
    IN INT lNumbits,
    IN long lLength, 
    IN DES_key_schedule *pstKs1,
    IN DES_key_schedule *pstKs2,
    IN DES_key_schedule *pstKs3, 
    INOUT DES_cblock *pstIvec,
	IN BOOL_T bIsEnc
)
{
    DES_ede3_cfb_encrypt(pucIn, pucOut, lNumbits, lLength, pstKs1, pstKs2, pstKs3, pstIvec, bIsEnc);
}

void DES_Ede3Ofb64Encrypt
(
    IN UCHAR *pucIn,
    OUT UCHAR *pucOut,
    IN long lLength, 
    IN DES_key_schedule *pstKs1,
    IN DES_key_schedule *pstKs2,
    IN DES_key_schedule *pstKs3, 
    INOUT DES_cblock *pstIvec,
    INOUT INT *plNum
)
{
    DES_ede3_ofb64_encrypt(pucIn, pucOut, lLength, pstKs1, pstKs2, pstKs3, pstIvec, plNum);
}

