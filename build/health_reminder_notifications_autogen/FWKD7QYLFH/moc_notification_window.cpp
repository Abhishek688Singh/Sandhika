/****************************************************************************
** Meta object code from reading C++ file 'notification_window.h'
**
** Created by: The Qt Meta Object Compiler version 68 (Qt 6.4.2)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include <memory>
#include "../../../include/health_reminder/notifications/notification_window.h"
#include <QtGui/qtextcursor.h>
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'notification_window.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 68
#error "This file was generated using the moc from 6.4.2. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

#ifndef Q_CONSTINIT
#define Q_CONSTINIT
#endif

QT_BEGIN_MOC_NAMESPACE
QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
namespace {
struct qt_meta_stringdata_health_reminder__notifications__NotificationWindow_t {
    uint offsetsAndSizes[14];
    char stringdata0[51];
    char stringdata1[17];
    char stringdata2[1];
    char stringdata3[7];
    char stringdata4[11];
    char stringdata5[12];
    char stringdata6[7];
};
#define QT_MOC_LITERAL(ofs, len) \
    uint(sizeof(qt_meta_stringdata_health_reminder__notifications__NotificationWindow_t::offsetsAndSizes) + ofs), len 
Q_CONSTINIT static const qt_meta_stringdata_health_reminder__notifications__NotificationWindow_t qt_meta_stringdata_health_reminder__notifications__NotificationWindow = {
    {
        QT_MOC_LITERAL(0, 50),  // "health_reminder::notification..."
        QT_MOC_LITERAL(51, 16),  // "updateSkipButton"
        QT_MOC_LITERAL(68, 0),  // ""
        QT_MOC_LITERAL(69, 6),  // "onDone"
        QT_MOC_LITERAL(76, 10),  // "onSnooze5m"
        QT_MOC_LITERAL(87, 11),  // "onSnooze10m"
        QT_MOC_LITERAL(99, 6)   // "onSkip"
    },
    "health_reminder::notifications::NotificationWindow",
    "updateSkipButton",
    "",
    "onDone",
    "onSnooze5m",
    "onSnooze10m",
    "onSkip"
};
#undef QT_MOC_LITERAL
} // unnamed namespace

Q_CONSTINIT static const uint qt_meta_data_health_reminder__notifications__NotificationWindow[] = {

 // content:
      10,       // revision
       0,       // classname
       0,    0, // classinfo
       5,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       0,       // signalCount

 // slots: name, argc, parameters, tag, flags, initial metatype offsets
       1,    0,   44,    2, 0x08,    1 /* Private */,
       3,    0,   45,    2, 0x08,    2 /* Private */,
       4,    0,   46,    2, 0x08,    3 /* Private */,
       5,    0,   47,    2, 0x08,    4 /* Private */,
       6,    0,   48,    2, 0x08,    5 /* Private */,

 // slots: parameters
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,

       0        // eod
};

Q_CONSTINIT const QMetaObject health_reminder::notifications::NotificationWindow::staticMetaObject = { {
    QMetaObject::SuperData::link<QDialog::staticMetaObject>(),
    qt_meta_stringdata_health_reminder__notifications__NotificationWindow.offsetsAndSizes,
    qt_meta_data_health_reminder__notifications__NotificationWindow,
    qt_static_metacall,
    nullptr,
    qt_incomplete_metaTypeArray<qt_meta_stringdata_health_reminder__notifications__NotificationWindow_t,
        // Q_OBJECT / Q_GADGET
        QtPrivate::TypeAndForceComplete<NotificationWindow, std::true_type>,
        // method 'updateSkipButton'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'onDone'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'onSnooze5m'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'onSnooze10m'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'onSkip'
        QtPrivate::TypeAndForceComplete<void, std::false_type>
    >,
    nullptr
} };

void health_reminder::notifications::NotificationWindow::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<NotificationWindow *>(_o);
        (void)_t;
        switch (_id) {
        case 0: _t->updateSkipButton(); break;
        case 1: _t->onDone(); break;
        case 2: _t->onSnooze5m(); break;
        case 3: _t->onSnooze10m(); break;
        case 4: _t->onSkip(); break;
        default: ;
        }
    }
    (void)_a;
}

const QMetaObject *health_reminder::notifications::NotificationWindow::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *health_reminder::notifications::NotificationWindow::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_health_reminder__notifications__NotificationWindow.stringdata0))
        return static_cast<void*>(this);
    return QDialog::qt_metacast(_clname);
}

int health_reminder::notifications::NotificationWindow::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QDialog::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 5)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 5;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 5)
            *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType();
        _id -= 5;
    }
    return _id;
}
QT_WARNING_POP
QT_END_MOC_NAMESPACE
