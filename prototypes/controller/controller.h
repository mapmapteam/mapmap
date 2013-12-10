#include <QVariant>
#include <string>
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
        bool createObject(const char *objType, std::string &objName);
        bool listObjects(const char *objType, QList<QString> &objNames);
        bool destroyObject(const char *objName);
        bool setObjectProperty(const char *objName, const char *propName,
            const QVariantList &value);
        bool getObjectProperty(const char *objName, const char *propName,
            QVariantList &value);
        bool listObjectProperties(const char *objName, QList<QString> &values);
        bool saveProject(const char *fileName);
        bool loadProject(const char *fileName);
        bool quit();
    private:
        MainWindow *_owner;
};

int main(int argc, char **argv)
{
    /* Example: */
    Controller *controller = new Controller(NULL);

    std::string quadName;
    std::string imageName;

    controller->createObject("image", imageName);
    controller->createObject("quad", quadName);
    controller->setObjectProperty(sourceName, "uri", QVariant(QString("image.jpg")));
    controller->setObjectProperty(quadName, "source", QVariant(QString(imageName.c_str())));
    controller->setObjectProperty(quadName, "x1", QVariant(3.14159));
    controller->saveProject("project.lmp");
    controller->quit();

    delete controller;
    return 0;
}
