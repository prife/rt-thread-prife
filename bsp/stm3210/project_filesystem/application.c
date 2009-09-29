/*
 * File      : application.c
 * This file is part of RT-Thread RTOS
 * COPYRIGHT (C) 2006, RT-Thread Development Team
 *
 * The license and distribution terms for this file may be
 * found in the file LICENSE in this distribution or at
 * http://www.rt-thread.org/license/LICENSE
 *
 * Change Logs:
 * Date           Author       Notes
 * 2009-01-05     Bernard      the first version
 */

/**
 * @addtogroup STM32
 */
/*@{*/

#include <rtthread.h>

#ifdef RT_USING_DFS
/* dfs init */
#include <dfs_init.h>
/* dfs filesystem:FAT filesystem init */
#include <dfs_fat.h>
/* dfs filesystem:EFS filesystem init */
#include <dfs_efs.h>
/* dfs Filesystem APIs */
#include <dfs_fs.h>
#endif

/* filesystem test */
#include <dfs_posix.h>
static char fullpath[256 + 1];
void ls_root()
{
	DIR *dir;
	
	dir = opendir("/");
	if (dir != RT_NULL)
	{
		struct dfs_dirent* dirent;
		struct dfs_stat s;

		do 
		{
			dirent = readdir(dir);
			if (dirent == RT_NULL) break;
			rt_memset(&s, 0, sizeof(struct dfs_stat));

			/* build full path for each file */
			rt_sprintf(fullpath, "/%s", dirent->d_name);

			stat(fullpath, &s);
			if ( s.st_mode & DFS_S_IFDIR )
			{
				rt_kprintf("%s\t\t<DIR>\n", dirent->d_name);
			}
			else
			{
				rt_kprintf("%s\t\t%lu\n", dirent->d_name, s.st_size);
			}
		} while (dirent != RT_NULL);

		closedir(dir);
	}
	else rt_kprintf("open root directory failed\n");
}

void rt_init_thread_entry(void* parameter)
{
/* Filesystem Initialization */
#ifdef RT_USING_DFS
	{
		/* init the device filesystem */
		dfs_init();
		/* init the efsl filesystam*/
		efsl_init();

		/* mount sd card fat partition 1 as root directory */
		if (dfs_mount("sd0", "/", "efs", 0, 0) == 0)
		{
			rt_kprintf("File System initialized!\n");
			ls_root();
		}
		else
			rt_kprintf("File System init failed!\n");

	}
#endif
}

int rt_application_init()
{
	rt_thread_t init_thread;

#if (RT_THREAD_PRIORITY_MAX == 32)
	init_thread = rt_thread_create("init",
								rt_init_thread_entry, RT_NULL,
								2048, 8, 20);
#else
	init_thread = rt_thread_create("init",
								rt_init_thread_entry, RT_NULL,
								2048, 80, 20);
#endif

	if (init_thread != RT_NULL)
		rt_thread_startup(init_thread);

	return 0;
}

/*@}*/
