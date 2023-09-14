#ifndef OPTIONSMANAGER_H
#define OPTIONSMANAGER_H

#include <QMap>
#include <QObject>

class OptionsManager : public QObject
{
    Q_OBJECT

public:
    OptionsManager();
    ~OptionsManager();

    static OptionsManager *instance();

    QMap<QString, QStringList> getUserOptions() const;
    QStringList getUserOption(const QString &option) const;

    void setUserOptions(const QMap<QString, QStringList> &value);
    void addUserOption(const QString &option, const QStringList &value);

private:
    QMap<QString, QStringList> userOptions;
};

namespace Options {
inline constexpr char kUser[] { "user" };
inline constexpr char kFile[] { "file" };
inline constexpr char kApp[] { "app" };
inline constexpr char kBrowserBookmarks[] { "browserbookmarks" };
inline constexpr char kConfig[] { "config" };
}

#endif
