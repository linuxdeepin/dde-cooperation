// SPDX-FileCopyrightText: 2025 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "compressutil.h"
#include <QDebug>

static bool copyData(QIODevice &inFile, QIODevice &outFile)
{
    while (!inFile.atEnd()) {
        char buf[4096];
        qint64 readLen = inFile.read(buf, 4096);
        if (readLen <= 0)
            return false;
        if (outFile.write(buf, readLen) != readLen)
            return false;
    }
    return true;
}

bool CompressUtil::compressFile(QuaZip* zip, QString fileName, QString fileDest) {
    // zip: oggetto dove aggiungere il file
    // fileName: nome del file reale
    // fileDest: nome del file all'interno del file compresso

    // Controllo l'apertura dello zip
    if (!zip) return false;
    if (zip->getMode()!=QuaZip::mdCreate &&
        zip->getMode()!=QuaZip::mdAppend &&
        zip->getMode()!=QuaZip::mdAdd) return false;

    // Apro il file originale
    QFile inFile;
    inFile.setFileName(fileName);
    if(!inFile.open(QIODevice::ReadOnly)) return false;

    // Apro il file risulato
    QuaZipFile outFile(zip);
    if(!outFile.open(QIODevice::WriteOnly, QuaZipNewInfo(fileDest, inFile.fileName()))) return false;

    // Copio i dati
    if (!copyData(inFile, outFile) || outFile.getZipError()!=UNZ_OK) {
        return false;
    }

    // Chiudo i file
    outFile.close();
    if (outFile.getZipError()!=UNZ_OK) return false;
    inFile.close();

    return true;
}

bool CompressUtil::compressSubDir(QuaZip* zip, QString dir, QString origDir, bool recursive, QDir::Filters filters) {
    // zip: oggetto dove aggiungere il file
    // dir: cartella reale corrente
    // origDir: cartella reale originale
    // (path(dir)-path(origDir)) = path interno all'oggetto zip

    // Controllo l'apertura dello zip
    if (!zip) return false;
    if (zip->getMode()!=QuaZip::mdCreate &&
        zip->getMode()!=QuaZip::mdAppend &&
        zip->getMode()!=QuaZip::mdAdd) return false;

    // Controllo la cartella
    QDir directory(dir);
    if (!directory.exists()) return false;

    QDir origDirectory(origDir);
	if (dir != origDir) {
		QuaZipFile dirZipFile(zip);
		if (!dirZipFile.open(QIODevice::WriteOnly,
			QuaZipNewInfo(origDirectory.relativeFilePath(dir) + "/", dir), 0, 0, 0)) {
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
            if(!compressSubDir(zip,file.absoluteFilePath(),origDir,recursive,filters)) return false;
        }
    }

    // Per ogni file nella cartella
    QFileInfoList files = directory.entryInfoList(QDir::Files|filters);
    Q_FOREACH (QFileInfo file, files) {
        // Se non e un file o e il file compresso che sto creando
        if(!file.isFile()||file.absoluteFilePath()==zip->getZipName()) continue;

        // Creo il nome relativo da usare all'interno del file compresso
        QString filename = origDirectory.relativeFilePath(file.absoluteFilePath());

        // Comprimo il file
        if (!compressFile(zip,file.absoluteFilePath(),filename)) return false;
    }

    return true;
}

