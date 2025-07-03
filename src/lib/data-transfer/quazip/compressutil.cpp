// SPDX-FileCopyrightText: 2025 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "compressutil.h"
#include <QDebug>

static bool copyData(QIODevice &inFile, QIODevice &outFile)
{
    qInfo() << "Copying data between IODevices";
    while (!inFile.atEnd()) {
        char buf[4096];
        qint64 readLen = inFile.read(buf, 4096);
        if (readLen <= 0) {
            qInfo() << "read file failed";
            return false;
        }
        if (outFile.write(buf, readLen) != readLen) {
            qInfo() << "write file failed";
            return false;
        }
    }
    return true;
}

bool CompressUtil::compressFile(QuaZip* zip, QString fileName, QString fileDest) {
    qInfo() << "Compressing file:" << fileName << "to destination:" << fileDest;
    // zip: oggetto dove aggiungere il file
    // fileName: nome del file reale
    // fileDest: nome del file all'interno del file compresso

    // Controllo l'apertura dello zip
    if (!zip) {
        qInfo() << "zip is null";
        return false;
    }
    if (zip->getMode()!=QuaZip::mdCreate &&
        zip->getMode()!=QuaZip::mdAppend &&
        zip->getMode()!=QuaZip::mdAdd) {
        qInfo() << "zip mode is not create, append or add";
        return false;
    }

    // Apro il file originale
    QFile inFile;
    inFile.setFileName(fileName);
    if(!inFile.open(QIODevice::ReadOnly)) {
        qInfo() << "open file failed:" << fileName;
        return false;
    }

    // Apro il file risulato
    QuaZipFile outFile(zip);
    if(!outFile.open(QIODevice::WriteOnly, QuaZipNewInfo(fileDest, inFile.fileName()))) {
        qInfo() << "open quazip file failed:" << fileDest;
        return false;
    }

    // Copio i dati
    if (!copyData(inFile, outFile) || outFile.getZipError()!=UNZ_OK) {
        qInfo() << "copy data failed, error:" << outFile.getZipError();
        return false;
    }

    // Chiudo i file
    outFile.close();
    if (outFile.getZipError()!=UNZ_OK) {
        qInfo() << "close quazip file failed, error:" << outFile.getZipError();
        return false;
    }
    inFile.close();

    return true;
}

bool CompressUtil::compressSubDir(QuaZip* zip, QString dir, QString origDir, bool recursive, QDir::Filters filters) {
    qInfo() << "Compressing sub-directory:" << dir << "original base:" << origDir << "recursive:" << recursive;
    // zip: oggetto dove aggiungere il file
    // dir: cartella reale corrente
    // origDir: cartella reale originale
    // (path(dir)-path(origDir)) = path interno all'oggetto zip

    // Controllo l'apertura dello zip
    if (!zip) {
        qInfo() << "zip is null";
        return false;
    }
    if (zip->getMode()!=QuaZip::mdCreate &&
        zip->getMode()!=QuaZip::mdAppend &&
        zip->getMode()!=QuaZip::mdAdd) {
        qInfo() << "zip mode is not create, append or add";
        return false;
    }

    // Controllo la cartella
    QDir directory(dir);
    if (!directory.exists()) {
        qInfo() << "directory not exists:" << dir;
        return false;
    }

    QDir origDirectory(origDir);
	if (dir != origDir) {
		QuaZipFile dirZipFile(zip);
		if (!dirZipFile.open(QIODevice::WriteOnly,
			QuaZipNewInfo(origDirectory.relativeFilePath(dir) + "/", dir), 0, 0, 0)) {
                qInfo() << "open quazip file failed!";
				return false;
		}
		dirZipFile.close();
	}


    // Se comprimo anche le sotto cartelle
    if (recursive) {
        // Per ogni sotto cartella
        QFileInfoList files = directory.entryInfoList(QDir::AllDirs|QDir::NoDotAndDotDot|filters);
        Q_FOREACH (QFileInfo file, files) {
            // Comprimo la sotto cartella
            if(!compressSubDir(zip,file.absoluteFilePath(),origDir,recursive,filters)) {
                qInfo() << "compress sub dir failed!";
                return false;
            }
        }
    }

    // Per ogni file nella cartella
    QFileInfoList files = directory.entryInfoList(QDir::Files|filters);
    Q_FOREACH (QFileInfo file, files) {
        // Se non e un file o e il file compresso che sto creando
        if(!file.isFile()||file.absoluteFilePath()==zip->getZipName()) {
            qInfo() << "file is not a file or is the zip file itself";
            continue;
        }

        // Creo il nome relativo da usare all'interno del file compresso
        QString filename = origDirectory.relativeFilePath(file.absoluteFilePath());

        // Comprimo il file
        if (!compressFile(zip,file.absoluteFilePath(),filename)) {
            qInfo() << "compress file failed!";
            return false;
        }
    }

    return true;
}

