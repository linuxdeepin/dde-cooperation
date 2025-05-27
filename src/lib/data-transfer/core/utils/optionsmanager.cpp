#include "optionsmanager.h"
#include "common/log.h"

OptionsManager::OptionsManager() : QObject() { }

OptionsManager::~OptionsManager() { }

OptionsManager *OptionsManager::instance()
{
    DLOG << "Accessing OptionsManager singleton instance";
    static OptionsManager ins;
    return &ins;
}

QMap<QString, QStringList> OptionsManager::getUserOptions() const
{
    return userOptions;
}

QStringList OptionsManager::getUserOption(const QString &option) const
{
    DLOG << "Getting user option:" << option.toStdString();
    if (userOptions.contains(option)) {
        DLOG << "Found option with" << userOptions[option].size() << "values";
        return userOptions[option];
    } else {
        WLOG << "Option not found:" << option.toStdString();
        return QStringList();
    }
}

void OptionsManager::setUserOptions(const QMap<QString, QStringList> &value)
{
    DLOG << "Setting all user options, count:" << value.size();
    userOptions = value;
}

void OptionsManager::addUserOption(const QString &option, const QStringList &value)
{
    DLOG << "Adding/updating option:" << option.toStdString() << "with" << value.size() << "values";
    if (userOptions.contains(option)) {
        DLOG << "Replacing existing option:" << option.toStdString();
        userOptions.remove(option);
    }
    userOptions[option] = value;
}

void OptionsManager::clear()
{
    DLOG << "Clearing all user options";
    userOptions.clear();
}
