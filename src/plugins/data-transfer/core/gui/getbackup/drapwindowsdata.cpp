#include "drapwindowsdata.h"
#include <tchar.h>
#include <QDebug>
#include <QFile>

#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QProcess>
#include <QDateTime>

#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QFileInfo>

#include <QStandardPaths>
#include <QHostInfo>
#include <QApplication>
#include <QDir>
#include <QPixmap>
#include <QNetworkInterface>

#define MAXNAME 256

namespace Registry {
inline constexpr char BrowerRegistryPath[]{ "SOFTWARE\\Clients\\StartMenuInternet" };
inline constexpr char ApplianceRegistryPath1[]{
    "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall"
};
inline constexpr char ApplianceRegistryPath2[]{
    "SOFTWARE\\Wow6432Node\\Microsoft\\Windows\\CurrentVersion\\Uninstall"
};
inline constexpr char DesktopwallpaperRegistryPath[]{ "Control Panel\\Desktop" };
} // namespace Registry

namespace BrowerPath {
inline constexpr char MicrosoftEdgeBookMark[]{
    "\\AppData\\Local\\Microsoft\\Edge\\User Data\\Default\\Bookmarks"
};
inline constexpr char GoogleChromeBookMark[]{
    "\\AppData\\Local\\Google\\Chrome\\User Data\\Default\\Bookmarks"
};
inline constexpr char MozillaFirefoxBookMark[]{ "\\AppData\\Roaming\\Mozilla\\Firefox" };
} // namespace BrowerPath

DrapWindowsData::DrapWindowsData() { }

DrapWindowsData *DrapWindowsData::instance()
{
    static DrapWindowsData ins;
    return &ins;
}

DrapWindowsData::~DrapWindowsData() { }
QSet<QString> DrapWindowsData::getBrowserList()
{
    if (browserList.isEmpty())
        getBrowserListInfo();
    return browserList;
}

void DrapWindowsData::getBrowserBookmarkHtml(QString &htmlPath)
{
    if (htmlPath.isEmpty()) {
        htmlPath = QString::fromLocal8Bit(".");
    }

    QStringList bookmarkItems;
    for (const QPair<QString, QString> &bookmark : browserBookmarkList) {
        QString bookmarkItem =
                QString("<a href=\"%1\">%2</a>").arg(bookmark.second).arg(bookmark.first);
        bookmarkItems.append(bookmarkItem);
    }

    QString htmlTemplate = QString::fromLocal8Bit(
            "<!DOCTYPE NETSCAPE-Bookmark-file-1>\n"
            "<!-- This is an automatically generated file. It will be read and overwritten."
            "DO NOT EDIT! -->\n"
            "<META HTTP-EQUIV=\"Content-Type\" CONTENT=\"text/html; charset=UTF-8\">\n"
            "<TITLE>Bookmarks</TITLE>\n"
            "<H1>Bookmarks</H1>\n"
            "<DL><p>\n"
            "<DT><H3 ADD_DATE=\"1688527902\" LAST_MODIFIED=\"1693460686\" "
            "PERSONAL_TOOLBAR_FOLDER=\"true\">书签栏</H3>\n"
            "<DL><p>\n"
            "<urlAndtile>\n"
            "</DL><p>\n"
            "</DL><p>\n");

    QString bookmarkList = bookmarkItems.join("\n");
    QString htmlContent = htmlTemplate.replace("<urlAndtile>", bookmarkList);
    QString htmlFile = htmlPath + "/bookmarks.html";

    QFile outputFile(htmlFile);
    if (outputFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream out(&outputFile);
        out.setCodec("UTF-8");
        out << htmlContent;
        outputFile.close();
    } else {
        qDebug() << "Failed to open file";
        return;
    }
}

QSet<QString> DrapWindowsData::getApplianceList()
{
    if (applianceList.isEmpty())
        getApplianceListInfo();
    return applianceList;
}

QString DrapWindowsData::getDesktopWallpaperPath()
{
    if (desktopWallpaperPath.isEmpty()) {
        getDesktopWallpaperPathRegistInfo();
    }
    if (desktopWallpaperPath.isEmpty()) {
        getDesktopWallpaperPathAbsolutePathInfo();
    }
    return desktopWallpaperPath;
}

