/*
    This file is part of the KDE libraries

    SPDX-FileCopyrightText: 2011 David Faure <faure@kde.org>

    SPDX-License-Identifier: LGPL-2.1-only
*/

#include "kfilesystemtype.h"
#include "kcoreaddons_debug.h"

#include <QFile>

#ifndef Q_OS_WIN
inline KFileSystemType::Type kde_typeFromName(const char *name)
{
    /* clang-format off */
    if (qstrncmp(name, "nfs", 3) == 0
        || qstrncmp(name, "autofs", 6) == 0
        || qstrncmp(name, "cachefs", 7) == 0
        || qstrncmp(name, "fuse.sshfs", 10) == 0
        || qstrncmp(name, "xtreemfs@", 9) == 0) { // #178678

        return KFileSystemType::Nfs;
    }
    if (qstrncmp(name, "fat", 3) == 0
        || qstrncmp(name, "vfat", 4) == 0
        || qstrncmp(name, "msdos", 5) == 0) {
        return KFileSystemType::Fat;
    }
    if (qstrncmp(name, "cifs", 4) == 0
        || qstrncmp(name, "smbfs", 5) == 0) {
        return KFileSystemType::Smb;
    }
    if (qstrncmp(name, "ramfs", 5) == 0) {
        return KFileSystemType::Ramfs;
    }
    /* clang-format on */

    return KFileSystemType::Other;
}

#if defined(Q_OS_BSD4) && !defined(Q_OS_NETBSD)
#include <sys/mount.h>
#include <sys/param.h>

KFileSystemType::Type determineFileSystemTypeImpl(const QByteArray &path)
{
    struct statfs buf;
    if (statfs(path.constData(), &buf) != 0) {
        return KFileSystemType::Unknown;
    }
    return kde_typeFromName(buf.f_fstypename);
}

#elif defined(Q_OS_LINUX) || defined(Q_OS_HURD)
#include <sys/vfs.h>
#ifdef QT_LINUXBASE
// LSB 3.2 has statfs in sys/statfs.h, sys/vfs.h is just an empty dummy header
#include <sys/statfs.h>
#endif
#ifndef NFS_SUPER_MAGIC
#define NFS_SUPER_MAGIC 0x00006969
#endif
#ifndef AUTOFS_SUPER_MAGIC
#define AUTOFS_SUPER_MAGIC 0x00000187
#endif
#ifndef AUTOFSNG_SUPER_MAGIC
#define AUTOFSNG_SUPER_MAGIC 0x7d92b1a0
#endif
#ifndef MSDOS_SUPER_MAGIC
#define MSDOS_SUPER_MAGIC 0x00004d44
#endif
#ifndef SMB_SUPER_MAGIC
#define SMB_SUPER_MAGIC 0x0000517B
#endif
#ifndef SMB2_MAGIC_NUMBER
#define SMB2_MAGIC_NUMBER 0xFE534D42
#endif
#ifndef CIFS_MAGIC_NUMBER
#define CIFS_MAGIC_NUMBER 0xFF534D42
#endif
#ifndef FUSE_SUPER_MAGIC
#define FUSE_SUPER_MAGIC 0x65735546
#endif
#ifndef RAMFS_MAGIC
#define RAMFS_MAGIC 0x858458F6
#endif

// Reverse-engineering without C++ code:
// strace stat -f /mnt 2>&1|grep statfs|grep mnt, and look for f_type

static KFileSystemType::Type determineFileSystemTypeImpl(const QByteArray &path)
{
    struct statfs buf;
    if (statfs(path.constData(), &buf) != 0) {
        return KFileSystemType::Unknown;
    }
    switch (static_cast<unsigned long>(buf.f_type)) {
    case NFS_SUPER_MAGIC:
    case AUTOFS_SUPER_MAGIC:
    case AUTOFSNG_SUPER_MAGIC:
    case FUSE_SUPER_MAGIC: // TODO could be anything. Need to use statfs() to find out more.
        return KFileSystemType::Nfs;
    case SMB_SUPER_MAGIC:
    case SMB2_MAGIC_NUMBER:
    case CIFS_MAGIC_NUMBER:
        return KFileSystemType::Smb;
    case MSDOS_SUPER_MAGIC:
        return KFileSystemType::Fat;
    case RAMFS_MAGIC:
        return KFileSystemType::Ramfs;
    default:
        return KFileSystemType::Other;
    }
}

#elif defined(Q_OS_SOLARIS) || defined(Q_OS_IRIX) || defined(Q_OS_AIX) || defined(Q_OS_HPUX) || defined(Q_OS_OSF) || defined(Q_OS_QNX) || defined(Q_OS_SCO)    \
    || defined(Q_OS_UNIXWARE) || defined(Q_OS_RELIANT) || defined(Q_OS_NETBSD)
#include <sys/statvfs.h>

KFileSystemType::Type determineFileSystemTypeImpl(const QByteArray &path)
{
    struct statvfs buf;
    if (statvfs(path.constData(), &buf) != 0) {
        return KFileSystemType::Unknown;
    }
#if defined(Q_OS_NETBSD)
    return kde_typeFromName(buf.f_fstypename);
#else
    return kde_typeFromName(buf.f_basetype);
#endif
}
#endif
#else
KFileSystemType::Type determineFileSystemTypeImpl(const QByteArray &path)
{
    return KFileSystemType::Unknown;
}
#endif

KFileSystemType::Type KFileSystemType::fileSystemType(const QString &path)
{
    return determineFileSystemTypeImpl(QFile::encodeName(path));
}