bool CompressUtil::extractFile(QuaZip* zip, QString fileName, QString fileDest) {
    qInfo() << "Extracting file:" << fileName << "to destination:" << fileDest;
    // zip: oggetto dove aggiungere il file
    // filename: nome del file reale
    // fileincompress: nome del file all'interno del file compresso

    // Controllo l'apertura dello zip
    if (!zip) {
        qInfo() << "zip is null";
        return false;
    }
    if (zip->getMode()!=QuaZip::mdUnzip) {
        qInfo() << "zip mode is not unzip";
        return false;
    }

    // Apro il file compresso
    if (!fileName.isEmpty()) {
        qInfo() << "set current file:" << fileName;
        zip->setCurrentFile(fileName);
    }
    QuaZipFile inFile(zip);
    if(!inFile.open(QIODevice::ReadOnly) || inFile.getZipError()!=UNZ_OK) {
        qInfo() << "open quazip file failed, error:" << inFile.getZipError();
        return false;
    }

    // Controllo esistenza cartella file risultato
    QDir curDir;
    if (fileDest.endsWith('/')) {
        if (!curDir.mkpath(fileDest)) {
            qInfo() << "mkpath failed:" << fileDest;
            return false;
        }
    } else {
        if (!curDir.mkpath(QFileInfo(fileDest).absolutePath())) {
            qInfo() << "mkpath failed:" << QFileInfo(fileDest).absolutePath();
            return false;
        }
    }

    QuaZipFileInfo64 info;
    if (!zip->getCurrentFileInfo(&info)) {
        qInfo() << "get current file info failed";
        return false;
    }

    QFile::Permissions srcPerm = info.getPermissions();
    if (fileDest.endsWith('/') && QFileInfo(fileDest).isDir()) {
        if (srcPerm != 0) {
            QFile(fileDest).setPermissions(srcPerm);
        }
        return true;
    }

    // Apro il file risultato
    QFile outFile;
    outFile.setFileName(fileDest);
    if(!outFile.open(QIODevice::WriteOnly)) {
        qInfo() << "open file failed:" << fileDest;
        return false;
    }

    // Copio i dati
    if (!copyData(inFile, outFile) || inFile.getZipError()!=UNZ_OK) {
        qInfo() << "copy data failed, error:" << inFile.getZipError();
        outFile.close();
        removeFile(QStringList(fileDest));
        return false;
    }
    outFile.close();

    // Chiudo i file
    inFile.close();
    if (inFile.getZipError()!=UNZ_OK) {
        qInfo() << "close quazip file failed, error:" << inFile.getZipError();
        removeFile(QStringList(fileDest));
        return false;
    }

    if (srcPerm != 0) {
        outFile.setPermissions(srcPerm);
    }
    return true;
}

bool CompressUtil::removeFile(QStringList listFile) {
    qInfo() << "Removing files:" << listFile.size();
    bool ret = true;
    // Per ogni file
    for (int i=0; i<listFile.count(); i++) {
        // Lo elimino
        ret = ret && QFile::remove(listFile.at(i));
    }
    return ret;
}

bool CompressUtil::compressFile(QString fileCompressed, QString file) {
    qInfo() << "Compressing single file:" << file << "into new archive:" << fileCompressed;
    // Creo lo zip
    QuaZip zip(fileCompressed);
    QDir().mkpath(QFileInfo(fileCompressed).absolutePath());
    if(!zip.open(QuaZip::mdCreate)) {
        qInfo() << "open quazip file failed:" << fileCompressed;
        QFile::remove(fileCompressed);
        return false;
    }

    // Aggiungo il file
    if (!compressFile(&zip,file,QFileInfo(file).fileName())) {
        qInfo() << "compress file failed:" << file;
        QFile::remove(fileCompressed);
        return false;
    }

    // Chiudo il file zip
    zip.close();
    if(zip.getZipError()!=0) {
        qInfo() << "close quazip file failed, error:" << zip.getZipError();
        QFile::remove(fileCompressed);
        return false;
    }

    return true;
}