void DrapWindowsData::readFirefoxBookmarks(const QString &dbPath)
{
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE");
    db.setDatabaseName(dbPath);

    if (!db.open()) {
        qDebug() << "Error opening firefox bookmark database:" << db.lastError();
        return;
    }

    QSqlQuery query;
    if (query.exec("SELECT moz_places.url, moz_bookmarks.title FROM moz_places "
                   "INNER JOIN moz_bookmarks ON moz_places.id = moz_bookmarks.fk")) {
        while (query.next()) {
            QString url = query.value(0).toString();
            QString title = query.value(1).toString();
            QPair<QString, QString> titleAndUrl(title, url);
            insertBrowserBookmarkList(titleAndUrl);
        }
    } else {
        qDebug() << "read firefox bookmark failed:" << query.lastError();
    }
    db.close();
}

void DrapWindowsData::readMicrosoftEdgeAndGoogleChromeBookmark(const QString &jsonPath)
{
    QFile file(jsonPath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qDebug() << "Failed to open file";
        return;
    }

    QByteArray jsonData = file.readAll();
    file.close();

    QJsonDocument doc = QJsonDocument::fromJson(jsonData);
    QJsonObject obj = doc.object();

    QJsonObject roots = obj["roots"].toObject();
    for (const QString &key : roots.keys()) {
        browserBookmarkJsonNode(roots[key].toObject());
    }
}

QVector<QPair<QString, QString>> DrapWindowsData::getBrowserBookmarkPaths()
{
    return browserBookmarkPath;
}

QSet<QPair<QString, QString>> DrapWindowsData::getBrowserBookmarkList()
{
    return browserBookmarkList;
}

void DrapWindowsData::getBrowserBookmarkPathInfo()
{
    if (browserList.isEmpty()) {
        getBrowserListInfo();
    }

    QString appData = std::getenv("USERPROFILE");

    if (browserList.find(BrowserName::MicrosoftEdge) != browserList.end()) {
        QString path = appData + BrowerPath::MicrosoftEdgeBookMark;
        auto bookMark = QPair<QString, QString>(BrowserName::MicrosoftEdge, path);
        browserBookmarkPath.push_back(bookMark);
    }

    if (browserList.find(BrowserName::GoogleChrome) != browserList.end()) {
        QString path = appData + BrowerPath::GoogleChromeBookMark;
        auto bookMark = QPair<QString, QString>(BrowserName::GoogleChrome, path);
        browserBookmarkPath.push_back(bookMark);
    }

    if (browserList.find(BrowserName::MozillaFirefox) != browserList.end()) {
        QString path = appData + BrowerPath::MozillaFirefoxBookMark;
        QString installIni = path + QString("\\installs.ini");
        QFile file(installIni);
        QString bookMarkPath;
        if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            QTextStream in(&file);
            while (!in.atEnd()) {
                QString line = in.readLine();
                if (line.contains("Default")) {
                    bookMarkPath = "\\" + line.split("=").at(1) + "\\places.sqlite";
                }
            }
            file.close();
        } else {
            qDebug() << "Can not open file:" << installIni;
        }

        if (!bookMarkPath.isEmpty()) {
            path = path + bookMarkPath;
            auto bookMark = QPair<QString, QString>(BrowserName::MozillaFirefox, path);
            browserBookmarkPath.push_back(bookMark);
        } else {
            qDebug() << "Can not find bookMark path in installs.ini";
        }
    }
}

void DrapWindowsData::getBrowserBookmarkInfo(const QSet<QString> &Browsername)
{
    if (browserBookmarkPath.isEmpty()) {
        getBrowserBookmarkPathInfo();
    }
    // clear browserBookmark
    browserBookmarkList.clear();

    if (!Browsername.isEmpty()) {
        for (auto &value : browserBookmarkPath) {
            if (value.first == BrowserName::MozillaFirefox) {
                if (Browsername.contains(BrowserName::MozillaFirefox))
                    readFirefoxBookmarks(value.second);
            } else if (value.first == BrowserName::MicrosoftEdge) {
                if (Browsername.contains(BrowserName::MicrosoftEdge))
                    readMicrosoftEdgeAndGoogleChromeBookmark(value.second);
            } else if (value.first == BrowserName::GoogleChrome) {
                if (Browsername.contains(BrowserName::GoogleChrome))
                    readMicrosoftEdgeAndGoogleChromeBookmark(value.second);
            }
        }
    }
}

