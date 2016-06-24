#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define SEEK_SET	       0 /* 文件开头的标记 */
#define SEEK_CUR	1 /* 当前位置的标记 */
#define SEEK_END	2 /* 文件末尾的标记 */

#define BUFSIZE                 (24*1024)
#define IMG_SIZE                (24*1024)
#define SPL_HEADER_SIZE         16
#define SPL_HEADER              "S5PC110 HEADER  "

int main (int argc, char *argv[])
{
	FILE		*fp;    /* 定义一个文件指针 */
	char		*Buf, *a;
	int		BufLen;
	int		nbytes, fileLen;
	unsigned int	checksum, count;
	int		i;

	if (argc != 3) /* 如果参数个数错误，打印帮助信息 */
	{
		/* 应用工具时,格式必须是 ./mktiny210spl.exe old.bin new.bin */
		printf("Usage: mkbl1 <source file> <destination file>\n");
		return -1;
	}

	BufLen = BUFSIZE;
	Buf = (char *)malloc(BufLen); /* 动态分配一段24k的内存空间 */
	if (!Buf) /* 分配失败，将返回0 */
	{
		printf("Alloc buffer failed!\n");
		return -1;
	}

	memset(Buf, 0x00, BufLen); /* 将上面分配的空间清零 */

	fp = fopen(argv[1], "rb"); /* 以读二进制的方式打开没有头部信号的old.bin文件 */
	if( fp == NULL)
	{
		printf("source file open error\n");
		free(Buf); /* 如果打开失败，释放掉原来分配的内存，否则会造成内存泄漏 */
		return -1;
	}

	fseek(fp, 0L, SEEK_END); /* 让文件位置指针指向文件末尾，便于下行的统计大小的操作 */
	fileLen = ftell(fp); /* 用于得到文件位置指针当前位置相对于文件首的偏移字节数,即文件大小*/
	fseek(fp, 0L, SEEK_SET); /* 让文件位置指针指向文件开始 */

	/* 如果old.bin文件的大小小于规定的最大大小，则count等于该文件的大小，否则等于最大大小 */
	count = (fileLen < (IMG_SIZE - SPL_HEADER_SIZE))
		? fileLen : (IMG_SIZE - SPL_HEADER_SIZE);

	memcpy(&Buf[0], SPL_HEADER, SPL_HEADER_SIZE); /* 拷贝16字节的数据到Buf中，即初始化头部信息的位置 */

	nbytes = fread(Buf + SPL_HEADER_SIZE, 1, count, fp); /* 将编译生成的old.bin文件拷贝到buf中，紧接着头部信息开始拷贝 */

	if ( nbytes != count ) /* 返回值等于拷贝的元素的个数 */
	{
		printf("source file read error\n"); /* 如果个数和实际的不想等，则失败 */
		free(Buf); /* 释放内存 */
		fclose(fp); /* 关闭文件 */
		return -1;
	}

	fclose(fp); /* 关闭文件 */

	/* 以下三行，用于动态生成checksum，公式见上面的尝试一 */
	a = Buf + SPL_HEADER_SIZE;
	for(i = 0, checksum = 0; i < IMG_SIZE - SPL_HEADER_SIZE; i++)
		checksum += (0x000000FF) & *a++;

	/* 将checksum写入buf的第三个字节处，即该是checksum的位置处 */
	a = Buf + 8;
	*( (unsigned int *)a ) = checksum;

	fp = fopen(argv[2], "wb"); /* 以二进制写的方式创建一个新的二进制文件 */
	if (fp == NULL)
	{
		printf("destination file open error\n");
		free(Buf); /* 释放内存 */
		return -1;
	}

	a = Buf; /* 指向内存的首地址 */
	nbytes	= fwrite( a, 1, BufLen, fp); /* 把buf中的数据写入新创建的bin文件 */

	if ( nbytes != BufLen ) /* 返回值等于写入的元素的个数 */
	{
		printf("destination file write error\n");
		free(Buf); /* 释放内存 */
		fclose(fp);/* 关闭文件 */
		return -1;
	}

	free(Buf); /* 释放内存 */
	fclose(fp);/* 关闭文件 */

	return 0;
}