bool CompressUtil::compressFiles(QString fileCompressed, QStringList files) {
    qInfo() << "Compressing multiple files into new archive:" << fileCompressed;
    // Creo lo zip
    QuaZip zip(fileCompressed);
    QDir().mkpath(QFileInfo(fileCompressed).absolutePath());
    if(!zip.open(QuaZip::mdCreate)) {
        qInfo() << "open quazip file failed:" << fileCompressed;
        QFile::remove(fileCompressed);
        return false;
    }

    // Comprimo i file
    QFileInfo info;
    Q_FOREACH (QString file, files) {
        info.setFile(file);
        if (!info.exists() || !compressFile(&zip,file,info.fileName())) {
            // qInfo() << "compress file failed:" << file;
            QFile::remove(fileCompressed);
            return false;
        }
    }

    // Chiudo il file zip
    zip.close();
    if(zip.getZipError()!=0) {
        qInfo() << "close quazip file failed, error:" << zip.getZipError();
        QFile::remove(fileCompressed);
        return false;
    }

    return true;
}

bool CompressUtil::compressDir(QString fileCompressed, QString dir, bool recursive) {
    qInfo() << "Compressing directory";
    return compressDir(fileCompressed, dir, recursive, QDir::NoFilter);
}

bool CompressUtil::compressDir(QString fileCompressed, QString dir,
                             bool recursive, QDir::Filters filters)
{
    qInfo() << "Compressing directory with filters:" << dir << "into new archive:" << fileCompressed << "recursive:" << recursive;
    // Creo lo zip
    QuaZip zip(fileCompressed);
    QDir().mkpath(QFileInfo(fileCompressed).absolutePath());
    if(!zip.open(QuaZip::mdCreate)) {
        qInfo() << "open quazip file failed:" << fileCompressed;
        QFile::remove(fileCompressed);
        return false;
    }

    // Aggiungo i file e le sotto cartelle
    if (!compressSubDir(&zip,dir,dir,recursive, filters)) {
        qInfo() << "compress sub dir failed:" << dir;
        QFile::remove(fileCompressed);
        return false;
    }

    // Chiudo il file zip
    zip.close();
    if(zip.getZipError()!=0) {
        qInfo() << "close quazip file failed, error:" << zip.getZipError();
        QFile::remove(fileCompressed);
        return false;
    }

    return true;
}

QString CompressUtil::extractFile(QString fileCompressed, QString fileName, QString fileDest) {
    qInfo() << "Extracting file:" << fileName << "from archive:" << fileCompressed << "to:" << fileDest;
    // Apro lo zip
    QuaZip zip(fileCompressed);
    return extractFile(zip, fileName, fileDest);
}

QString CompressUtil::extractFile(QuaZip &zip, QString fileName, QString fileDest)
{
    qInfo() << "Extracting file:" << fileName << "to:" << fileDest << "using existing QuaZip object";
    if(!zip.open(QuaZip::mdUnzip)) {
        qInfo() << "open quazip file failed";
        return QString();
    }

    // Estraggo il file
    if (fileDest.isEmpty())
        fileDest = fileName;
    if (!extractFile(&zip,fileName,fileDest)) {
        qInfo() << "extract file failed:" << fileName;
        return QString();
    }

    // Chiudo il file zip
    zip.close();
    if(zip.getZipError()!=0) {
        qInfo() << "close quazip file failed, error:" << zip.getZipError();
        removeFile(QStringList(fileDest));
        return QString();
    }
    return QFileInfo(fileDest).absoluteFilePath();
}

QStringList CompressUtil::extractFiles(QString fileCompressed, QStringList files, QString dir) {
    qInfo() << "Extracting multiple files from archive:" << fileCompressed << "to directory:" << dir;
    // Creo lo zip
    QuaZip zip(fileCompressed);
    return extractFiles(zip, files, dir);
}