QString DrapWindowsData::getBrowserBookmarkJSON(QString &jsonPath)
{
    if (jsonPath.isEmpty()) {
        jsonPath = QString::fromLocal8Bit(".");
    }

    QJsonArray childrenArray;
    int id = 0;
    for (auto bookmark : browserBookmarkList) {
        QJsonObject bookmarkJsonObject;
        bookmarkJsonObject["date_added"] = QString::number(QDateTime::currentMSecsSinceEpoch());
        bookmarkJsonObject["id"] = QString::number(id);
        bookmarkJsonObject["name"] = bookmark.first;
        bookmarkJsonObject["type"] = "url";
        bookmarkJsonObject["url"] = bookmark.second;
        childrenArray.append(bookmarkJsonObject);
        id++;
    }

    QJsonObject bookmarkBarObject;
    bookmarkBarObject["children"] = childrenArray;
    bookmarkBarObject["date_added"] = QString::number(QDateTime::currentMSecsSinceEpoch());
    bookmarkBarObject["date_modified"] = "0";
    bookmarkBarObject["id"] = "1";
    bookmarkBarObject["name"] = "Bookmarks Bar";
    bookmarkBarObject["type"] = "folder";

    QJsonObject otherBookmarksObject;
    otherBookmarksObject["children"] = QJsonArray();
    otherBookmarksObject["date_added"] = QString::number(QDateTime::currentMSecsSinceEpoch());
    otherBookmarksObject["date_modified"] = "0";
    otherBookmarksObject["id"] = "2";
    otherBookmarksObject["name"] = "Other Bookmarks";
    otherBookmarksObject["type"] = "folder";

    QJsonObject syncedBookmarksObject;
    syncedBookmarksObject["children"] = QJsonArray();
    syncedBookmarksObject["date_added"] = QString::number(QDateTime::currentMSecsSinceEpoch());
    syncedBookmarksObject["date_modified"] = "0";
    syncedBookmarksObject["id"] = "3";
    syncedBookmarksObject["name"] = "synced Bookmarks";
    syncedBookmarksObject["type"] = "folder";

    QJsonObject rootsObject;
    rootsObject["bookmark_bar"] = bookmarkBarObject;
    rootsObject["other"] = otherBookmarksObject;
    rootsObject["synced"] = syncedBookmarksObject;

    QJsonObject rootObject;
    rootObject["roots"] = rootsObject;
    rootObject["version"] = 1;

    QJsonDocument doc(rootObject);
    QString jsonfilePath = jsonPath + "/bookmarks.json";
    QFile file(jsonfilePath);
    if (file.open(QFile::WriteOnly | QFile::Text)) {
        QTextStream stream(&file);
        stream.setCodec("UTF-8");
        stream << doc.toJson();
        file.close();
        qDebug() << "JSON file saved successfully.";
        return jsonfilePath;
    } else {
        qWarning() << "Failed to save JSON file.";
        return QString();
    }
}

QString DrapWindowsData::getUserName()
{
    QString userDir = QStandardPaths::writableLocation(QStandardPaths::HomeLocation);

    QFileInfo fileInfo(userDir);
    QString userName = fileInfo.fileName();

    qDebug() << "User Name: " << userName;
    return userName;
}

QString DrapWindowsData::getIP()
{
    QString hostname = QHostInfo::localHostName();
    QHostInfo hostinfo = QHostInfo::fromName(hostname);
    QList<QHostAddress> addList = hostinfo.addresses();
    QList<QString> address;
    if (!addList.isEmpty())
        for (int i = 0; i < addList.size(); i++) {
            QHostAddress aHost = addList.at(i);
            if (QAbstractSocket::IPv4Protocol == aHost.protocol()) {
                address.append(aHost.toString());
            }
        }

    QString ipaddress = address.count() > 2 ? address[1] : "";
    return ipaddress;
}

void DrapWindowsData::getLinuxApplist(QList<UosApp> &list)
{
    QFile file(":/fileResource/apps.json");
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qWarning() << "can not open app json";
        return;
    }

    QByteArray jsonData = file.readAll();
    file.close();

    QJsonDocument jsonDoc = QJsonDocument::fromJson(jsonData);
    if (jsonDoc.isNull()) {
        qWarning() << "app json Parsing failed";
        return;
    }

    QJsonObject jsonObj = jsonDoc.object();

    QStringList keys = jsonObj.keys();

    for (const QString &key : keys) {
        UosApp app;
        QJsonObject appValue = jsonObj.value(key).toObject();

        QVariantList variantList = appValue.value("feature").toArray().toVariantList();
        QStringList featureList;
        for (const QVariant &variant : variantList) {
            if (variant.canConvert<QString>()) {
                featureList.append(variant.toString());
            }
        }
        app.feature = featureList;
        app.windowsName = key;
        app.UosName = appValue.value("packageName").toString();
        list.push_back(app);
    }

    return;
}

