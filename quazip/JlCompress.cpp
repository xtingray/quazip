/*
Copyright (C) 2010 Roberto Pompermaier
Copyright (C) 2005-2014 Sergey A. Tachenov

This file is part of QuaZIP.

QuaZIP is free software: you can redistribute it and/or modify
it under the terms of the GNU Lesser General Public License as published by
the Free Software Foundation, either version 2.1 of the License, or
(at your option) any later version.

QuaZIP is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public License
along with QuaZIP.  If not, see <http://www.gnu.org/licenses/>.

See COPYING file for the full LGPL text.

Original ZIP package is copyrighted by Gilles Vollant and contributors,
see quazip/(un)zip.h files for details. Basically it's the zlib license.
*/

#include "JlCompress.h"
#ifdef Q_DEBUG
#include <QDebug>
#endif

static bool copyData(QIODevice &inFile, QIODevice &outFile)
{
    #ifdef Q_DEBUG
        qDebug() << "*** JlCompress::copyData() - Tracing...";
    #endif

    while (!inFile.atEnd()) {
        char buf[4096];
        qint64 readLen = inFile.read(buf, 4096);
        if (readLen <= 0) {
            #ifdef Q_DEBUG
                qDebug() << "**** JlCompress::copyData() - Fatal error while reading input file";
            #endif
            return false;
        }

        if (outFile.write(buf, readLen) != readLen) {
            #ifdef Q_DEBUG
                qDebug() << "**** JlCompress::copyData() - Fatal error while writing output file";
            #endif
            return false;
        }
    }

    #ifdef Q_DEBUG
        qDebug() << "*** JlCompress::copyData() - Copy operation was successful";
    #endif

    return true;
}

bool JlCompress::compressFile(QuaZip* zip, QString fileName, QString fileDest) {
    #ifdef Q_DEBUG
        qDebug() << "*** JlCompress::compressFile() - fileName -> " << fileName;
        qDebug() << "*** JlCompress::compressFile() - fileDest -> " << fileDest;
    #endif

    // zip: object where to add the file
    // fileName: name of the real file
    // fileDest: name of the file inside the compressed file

    // I check the opening of the zip
    if (!zip) { 
        #ifdef Q_DEBUG
            qDebug() << "**** JlCompress::compressFile() - Fatal error: zip file is NULL";
        #endif
        return false;
    }

    if (zip->getMode()!=QuaZip::mdCreate &&
        zip->getMode()!=QuaZip::mdAppend &&
        zip->getMode()!=QuaZip::mdAdd) {
        #ifdef Q_DEBUG
            qDebug() << "**** JlCompress::compressFile() - Fatal error: can't create/edit zip file";
        #endif
        return false;
    }

    // I open the original file
    QFile inFile;
    inFile.setFileName(fileName);
    if (!inFile.open(QIODevice::ReadOnly)) {
        #ifdef Q_DEBUG
            qDebug() << "**** JlCompress::compressFile() - Fatal error: can't open input file (no read permission) -> " << fileName;
        #endif
        return false;
    }

    // I open the result file
    QuaZipFile outFile(zip);
    if (!outFile.open(QIODevice::WriteOnly, QuaZipNewInfo(fileDest, inFile.fileName()))) {
        #ifdef Q_DEBUG
            qDebug() << "**** JlCompress::compressFile() - Fatal error: can't open output file -> " << fileDest;
        #endif
        return false;
    }

    // I copy the data
    if (!copyData(inFile, outFile) || outFile.getZipError() != UNZ_OK) {
        #ifdef Q_DEBUG
            qDebug() << "**** JlCompress::compressFile() - Fatal error: can't copy input file into output file -> " << fileDest;
        #endif
        return false;
    }

    // I close the files
    outFile.close();

    if (outFile.getZipError() != UNZ_OK) { 
        #ifdef Q_DEBUG
            qDebug() << "**** JlCompress::compressFile() - Fatal error: can't close output zip file";
        #endif
        return false;
    }

    inFile.close();

    #ifdef Q_DEBUG
        qDebug() << "**** JlCompress::compressFile() - Operation was successful";
    #endif

    return true;
}

