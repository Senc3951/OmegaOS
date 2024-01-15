#pragma once

#define	FREAD		0x0001
#define	FWRITE		0x0002
#define	O_NONBLOCK	0x0004		/* no delay */
#define	O_APPEND	0x0008		/* set append mode */
#define	O_SHLOCK	0x0010		/* open with shared file lock */
#define	O_EXLOCK	0x0020		/* open with exclusive file lock */
#define	O_ASYNC		0x0040		/* signal pgrp when data ready */
#define	O_FSYNC		0x0080		/* synchronous writes */
#define	O_SYNC		0x0080		/* POSIX synonym for O_FSYNC */
#define	O_NOFOLLOW	0x0100		/* don't follow symlinks */
#define	O_CREAT		0x0200		/* create if nonexistent */
#define	O_TRUNC		0x0400		/* truncate to zero length */
#define	O_EXCL		0x0800		/* error if already exists */