void DrapWindowsData::getApplianceListInfo()
{
    applianceFromRegistry(HKEY_LOCAL_MACHINE, _T(Registry::ApplianceRegistryPath1));
    applianceFromRegistry(HKEY_LOCAL_MACHINE, _T(Registry::ApplianceRegistryPath2));
    applianceFromRegistry(HKEY_CURRENT_USER, _T(Registry::ApplianceRegistryPath1));
}

void DrapWindowsData::getBrowserListInfo()
{
    HKEY hKey;
    LSTATUS queryStatus;
    LPCTSTR lpSubKey;
    lpSubKey = _T(Registry::BrowerRegistryPath);
    queryStatus = RegOpenKeyEx(HKEY_LOCAL_MACHINE, lpSubKey, 0, KEY_READ, &hKey);
    if (queryStatus == ERROR_SUCCESS) {
        DWORD index = 0;
        CHAR subKeyName[MAX_PATH];
        DWORD subKeyNameSize = sizeof(subKeyName);

        while (RegEnumKeyEx(hKey, index, subKeyName, &subKeyNameSize, NULL, NULL, NULL, NULL)
               != ERROR_NO_MORE_ITEMS) {
            QString strBuffer(subKeyName);
            QString strMidReg;
            strMidReg = (QString)lpSubKey + ("\\") + strBuffer;

            char browerNameBuffer[MAXNAME];
            DWORD bufferSize = sizeof(browerNameBuffer);
            DWORD valueType;
            HKEY hkRKey;

            QByteArray byteArray = strMidReg.toLatin1();
            LPCSTR strMidReglpcstr = byteArray.constData();

            LSTATUS status =
                    RegOpenKeyEx(HKEY_LOCAL_MACHINE, strMidReglpcstr, 0, KEY_READ, &hkRKey);
            if (status == ERROR_SUCCESS) {
                status = RegQueryValueEx(hkRKey, NULL, NULL, &valueType, (LPBYTE)browerNameBuffer,
                                         &bufferSize);
                if (status == ERROR_SUCCESS) {
                    QString name = QString::fromLocal8Bit(browerNameBuffer);

                    if ((!name.isEmpty()) && (browserList.find(name) == browserList.end())) {
                        browserList.insert(name);
                    }
                } else {
                    qDebug() << "Failed to read brower name on registry. error code:" << status;
                }
            } else {
                qDebug() << "Failed to open registry HKEY_LOCAL_MACHINE\\" << strMidReglpcstr
                         << " error code:" << status;
            }
            index++;
            subKeyNameSize = sizeof(subKeyName);
        }
        RegCloseKey(hKey);
    } else {
        qDebug() << "Failed to open registry HKEY_LOCAL_MACHINE\\" << lpSubKey
                 << " error code:" << queryStatus;
    }
}

void DrapWindowsData::getDesktopWallpaperPathRegistInfo()
{
    WCHAR wallpaperPath[MAX_PATH];
    if (SystemParametersInfo(SPI_GETDESKWALLPAPER, MAX_PATH, wallpaperPath, 0)) {
        QString wallpaperPathStr = QString::fromWCharArray(wallpaperPath);
        QFileInfo fileInfo(wallpaperPathStr);
        if (fileInfo.exists()) {
            qDebug() << "Current wallpaper path: " << wallpaperPathStr;
            desktopWallpaperPath = wallpaperPathStr;
        } else {
            qDebug() << "Wallpaper file does not exist.";
        }
    } else {
        qDebug() << "Failed to retrieve wallpaper path.";
    }
}

void DrapWindowsData::getDesktopWallpaperPathAbsolutePathInfo()
{
    QString appDataPath = QStandardPaths::writableLocation(QStandardPaths::HomeLocation);
    QString wallpaperFilePath =
            appDataPath + "/AppData/Roaming/Microsoft/Windows/Themes/TranscodedWallpaper";
    QPixmap wallpaperPixmap(wallpaperFilePath);
    if (!wallpaperPixmap.isNull()) {
        QImage wallpaperImage = wallpaperPixmap.toImage();
        QString wallpaperPathStr =
                QCoreApplication::applicationDirPath() + "/ConvertedWallpaper.png";
        if (wallpaperImage.save(wallpaperPathStr, "PNG")) {
            qDebug() << "TranscodedWallpaper converted and saved as PNG to: " << wallpaperPathStr;
            desktopWallpaperPath = wallpaperPathStr;
        } else {
            qDebug() << "Failed to save the converted wallpaper.";
        }
    } else {
        qDebug() << "Failed to load TranscodedWallpaper as QPixmap.";
    }
}

