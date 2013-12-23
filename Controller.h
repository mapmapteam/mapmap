#include <QVariant>
#include <QMetaObject>
#include <QHash>
#include <QPair>
#include <string>
#include <map>
#include "MainWindow.h"

/**
 * Facade to control the application.
 *
 * TODO: implement it.
 * TODO: include it in the project
 * TODO: add signals
 * TODO: the whole app should use actual QObject for properties of everything
 */
class Controller
{
    public:
        Controller(MainWindow *owner);
        // CRUD
        template<class T> void registerObjType(const char* objType);
        bool createObject(const char *objType, const char *objName);
        bool listObjects(const char *objType, QList<QString> &objNames);
        bool destroyObject(const char *objName);
        //FIXME: The original prototype functions for these had QVariantList
        // arguments. FOr now these 2 functions set/get a single QVariant for a
        // single property, but  we can change this easily.
        bool setObjectProperty(const char* objName, const char* propName,
            const QVariant &value);
        bool getObjectProperty(const char *objName, const char *propName,
            QVariant &value);
        bool listObjectProperties(const char *objName, QList<QString> &names,
            QVariantList &values);
        bool saveProject(const char *fileName);
        bool loadProject(const char *fileName);
        bool quit();
        struct strCmp {
          bool operator()( const char* s1, const char* s2 ) const {
            return strcmp( s1, s2 ) < 0;
          }
        };
    private:
        MainWindow *_owner;
        std::map<const char*, const QMetaObject *, strCmp> _mControllerTypes;
        std::map<const char*, QObject *, strCmp> _mControllerObjects;
};
