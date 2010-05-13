#include <rtthread.h>
#include <string.h>
#include <dfs_posix.h>
#include "utils.h"

rt_uint32_t read_line(int fd, char* line, rt_uint32_t line_size)
{
	char *pos, *next;
	rt_uint32_t length;

	length = read(fd, line, line_size);
	if (length > 0)
	{
		pos = strstr(line, "\r\n");
		if (pos == RT_NULL)
		{
			pos = strstr(line, "\n");
			next = pos ++;
		}
		else next = pos + 2;

		if (pos != RT_NULL)
		{
			*pos = '\0';

			/* move back */
			lseek(fd, -(length - (next - line)), SEEK_CUR);

			length = pos - line;
		}
		else length = 0;
	}

	return length;
}

char *strncasestr(const char *haystack, const char *needle)
{
	size_t nl=strlen(needle);
	size_t hl=strlen(haystack);
	size_t i;

	if (!nl) goto found;
	if (nl>hl) return 0;
	for (i=hl-nl+1; i; --i)
	{
		if (*haystack==*needle && !strncasecmp(haystack,needle,nl))
found:
			return (char*)haystack;
		++haystack;
	}
	return 0;
}

int media_type(const char* fn)
{
	if (strstr(fn, "http://") != RT_NULL ||
		strstr(fn, "HTTP://") != RT_NULL)
		return MEDIA_RADIO;

	if (strstr(fn, ".mp3") != RT_NULL ||
			strstr(fn, ".MP3") != RT_NULL)
		return MEDIA_MP3;

	if (strstr(fn, ".wav") != RT_NULL ||
			strstr(fn, ".WAV") != RT_NULL)
		return MEDIA_WAV;

	if (strstr(fn, ".wma") != RT_NULL ||
			strstr(fn, ".WMA") != RT_NULL)
		return MEDIA_WMA;

	if (strstr(fn, ".m3u") != RT_NULL ||
			strstr(fn, ".M3U") != RT_NULL)
		return MEDIA_M3U;

	return MEDIA_UNKNOWN;
}

rt_bool_t is_directory(const char* path)
{
	struct stat s;

	stat(path, &s);
	if ( s.st_mode & DFS_S_IFDIR )
		return RT_TRUE;

	return RT_FALSE;
}