void DrapWindowsData::applianceFromRegistry(const HKEY &RootKey, const LPCTSTR &lpSubKey)
{
    HKEY hKey;
    LSTATUS status;
    status = RegOpenKeyEx(RootKey, lpSubKey, 0, KEY_READ, &hKey);
    if (status == ERROR_SUCCESS) {
        char subKeyName[MAXNAME];
        DWORD subKeyNameSize = sizeof(subKeyName);
        DWORD i = 0;
        while (RegEnumKeyEx(hKey, i, subKeyName, &subKeyNameSize, NULL, NULL, NULL, NULL)
               == ERROR_SUCCESS) {
            HKEY subKey;
            if (RegOpenKeyEx(hKey, subKeyName, 0, KEY_READ, &subKey) == ERROR_SUCCESS) {
                if (!isControlPanelProgram(subKey)) {
                    char displayName[MAXNAME];
                    DWORD displayNameSize = sizeof(displayName);

                    if (RegQueryValueEx(subKey, "DisplayName", NULL, NULL, (LPBYTE)displayName,
                                        &displayNameSize)
                        == ERROR_SUCCESS) {
                        QString name = QString::fromLocal8Bit(displayName);
                        if (applianceList.find(name) == applianceList.end()) {
                            applianceList.insert(name);
                        }
                    }
                }
                RegCloseKey(subKey);
            }
            i++;
            subKeyNameSize = sizeof(subKeyName);
        }
        RegCloseKey(hKey);
    } else {
        qDebug() << "Failed to open registry get applianceinfo:" << lpSubKey
                 << " Error code: " << status;
    }
}

bool DrapWindowsData::isControlPanelProgram(const HKEY &subKey)
{
    char systemComponent[MAXNAME];
    DWORD systemComponentSize = sizeof(systemComponent);

    if (RegQueryValueEx(subKey, "SystemComponent", NULL, NULL, (LPBYTE)systemComponent,
                        &systemComponentSize)
        == ERROR_SUCCESS) {
        if (systemComponentSize > 0 && systemComponent[0] != '\0') {
            return true;
        }
    }
    char parentKeyName[MAXNAME];
    DWORD parentKeyNameSize = sizeof(parentKeyName);

    if (RegQueryValueEx(subKey, "ParentKeyName", NULL, NULL, (LPBYTE)parentKeyName,
                        &parentKeyNameSize)
        == ERROR_SUCCESS) {
        if (parentKeyNameSize > 0 && parentKeyName[0] != '\0') {
            return true;
        }
    }
    return false;
}

void DrapWindowsData::browserBookmarkJsonNode(QJsonObject node)
{
    if (node.contains("name") && node.contains("url")) {
        QString url = node["url"].toString();
        QString title = node["name"].toString();
        QPair<QString, QString> titleAndUrl(title, url);
        insertBrowserBookmarkList(titleAndUrl);
    }

    if (node.contains("children")) {
        QJsonArray children = node["children"].toArray();
        for (const QJsonValue &child : children) {
            browserBookmarkJsonNode(child.toObject());
        }
    }
}

void DrapWindowsData::insertBrowserBookmarkList(const QPair<QString, QString> &titleAndUrl)
{
    auto find = std::find_if(browserBookmarkList.begin(), browserBookmarkList.end(),
                             [&titleAndUrl](const QPair<QString, QString> &mem) {
                                 if (mem.second == titleAndUrl.second) {
                                     return true;
                                 }
                                 return false;
                             });
    if (find == browserBookmarkList.end()) {
        browserBookmarkList.insert(titleAndUrl);
       // qDebug() << titleAndUrl.first << ": " << titleAndUrl.second;
    }
}

bool DrapWindowsData::containsAnyString(const QString &haystack, const QStringList &needles)
{
    for (const QString &needle : needles) {
        if (!haystack.contains(needle, Qt::CaseInsensitive)) {
            return false;
        }
    }
    return true;
}

QMap<QString, QString> DrapWindowsData::RecommendedInstallationAppList()
{
    QStringList dataStructure;
    QSet<QString> applist = getApplianceList();
    for (auto value : applist) {
        dataStructure.push_back(value);
    }

    QList<UosApp> MatchFielddata;
    getLinuxApplist(MatchFielddata);

    QMap<QString, QString> resultAPP;
    for (QString &valueA : dataStructure) {
        bool result;
        int i = 0;
        for (UosApp &uosValue : MatchFielddata) {
            QStringList valueB = uosValue.feature;
            result = containsAnyString(valueA, valueB);
            if (result) {
                resultAPP[uosValue.windowsName] = uosValue.UosName;
                break;
            }
            i++;
        }
    }

    return resultAPP;
}
