#ifndef __UTILS_H__
#define __UTILS_H__

/* some utilities in stm32 radio */

#define MEDIA_UNKNOWN	-1
#define MEDIA_WAV		0
#define MEDIA_MP3		1
#define MEDIA_WMA		2
#define MEDIA_M3U		3
#define MEDIA_RADIO		4

int media_type(const char* fn);
char *strncasestr(const char *haystack, const char *needle);
rt_uint32_t read_line(int fd, char* line, rt_uint32_t line_size);
rt_bool_t is_directory(const char* path);

#endif
