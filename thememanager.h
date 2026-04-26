#ifndef THEMEMANAGER_H
#define THEMEMANAGER_H

#include <QObject>
#include <QString>

class QTextEdit;
class QPushButton;
class QLabel;

class ThemeManager : public QObject {
    Q_OBJECT

public:
    explicit ThemeManager(QTextEdit *logConsole,
                          QPushButton *stopBtn,
                          QLabel *statusLabel,
                          QObject *parent = nullptr);
    ~ThemeManager() override = default;

    void applyTheme();

private slots:
    void onColorSchemeChanged(Qt::ColorScheme newScheme);

private:
    QString resolveFontFamily() const;
    QString buildLogConsoleStyle(bool dark) const;
    QString buildStopBtnStyle(bool dark) const;
    QString buildStatusLabelStyle(bool dark) const;

    QTextEdit *m_logConsole;
    QPushButton *m_stopBtn;
    QLabel *m_statusLabel;
    QString m_fontFamily;
};

#endif // THEMEMANAGER_H
