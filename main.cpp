#include<stdio.h>
#include<math.h>
#include<vector>
using namespace std;
int g_SampleRate = 44100;
int g_channels = 2;
#define FMT s16le

#define DEF_MAX 32767
#define DEF_MIN -32768

typedef struct STR_WEBRTC
{
	int iEnergy;//能量值
	bool bMix;//是否进行mix
	short *Data;//真实数据

}STR_WEBRTC;

std::vector<STR_WEBRTC>g_mixList;
std::vector<STR_WEBRTC>g_rampOutList;

//80
const float rampArray[] = {
0.0000f, 0.0127f, 0.0253f, 0.0380f,
0.0506f, 0.0633f, 0.0759f, 0.0886f,
0.1013f, 0.1139f, 0.1266f, 0.1392f,
0.1519f, 0.1646f, 0.1772f, 0.1899f,
0.2025f, 0.2152f, 0.2278f, 0.2405f,
0.2532f, 0.2658f, 0.2785f, 0.2911f,
0.3038f, 0.3165f, 0.3291f, 0.3418f,
0.3544f, 0.3671f, 0.3797f, 0.3924f,
0.4051f, 0.4177f, 0.4304f, 0.4430f,
0.4557f, 0.4684f, 0.4810f, 0.4937f,
0.5063f, 0.5190f, 0.5316f, 0.5443f,
0.5570f, 0.5696f, 0.5823f, 0.5949f,
0.6076f, 0.6203f, 0.6329f, 0.6456f,
0.6582f, 0.6709f, 0.6835f, 0.6962f,
0.7089f, 0.7215f, 0.7342f, 0.7468f,
0.7595f, 0.7722f, 0.7848f, 0.7975f,
0.8101f, 0.8228f, 0.8354f, 0.8481f,
0.8608f, 0.8734f, 0.8861f, 0.8987f,
0.9114f, 0.9241f, 0.9367f, 0.9494f,
0.9620f, 0.9747f, 0.9873f, 1.0000f };


