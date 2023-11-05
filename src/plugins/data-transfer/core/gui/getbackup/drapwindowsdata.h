#ifndef DRAPWINDOWSDATA_H
#define DRAPWINDOWSDATA_H

#include <QSet>
#include <QString>
#include <Windows.h>
class QJsonObject;

namespace BrowserName {
inline constexpr char MicrosoftEdge[]{ "Microsoft Edge" };
inline constexpr char GoogleChrome[]{ "Google Chrome" };
inline constexpr char MozillaFirefox[]{ "Mozilla Firefox" };
} // namespace BrowserName

struct UosApp
{
    QString UosName;
    QString windowsName;
    QStringList feature;
};
class DrapWindowsData
{
public:
    ~DrapWindowsData();

    static DrapWindowsData *instance();

    QSet<QString> getApplianceList();
    QString getDesktopWallpaperPath();
    QVector<QPair<QString, QString>> getBrowserBookmarkPaths();
    QSet<QPair<QString, QString>> getBrowserBookmarkList();
    QSet<QString> getBrowserList();
    void getBrowserBookmarkHtml(QString &htmlPath);
    void getBrowserBookmarkInfo(const QSet<QString> &Browsername);
    QString getBrowserBookmarkJSON(QString &jsonPath);

    QString getUserName();
    QString getIP();

    void getLinuxApplist(QList<UosApp> &list);
    QMap<QString, QString> RecommendedInstallationAppList();

private:
    DrapWindowsData();

    bool containsAnyString(const QString &haystack, const QStringList &needles);
    void getBrowserBookmarkPathInfo();

    void getApplianceListInfo();
    void getBrowserListInfo();
    void getDesktopWallpaperPathRegistInfo();
    void getDesktopWallpaperPathAbsolutePathInfo();

    void applianceFromRegistry(const HKEY &RootKey, const LPCTSTR &lpSubKey);
    bool isControlPanelProgram(const HKEY &subKey);

    void readFirefoxBookmarks(const QString &dbPath);
    void readMicrosoftEdgeAndGoogleChromeBookmark(const QString &jsonPath);
    void browserBookmarkJsonNode(QJsonObject node);
    void insertBrowserBookmarkList(const QPair<QString, QString> &titleAndUrl);

    QSet<QString> applianceList;
    QSet<QString> browserList;
    QString desktopWallpaperPath;
    QVector<QPair<QString, QString>> browserBookmarkPath;
    QSet<QPair<QString, QString>> browserBookmarkList;
};

#endif // DRAPWINDOWSDATA_H