bool JlCompress::compressSubDir(QuaZip* zip, QString dir, QString origDir, bool recursive, QDir::Filters filters) {
    #ifdef Q_DEBUG
        qDebug() << "** JlCompress::compressSubDir() - Compressing dir -> " << dir;
        qDebug() << "** JlCompress::compressSubDir() - origDir -> " << origDir;
    #endif

    // zip: object where to add the file
    // dir: current real folder
    // origDir: original real folder
    // (path (dir) -path (origDir)) = path inside the zip object

    // I check the opening of the zip
    if (!zip) { 
        #ifdef Q_DEBUG
            qDebug() << "*** JlCompress::compressSubDir() - Fatal error: zip file is NULL";
        #endif
        return false;
    }

    if (zip->getMode() != QuaZip::mdCreate &&
        zip->getMode() != QuaZip::mdAppend &&
        zip->getMode() != QuaZip::mdAdd) {
        #ifdef Q_DEBUG
            qDebug() << "*** JlCompress::compressSubDir() - Fatal error: can't create/edit zip file";
        #endif
        return false;
    }

    // I check the folder
    QDir directory(dir);
    if (!directory.exists()) {
        #ifdef Q_DEBUG
            qDebug() << "*** JlCompress::compressSubDir() - Fatal error: folder doesn't exist";
        #endif
        return false;
    }

    QDir origDirectory(origDir);

    if (recursive) {
        // For each subfolder
        QFileInfoList files = directory.entryInfoList(QDir::AllDirs | QDir::NoDotAndDotDot | filters);
        for (int index = 0; index < files.size(); ++index) {
            const QFileInfo & file(files.at(index));
            // I compress the subfolder
            if (!compressSubDir(zip, file.absoluteFilePath(), origDir, recursive, filters)) {
                #ifdef Q_DEBUG
                    qDebug() << "*** JlCompress::compressSubDir() - Fatal error: can't compress item -> " << file.absoluteFilePath();
                #endif
                return false;
            }
        }
    }

    // For each file in the folder
    QFileInfoList files = directory.entryInfoList(QDir::Files | filters);
    for (int index = 0; index < files.size(); ++index ) {
        const QFileInfo & file(files.at(index));
        // If it's not a file or it's the compressed file I'm creating
        if (!file.isFile() || file.absoluteFilePath() == zip->getZipName()) 
            continue;

        // I create the relative name to use within the compressed file
        QString filename = origDirectory.dirName() + QLatin1String("/") + origDirectory.relativeFilePath(file.absoluteFilePath());

        // I compress the file
        if (!compressFile(zip, file.absoluteFilePath(), filename)) {
            #ifdef Q_DEBUG
                qDebug() << "*** JlCompress::compressSubDir() - Fatal error: can't compress item -> " << file.absoluteFilePath();
            #endif
            return false;
        }
    }

    #ifdef Q_DEBUG
        qDebug() << "*** JlCompress::compressSubDir() - Operation successful for item -> " << dir;
    #endif

    return true;
}