QStringList CompressUtil::extractFiles(QuaZip &zip, const QStringList &files, const QString &dir)
{
    qInfo() << "Extracting multiple files to directory:" << dir << "using existing QuaZip object";
    if(!zip.open(QuaZip::mdUnzip)) {
        qInfo() << "open quazip file failed";
        return QStringList();
    }

    // Estraggo i file
    QStringList extracted;
    for (int i=0; i<files.count(); i++) {
        QString absPath = QDir(dir).absoluteFilePath(files.at(i));
        if (!extractFile(&zip, files.at(i), absPath)) {
            // qInfo() << "extract file failed:" << files.at(i);
            removeFile(extracted);
            return QStringList();
        }
        extracted.append(absPath);
    }

    // Chiudo il file zip
    zip.close();
    if(zip.getZipError()!=0) {
        qInfo() << "close quazip file failed, error:" << zip.getZipError();
        removeFile(extracted);
        return QStringList();
    }

    return extracted;
}

QStringList CompressUtil::extractDir(QString fileCompressed, QString dir) {
    qInfo() << "Extracting entire directory from archive:" << fileCompressed << "to:" << dir;
    // Apro lo zip
    QuaZip zip(fileCompressed);
    return extractDir(zip, dir);
}

QStringList CompressUtil::extractDir(QuaZip &zip, const QString &dir)
{
    qInfo() << "Extracting entire directory to:" << dir << "using existing QuaZip object";
    if(!zip.open(QuaZip::mdUnzip)) {
        qInfo() << "open quazip file failed";
        return QStringList();
    }

    QDir directory(dir);
    QStringList extracted;
    if (!zip.goToFirstFile()) {
        qInfo() << "go to first file failed";
        return QStringList();
    }
    do {
        QString name = zip.getCurrentFileName();
        QString absFilePath = directory.absoluteFilePath(name);
        if (!extractFile(&zip, "", absFilePath)) {
            qInfo() << "extract file failed:" << name;
            removeFile(extracted);
            return QStringList();
        }
        extracted.append(absFilePath);
    } while (zip.goToNextFile());

    // Chiudo il file zip
    zip.close();
    if(zip.getZipError()!=0) {
        qInfo() << "close quazip file failed, error:" << zip.getZipError();
        removeFile(extracted);
        return QStringList();
    }

    return extracted;
}

QStringList CompressUtil::getFileList(QString fileCompressed) {
    qInfo() << "Getting file list from archive:" << fileCompressed;
    // Apro lo zip
    QuaZip* zip = new QuaZip(QFileInfo(fileCompressed).absoluteFilePath());
    return getFileList(zip);
}

QStringList CompressUtil::getFileList(QuaZip *zip)
{
    qInfo() << "Getting file list using existing QuaZip object";
    if(!zip->open(QuaZip::mdUnzip)) {
        qInfo() << "open quazip file failed";
        delete zip;
        return QStringList();
    }

    // Estraggo i nomi dei file
    QStringList lst;
    QuaZipFileInfo64 info;
    for(bool more=zip->goToFirstFile(); more; more=zip->goToNextFile()) {
      if(!zip->getCurrentFileInfo(&info)) {
          qInfo() << "get current file info failed";
          delete zip;
          return QStringList();
      }
      lst << info.name;
      //info.name.toLocal8Bit().constData()
    }

    // Chiudo il file zip
    zip->close();
    if(zip->getZipError()!=0) {
        qInfo() << "close quazip file failed, error:" << zip->getZipError();
        delete zip;
        return QStringList();
    }
    delete zip;
    return lst;
}

QStringList CompressUtil::extractDir(QIODevice *ioDevice, QString dir)
{
    qInfo() << "Extracting directory from IODevice to:" << dir;
    QuaZip zip(ioDevice);
    return extractDir(zip, dir);
}

QStringList CompressUtil::getFileList(QIODevice *ioDevice)
{
    qInfo() << "Getting file list from IODevice";
    QuaZip *zip = new QuaZip(ioDevice);
    return getFileList(zip);
}

QString CompressUtil::extractFile(QIODevice *ioDevice, QString fileName, QString fileDest)
{
    qInfo() << "Extracting file from IODevice, source:" << fileName << "dest:" << fileDest;
    QuaZip zip(ioDevice);
    return extractFile(zip, fileName, fileDest);
}

QStringList CompressUtil::extractFiles(QIODevice *ioDevice, QStringList files, QString dir)
{
    qInfo() << "Extracting multiple files from IODevice to directory:" << dir;
    QuaZip zip(ioDevice);
    return extractFiles(zip, files, dir);
}