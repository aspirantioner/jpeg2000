#include "j2kCoder.h"

int j2kCoder::j2kEncode(jp2Image *img,CodeParam *comp,char *output,int len)
{

	FILE *file=NULL;
	if(cp->isIntermedFile==1)
	{
		file=fopen(output,"wb");
		if(!file)
			return -1;
	}
	unsigned char *dest=(unsigned char *)malloc(len);
	IOStream::init(dest,len);

	image=img;
	cp=comp;

	state=J2K_STATE_MHSOC;//SOC码流开始 
	writeSOC();
	state = J2K_STATE_MHSIZ;//SIZ标记,图像和拼接块大小
	writeSIZ();

	state = J2K_STATE_MH;

	writeCOD();//COD标记,写入默认编码形式
	writeQCD();//QCD标记,写入默认量化 

	//遍历分量看对应的tile是否有开ROI,有就设置标记
	for(int compno=0;compno<image->numComponents;compno++)
	{
		TileCodeParam *tcp=&cp->tcps[0];
		if(tcp->tccps[compno].isROI)
			writeROI(compno,0);
	}

	if(cp->comment!=NULL)
	{
		writeComment();
	}

	if(cp->isIntermedFile==1)
	{

		currectPos=IOStream::getPosition();
		fwrite(dest,1,currectPos,file);
	}

	tileCoder=new j2kTileCoder(cp,image,currectTileNo);
	for(int tileno=0;tileno<cp->tw*cp->th;tileno++)
	{
		if(cp->isIntermedFile==1)
		{
			free(dest);
			dest=(unsigned char*)malloc(len);
			IOStream::init(dest,len);
		}
		currectTileNo=tileno;
		if(tileno==0)
		{	
			tileCoder->tcdMallocEncode();
		}else{
			tileCoder->tcdInitEncode();

			state = J2K_STATE_TPHSOT;//SOT标记,拼接块标头
			writeSOT();
			state = J2K_STATE_TPH;

			for(int compno=1;compno<img->numComponents;compno++)
			{
				writeCOC(compno);
				writeQCC(compno);

			}

			if(cp->tcps[tileno].numPocs)
				writePOC();

			writeSOD();

			if(cp->isIntermedFile==1)
			{
				fwrite(dest,1,IOStream::getPosition(),file);
				currectPos=IOStream::getPosition()+currectPos;
			}
		}

		if(cp->isIntermedFile==1)
		{
			free(dest);
			dest=(unsigned char*)malloc(len);
			IOStream::init(dest,len);
		}

		writeEOC();

		if(cp->isIntermedFile==1)
		{

			fwrite(dest,1,2,file);
			free(dest);
			fclose(file);

		}

		return IOStream::getPosition()+currectPos;
	}
}
void j2kCoder::writeSOC()
{
	IOStream::writeBytes(J2K_MS_SOC, 2);
}
void j2kCoder::writeSIZ()
{
	int i;
	int lenp, len;

	IOStream::writeBytes(J2K_MS_SIZ, 2);	/* SIZ                 */
	lenp = IOStream::getPosition();
	IOStream::skipBytes(2);
	IOStream::writeBytes(0, 2);		/* Rsiz (capabilities) */
	IOStream::writeBytes(image->Xsiz, 4);	/* Xsiz                */
	IOStream::writeBytes(image->Ysiz, 4);	/* Ysiz                */
	IOStream::writeBytes(image->XOsiz, 4);	/* X0siz               */
	IOStream::writeBytes(image->YOsiz, 4);	/* Y0siz               */
	IOStream::writeBytes(cp->XTsiz, 4);	/* XTsiz               */
	IOStream::writeBytes(cp->YTsiz, 4);	/* YTsiz               */
	IOStream::writeBytes(cp->XTOsiz, 4);	/* XT0siz              */
	IOStream::writeBytes(cp->YTOsiz, 4);	/* YT0siz              */
	IOStream::writeBytes(image->numComponents, 2);	/* Csiz                */
	for (i = 0; i < image->numComponents; i++) {
		IOStream::writeBytes(image->comps[i].precision - 1 + (image->comps[i].sgnd << 7), 1);	/* Ssiz_i */
		IOStream::writeBytes(image->comps[i].XRsiz, 1);	/* XRsiz_i             */
		IOStream::writeBytes(image->comps[i].YRsiz, 1);	/* YRsiz_i             */
	}
	len = IOStream::getPosition() - lenp;
	IOStream::setPosition(lenp);
	IOStream::writeBytes(len, 2);		/* Lsiz                */
	IOStream::setPosition(lenp + len);
}
void j2kCoder::writeSOT()
{
	int lenp, len;

	SOT_Start = IOStream::getPosition();
	IOStream::writeBytes(J2K_MS_SOT, 2);	/* SOT */
	lenp = IOStream::getPosition();
	IOStream::skipBytes(2);			/* Lsot (further) */
	IOStream::writeBytes(currectTileNo, 2);	/* Isot */
	IOStream::skipBytes(4);			/* Psot (further in j2k_write_sod) */
	IOStream::writeBytes(0, 1);		/* TPsot */
	IOStream::writeBytes(1, 1);		/* TNsot */
	len = IOStream::getPosition() - lenp;
	IOStream::setPosition(lenp);
	IOStream::writeBytes(len, 2);		/* Lsot */
	IOStream::setPosition(lenp + len);
}
void j2kCoder::writeEOC()
{
	IOStream::writeBytes(J2K_MS_EOC, 2);
}
void j2kCoder::writeCOD()
{
	TileCodeParam *tcp;
	int lenp, len;

	IOStream::writeBytes(J2K_MS_COD, 2);	/* COD */

	lenp = IOStream::getPosition();
	IOStream::skipBytes(2);

	tcp = &cp->tcps[currectTileNo];
	IOStream::writeBytes(tcp->codingStyle, 1);	/* Scod */
	IOStream::writeBytes(tcp->progressionOrder, 1);	/* SGcod (A) */
	IOStream::writeBytes(tcp->numLayers, 2);	/* SGcod (B) */
	IOStream::writeBytes(tcp->isMCT, 1);	/* SGcod (C) */

	writeCOX(0);
	len = IOStream::getPosition()- lenp;
	IOStream::setPosition(lenp);
	IOStream::writeBytes(len, 2);		/* Lcod */
	IOStream::setPosition(lenp + len);
}
void j2kCoder::writeQCD()
{
	int lenp, len;

	IOStream::writeBytes(J2K_MS_QCD, 2);	/* QCD */
	lenp = IOStream::getPosition();
	IOStream::skipBytes(2);
	writeQCX(0);
	len = IOStream::getPosition() - lenp;
	IOStream::setPosition(lenp);
	IOStream::writeBytes(len, 2);		/* Lqcd */
	IOStream::setPosition(lenp + len);

}
void j2kCoder::writeROI(int compno,int tileno)
{
	TileCodeParam *tcp = &cp->tcps[tileno];

	IOStream::writeBytes(J2K_MS_RGN, 2);	/* RGN  */
	IOStream::writeBytes(image->numComponents<= 256 ? 5 : 6, 2);	/* Lrgn */
	IOStream::writeBytes(compno, image->numComponents <= 256 ? 1 : 2);	/* Crgn */
	IOStream::writeBytes(0, 1);		/* Srgn */
	IOStream::writeBytes(tcp->tccps[compno].isROI, 1);	/* SPrgn */
}
void j2kCoder::writeComment()
{
	unsigned int i;
	int lenp, len;
	char str[256];
	sprintf(str, "%s", cp->comment);

	IOStream::writeBytes(J2K_MS_COM, 2);
	lenp = IOStream::getPosition();
	IOStream::skipBytes(2);
	IOStream::writeBytes(0, 2);
	for (i = 0; i < strlen(str); i++) {
		IOStream::writeBytes(str[i], 1);
	}
	len = IOStream::getPosition() - lenp;
	IOStream::setPosition(lenp);
	IOStream::writeBytes(len, 2);
	IOStream::setPosition(lenp + len);
}
void j2kCoder::writePOC()
{
	int len, numpchgs, i;
	TileCodeParam *tcp;
	TileCompCodeParam *tccp;

	tcp = &cp->tcps[currectTileNo];
	tccp = &tcp->tccps[0];
	numpchgs = tcp->numPocs;
	IOStream::writeBytes(J2K_MS_POC, 2);	/* POC  */
	len = 2 + (5 + 2 * (image->numComponents <= 256 ? 1 : 2)) * numpchgs;
	IOStream::writeBytes(len, 2);		/* Lpoc */
	for (i = 0; i < numpchgs; i++) {
		// MODIF
		POC *poc;
		poc = &tcp->pocs[i];
		IOStream::writeBytes(poc->resolutionStart, 1);	/* RSpoc_i */
		IOStream::writeBytes(poc->componentStart, (image->numComponents <= 256 ? 1 : 2));	/* CSpoc_i */
		IOStream::writeBytes(poc->layerEnd, 2);	/* LYEpoc_i */
		poc->layerEnd = int_min(poc->layerEnd, tcp->numLayers);
		IOStream::writeBytes(poc->resolutionEnd, 1);	/* REpoc_i */
		poc->resolutionEnd = int_min(poc->resolutionEnd, tccp->numResolutions);
		IOStream::writeBytes(poc->componentEnd, (image->numComponents <= 256 ? 1 : 2));	/* CEpoc_i */
		poc->componentEnd = int_min(poc->componentEnd, image->numComponents);
		IOStream::writeBytes(poc->progressionOrder, 1);	/* Ppoc_i */
	}
}
void j2kCoder::writeQCC(int compno){
	int lenp, len;

	IOStream::writeBytes(J2K_MS_QCC, 2);	/* QCC */
	lenp = IOStream::getPosition();
	IOStream::skipBytes(2);
	IOStream::writeBytes(compno, image->numComponents <= 256 ? 1 : 2);	/* Cqcc */
	writeQCX(compno);
	len = IOStream::getPosition() - lenp;
	IOStream::setPosition(lenp);
	IOStream::writeBytes(len, 2);		/* Lqcc */
	IOStream::setPosition(lenp + len);
}
void j2kCoder::writeCOC(int compno)
{
	TileCodeParam *tcp;
	int lenp, len;

	IOStream::writeBytes(J2K_MS_COC, 2);	/* COC */
	lenp = IOStream::getPosition();
	IOStream::skipBytes(2);
	tcp = &cp->tcps[currectTileNo];
	IOStream::writeBytes(compno, image->numComponents <= 256 ? 1 : 2);	/* Ccoc */
	IOStream::writeBytes(tcp->tccps[compno].codingStyle, 1);	/* Scoc */
	writeCOX(compno);
	len = IOStream::getPosition() - lenp;
	IOStream::setPosition(lenp);
	IOStream::writeBytes(len, 2);		/* Lcoc */
	IOStream::setPosition(lenp + len);
}
void j2kCoder::writeCOX(int compno)
{
	int i;
	TileCodeParam *tcp;
	TileCompCodeParam *tccp;
	tcp = &cp->tcps[currectTileNo];
	tccp = &tcp->tccps[compno];

	IOStream::writeBytes(tccp->numResolutions - 1, 1);	/* SPcox (D) */
	IOStream::writeBytes(tccp->codeBlockWidth - 2, 1);	/* SPcox (E) */
	IOStream::writeBytes(tccp->codeBlockWidth - 2, 1);	/* SPcox (F) */
	IOStream::writeBytes(tccp->codeBlockStyle, 1);	/* SPcox (G) */
	IOStream::writeBytes(tccp->isReversibleDWT, 1);	/* SPcox (H) */

	if (tccp->codeBlockStyle & J2K_CCP_CSTY_PRT) {
		for (i = 0; i < tccp->numResolutions; i++) {
			IOStream::writeBytes(tccp->precinctWidth[i] + (tccp->precinctHeight[i] << 4), 1);	/* SPcox (I_i) */
		}
	}
}
void j2kCoder::writeQCX(int compno)
{
	TileCodeParam *tcp;
	TileCompCodeParam *tccp;
	int bandno, numbands;
	int expn, mant;

	tcp = &cp->tcps[currectTileNo];
	tccp = &tcp->tccps[compno];

	IOStream::writeBytes(tccp->quantisationStyle + (tccp->numGuardBits << 5), 1);	/* Sqcx */
	numbands =
		tccp->quantisationStyle ==J2K_CCP_QNTSTY_SIQNT ? 1 : tccp->numResolutions * 3 - 2;

	for (bandno = 0; bandno < numbands; bandno++) {
		expn = tccp->stepsizes[bandno].expn;
		mant = tccp->stepsizes[bandno].mant;

		if (tccp->quantisationStyle == J2K_CCP_QNTSTY_NOQNT) {
			IOStream::writeBytes(expn << 3, 1);	/* SPqcx_i */
		} else {
			IOStream::writeBytes((expn << 11) + mant, 2);	/* SPqcx_i */
		}
	}
}
void j2kCoder::writeSOD()
{
	int l, layno;
	int totlen;
	TileCodeParam *tcp;

	IOStream::writeBytes(J2K_MS_SOD, 2);//写入SOD码流标记 

	if (currectTileNo == 0) {
		//如果是第一个tile
		SOD_Start = IOStream::getPosition() + currectPos;//记录SOD开始的位置
	}

	tcp = &cp->tcps[currectTileNo];
	for (layno = 0; layno < tcp->numLayers; layno++) {
		//遍历质量层,对每层质量层赋值
		tcp->rates[layno] = tcp->rates[layno]-(SOD_Start / (cp->th * cp->tw));
	}

	if (cp->imageType)//输入图像后缀为非pgx
		l = tileCoder->tcdEncodeTilePxm(currectTileNo,IOStream::getCurrectChar(),IOStream::getLeftBytesLength() -2 );//jp2用的是这种编码方式方式

	/* Writing Psot in SOT marker */
	totlen = IOStream::getPosition() + l - SOT_Start;
	IOStream::setPosition(SOT_Start + 6);
	IOStream::writeBytes(totlen, 4);
	IOStream::setPosition(SOT_Start + totlen);
}