bool JlCompress::extractFile(QuaZip* zip, QString fileName, QString fileDest) {
    #ifdef Q_DEBUG
        // qDebug() << "** JlCompress::extractFile() - fileName -> " << fileName;
        qDebug() << "** JlCompress::extractFile() - fileDest -> " << fileDest;
    #endif

    // zip: object where to add the file
    // filename: real file name
    // fileincompress: name of the file inside the compressed file

    // I check the opening of the zip
    if (!zip) {
        #ifdef Q_DEBUG
            qDebug() << "*** JlCompress::extractFile() - Fatal error: zip file is NULL";
        #endif
        return false;
    }

    if (zip->getMode() != QuaZip::mdUnzip) {
        #ifdef Q_DEBUG
            qDebug() << "*** JlCompress::extractFile() - Fatal error: zip mode is invalid";
        #endif
        return false;
    }

    // I open the compressed file
    if (!fileName.isEmpty()) 
        zip->setCurrentFile(fileName);

    QuaZipFile inFile(zip);
    if (!inFile.open(QIODevice::ReadOnly) || inFile.getZipError() != UNZ_OK)  {
        #ifdef Q_DEBUG
            qDebug() << "*** JlCompress::extractFile() - Fatal error: invalid read permissions for the zip file";
        #endif
        return false;
    }

    // I check the existence of folder file result
    QDir curDir;
    if (fileDest.endsWith(QLatin1String("/"))) {
        if (!curDir.mkpath(fileDest)) {
            #ifdef Q_DEBUG
                qDebug() << "*** JlCompress::extractFile() - Fatal error: can't create zip dir path - fileDest -> " << fileDest;
            #endif
            return false;
        }
    } else {
        if (!curDir.mkpath(QFileInfo(fileDest).absolutePath())) {
            #ifdef Q_DEBUG
                qDebug() << "*** JlCompress::extractFile() - Fatal error: can't create zip dir path - fileDest -> " << fileDest;
            #endif
            return false;
        }
    }

    QuaZipFileInfo64 info;
    if (!zip->getCurrentFileInfo(&info)) {
        #ifdef Q_DEBUG
            qDebug() << "*** JlCompress::extractFile() - Fatal error: can't zip info file";
        #endif
        return false;
    }

    QFile::Permissions srcPerm = info.getPermissions();
    if (fileDest.endsWith(QLatin1String("/")) && QFileInfo(fileDest).isDir()) {
        if (srcPerm != 0) {
            QFile(fileDest).setPermissions(srcPerm);
        }
        return true;
    }

    // I open the result file
    QFile outFile;
    outFile.setFileName(fileDest);
    if (!outFile.open(QIODevice::WriteOnly)) {
        #ifdef Q_DEBUG
            qDebug() << "*** JlCompress::extractFile() - Fatal error: can't open output file -> " << fileDest;
        #endif
        return false;
    }

    // I copy the data
    if (!copyData(inFile, outFile) || inFile.getZipError() != UNZ_OK) {
        #ifdef Q_DEBUG
            qDebug() << "*** JlCompress::extractFile() - Fatal error: can't copy data from input file into output file";
        #endif
        outFile.close();
        removeFile(QStringList(fileDest));
        return false;
    }
    outFile.close();

    // I close the files
    inFile.close();
    if (inFile.getZipError() != UNZ_OK) {
        #ifdef Q_DEBUG
           qDebug() << "*** JlCompress::extractFile() - Fatal error: can't close input zip file";
        #endif
        removeFile(QStringList(fileDest));
        return false;
    }

    if (srcPerm != 0) {
        outFile.setPermissions(srcPerm);
    }

    #ifdef Q_DEBUG
        qDebug() << "*** JlCompress::extractFile() - Operation was successful - file -> " << fileName;
    #endif

    return true;
}

bool JlCompress::removeFile(QStringList listFile) {
    #ifdef Q_DEBUG
        qDebug() << "* JlCompress::removeFile() - listFile -> " << listFile;
    #endif

    bool ret = true;
    // For each file
    for (int i=0; i<listFile.count(); i++) {
        // I eliminate it
        ret = ret && QFile::remove(listFile.at(i));
    }
    return ret;
}

