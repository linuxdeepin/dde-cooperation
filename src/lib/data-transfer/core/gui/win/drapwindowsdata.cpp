#include "drapwindowsdata.h"
#include "common/log.h"
//#include "common/commonutils.h"

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
#include <QApplication>
#include <QDir>
#include <QPixmap>
#include <QSettings>
#include <ShlObj.h>
#include <QtWin>

#define MAXNAME 256

namespace Registry {
inline constexpr char BrowerRegistryPath[]{ "SOFTWARE\\Clients\\StartMenuInternet" };
inline constexpr char ApplianceRegistryPath1[]{
    "HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall"
};
inline constexpr char ApplianceRegistryPath2[]{
    "HKEY_CURRENT_USER\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall"
};
inline constexpr char ApplianceRegistryPath3[]{
    "HKEY_LOCAL_MACHINE\\SOFTWARE\\Wow6432Node\\Microsoft\\Windows\\CurrentVersion\\Uninstall"
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
    DLOG << "Initializing Windows data collector";
}

DrapWindowsData *DrapWindowsData::instance()
{
    static DrapWindowsData ins;
    return &ins;
}

DrapWindowsData::~DrapWindowsData()
{
    DLOG << "Destroying Windows data collector";
}
QStringList DrapWindowsData::getBrowserList()
{
    DLOG << "Getting browser list";
    if (browserList.isEmpty()) {
        DLOG << "Browser list is empty, getting browser list info";
        getBrowserListInfo();
    }
    return browserList;
}

void DrapWindowsData::getBrowserBookmarkHtml(QString &htmlPath)
{
    DLOG << "Getting browser bookmark HTML";
    if (htmlPath.isEmpty()) {
        DLOG << "HTML path is empty, setting to current directory";
        htmlPath = QString::fromLocal8Bit(".");
    }

    QStringList bookmarkItems;
    for (const QPair<QString, QString> &bookmark : browserBookmarkList) {
        QString bookmarkItem =
                QString("<a href=\"%1\">%2</a>").arg(bookmark.second).arg(bookmark.first);
        bookmarkItems.append(bookmarkItem);
        DLOG << "Added bookmark item:" << bookmark.first.toStdString();
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
        DLOG << "HTML file saved successfully to:" << htmlFile.toStdString();
    } else {
        DLOG << "Failed to open file for writing:" << htmlFile.toStdString();
        return;
    }
    DLOG << "Browser bookmark HTML collection complete";
}

QList<WinApp> DrapWindowsData::getApplianceList()
{
    DLOG << "Getting appliance list";
    if (applianceList.isEmpty()) {
        DLOG << "Appliance list is empty, getting appliance list info";
        getApplianceListInfo();
    }

//    for (auto value : applianceList)
//        LOG << "app name:" << value.name.toStdString();
//    LOG << "applianceList.size:" << applianceList.size();
    return applianceList;
}

QString DrapWindowsData::getDesktopWallpaperPath()
{
    DLOG << "Getting desktop wallpaper path";
    if (desktopWallpaperPath.isEmpty()) {
        DLOG << "Desktop wallpaper path is empty, trying registry info";
        getDesktopWallpaperPathRegistInfo();
    }
    if (desktopWallpaperPath.isEmpty()) {
        DLOG << "Desktop wallpaper path is still empty, trying absolute path info";
        getDesktopWallpaperPathAbsolutePathInfo();
    }
    return desktopWallpaperPath;
}

void DrapWindowsData::readFirefoxBookmarks(const QString &dbPath)
{
    DLOG << "Reading Firefox bookmarks from:" << dbPath.toStdString();
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE");
    db.setDatabaseName(dbPath);

    if (!db.open()) {
        DLOG << "Error opening firefox bookmark database";
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
            DLOG << "Read Firefox bookmark - Title:" << title.toStdString() << "URL:" << url.toStdString();
        }
    } else {
        DLOG << "read firefox bookmark failed";
    }
    db.close();
}

