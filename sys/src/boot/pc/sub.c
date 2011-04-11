#include <u.h>
#include <a.out.h>
#include "fns.h"
#include "mem.h"

void
memset(void *p, int v, int n)
{
	uchar *d = p;
	while(n > 0){
		*d++ = v;
		n--;
	}
}

void
memmove(void *dst, void *src, int n)
{
	uchar *d = dst;
	uchar *s = src;
	while(n > 0){
		*d++ = *s++;
		n--;
	}
}

int
memcmp(void *src, void *dst, int n)
{
	uchar *d = dst;
	uchar *s = src;
	int r = 0;
	while((n > 0) && (r = (*d++ - *s++)) == 0)
		n--;
	return r;
}

int
strlen(char *s)
{
	char *p = s;
	while(*p)
		p++;
	return p - s;
}

char*
strchr(char *s, int c)
{
	for(; *s; s++)
		if(*s == c)
			return s;
	return 0;
}

char*
strrchr(char *s, int c)
{
	char *r;
	r = 0;
	while(s = strchr(s, c))
		r = s++;
	return r;
}

void
print(char *s)
{
	while(*s)
		putc(*s++);
}

int
readn(void *f, void *data, int len)
{
	uchar *p, *e;

	p = data;
	e = p + len;
	while(p < e){
		if((len = read(f, p, e - p)) <= 0)
			break;
		p += len;
	}
	return p - (uchar*)data;
}

static ushort
beswab(ushort s)
{
	uchar *p;

	p = (uchar*)&s;
	return (p[0]<<8) | p[1];
}

static ulong
beswal(ulong l)
{
	uchar *p;

	p = (uchar*)&l;
	return (p[0]<<24) | (p[1]<<16) | (p[2]<<8) | p[3];
}

char*
bootkern(void *f)
{
	uchar *e, *d;
	Exec ex;
	int n;

	a20();
	if(readn(f, &ex, sizeof(ex)) != sizeof(ex))
		return "bad header";
	if(beswal(ex.magic) != I_MAGIC)
		return "bad magic";
	e = (uchar*)(beswal(ex.entry) & ~0xF0000000UL);
	n = beswal(ex.text);
	if(readn(f, e, n) != n)
		goto err;
	d = (uchar*)(((ulong)e + n + 0xFFF) & ~0xFFFUL);
	n = beswal(ex.data);
	if(readn(f, d, n) != n)
		goto err;
	close(f);
	jump(e);
err:		
	return "i/o error";
}

static int
readline(void *f, char buf[64])
{
	static char white[] = "\t ";
	char *p;

	p = buf;
	do{
		if(!f)
			putc('>');
		while(p < buf + 64-1){
			if(!f){
				putc(*p = getc());
				if(*p == '\r')
					putc('\n');
				else if(*p == 0x08 && p > buf){
					p--;
					continue;
				}
			}else if(read(f, p, 1) <= 0)
				return 0;
			if(p == buf && strchr(white, *p))
				continue;
			if(strchr(crnl, *p))
				break;
			p++;
		}
		while(p > buf && strchr(white, p[-1]))
			p--;
	}while(p == buf);
	*p = 0;
	return p - buf;
}

char*
configure(void *f, char *path)
{
	char line[64], *p, *kern;
	int inblock, n;

	kern = 0;
	inblock = 0;
	p = (char*)((CONFADDR + 64) & ~0xF0000000UL);
Loop:
	while((n = readline(f, line)) > 0){
		if(*line == 0 || strchr("#;=", *line))
			continue;
		if(*line == '['){
			inblock = memcmp("[common]", line, 8);
			continue;
		}
		if(memcmp("boot", line, 5) == 0)
			break;
		if(inblock || !strrchr(line, '='))
			continue;
		print(line); print(crnl);
		if(memcmp("bootfile=", line, 9) == 0)
			memmove(kern = path, line+9, 1 + n-9);
		memmove(p, line, n); p += n;
		*p++ = '\n';
	}
	*p = 0;
	if(f){
		close(f);
		f = 0;
	}
	if(!kern){
		print("no bootfile\r\n");
		goto Loop;
	}
	for(n=0; n<10000; n++)
		if(gotc())
			goto Loop;
	if(p = strrchr(kern, '!'))
		kern = p+1;
	return kern;
}