bool JlCompress::compressFile(QString fileCompressed, QString file) {
    #ifdef Q_DEBUG
        qDebug() << "** JlCompress::compressFile() - fileCompressed -> " << fileCompressed;
        qDebug() << "** JlCompress::compressFile() - file -> " << file;
    #endif

    // I create the zip
    QuaZip zip(fileCompressed);
    QDir().mkpath(QFileInfo(fileCompressed).absolutePath());
    if (!zip.open(QuaZip::mdCreate)) {
        #ifdef Q_DEBUG
            qDebug() << "*** JlCompress::compressFile() - Fatal error: can't open zip object";
        #endif
        QFile::remove(fileCompressed);
        return false;
    }

    // I add the file
    if (!compressFile(&zip, file, QFileInfo(file).fileName())) {
        #ifdef Q_DEBUG
            qDebug() << "*** JlCompress::compressFile() - Fatal error: can't compress/add file -> " << file;
        #endif
        QFile::remove(fileCompressed);
        return false;
    }

    // I close the zip file
    zip.close();
    if (zip.getZipError() != 0) {
        #ifdef Q_DEBUG
            qDebug() << "*** JlCompress::compressFile() - Fatal error: can't close zip file";
        #endif
        QFile::remove(fileCompressed);
        return false;
    }

    #ifdef Q_DEBUG
        qDebug() << "*** JlCompress::compressFile() - Operation was successful - file -> " << file;
    #endif

    return true;
}

bool JlCompress::compressFiles(QString fileCompressed, QStringList files) {
    #ifdef Q_DEBUG
        qDebug() << "* JlCompress::compressFiles() - fileCompressed -> " << fileCompressed;
        qDebug() << "* JlCompress::compressFiles() - files -> " << files;
    #endif

    // I create the zip
    QuaZip zip(fileCompressed);
    QDir().mkpath(QFileInfo(fileCompressed).absolutePath());
    if (!zip.open(QuaZip::mdCreate)) {
        #ifdef Q_DEBUG
            qDebug() << "** JlCompress::compressFiles() - Fatal error: can't open zip object";
        #endif
        QFile::remove(fileCompressed);
        return false;
    }

    // I compress the files
    QFileInfo info;
    for (int index = 0; index < files.size(); ++index ) {
        const QString & file(files.at(index));
        info.setFile(file);
        if (!info.exists() || !compressFile(&zip, file, info.fileName())) {
            #ifdef Q_DEBUG
                qDebug() << "** JlCompress::compressFiles() - Fatal error: can't compress/add file -> " << file;
            #endif
            QFile::remove(fileCompressed);
            return false;
        }
    }

    // I close the zip file
    zip.close();
    if(zip.getZipError() != 0) {
        #ifdef Q_DEBUG
            qDebug() << "** JlCompress::compressFiles() - Fatal error: can't close zip file";
        #endif
        QFile::remove(fileCompressed);
        return false;
    }

    return true;
}

bool JlCompress::compressDir(QString fileCompressed, QString dir, bool recursive) {
    return compressDir(fileCompressed, dir, recursive, 0);
}

bool JlCompress::compressDir(QString fileCompressed, QString dir,
                             bool recursive, QDir::Filters filters)
{
    #ifdef Q_DEBUG
        qDebug() << "--";
        qDebug() << "* JlCompress::compressDir() - Compressing dir -> " << dir; 
    #endif

    // I create the zip
    QuaZip zip(fileCompressed);
    QDir().mkpath(QFileInfo(fileCompressed).absolutePath());
    if (!zip.open(QuaZip::mdCreate)) {
        #ifdef Q_DEBUG
            qDebug() << "** JlCompress::compressDir() - Fatal error while creating zip file";
        #endif
        QFile::remove(fileCompressed);
        return false;
    }

    // I add the files and sub folders
    if (!compressSubDir(&zip, dir, dir, recursive, filters)) {
        #ifdef Q_DEBUG
            qDebug() << "** JlCompress::compressDir() - Fatal error while compressing subdir -> " << dir;
        #endif
        QFile::remove(fileCompressed);
        return false;
    }

    // I close the zip file
    zip.close();
    if (zip.getZipError() != 0) {
        #ifdef Q_DEBUG
            qDebug() << "** JlCompress::compressDir() - Fatal error while closing zip file";
        #endif
        QFile::remove(fileCompressed);
        return false;
    }

    #ifdef Q_DEBUG
        qDebug() << "* JlCompress::compressDir() - Operation was successful";  
        qDebug() << "--";
    #endif

    return true;
}