void DrapWindowsData::readMicrosoftEdgeAndGoogleChromeBookmark(const QString &jsonPath)
{
    DLOG << "Reading Edge/Chrome bookmarks from:" << jsonPath.toStdString();
    QFile file(jsonPath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        ELOG << "Failed to open file :" << jsonPath.toStdString();
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
    DLOG << "Reading Edge/Chrome bookmarks complete";
}

QList<QPair<QString, QString>> DrapWindowsData::getBrowserBookmarkPaths()
{
    DLOG << "Getting browser bookmark paths";
    return browserBookmarkPath;
}

QList<QPair<QString, QString>> DrapWindowsData::getBrowserBookmarkList()
{
    DLOG << "Getting browser bookmark list";
    return browserBookmarkList;
}

void DrapWindowsData::getBrowserBookmarkPathInfo()
{
    DLOG << "Getting browser bookmark path info";
    if (browserList.isEmpty()) {
        DLOG << "Browser list is empty, getting browser list info";
        getBrowserListInfo();
    }

    QString appData = std::getenv("USERPROFILE");

    if (browserList.contains(BrowserName::MicrosoftEdge)) {
        DLOG << "Browser list contains MicrosoftEdge";
        QString path = appData + BrowerPath::MicrosoftEdgeBookMark;
        auto bookMark = QPair<QString, QString>(BrowserName::MicrosoftEdge, path);
        browserBookmarkPath.push_back(bookMark);
    }

    if (browserList.contains(BrowserName::GoogleChrome)) {
        DLOG << "Browser list contains GoogleChrome";
        QString path = appData + BrowerPath::GoogleChromeBookMark;
        auto bookMark = QPair<QString, QString>(BrowserName::GoogleChrome, path);
        browserBookmarkPath.push_back(bookMark);
    }

    if (browserList.contains(BrowserName::MozillaFirefox)) {
        DLOG << "Browser list contains MozillaFirefox";
        QString path = appData + BrowerPath::MozillaFirefoxBookMark;
        QString installIni = path + QString("\\installs.ini");
        QFile file(installIni);
        QString bookMarkPath;
        if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            DLOG << "Opened installs.ini for MozillaFirefox";
            QTextStream in(&file);
            while (!in.atEnd()) {
                QString line = in.readLine();
                if (line.contains("Default")) {
                    DLOG << "Found Default profile in installs.ini";
                    bookMarkPath = "\\" + line.split("=").at(1) + "\\places.sqlite";
                }
            }
            file.close();
        } else {
            DLOG << "Can not open file:" << installIni.toStdString();
        }

        if (!bookMarkPath.isEmpty()) {
            DLOG << "Bookmark path is not empty, adding to browserBookmarkPath";
            path = path + bookMarkPath;
            auto bookMark = QPair<QString, QString>(BrowserName::MozillaFirefox, path);
            browserBookmarkPath.push_back(bookMark);
        } else {
            DLOG << "Can not find bookMark path in installs.ini";
        }
    }
    DLOG << "Reading Firefox bookmarks complete";
}

void DrapWindowsData::getBrowserBookmarkInfo(const QSet<QString> &Browsername)
{
    DLOG << "Getting browser bookmark info";
    if (browserBookmarkPath.isEmpty()) {
        DLOG << "Browser bookmark path is empty, getting bookmark path info";
        getBrowserBookmarkPathInfo();
    }
    // clear browserBookmark
    browserBookmarkList.clear();

    if (!Browsername.isEmpty()) {
        DLOG << "Browser name list is not empty";
        for (auto &value : browserBookmarkPath) {
            if (value.first == BrowserName::MozillaFirefox) {
                if (Browsername.contains(BrowserName::MozillaFirefox)) {
                    DLOG << "Browser name contains MozillaFirefox, reading bookmarks";
                    readFirefoxBookmarks(value.second);
                } else {
                    DLOG << "Browser name does not contain MozillaFirefox, skipping";
                }
            } else if (value.first == BrowserName::MicrosoftEdge) {
                if (Browsername.contains(BrowserName::MicrosoftEdge)) {
                    DLOG << "Browser name contains MicrosoftEdge, reading bookmarks";
                    readMicrosoftEdgeAndGoogleChromeBookmark(value.second);
                } else {
                    DLOG << "Browser name does not contain MicrosoftEdge, skipping";
                }
            } else if (value.first == BrowserName::GoogleChrome) {
                if (Browsername.contains(BrowserName::GoogleChrome)) {
                    DLOG << "Browser name contains GoogleChrome, reading bookmarks";
                    readMicrosoftEdgeAndGoogleChromeBookmark(value.second);
                } else {
                    DLOG << "Browser name does not contain GoogleChrome, skipping";
                }
            } else {
                DLOG << "Unknown browser name:" << value.first.toStdString();
            }
        }
    } else {
        DLOG << "Browser name list is empty, skipping bookmark info retrieval";
    }
}

QString DrapWindowsData::getBrowserBookmarkJSON(QString &jsonPath)
{
    DLOG << "Getting browser bookmark JSON";
    if (jsonPath.isEmpty()) {
        DLOG << "JSON path is empty, setting to current directory";
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
//        DLOG << "JSON file saved successfully.";
        return jsonfilePath;
    } else {
        WLOG << "Failed to save JSON file.";
        return QString();
    }
    DLOG << "JSON file saved successfully.";
}

QString DrapWindowsData::getUserName()
{
    DLOG << "Getting user name";
    QString userDir = QStandardPaths::writableLocation(QStandardPaths::HomeLocation);

    QFileInfo fileInfo(userDir);
    QString userName = fileInfo.fileName();

//    DLOG << "User Name: " << userName.toStdString();
    return userName;
}

