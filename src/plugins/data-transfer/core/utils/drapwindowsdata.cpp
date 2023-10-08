#ifdef WIN32

#    include "drapwindowsdata.h"
#    include <tchar.h>
#    include <QDebug>
#    include <QFile>

#    include <QSqlDatabase>
#    include <QSqlQuery>
#    include <QSqlError>
#    include <QProcess>
#    include <QDateTime>

#    include <QJsonDocument>
#    include <QJsonObject>
#    include <QJsonArray>

#    define MAXNAME 256

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

DrapWindowsData::DrapWindowsData()
{
    //    getApplianceListInfo();
    //    getBrowerListInfo();
    //    getDesktopWallpaperPathInfo();
    //    getBrowerBookmarkPathInfo();
    //    getBrowerBookmarkInfo();
}


DrapWindowsData *DrapWindowsData::instance()
{
    static DrapWindowsData ins;
    return &ins;
}

DrapWindowsData::~DrapWindowsData() { }
QSet<QString> DrapWindowsData::getBrowserList()
{
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
    return applianceList;
}

QString DrapWindowsData::getDesktopWallpaperPath()
{
    if(desktopWallpaperPath.isEmpty())
    {
        getDesktopWallpaperPathInfo();
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

    if (browserList.find(BrowerName::MicrosoftEdge) != browserList.end()) {
        QString path = appData + BrowerPath::MicrosoftEdgeBookMark;
        auto bookMark = QPair<QString, QString>(BrowerName::MicrosoftEdge, path);
        browserBookmarkPath.push_back(bookMark);
    }

    if (browserList.find(BrowerName::GoogleChrome) != browserList.end()) {
        QString path = appData + BrowerPath::GoogleChromeBookMark;
        auto bookMark = QPair<QString, QString>(BrowerName::GoogleChrome, path);
        browserBookmarkPath.push_back(bookMark);
    }

    if (browserList.find(BrowerName::MozillaFirefox) != browserList.end()) {
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
            auto bookMark = QPair<QString, QString>(BrowerName::MozillaFirefox, path);
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
            if (value.first == BrowerName::MozillaFirefox) {
                if (Browsername.contains(BrowerName::MozillaFirefox) )
                    readFirefoxBookmarks(value.second);
            } else if (value.first == BrowerName::MicrosoftEdge) {
                if (Browsername.contains(BrowerName::MicrosoftEdge))
                    readMicrosoftEdgeAndGoogleChromeBookmark(value.second);
            } else if (value.first == BrowerName::GoogleChrome) {
                if (Browsername.contains(BrowerName::GoogleChrome))
                    readMicrosoftEdgeAndGoogleChromeBookmark(value.second);
            }
        }
    }
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

void DrapWindowsData::getDesktopWallpaperPathInfo()
{
    HKEY hKey;
    LSTATUS status;
    status = RegOpenKeyEx(HKEY_CURRENT_USER, _T(Registry::DesktopwallpaperRegistryPath), 0,
                          KEY_READ, &hKey);
    if (status == ERROR_SUCCESS) {
        char wallpaperPath[MAX_PATH];
        DWORD wallpaperPathSize = sizeof(wallpaperPath);
        LSTATUS queryStatus;

        queryStatus = RegQueryValueEx(hKey, "Wallpaper", NULL, NULL, (LPBYTE)(wallpaperPath),
                                      &wallpaperPathSize);
        if (queryStatus == ERROR_SUCCESS) {
            desktopWallpaperPath = QString::fromLocal8Bit(wallpaperPath);
        } else {
            qDebug() << "Failed to read wallpaper path from registry. Error code: " << queryStatus;
        }
        RegCloseKey(hKey);
    } else {
        qDebug() << "Failed to open registry HKEY_CURRENT_USER\\"
                 << Registry::DesktopwallpaperRegistryPath << " Error code: " << status;
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
                             });
    if (find == browserBookmarkList.end()) {
        browserBookmarkList.insert(titleAndUrl);
        qDebug() << titleAndUrl.first << ": " << titleAndUrl.second;
    }
}

#endif