void pcm_mix_webrtc_update(STR_WEBRTC &p,int T)
{
	//获取能量最大的三路
	//判断能量最大的三路是否需要进行RampIn

	if (p.bMix == false)
	{
		for (int i = 0; i < 80; i++)
		{
			p.Data[i] = p.Data[i] * rampArray[i];
		}
	}

	for (int i = 0; i < T; i++)
	{
		p.iEnergy += p.Data[i] * p.Data[i];
	}

	if (g_mixList.size() >= 3)
	{
		for (std::vector<STR_WEBRTC>::iterator iter = g_mixList.begin(); iter != g_mixList.end(); iter++)
		{
			if (p.iEnergy > iter->iEnergy)
			{
				if (iter->bMix == true)
				{
					for (int i = 0; i < 80; i++)
					{
						iter->Data[i] = iter->Data[i] * rampArray[80 - i - 1];
					}
					g_rampOutList.push_back(*iter);
				}

				g_mixList.erase(iter);
				g_mixList.push_back(p);
				break;
			}
		}
	}
	else
	{
		g_mixList.push_back(p);
	}
	
}
void pcm_mix_webrtc_list(short *bufMix,int T)
{
	for (int i = 0; i < T; i++)
	{
		int iT = 0;
		if (!g_rampOutList.empty())
		{
			 iT = g_mixList.at(0).Data[i] + g_mixList.at(1).Data[i] + g_mixList.at(2).Data[i] + g_rampOutList.at(0).Data[i];
			 iT /= 2;
		}
		else
		{
			iT = g_mixList.at(0).Data[i] + g_mixList.at(1).Data[i] + g_mixList.at(2).Data[i];
			iT /= 2;
		}
		
		if (iT > DEF_MAX)
		{
			printf("pcm_mix_webrtc_list DEF_MAX\n");
			iT = DEF_MAX;
		}
		if (iT < DEF_MIN)
		{
			printf("pcm_mix_webrtc_list DEF_MIN\n");
			iT = DEF_MIN;
		}

		bufMix[i] = iT;
	}
}
//利用webRtc原理实现mix
void  pcm_mix_webrtc(short *bufMix,STR_WEBRTC &p1, STR_WEBRTC &p2, STR_WEBRTC &p3, STR_WEBRTC &p4,int T)
{
	g_mixList.clear();
	g_rampOutList.clear();

	pcm_mix_webrtc_update(p1, T);
	pcm_mix_webrtc_update(p2, T);
	pcm_mix_webrtc_update(p3, T);
	pcm_mix_webrtc_update(p4, T);

	for (auto iter = g_mixList.begin(); iter != g_mixList.end(); iter++)
	{
		if (iter->Data == p1.Data)
		{
			p1.bMix = true;
		}
		if (iter->Data == p2.Data)
		{
			p2.bMix = true;
		}
		if (iter->Data == p3.Data)
		{
			p3.bMix = true;
		}
		if (iter->Data == p4.Data)
		{
			p4.bMix = true;
		}
	}
	for (auto iter = g_rampOutList.begin(); iter != g_rampOutList.end(); iter++)
	{
		if (iter->Data == p1.Data)
		{
			p1.bMix = false;
		}
		if (iter->Data == p2.Data)
		{
			p2.bMix = false;
		}
		if (iter->Data == p3.Data)
		{
			p3.bMix = false;
		}
		if (iter->Data == p4.Data)
		{
			p4.bMix = false;
		}
	}

	pcm_mix_webrtc_list(bufMix, T);
}
//
void pcm_mix_auto_newlc(short *bufMix,short *buf,short *buf2,int T)
{
	for (int i = 0; i < T; i++)
	{
		int iT = 0;
		
		if (buf[i] < 0 && buf2[i] < 0)
		{
			iT = buf[i] * buf2[i] / -(pow(2, 16 - 1) - 1);
		}
		else
		{
			iT = buf[i] * buf2[i] / (pow(2, 16 - 1) - 1);
		}

		int iMax = buf[i] + buf2[i] - iT;
		if (iMax > DEF_MAX)
		{
			printf("pcm_mix_auto_newlc DEF_MAX\n");
		}
		if (iMax < DEF_MIN)
		{
			printf("pcm_mix_auto_newlc DEF_MIN\n");
		}

		bufMix[i] = iMax;
	}
}

//自动对齐算法,以自身的比例为权重.
//a的权重为a/(a+b+c),b的权重为b/(a+b+c)，则最终a的权重乘以a的值，加上b的权重乘以b的值.
//(a*|a| + |b|*b + |c|*c)/(|a|+|b|+|c|)该方法导致高频数据损失
int get_mix_auto_align_sgn(short p)
{
	int isgn = 0;

	if (p > 0)
	{
		isgn = 1;
	}
	else if (p < 0)
	{
		isgn = -1;
	}
	else {
		isgn = 0;
	}

	int ibuf = abs(p * p);

	return isgn * ibuf;
}
void pcm_mix_auto_align(short *bufMix,short *buf,short *buf2,int T)
{
	
	for (int i = 0; i < T; i++)
	{
		//int ibuf = get_mix_auto_align_sgn(buf[i]);
		//int ibuf2 = get_mix_auto_align_sgn(buf2[i]);
		
		if (buf[i]==0 && buf2[i] == 0)
		{
			bufMix[i] = 0;
			continue;
		}

		int iT = (abs(buf[i])*buf[i] +abs(buf2[i])*buf2[i]) / (abs(buf[i]) + abs(buf2[i]));
		
		if (iT > DEF_MAX)
		{
			printf("pcm_mix_auto_align DEF_MAX\n");
		}
		if (iT < DEF_MIN)
		{
			printf("pcm_mix_auto_align DEF_MIN\n");
		}

		bufMix[i] = iT;
	}
	
}

