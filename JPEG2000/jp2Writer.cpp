#include "jp2Writer.h"

int jp2Writer::encode(jp2Struct *jp2_struct,CodeParam *cp,char *output)
{
	writeSignature();/*　写入signature框到buffer */
	writeFileType(jp2_struct);/* 写入 FILE_TYPE框到buffer */
	writeHeader(jp2_struct);/* 写入JP2 HEADER框到buffer */
	int len = writeContent(jp2_struct->image, cp, output);
	return IOStream::getPosition();
}

void jp2Writer::writeSignature()
{
	jp2Box box;
	box.init_pos = IOStream::getPosition();//获取当前字节流入口
	IOStream::skipBytes(4);//跳过signature 长度框
	IOStream::writeBytes(JP2_JP, 4);		// 写入'jp'
	IOStream::writeBytes(0x0d0a870a, 4);//signature的C框内容

	box.length = IOStream::getPosition() - box.init_pos;//计算读取的长度
	IOStream::setPosition(box.init_pos);//回到开始 
	IOStream::writeBytes(box.length, 4);	/*    写入框长度       */
	IOStream::setPosition(box.init_pos + box.length);

}

void jp2Writer::writeFileType(jp2Struct *jp2_struct)
{
	unsigned int i;
	jp2Box box;

	//前边已经来到了file type 框前
	box.init_pos = IOStream::getPosition();
	IOStream::skipBytes(4);//跳过长度框
	IOStream::writeBytes(JP2_FTYP, 4);	/* FTYP       */

	IOStream::writeBytes(jp2_struct->brand, 4);	/* BR   ,定义所采用的具体文件格式      */
	IOStream::writeBytes(jp2_struct->minversion, 4);	/* MinV,定义商标最小版本号       */

	for (i = 0; i < jp2_struct->numcl; i++)
		IOStream::writeBytes(jp2_struct->cl[i], 4);	/* CL ,兼容列表          */

	box.length = IOStream::getPosition() - box.init_pos;//计算长度
	IOStream::setPosition(box.init_pos);
	IOStream::writeBytes(box.length, 4);	/*    L       */
	IOStream::setPosition(box.init_pos + box.length);
}

void jp2Writer::writeHeader(jp2Struct *jp2_struct)
{
	jp2Box box;

	box.init_pos =  IOStream::getPosition();
	IOStream::skipBytes(4);
	IOStream::writeBytes(JP2_JP2H, 4);	/* JP2H */

	writeIHDR(jp2_struct);

	if (jp2_struct->bpc == 255)
		writeBPCC(jp2_struct);//写入bits per component框

	writeColor(jp2_struct);//写入 COLOR SPECIFICATION 框 

	box.length = IOStream::getPosition() - box.init_pos;
	IOStream::setPosition(box.init_pos);
	IOStream::writeBytes(box.length, 4);	/*    L       */
	IOStream::setPosition(box.init_pos + box.length);
}

void jp2Writer::writeIHDR(jp2Struct * jp2_struct)
{
	jp2Box box;

	box.init_pos = IOStream::getPosition();
	IOStream::skipBytes(4);
	IOStream::writeBytes(JP2_IHDR, 4);	// IHDR,Image Header

	IOStream::writeBytes(jp2_struct->height, 4);	// HEIGHT,图像区域中的高度
	IOStream::writeBytes(jp2_struct->width, 4);	// WIDTH,图像区域中的宽度
	IOStream::writeBytes(jp2_struct->numComponents, 2);	// NC

	IOStream::writeBytes(jp2_struct->bpc, 1);	// BPC  

	IOStream::writeBytes(jp2_struct->C, 1);	// C : Always 7
	IOStream::writeBytes(jp2_struct->UnkC, 1);	// UnkC, colorspace unknow
	IOStream::writeBytes(jp2_struct->IPR, 1);	// IPR, no intellectual property

	box.length = IOStream::getPosition() - box.init_pos;
	IOStream::setPosition(box.init_pos);
	IOStream::writeBytes(box.length, 4);	/*    L       */
	IOStream::setPosition(box.init_pos + box.length);
}

void jp2Writer::writeBPCC(jp2Struct * jp2_struct)
{
	unsigned int i;
	jp2Box box;

	box.init_pos = IOStream::getPosition();
	IOStream::skipBytes(4);
	IOStream::writeBytes(JP2_BPCC, 4);	// BPCC

	for (i = 0; i < jp2_struct->numComponents; i++)
		IOStream::writeBytes(jp2_struct->comps[i].bpcc, 1);//输出每一个图像分量的位深度

	box.length = IOStream::getPosition() - box.init_pos;
	IOStream::setPosition(box.init_pos);
	IOStream::writeBytes(box.length, 4);	/*    L       */
	IOStream::setPosition(box.init_pos + box.length);
}

void jp2Writer::writeColor(jp2Struct * jp2_struct)
{
	jp2Box box;

	box.init_pos = IOStream::getPosition();
	IOStream::skipBytes(4);
	IOStream::writeBytes(JP2_COLR, 4);	// COLR

	IOStream::writeBytes(jp2_struct->meth, 1);	// METH,M参数
	IOStream::writeBytes(jp2_struct->precedence, 1);	// PRECEDENCE
	IOStream::writeBytes(jp2_struct->approx, 1);	// APPROX

	if (jp2_struct->meth == 1)
		IOStream::writeBytes(jp2_struct->ECS, 4);	// EnumCS,ECS
	else
		IOStream::writeBytes(0, 1);		// PROFILE (??)

	box.length = IOStream::getPosition() - box.init_pos;
	IOStream::setPosition(box.init_pos);
	IOStream::writeBytes(box.length, 4);	/*    L       */
	IOStream::setPosition(box.init_pos + box.length);
}

int jp2Writer::writeContent(jp2Image *img,CodeParam *cp,char *buffer)
{
	int len;
	jp2Box box;

	box.init_pos = IOStream::getPosition();
	IOStream::skipBytes(4);
	IOStream::writeBytes(JP2_JP2C, 4);	// JP2C

	j2kCoder *coder=new j2kCoder();
	len = coder->j2kEncode(
		img, 
		cp, 
		buffer,
		cp->XTsiz * cp->XTsiz * cp->th * cp->tw * 2 /* (tile大小*tile总数*2) */
		);//数据长度,单位byte

	box.length = IOStream::getPosition() - box.init_pos;
	IOStream::setPosition(box.init_pos);
	IOStream::writeBytes(box.length, 4);	/*    L       */
	IOStream::setPosition(box.init_pos + box.length);
	return box.length;
}