bool CompressUtil::extractFile(QuaZip* zip, QString fileName, QString fileDest) {
    // zip: oggetto dove aggiungere il file
    // filename: nome del file reale
    // fileincompress: nome del file all'interno del file compresso

    // Controllo l'apertura dello zip
    if (!zip) return false;
    if (zip->getMode()!=QuaZip::mdUnzip) return false;

    // Apro il file compresso
    if (!fileName.isEmpty())
        zip->setCurrentFile(fileName);
    QuaZipFile inFile(zip);
    if(!inFile.open(QIODevice::ReadOnly) || inFile.getZipError()!=UNZ_OK) return false;

    // Controllo esistenza cartella file risultato
    QDir curDir;
    if (fileDest.endsWith('/')) {
        if (!curDir.mkpath(fileDest)) {
            return false;
        }
    } else {
        if (!curDir.mkpath(QFileInfo(fileDest).absolutePath())) {
            return false;
        }
    }

    QuaZipFileInfo64 info;
    if (!zip->getCurrentFileInfo(&info))
        return false;

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
    if(!outFile.open(QIODevice::WriteOnly)) return false;

    // Copio i dati
    if (!copyData(inFile, outFile) || inFile.getZipError()!=UNZ_OK) {
        outFile.close();
        removeFile(QStringList(fileDest));
        return false;
    }
    outFile.close();

    // Chiudo i file
    inFile.close();
    if (inFile.getZipError()!=UNZ_OK) {
        removeFile(QStringList(fileDest));
        return false;
    }

    if (srcPerm != 0) {
        outFile.setPermissions(srcPerm);
    }
    return true;
}

bool CompressUtil::removeFile(QStringList listFile) {
    bool ret = true;
    // Per ogni file
    for (int i=0; i<listFile.count(); i++) {
        // Lo elimino
        ret = ret && QFile::remove(listFile.at(i));
    }
    return ret;
}

bool CompressUtil::compressFile(QString fileCompressed, QString file) {
    // Creo lo zip
    QuaZip zip(fileCompressed);
    QDir().mkpath(QFileInfo(fileCompressed).absolutePath());
    if(!zip.open(QuaZip::mdCreate)) {
        QFile::remove(fileCompressed);
        return false;
    }

    // Aggiungo il file
    if (!compressFile(&zip,file,QFileInfo(file).fileName())) {
        QFile::remove(fileCompressed);
        return false;
    }

    // Chiudo il file zip
    zip.close();
    if(zip.getZipError()!=0) {
        QFile::remove(fileCompressed);
        return false;
    }

    return true;
}

bool CompressUtil::compressFiles(QString fileCompressed, QStringList files) {
    // Creo lo zip
    QuaZip zip(fileCompressed);
    QDir().mkpath(QFileInfo(fileCompressed).absolutePath());
    if(!zip.open(QuaZip::mdCreate)) {
        QFile::remove(fileCompressed);
        return false;
    }

    // Comprimo i file
    QFileInfo info;
    Q_FOREACH (QString file, files) {
        info.setFile(file);
        if (!info.exists() || !compressFile(&zip,file,info.fileName())) {
            QFile::remove(fileCompressed);
            return false;
        }
    }

    // Chiudo il file zip
    zip.close();
    if(zip.getZipError()!=0) {
        QFile::remove(fileCompressed);
        return false;
    }

    return true;
}

bool CompressUtil::compressDir(QString fileCompressed, QString dir, bool recursive) {
    return compressDir(fileCompressed, dir, recursive, QDir::NoFilter);
}

bool CompressUtil::compressDir(QString fileCompressed, QString dir,
                             bool recursive, QDir::Filters filters)
{
    // Creo lo zip
    QuaZip zip(fileCompressed);
    QDir().mkpath(QFileInfo(fileCompressed).absolutePath());
    if(!zip.open(QuaZip::mdCreate)) {
        QFile::remove(fileCompressed);
        return false;
    }

    // Aggiungo i file e le sotto cartelle
    if (!compressSubDir(&zip,dir,dir,recursive, filters)) {
        QFile::remove(fileCompressed);
        return false;
    }

    // Chiudo il file zip
    zip.close();
    if(zip.getZipError()!=0) {
        QFile::remove(fileCompressed);
        return false;
    }

    return true;
}

QString CompressUtil::extractFile(QString fileCompressed, QString fileName, QString fileDest) {
    // Apro lo zip
    QuaZip zip(fileCompressed);
    return extractFile(zip, fileName, fileDest);
}