void DrapWindowsData::getLinuxApplist(QList<UosApp> &list)
{
    DLOG << "Getting Linux application list";
    QFile file(":/fileResource/apps.json");
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
//        WLOG << "can not open app json";
        DLOG << "Failed to open app json file";
        return;
    }

    QByteArray jsonData = file.readAll();
    file.close();

    QJsonDocument jsonDoc = QJsonDocument::fromJson(jsonData);
    if (jsonDoc.isNull()) {
//        WLOG << "app json Parsing failed";
        DLOG << "App json parsing failed";
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

    DLOG << "Linux application list loaded successfully";
}

//QString DrapWindowsData::getIP()
//{
//    QString ip = deepin_cross::CommonUitls::getFirstIp().data();
//    return ip;
//}

void DrapWindowsData::getApplianceListInfo()
{
    DLOG << "Getting installed application list from registry";

    QSettings settings1(Registry::ApplianceRegistryPath1, QSettings::NativeFormat);
    applianceFromSetting(settings1);

    QSettings settings2(Registry::ApplianceRegistryPath2, QSettings::NativeFormat);
    applianceFromSetting(settings2);

    QSettings settings3(Registry::ApplianceRegistryPath3, QSettings::NativeFormat);
    applianceFromSetting(settings3);
}

void DrapWindowsData::getBrowserListInfo()
{
    DLOG << "Getting browser list from registry";

    HKEY hKey;
    LSTATUS queryStatus;
    LPCTSTR lpSubKey;
    lpSubKey = _T(Registry::BrowerRegistryPath);
    queryStatus = RegOpenKeyEx(HKEY_LOCAL_MACHINE, lpSubKey, 0, KEY_READ, &hKey);
    if (queryStatus == ERROR_SUCCESS) {
        DLOG << "Browser registry key opened successfully";
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

                    if ((!name.isEmpty()) && (!browserList.contains(name))) {
                        browserList.push_back(name);
                    }
                } else {
                   DLOG << "Failed to read brower name on registry.";
                }
            } else {
                DLOG << "Failed to open registry HKEY_LOCAL_MACHINE";
            }
            index++;
            subKeyNameSize = sizeof(subKeyName);
        }
        RegCloseKey(hKey);
    } else {
        DLOG << "Failed to open registry HKEY_LOCAL_MACHINE";
    }
}

void DrapWindowsData::getDesktopWallpaperPathRegistInfo()
{
    DLOG << "Getting desktop wallpaper path from registry";

    WCHAR wallpaperPath[MAX_PATH];
    if (SystemParametersInfo(SPI_GETDESKWALLPAPER, MAX_PATH, wallpaperPath, 0)) {
        DLOG << "Wallpaper path retrieved successfully";
        QString wallpaperPathStr = QString::fromWCharArray(wallpaperPath);
        QFileInfo fileInfo(wallpaperPathStr);
        if (fileInfo.exists()) {
           DLOG << "Current wallpaper path: " << wallpaperPathStr.toStdString();
            desktopWallpaperPath = wallpaperPathStr;
        } else {
           DLOG << "Wallpaper file does not exist.";
        }
    } else {
       DLOG << "Failed to retrieve wallpaper path.";
    }
}

void DrapWindowsData::getDesktopWallpaperPathAbsolutePathInfo()
{
    DLOG << "Trying to get wallpaper from absolute path";

    QString appDataPath = QStandardPaths::writableLocation(QStandardPaths::HomeLocation);
    QString wallpaperFilePath =
            appDataPath + "/AppData/Roaming/Microsoft/Windows/Themes/TranscodedWallpaper";
    QPixmap wallpaperPixmap(wallpaperFilePath);
    if (!wallpaperPixmap.isNull()) {
        QImage wallpaperImage = wallpaperPixmap.toImage();
        QString wallpaperPathStr = QDir::tempPath() + "/ConvertedWallpaper.png";
        if (wallpaperImage.save(wallpaperPathStr, "PNG")) {
            DLOG << "TranscodedWallpaper converted and saved as PNG to: " << wallpaperPathStr.toStdString();
            desktopWallpaperPath = wallpaperPathStr;
        } else {
            DLOG << "Failed to save the converted wallpaper.";
        }
    } else {
        DLOG << "Failed to load TranscodedWallpaper as QPixmap.";
    }
}

