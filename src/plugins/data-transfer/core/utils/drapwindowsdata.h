#ifndef DRAPWINDOWSDATA_H
#define DRAPWINDOWSDATA_H
#ifdef WIN32

#include <QSet>
#include <QString>
#include <windows.h>
#pragma execution_chatacter_set("utf_8")
class QJsonObject;

class drapWindowsData
{
public:
    drapWindowsData();
    ~drapWindowsData();
    QSet<QString> getApplianceList();
    QString getDesktopWallpaperPath();
    QVector<QPair<QString,QString>>  getBrowerBookmarkPaths();
    QSet<QPair<QString,QString>> getBrowerBookmarkList();
    QSet<QString> getBrowerList();
    void getBrowerBookmarkHtml(QString& htmlPath = QString());

private:
    void getBrowerBookmarkPathInfo();
    void getBrowerBookmarkInfo();
    void getApplianceListInfo();
    void getBrowerListInfo();
    void getDesktopWallpaperPathInfo();
    void applianceFromRegistry(const HKEY &RootKey,const LPCTSTR &lpSubKey);
    bool isControlPanelProgram(const HKEY &subKey);
    void readFirefoxBookmarks(const QString &dbPath);
    void readMicrosoftEdgeAndGoogleChromeBookmark(const QString &jsonPath);
    void browerBookmarkJsonNode(QJsonObject node);
    void insertBrowerBookmarkList(const QPair<QString,QString>& titleAndUrl);

    QSet<QString> applianceList;
    QSet<QString> browerList;
    QString desktopWallpaperPath;
    QVector<QPair<QString,QString>> browerBookmarkPath;
    QSet<QPair<QString,QString>> browerBookmarkList;
};
#endif

#endif // DRAPWINDOWSDATA_H