QString CompressUtil::extractFile(QuaZip &zip, QString fileName, QString fileDest)
{
    if(!zip.open(QuaZip::mdUnzip)) {
        return QString();
    }

    // Estraggo il file
    if (fileDest.isEmpty())
        fileDest = fileName;
    if (!extractFile(&zip,fileName,fileDest)) {
        return QString();
    }

    // Chiudo il file zip
    zip.close();
    if(zip.getZipError()!=0) {
        removeFile(QStringList(fileDest));
        return QString();
    }
    return QFileInfo(fileDest).absoluteFilePath();
}

QStringList CompressUtil::extractFiles(QString fileCompressed, QStringList files, QString dir) {
    // Creo lo zip
    QuaZip zip(fileCompressed);
    return extractFiles(zip, files, dir);
}

QStringList CompressUtil::extractFiles(QuaZip &zip, const QStringList &files, const QString &dir)
{
    if(!zip.open(QuaZip::mdUnzip)) {
        return QStringList();
    }

    // Estraggo i file
    QStringList extracted;
    for (int i=0; i<files.count(); i++) {
        QString absPath = QDir(dir).absoluteFilePath(files.at(i));
        if (!extractFile(&zip, files.at(i), absPath)) {
            removeFile(extracted);
            return QStringList();
        }
        extracted.append(absPath);
    }

    // Chiudo il file zip
    zip.close();
    if(zip.getZipError()!=0) {
        removeFile(extracted);
        return QStringList();
    }

    return extracted;
}

QStringList CompressUtil::extractDir(QString fileCompressed, QString dir) {
    // Apro lo zip
    QuaZip zip(fileCompressed);
    return extractDir(zip, dir);
}

QStringList CompressUtil::extractDir(QuaZip &zip, const QString &dir)
{
    if(!zip.open(QuaZip::mdUnzip)) {
        return QStringList();
    }

    QDir directory(dir);
    QStringList extracted;
    if (!zip.goToFirstFile()) {
        return QStringList();
    }
    do {
        QString name = zip.getCurrentFileName();
        QString absFilePath = directory.absoluteFilePath(name);
        if (!extractFile(&zip, "", absFilePath)) {
            removeFile(extracted);
            return QStringList();
        }
        extracted.append(absFilePath);
    } while (zip.goToNextFile());

    // Chiudo il file zip
    zip.close();
    if(zip.getZipError()!=0) {
        removeFile(extracted);
        return QStringList();
    }

    return extracted;
}

QStringList CompressUtil::getFileList(QString fileCompressed) {
    // Apro lo zip
    QuaZip* zip = new QuaZip(QFileInfo(fileCompressed).absoluteFilePath());
    return getFileList(zip);
}

QStringList CompressUtil::getFileList(QuaZip *zip)
{
    if(!zip->open(QuaZip::mdUnzip)) {
        delete zip;
        return QStringList();
    }

    // Estraggo i nomi dei file
    QStringList lst;
    QuaZipFileInfo64 info;
    for(bool more=zip->goToFirstFile(); more; more=zip->goToNextFile()) {
      if(!zip->getCurrentFileInfo(&info)) {
          delete zip;
          return QStringList();
      }
      lst << info.name;
      //info.name.toLocal8Bit().constData()
    }

    // Chiudo il file zip
    zip->close();
    if(zip->getZipError()!=0) {
        delete zip;
        return QStringList();
    }
    delete zip;
    return lst;
}

QStringList CompressUtil::extractDir(QIODevice *ioDevice, QString dir)
{
    QuaZip zip(ioDevice);
    return extractDir(zip, dir);
}

QStringList CompressUtil::getFileList(QIODevice *ioDevice)
{
    QuaZip *zip = new QuaZip(ioDevice);
    return getFileList(zip);
}

QString CompressUtil::extractFile(QIODevice *ioDevice, QString fileName, QString fileDest)
{
    QuaZip zip(ioDevice);
    return extractFile(zip, fileName, fileDest);
}

QStringList CompressUtil::extractFiles(QIODevice *ioDevice, QStringList files, QString dir)
{
    QuaZip zip(ioDevice);
    return extractFiles(zip, files, dir);
}