QString JlCompress::extractFile(QString fileCompressed, QString fileName, QString fileDest) {
    // I open the zip
    QuaZip zip(fileCompressed);
    return extractFile(zip, fileName, fileDest);
}

QString JlCompress::extractFile(QuaZip &zip, QString fileName, QString fileDest)
{
    #ifdef Q_DEBUG
        qDebug() << "* JlCompress::extractFile() - fileName -> " << fileName;
        qDebug() << "* JlCompress::extractFile() - fileDest -> " << fileDest;
    #endif

    if (!zip.open(QuaZip::mdUnzip)) {
        return QString();
    }

    // I extract the file
    if (fileDest.isEmpty())
        fileDest = fileName;
    if (!extractFile(&zip, fileName, fileDest)) {
        return QString();
    }

    // I close the zip file
    zip.close();
    if (zip.getZipError() != 0) {
        #ifdef Q_DEBUG
            qDebug() << "** JlCompress::extractFile() - Fatal error while closing zip file";
        #endif
        removeFile(QStringList(fileDest));
        return QString();
    }

    return QFileInfo(fileDest).absoluteFilePath();
}

QStringList JlCompress::extractFiles(QString fileCompressed, QStringList files, QString dir) {
    // I create the zip
    QuaZip zip(fileCompressed);
    return extractFiles(zip, files, dir);
}

QStringList JlCompress::extractFiles(QuaZip &zip, const QStringList &files, const QString &dir)
{
    #ifdef Q_DEBUG
        qDebug() << "* JlCompress::extractFiles() - files -> " << files;
        qDebug() << "* JlCompress::extractFiles() - dir -> " << dir;
    #endif

    if (!zip.open(QuaZip::mdUnzip)) {
        return QStringList();
    }

    // I extract the files
    QStringList extracted;
    for (int i=0; i<files.count(); i++) {
        QString absPath = QDir(dir).absoluteFilePath(files.at(i));
        if (!extractFile(&zip, files.at(i), absPath)) {
            #ifdef Q_DEBUG
                qDebug() << "** JlCompress::extractFiles() - Fatal error while expanding file -> " << files.at(i);
            #endif
            removeFile(extracted);
            return QStringList();
        }
        extracted.append(absPath);
    }

    // I close the zip file
    zip.close();
    if (zip.getZipError() != 0) {
        #ifdef Q_DEBUG
            qDebug() << "** JlCompress::extractFiles() - Fatal error while closing zip file";
        #endif
        removeFile(extracted);
        return QStringList();
    }

    return extracted;
}

QStringList JlCompress::extractDir(QString fileCompressed, QTextCodec* fileNameCodec, QString dir) 
{
    // I open the zip 
    QuaZip zip(fileCompressed);
    if (fileNameCodec)
        zip.setFileNameCodec(fileNameCodec);

    return extractDir(zip, dir);
}

QStringList JlCompress::extractDir(QString fileCompressed, QString dir) 
{
    #ifdef Q_DEBUG
        qDebug() << "--";
        qDebug() << "* JlCompress::extractDir() - Opening zip file -> " << fileCompressed;
    #endif

    return extractDir(fileCompressed, NULL, dir);
}