//线性叠加求平均.不会产生溢出，噪音较小。但是衰减过大，影响通话质量。
void pcm_mix_avg(short *bufMix,short *buf,short *buf2 ,int T)
{
	for (int i = 0; i < T; i++)
	{
		int iT = buf[i] + buf2[i];

		bufMix[i] = (short)(iT / 2);
		
		if (bufMix[i] > DEF_MAX)
		{
			bufMix[i] = DEF_MAX;
			printf("pcm_mix_avg DEF_MAX\n");
		}
		if (bufMix[i] < DEF_MIN)
		{
			bufMix[i] = DEF_MIN;
			printf("pcm_mix_avg DEF_MIN\n");
		}
	}
}
//自适应混音加权,使用可变的衰减因子对语音进行衰减，该衰减因子代表了语音的权重，
//衰减因子随着音频数据的变化而变化，当溢出时，衰减因子变小，使得后续的数据在衰减后处于临界值以内,
//没有溢出时，又让衰减因子慢慢增大，是数据较为平缓的变化.
void pcm_mix_Normalization(short *bufMix, short *buf, short *buf2, int T)
{
	double f = 1;
	int ioutput = 0;

	for (int i = 0; i < T; i++)
	{

		int iT = buf[i] + buf2[i];
		
		ioutput = iT * f;

		if (ioutput > DEF_MAX)
		{			
			f = (double)DEF_MAX / (double)(ioutput);
			ioutput = DEF_MAX;

			printf("pcm_mix_Normalization DEF_MAX f=%f\n",f);
		}

		if (ioutput < DEF_MIN)
		{
			f = (double)DEF_MIN / (double)(ioutput);
			ioutput = DEF_MIN;

			printf("pcm_mix_Normalization DEF_MIN f=%f\n", f);
		}

		if (f < 1)
		{
			f += (double)(1 - f) / (double)32;
		}
		bufMix[i] = (short)ioutput;
	}
}

//
int main(int argc, char *argv[])
{
	FILE *pPcm1 = nullptr, *pPcm2 = nullptr,*pPcm3 = nullptr,*pPcm4 = nullptr,*pPcmMix = nullptr;

	errno_t er = fopen_s(&pPcm1,"../Pcm/1.pcm","rb+");
	if (er != 0)
	{
		printf("");
	}
	er = fopen_s(&pPcm2,"../Pcm/2.pcm","rb+");
	if (er != 0)
	{
		printf("");
	}

	er = fopen_s(&pPcm3,"../Pcm/3.pcm","rb+");
	if(er != 0)
	{
		printf("");
	}

	er = fopen_s(&pPcm4, "../Pcm/4.pcm", "rb+");
	if (er = 0)
	{
		printf("");
	}

	er = fopen_s(&pPcmMix, "Mix.pcm", "wb+");
	if (er != 0)
	{
		printf("");
	}

	//10ms
	short bufMix[1920] = { 0};
	short buf1[1920] = { 0 };
	short buf2[1920] = { 0 };
	short buf3[1920] = { 0 };
	short buf4[1920] = { 0 };

	int ir1 = fread(buf1, 2, 1920, pPcm1);
	int ir2 = fread(buf2, 2, 1920, pPcm2);
	int ir3 = fread(buf3, 2, 1920, pPcm3);
	int ir4 = fread(buf4, 2, 1920, pPcm4);

	STR_WEBRTC s1{0,0,buf1 }, s2{ 0,0,buf2 }, s3{ 0,0,buf3 }, s4{ 0,0,buf4 };

	while (ir1 > 0 && ir2 > 0 && ir3 > 0 && ir4 > 0 )
	{
		
		//pcm_mix_avg(buf3, buf, buf2, 1024);
		//pcm_mix_Normalization(buf3,buf,buf2,1024);
		//pcm_mix_auto_align(buf3,buf,buf2,1024);
		//pcm_mix_auto_newlc(buf3, buf, buf2, 1024);
		
		pcm_mix_webrtc(bufMix, s1, s2, s3, s4,1920);

		fwrite(bufMix, 2, 1920, pPcmMix);

		ir1 = fread(buf1, 2, 1920, pPcm1);
		ir2 = fread(buf2, 2, 1920, pPcm2);
		ir3 = fread(buf3, 2, 1920, pPcm3);
		ir4 = fread(buf4, 2, 1920, pPcm4);
	}

	fclose(pPcm1);
	fclose(pPcm2);
	fclose(pPcm3);
	fclose(pPcm4);
	fclose(pPcmMix);
	
	printf("end.\n");

	for (;;);

	return 0;
}