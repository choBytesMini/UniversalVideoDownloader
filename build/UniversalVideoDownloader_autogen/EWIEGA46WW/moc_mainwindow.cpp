/****************************************************************************
** Meta object code from reading C++ file 'mainwindow.h'
**
** Created by: The Qt Meta Object Compiler version 69 (Qt 6.11.0)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../../mainwindow.h"
#include <QtGui/qtextcursor.h>
#include <QtCore/qmetatype.h>

#include <QtCore/qtmochelpers.h>

#include <memory>


#include <QtCore/qxptype_traits.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'mainwindow.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 69
#error "This file was generated using the moc from 6.11.0. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

#ifndef Q_CONSTINIT
#define Q_CONSTINIT
#endif

QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
QT_WARNING_DISABLE_GCC("-Wuseless-cast")
namespace {
struct qt_meta_tag_ZN10MainWindowE_t {};
} // unnamed namespace

template <> constexpr inline auto MainWindow::qt_create_metaobjectdata<qt_meta_tag_ZN10MainWindowE_t>()
{
    namespace QMC = QtMocConstants;
    QtMocHelpers::StringRefStorage qt_stringData {
        "MainWindow",
        "onAnalyzeClicked",
        "",
        "onDownloadClicked",
        "onStopClicked",
        "handleAnalyzeFinished",
        "exitCode",
        "QProcess::ExitStatus",
        "exitStatus",
        "handleDownloadOutput",
        "handleDownloadFinished",
        "handleProcessError",
        "QProcess::ProcessError",
        "error",
        "handleAria2cOutput",
        "handleAria2cError",
        "handleAria2cFinished",
        "onUrlsExtracted",
        "extractedUrls",
        "onUrlExtractFailed",
        "pageUrl",
        "onUrlExtractError",
        "errorMsg",
        "onVideoSelectionChanged",
        "index"
    };

    QtMocHelpers::UintData qt_methods {
        // Slot 'onAnalyzeClicked'
        QtMocHelpers::SlotData<void()>(1, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'onDownloadClicked'
        QtMocHelpers::SlotData<void()>(3, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'onStopClicked'
        QtMocHelpers::SlotData<void()>(4, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'handleAnalyzeFinished'
        QtMocHelpers::SlotData<void(int, QProcess::ExitStatus)>(5, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { QMetaType::Int, 6 }, { 0x80000000 | 7, 8 },
        }}),
        // Slot 'handleDownloadOutput'
        QtMocHelpers::SlotData<void()>(9, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'handleDownloadFinished'
        QtMocHelpers::SlotData<void(int, QProcess::ExitStatus)>(10, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { QMetaType::Int, 6 }, { 0x80000000 | 7, 8 },
        }}),
        // Slot 'handleProcessError'
        QtMocHelpers::SlotData<void(QProcess::ProcessError)>(11, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { 0x80000000 | 12, 13 },
        }}),
        // Slot 'handleAria2cOutput'
        QtMocHelpers::SlotData<void()>(14, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'handleAria2cError'
        QtMocHelpers::SlotData<void()>(15, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'handleAria2cFinished'
        QtMocHelpers::SlotData<void(int, QProcess::ExitStatus)>(16, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { QMetaType::Int, 6 }, { 0x80000000 | 7, 8 },
        }}),
        // Slot 'onUrlsExtracted'
        QtMocHelpers::SlotData<void(const QStringList &)>(17, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { QMetaType::QStringList, 18 },
        }}),
        // Slot 'onUrlExtractFailed'
        QtMocHelpers::SlotData<void(const QString &)>(19, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { QMetaType::QString, 20 },
        }}),
        // Slot 'onUrlExtractError'
        QtMocHelpers::SlotData<void(const QString &)>(21, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { QMetaType::QString, 22 },
        }}),
        // Slot 'onVideoSelectionChanged'
        QtMocHelpers::SlotData<void(int)>(23, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { QMetaType::Int, 24 },
        }}),
    };
    QtMocHelpers::UintData qt_properties {
    };
    QtMocHelpers::UintData qt_enums {
    };
    return QtMocHelpers::metaObjectData<MainWindow, qt_meta_tag_ZN10MainWindowE_t>(QMC::MetaObjectFlag{}, qt_stringData,
            qt_methods, qt_properties, qt_enums);
}
Q_CONSTINIT const QMetaObject MainWindow::staticMetaObject = { {
    QMetaObject::SuperData::link<QMainWindow::staticMetaObject>(),
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN10MainWindowE_t>.stringdata,
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN10MainWindowE_t>.data,
    qt_static_metacall,
    nullptr,
    qt_staticMetaObjectRelocatingContent<qt_meta_tag_ZN10MainWindowE_t>.metaTypes,
    nullptr
} };

void MainWindow::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    auto *_t = static_cast<MainWindow *>(_o);
    if (_c == QMetaObject::InvokeMetaMethod) {
        switch (_id) {
        case 0: _t->onAnalyzeClicked(); break;
        case 1: _t->onDownloadClicked(); break;
        case 2: _t->onStopClicked(); break;
        case 3: _t->handleAnalyzeFinished((*reinterpret_cast<std::add_pointer_t<int>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<QProcess::ExitStatus>>(_a[2]))); break;
        case 4: _t->handleDownloadOutput(); break;
        case 5: _t->handleDownloadFinished((*reinterpret_cast<std::add_pointer_t<int>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<QProcess::ExitStatus>>(_a[2]))); break;
        case 6: _t->handleProcessError((*reinterpret_cast<std::add_pointer_t<QProcess::ProcessError>>(_a[1]))); break;
        case 7: _t->handleAria2cOutput(); break;
        case 8: _t->handleAria2cError(); break;
        case 9: _t->handleAria2cFinished((*reinterpret_cast<std::add_pointer_t<int>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<QProcess::ExitStatus>>(_a[2]))); break;
        case 10: _t->onUrlsExtracted((*reinterpret_cast<std::add_pointer_t<QStringList>>(_a[1]))); break;
        case 11: _t->onUrlExtractFailed((*reinterpret_cast<std::add_pointer_t<QString>>(_a[1]))); break;
        case 12: _t->onUrlExtractError((*reinterpret_cast<std::add_pointer_t<QString>>(_a[1]))); break;
        case 13: _t->onVideoSelectionChanged((*reinterpret_cast<std::add_pointer_t<int>>(_a[1]))); break;
        default: ;
        }
    }
}

const QMetaObject *MainWindow::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *MainWindow::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_staticMetaObjectStaticContent<qt_meta_tag_ZN10MainWindowE_t>.strings))
        return static_cast<void*>(this);
    return QMainWindow::qt_metacast(_clname);
}

int MainWindow::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QMainWindow::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 14)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 14;
    }
    if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 14)
            *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType();
        _id -= 14;
    }
    return _id;
}
QT_WARNING_POP