void DrapWindowsData::applianceFromSetting(QSettings &settings, QString registryPath)
{
    DLOG << "Loading application information from registry path: " << registryPath.toStdString();
    settings.beginGroup(registryPath);

    QStringList appKeys = settings.childGroups();
    for (const QString &appKey : appKeys) {
        settings.beginGroup(appKey);
        QString displayName = settings.value("DisplayName").toString();
        QString installLocation = settings.value("InstallLocation").toString();
        QString displayIcon = settings.value("DisplayIcon").toString();
        bool isSystemComponent = settings.value("SystemComponent").toBool();
        if (!isSystemComponent && !displayName.isEmpty()) {
            WinApp app;
            app.name = displayName;
            app.iconPath = displayIcon;
            for (auto iteraotr = applianceList.begin(); iteraotr != applianceList.end();
                 ++iteraotr) {
                if (iteraotr->name == displayName)
                    break;
            }
            applianceList.push_back(app);
        }
        settings.endGroup();
    }
    settings.endGroup();
    DLOG << "Application information loading completed.";
}

void DrapWindowsData::browserBookmarkJsonNode(QJsonObject node)
{
    DLOG << "Parsing browser bookmark JSON node";
    if (node.contains("name") && node.contains("url")) {
        DLOG << "Browser bookmark JSON node contains name and url";
        QString url = node["url"].toString();
        QString title = node["name"].toString();
        QPair<QString, QString> titleAndUrl(title, url);
        insertBrowserBookmarkList(titleAndUrl);
    }

    if (node.contains("children")) {
        DLOG << "Browser bookmark JSON node contains children array";
        QJsonArray children = node["children"].toArray();
        for (const QJsonValue &child : children) {
            browserBookmarkJsonNode(child.toObject());
        }
    }
}

void DrapWindowsData::insertBrowserBookmarkList(const QPair<QString, QString> &titleAndUrl)
{
    DLOG << "Inserting browser bookmark into list";
    auto find = std::find_if(browserBookmarkList.begin(), browserBookmarkList.end(),
                             [&titleAndUrl](const QPair<QString, QString> &mem) {
                                 if (mem.second == titleAndUrl.second) {
                                     return true;
                                 }
                                 return false;
                             });
    if (find == browserBookmarkList.end()) {
        DLOG << "Browser bookmark not found in list";
        browserBookmarkList.push_back(titleAndUrl);
        // DLOG << titleAndUrl.first << ": " << titleAndUrl.second;
    }
}

QPixmap DrapWindowsData::getAppIcon(const QString &path)
{
    DLOG << "Getting icon for path: " << path.toStdString();
    if (path.isEmpty()) {
        DLOG << "Path is empty";
        return QPixmap();
    }
    HICON hIcon;
    QString tempStr = path;
    if (ExtractIconExW(tempStr.toStdWString().c_str(), 0, NULL, &hIcon, 1) <= 0) {
        DLOG << "Failed to extract icon from path";
        return QPixmap();
    }
   if (hIcon == 0) {
        DLOG << "Icon handle is null";
        DestroyIcon(hIcon);
        return QPixmap();
    }

    QPixmap pixmap = QtWin::fromHICON(hIcon);
    DestroyIcon(hIcon);
    if (pixmap.isNull()) {
        DLOG << "Pixmap is null";
        return pixmap;
    }
    return pixmap.scaled(20, 20);
}

bool DrapWindowsData::containsAnyString(const QString &haystack, const QStringList &needles)
{
    DLOG << "Checking if haystack contains any string in needles";
    for (const QString &needle : needles) {
        if (!haystack.contains(needle, Qt::CaseInsensitive)) {
            DLOG << "return false: Haystack does not contain needle: " << needle.toStdString();
            return false;
        }
    }
    DLOG << "return true: Haystack contains at least one needle";
    return true;
}

QMap<QString, QString>
DrapWindowsData::RecommendedInstallationAppList(QMap<QString, QString> &notRecommendedList)
{
    DLOG << "Getting recommended installation app list";
    notRecommendedList.clear();

    QList<WinApp> dataStructure;
    QList<WinApp> applist = getApplianceList();
    for (auto value : applist) {
        dataStructure.push_back(value);
    }

    QList<UosApp> MatchFielddata;
    getLinuxApplist(MatchFielddata);

    QMap<QString, QString> resultAPP;
    for (auto iterator = dataStructure.begin(); iterator != dataStructure.end();) {
        bool result;
        QString winApp = (*iterator).name;
        for (UosApp &uosValue : MatchFielddata) {
            QStringList valueB = uosValue.feature;
            result = containsAnyString(winApp, valueB);
            if (result) {
                resultAPP[uosValue.windowsName] = uosValue.UosName;
                iterator = dataStructure.erase(iterator);
                break;
            }
        }
        if (!result) {
            ++iterator;
        }
    }
    for (auto &value : dataStructure) {
        notRecommendedList[value.name] = value.iconPath;
    }
    DLOG << "Recommended installation app list size: " << resultAPP.size();
    return resultAPP;
}
