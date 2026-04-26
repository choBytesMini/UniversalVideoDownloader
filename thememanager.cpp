#include "thememanager.h"
#include "platformutils.h"

#include <QApplication>
#include <QLabel>
#include <QPushButton>
#include <QStyleHints>
#include <QTextEdit>

ThemeManager::ThemeManager(QTextEdit *logConsole,
                           QPushButton *stopBtn,
                           QLabel *statusLabel,
                           QObject *parent)
    : QObject(parent)
    , m_logConsole(logConsole)
    , m_stopBtn(stopBtn)
    , m_statusLabel(statusLabel)
    , m_fontFamily(resolveFontFamily())
{
    connect(QApplication::styleHints(), &QStyleHints::colorSchemeChanged,
            this, &ThemeManager::onColorSchemeChanged);

    applyTheme();
}

QString ThemeManager::resolveFontFamily() const
{
    if (PlatformUtils::isMacOS()) {
        return "'SF Mono', 'Menlo', 'Monaco', monospace";
    }
    if (PlatformUtils::isWindows()) {
        return "'Cascadia Code', 'Consolas', 'Lucida Console', monospace";
    }
    return "'Fira Code', 'Source Code Pro', 'DejaVu Sans Mono', 'Courier New', monospace";
}

void ThemeManager::applyTheme()
{
    bool dark = (QApplication::styleHints()->colorScheme() == Qt::ColorScheme::Dark);

    m_logConsole->setStyleSheet(buildLogConsoleStyle(dark));
    m_stopBtn->setStyleSheet(buildStopBtnStyle(dark));
    m_statusLabel->setStyleSheet(buildStatusLabelStyle(dark));
}

void ThemeManager::onColorSchemeChanged(Qt::ColorScheme /*newScheme*/)
{
    applyTheme();
}

QString ThemeManager::buildLogConsoleStyle(bool dark) const
{
    if (dark) {
        return QStringLiteral(
            "QTextEdit#logConsole {"
            "  background-color: #1e1e1e;"
            "  color: #d4d4d4;"
            "  font-family: %1;"
            "  font-size: 13px;"
            "  border: none;"
            "  padding: 8px;"
            "}"
        ).arg(m_fontFamily);
    }

    return QStringLiteral(
        "QTextEdit#logConsole {"
        "  background-color: #f8f8f5;"
        "  color: #2c2c2c;"
        "  font-family: %1;"
        "  font-size: 13px;"
        "  border: 1px solid #e0e0e0;"
        "  padding: 8px;"
        "}"
    ).arg(m_fontFamily);
}

QString ThemeManager::buildStopBtnStyle(bool dark) const
{
    if (dark) {
        return QStringLiteral(
            "QPushButton#stopBtn:enabled { color: #ff6b6b; }"
            "QPushButton#stopBtn:disabled { color: #665555; }"
        );
    }

    return QStringLiteral(
        "QPushButton#stopBtn:enabled { color: #d32f2f; }"
        "QPushButton#stopBtn:disabled { color: #cccccc; }"
    );
}

QString ThemeManager::buildStatusLabelStyle(bool dark) const
{
    if (dark) {
        return QStringLiteral("color: #aaaaaa; font-size: 13px;");
    }

    return QStringLiteral("color: #888888; font-size: 13px;");
}