QStringList JlCompress::extractDir(QuaZip &zip, const QString &dir)
{
    #ifdef Q_DEBUG
        qDebug() << "* JlCompress::extractDir() - Extracting files into dir -> " << dir;
    #endif

    if (!zip.open(QuaZip::mdUnzip)) {
        return QStringList();
    }

    QString cleanDir = QDir::cleanPath(dir);
    QDir directory(cleanDir);
    QString absCleanDir = directory.absolutePath();
    QStringList extracted;
    if (!zip.goToFirstFile()) {
        #ifdef Q_DEBUG
            qDebug() << "** JlCompress::extractDir() - Fatal error while reaching first file";
        #endif
        return QStringList();
    }

    do {
        QString name = zip.getCurrentFileName();
        QString absFilePath = directory.absoluteFilePath(name);
        QString absCleanPath = QDir::cleanPath(absFilePath);
        if (!absCleanPath.startsWith(absCleanDir + QLatin1String("/")))
            continue;
        if (!extractFile(&zip, QLatin1String(""), absFilePath)) {
            #ifdef Q_DEBUG
                qDebug() << "** JlCompress::extractDir() - Fatal error while extracting file -> " << absFilePath;
            #endif
            removeFile(extracted);
            return QStringList();
        }
        extracted.append(absFilePath);
    } while (zip.goToNextFile());

    // I close the zip file
    zip.close();
    if (zip.getZipError() != 0) {
        #ifdef Q_DEBUG
            qDebug() << "** JlCompress::extractDir() - Fatal error while closing zip file";
        #endif
        removeFile(extracted);
        return QStringList();
    }

    #ifdef Q_DEBUG
        qDebug() << "* JlCompress::extractDir() - Zip file extracted successfully";
        qDebug() << "* " << extracted;
        qDebug() << "--";
    #endif

    return extracted;
}

QStringList JlCompress::getFileList(QString fileCompressed) {
    // I open the zip
    QuaZip* zip = new QuaZip(QFileInfo(fileCompressed).absoluteFilePath());
    return getFileList(zip);
}

QStringList JlCompress::getFileList(QuaZip *zip)
{
    #ifdef Q_DEBUG
        qDebug() << "* JlCompress::getFileList() - Tracing...";
    #endif

    if(!zip->open(QuaZip::mdUnzip)) {
        #ifdef Q_DEBUG
            qDebug() << "** JlCompress::getFileList() - Fatal error: can't open zip file";
        #endif
        delete zip;
        return QStringList();
    }

    // I extract the file names
    QStringList lst;
    QuaZipFileInfo64 info;
    for (bool more=zip->goToFirstFile(); more; more=zip->goToNextFile()) {
        if (!zip->getCurrentFileInfo(&info)) {
            #ifdef Q_DEBUG
                qDebug() << "** JlCompress::getFileList() - Fatal error while accessing current file info";
            #endif
            delete zip;
            return QStringList();
        }
        lst << info.name;
        // info.name.toLocal8Bit().constData()
    }

    // I close the zip file
    zip->close();
    if (zip->getZipError() != 0) {
        #ifdef Q_DEBUG
            qDebug() << "** JlCompress::getFileList() - Fatal error while closing zip file";
        #endif
        delete zip;
        return QStringList();
    }
    delete zip;

    return lst;
}

QStringList JlCompress::extractDir(QIODevice* ioDevice, QTextCodec* fileNameCodec, QString dir)
{
    QuaZip zip(ioDevice);
    if (fileNameCodec)
        zip.setFileNameCodec(fileNameCodec);
    return extractDir(zip, dir);
}

QStringList JlCompress::extractDir(QIODevice *ioDevice, QString dir)
{
    return extractDir(ioDevice, NULL, dir);
}

QStringList JlCompress::getFileList(QIODevice *ioDevice)
{
    QuaZip *zip = new QuaZip(ioDevice);
    return getFileList(zip);
}

QString JlCompress::extractFile(QIODevice *ioDevice, QString fileName, QString fileDest)
{
    QuaZip zip(ioDevice);
    return extractFile(zip, fileName, fileDest);
}

QStringList JlCompress::extractFiles(QIODevice *ioDevice, QStringList files, QString dir)
{
    QuaZip zip(ioDevice);
    return extractFiles(zip, files, dir);
} 
